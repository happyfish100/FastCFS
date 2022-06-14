
# Vote Node (选举节点)配置及运行

如果你不需要实现双副本防脑裂（即双活互备防脑裂），可以跳过本文档。

本文档以FastCFS RPM包设定的路径（配置文件目录和程序工作目录等）进行说明，如果你采用自助编译安装方式的话，请自行对应。

为了防脑裂，建议配置3个节点（服务器），因选举服务占用资源较少，可以和其他服务共用服务器。

## 1. Vote Node配置文件目录结构

```
/etc/fastcfs/
        |
        |__ vote: 选举节点
             |__ cluster.conf: 服务器列表，配置服务器ID、IP和端口
             |__ server.conf: fcfs_voted对应的配置文件
             |__ client.conf: 客户端配置文件
```


## 2. voted程序工作目录

```
/opt/fastcfs/
        |
        |__ vote
             |__ voted.pid: 服务进程fcfs_voted的pid文件
             |__ logs: 日志文件目录
                  |__ fcfs_voted.log: 错误日志
```

开启共用选举服务需要设置选举节点、Auth server、FastDIR server和FastStore server。

## 3. 选举节点（vote server）配置

配置文件路径：/etc/fastcfs/vote

Vote集群内各个server配置的cluster.conf必须完全一样。

建议配置一次，分发到其他服务器即可。

### 3.1 把Vote集群中的所有服务实例配置到cluster.conf中

每个Vote服务实例包含2个服务端口：cluster 和 service

一个Vote服务实例需要配置一个[server-$id]的section，其中$id为实例ID。

### 3.2 配置 server.conf

* [cluster] 和 [service] 配置的端口（port）必须与cluster.conf中本机的一致，否则启动会报错

### 3.3 启动voted

voted命令直接重启：

```
/usr/bin/fcfs_voted /etc/fastcfs/vote/server.conf restart
```

或者系统服务命令启动：

```
sudo systemctl restart fastvote
```

查看日志：

```
tail /opt/fastcfs/vote/logs/fcfs_voted.log
```

## 4. Auth server

### 4.1 复制vote server上的如下配置文件到 /etc/fastcfs/vote/

```
/etc/fastcfs/vote/cluster.conf
/etc/fastcfs/vote/client.conf
```

修改 /etc/fastcfs/auth/cluster.conf，将vote_node_enabled 设置为true。
配置片段如下：

```
[master-election]
# if enable vote node when the number of servers is even
# the default value is false
vote_node_enabled = true

# the cluster config filename of the vote node
# this parameter is valid when vote_node_enabled is true
vote_node_cluster_filename = ../vote/cluster.conf
```

拷贝完成后重启认证服务（fcfs_authd）

## 5. FastDIR server

修改 /etc/fastcfs/fdir/cluster.conf，将vote_node_enabled 设置为true。

* 详情参见 4. Auth server 部分

拷贝和设置完成后重启FastDIR服务（fdir_serverd）

## 6. FastStore server

* 参见 4. Auth server 部分

修改 /etc/fastcfs/fstore/cluster.conf，将vote_node_enabled 设置为true。
配置片段如下：

```
[leader-election]

# if enable vote node when the number of servers is even
# the default value is false
vote_node_enabled = true

# the cluster config filename of the vote node
# this parameter is valid when vote_node_enabled is true
vote_node_cluster_filename = ../vote/cluster.conf
```

拷贝完成后重启FastStore服务（fs_serverd）

## 友情提示

* 4 ~ 6部分的配置及启动参见 [配置指南](CONFIGURE-zh_CN.md)
