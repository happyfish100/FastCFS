
# 配置及运行

本文档以FastCFS RPM包设定的路径（配置文件目录和程序工作目录等）进行说明，如果你采用自助编译安装方式的话，请自行对应。

FastCFS集群配置包含如下四部分：

* fastDIR server（服务实例）配置
* faststore server（服务实例）配置
* FastCFS客户端配置
* 认证配置（可选）


## 1. 配置文件目录结构

```
/etc/fastcfs/
        |
        |__ fcfs: fused服务
        |    |__ fuse.conf: fcfs_fused对应的配置文件
        |
        |__ fdir: FastDIR目录服务
        |    |__ cluster.conf: 服务器列表，配置服务器ID、IP和端口
        |    |__ server.conf: fdir_serverd对应的配置文件
        |    |__ storage.conf: 持久化存储配置文件
        |    |__ client.conf: 客户端配置文件
        |
        |__ fstore: faststore存储服务
             |__ cluster.conf: 服务器列表(配置服务器ID、IP和端口)，并配置服务器分组及数据分组之间的对应关系
             |__ storage.conf: 存储路径及空间分配和回收配置
             |__ server.conf: fs_serverd对应的配置文件
             |__ client.conf: 客户端配置文件
```

## 2. 程序工作目录

```
/opt/fastcfs/
        |
        |__ fcfs
        |    |__ fused.pid: 服务进程fcfs_fused的pid文件
        |    |__ logs: 日志文件目录
        |         |__ fcfs_fused.log: 错误日志
        |
        |__ fdir
        |    |__ serverd.pid: 服务进程fdir_serverd的pid文件
        |    |__ data: 系统数据文件目录，包含集群拓扑结构和binlog
        |    |    |__ cluster.info: 集群拓扑信息
        |    |    |__ .inode.sn: 当前inode顺序号
        |    |    |__ binlog: 存放binlog文件
        |    |
        |    |__ db: 存储引擎默认数据目录
        |    |
        |    |__ logs: 日志文件目录
        |         |__ fdir_serverd.log: 错误日志
        |         |__ slow.log: 慢查询日志
        |
        |__ fstore
              |__ serverd.pid: 服务进程fs_serverd的pid文件
              |__ data: 系统数据文件目录，包含集群拓扑结构和binlog
              |    |__ data_group.info: 集群数据分组信息
              |    |__ .store_path_index.dat: 存储路径索引
              |    |__ .trunk_id.dat: 当前trunk id信息
              |    |__ replica: replica binlog，一个DG对应一个子目录
              |    |__ slice: slice binlog
              |    |__ trunk: trunk binlog
              |    |__ recovery: slave追加master历史数据，一个DG对应一个子目录
              |    |__ migrate: 迁移出去的DG数据清理
              |    |__ rebuild: 单盘数据恢复
              |
              |__ logs: 日志文件目录
                   |__ fs_serverd.log: 错误日志
                   |__ slow.log: 慢查询日志
```

## 3. fastDIR server（服务实例）配置

配置文件路径：

> /etc/fastcfs/fdir

fastDIR集群内各个server配置的cluster.conf必须完全一样，建议配置一次，分发到集群中的所有服务器和客户端即可。

### 3.1 把fastDIR集群中的所有服务实例配置到cluster.conf中

每个 **fastDIR** 服务实例包含2个服务端口：**cluster** 和 **service**；

**cluster.conf** 中配置集群所有实例，一个实例由IP + 端口（包括 cluster和service 2个端口 ）组成；

一个fastDIR服务实例需要配置一个[server-$id]的section，其中$id为实例ID（从1开始编号）；

如果一台服务器上启动了多个实例，因端口与全局配置的不一致，此时必须指定端口。

一个服务实例的配置示例如下：

```
[server-3]
cluster-port = 11015
service-port = 11016
host = 172.16.168.128
```

### 3.2 配置 server.conf

* 如果需要修改数据存放路径，修改配置项 data_path 为绝对路径
* 本机端口包含cluster和service 2个端口，分配在[cluster] 和 [service] 中配置
* 本机IP + 本机端口必须配置在cluster.conf的一个实例中，否则启动时会出现下面类似的出错信息
```
ERROR - file: cluster_info.c, line: 119, cluster config file: /etc/fastcfs/fdir/cluster.conf,
      can't find myself by my local ip and listen port
```
* V3.0支持的持久化特性默认是关闭的，在 [storage-engine] 中设置，详情参见配置示例


### 3.3 配置 storage.conf

* 开启持久化存储特性的情况下，才需要配置storage.conf
* 通常采用默认设置即可

fastDIR重启：

```
/usr/bin/fdir_serverd /etc/fastcfs/fdir/server.conf restart
```
或者：

```
sudo systemctl restart fastdir
```

查看日志：

```
tail /opt/fastcfs/fdir/logs/fdir_serverd.log
```

## 2. faststore server（服务实例）配置

配置文件路径：

> /etc/fastcfs/fstore

faststore集群各个服务实例配置的cluster.conf必须完全一样，建议把cluster.conf一次性配置好，然后分发到集群中的所有服务器和客户端。

### 2.1 把 faststore 集群中的所有服务实例配置到 cluster.conf 中

每个 **faststore** 服务实例包含3个服务端口：cluster、replica 和 service，和 **fastDIR** 的 cluster.conf 相比，多了一个replica端口，二者配置方式完全相同。

### 2.2 在 cluster.conf 中配置服务器分组和数据分组对应关系

对于生产环境，为了便于今后扩容，建议数据分组数目至少为64，最好不要超过1024（视业务未来5年发展规模而定）。

### 2.3 在storage.conf 中配置存储路径等参数

支持配置多个存储路径。为了充分发挥出硬盘性能，建议挂载单盘，每块盘作为一个存储路径。

配置示例：

```
store_path_count = 1

[store-path-1]
path = /opt/faststore/data
```

### 2.4 配置 server.conf

* 如果需要修改binlog存放路径，修改配置项 data_path 为绝对路径
* 本机端口包含cluster、replica和service 3个端口，分配在[cluster]、[replica] 和 [service] 中配置
* 本机IP + 本机端口必须配置在cluster.conf的一个实例中，否则启动时会出现类似下面的出错信息

```
ERROR - file: server_group_info.c, line: 347, cluster config file: /etc/fastcfs/fstore/cluster.conf,
      can't find myself by my local ip and listen port
```

faststore重启：

```
/usr/bin/fs_serverd /etc/fastcfs/fstore/server.conf restart
```

或者：

```
sudo systemctl restart faststore
```

查看日志：

```
tail /opt/fastcfs/fstore/logs/fs_serverd.log
```

## 3. FastCFS客户端配置

fused 配置文件路径：

> /etc/fastcfs/fcfs

### 3.1 复制faststore server上的如下配置文件到 /etc/fastcfs/fstore/

```
/etc/fastcfs/fstore/cluster.conf
```

### 3.2 复制fastDIR server上的如下配置文件到 /etc/fastcfs/fdir/

```
/etc/fastcfs/fdir/cluster.conf
```

### 3.3 修改fuse挂载点（可选）

如有必要，可修改配置文件 **fuse.conf** 中的 **mountpoint** 参数来指定挂载点：

> mountpoint = /opt/fastcfs/fuse

fused 重启：

> /usr/bin/fcfs_fused /etc/fastcfs/fcfs/fuse.conf restart

或者：

> sudo systemctl restart fastcfs

查看日志：

> tail /opt/fastcfs/fcfs/logs/fcfs_fused.log

查看fastDIR集群状态：

> fdir_cluster_stat

查看单台fastDIR 服务状态：

> fdir_service_stat $IP:$port

**_友情提示：可以拷贝fdir_cluster_stat 输出的IP和服务端口_**

查看faststore集群状态：

> fs_cluster_stat

查看非ACTIVE的服务列表，使用：

> fs_cluster_stat -N

使用 -h 查看更多选项

查看FastCFS磁盘使用情况：

> df -h /opt/fastcfs/fuse | grep fuse

至此，mountpoint（如：/opt/fastcfs/fuse）可以作为本地文件目录访问了。

## 4. 认证配置（可选）

如果需要开启存储池或访问权限控制，请参阅 [认证配置文档](AUTH-zh_CN.md)

## 5. 共享数据配置（可选）

将FastCFS作为Oracle RAC等系统的共享存储，请参阅 [共享数据配置指南](shared-storage-guide-zh_CN.md)

祝顺利！ have a nice day!
