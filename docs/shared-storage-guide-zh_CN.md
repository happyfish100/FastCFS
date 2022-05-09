# 共享存储配置指南

FastCFS客户端缓存默认开启，这主要针对专属存储场景。

如果将FastCFS作为Oracle RAC等系统的共享存储，需要关闭客户端相关缓存。

默认安装的客户端配置文件为 /etc/fastcfs/fcfs/fuse.conf，下面列出需要修改的配置项：

```
[FastDIR]

# 对于文件追加写或文件truncate操作，通过加锁避免冲突
# if use sys lock for file append and truncate to avoid conflict
# set true when the files appended or truncated by many nodes (FUSE instances)
# default value is false
use_sys_lock_for_append = true

# 禁用异步报告文件属性（即采用同步报告方式）
# if async report file attributes (size, modify time etc.) to the FastDIR server
# default value is true
async_report_enabled = false

# 禁用合并写
[write-combine]
# if enable write combine feature for FastStore
# default value is true
enabled = false

# 禁用预读机制
[read-ahead]
# if enable read ahead feature for FastStore
# default value is true
enabled = false


# 禁用Linux对inode和文件属性缓存
[FUSE]
# cache time for file attribute in seconds
# default value is 1.0s
attribute_timeout = 0.0

# cache time for file entry in seconds
# default value is 1.0s
entry_timeout = 0.0

```

友情提示：配置文件修改后，需要重启生效。
