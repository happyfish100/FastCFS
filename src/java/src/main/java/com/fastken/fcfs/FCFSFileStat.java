package com.fastken.fcfs;

public class FCFSFileStat {
    private static native void doInit();

    static {
        doInit();
    }

    private long inode;  /* Inode number */
    private int mode;    /* File type and mode */
    private int links;   /* Number of hard links */
    private int uid;     /* User ID of owner */
    private int gid;     /* Group ID of owner */
    private int rdev;    /* Device ID (if special file) */
    private long size;   /* Total size, in bytes */
    private long atime;  /* Time of last access */
    private long mtime;  /* Time of last modification */
    private long ctime;  /* Time of last status change */

    public FCFSFileStat(long inode, int mode, int links, int uid, int gid,
            int rdev, long size, long atime, long mtime, long ctime)
    {
        this.inode = inode;
        this.mode = mode;
        this.links = links;
        this.uid = uid;
        this.gid = gid;
        this.rdev = rdev;
        this.size = size;
        this.atime = atime;
        this.mtime = mtime;
        this.ctime = ctime;
    }

    public long getInode() {
        return this.inode;
    }

    public long getMode() {
        return this.mode;
    }

    public long getLinks() {
        return this.links;
    }

    public long getUid() {
        return this.uid;
    }

    public long getGid() {
        return this.gid;
    }

    public long getRdev() {
        return this.rdev;
    }

    public long getSize() {
        return this.size;
    }

    public long getAtime() {
        return this.atime;
    }

    public long getMtime() {
        return this.mtime;
    }

    public long getCtime() {
        return this.ctime;
    }
}
