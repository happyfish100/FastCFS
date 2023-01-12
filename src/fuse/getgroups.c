/*
 * Copyright (c) 2020 YuQing <384681@qq.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include "fastcommon/logger.h"
#include "global.h"
#include "getgroups.h"

static inline int get_last_id(const char *buff, const char *tag_str,
        const int tag_len, const char **next)
{
    const char *start;
    const char *last;

    if ((start=strstr(buff, tag_str)) == NULL) {
        *next = buff;
        return -1;
    }

    start += tag_len;
    if ((last=strchr(start, '\n')) == NULL) {
        *next = buff;
        return -1;
    }

    *next = last;
    do {
        --last;
    } while (last >= start && (*last >= '0' && *last <= '9'));
    return strtol(last + 1, NULL, 10);
}

int fcfs_getgroups(const pid_t pid, const uid_t fsuid,
        const gid_t fsgid, const int size, gid_t *list)
{
#define UID_TAG_STR   "\nUid:"
#define UID_TAG_LEN   (sizeof(UID_TAG_STR) - 1)
#define GID_TAG_STR   "\nGid:"
#define GID_TAG_LEN   (sizeof(GID_TAG_STR) - 1)
#define GROUPS_TAG_STR   "\nGroups:"
#define GROUPS_TAG_LEN   (sizeof(GROUPS_TAG_STR) - 1)

    char filename[64];
    char buff[1024];
    const char *next;
    struct passwd *user;
    char *p;
    char *end;
    int fd;
    int len;
    int euid;
    int egid;
    gid_t val;
    int count;

    sprintf(filename, "/proc/%d/status", pid);
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return 0;
    }

    len = read(fd, buff, sizeof(buff));
    close(fd);
    if (len <= 0) {
        return 0;
    }

    buff[len - 1] = '\0';
    euid = get_last_id(buff, UID_TAG_STR, UID_TAG_LEN, &next);
    if ((egid=get_last_id(next, GID_TAG_STR, GID_TAG_LEN, &next)) < 0) {
        egid = get_last_id(buff, GID_TAG_STR, GID_TAG_LEN, &next);
    }
    if (fsuid == euid && fsgid == egid) {
        if ((p=strstr(next, GROUPS_TAG_STR)) == NULL) {
            if ((p=strstr(buff, GROUPS_TAG_STR)) == NULL) {
                return 0;
            }
        }

        p += GROUPS_TAG_LEN;
        count = 0;
        while (count < size) {
            while (*p == ' ' || *p == '\t') {
                ++p;
            }
            val = strtoul(p, &end, 10);
            if (end == p) {
                break;
            }

            if (val != fsgid) {
                list[count++] = val;
            }
            p = end;
        }
    } else {
        /*
        logInfo("line: %d, fsuid: %d, fsgid: %d, euid: %d, egid: %d",
                __LINE__, fsuid, fsgid, euid, egid);
                */
        if ((user=getpwuid(fsuid)) == NULL) {
            return 0;
        }

        count = size;
        if (getgrouplist(user->pw_name, fsgid, list, &count) < 0) {
            return 0;
        }
    }

    return count;
}

int fcfs_get_groups(const pid_t pid, const uid_t fsuid,
        const gid_t fsgid, char *buff)
{
    int count;
    gid_t list[FDIR_MAX_USER_GROUP_COUNT];
    const gid_t *group;
    const gid_t *end;
    char *p;

    count = fcfs_getgroups(pid, fsuid, fsgid,
            FDIR_MAX_USER_GROUP_COUNT, list);
    if (count <= 0) {
        count = 0;
    } else if (count == 1 && list[0] == fsgid) {
        count = 0;
    } else {
        end = list + count;
        for (group=list, p=buff; group<end; group++) {
            if (*group != fsgid) {
                int2buff(*group, p);
                p += 4;

            }
            logInfo("%d. gid: %d", (int)(group - list) + 1, *group);
        }
        count = (p - buff) / 4;
    }
    logInfo("line: %d, count: %d, first gid: %d", __LINE__, count, count > 0 ? buff2int(buff) : -1);
    return count;
}
