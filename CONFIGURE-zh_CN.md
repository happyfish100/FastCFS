
## 配置及运行

本文档以FastCFS RPM包设定的路径（配置文件目录和程序工作目录等）进行说明，如果你采用自助编译安装方式的话，请自行对应。


### 配置文件目录结构

```
/etc/fastcfs/
        |__ fcfs
        |    |__ fuse.conf: fcfs_fused对应的配置文件
        |
        |__ fdir
        |    |__ cluster_servers.conf: 服务器列表，配置服务器ID、IP和端口
        |    |__ server.conf: fdir_serverd对应的配置文件
        |    |__ client.conf: 客户端配置文件
        |
        |__ fstore
             |__ servers.conf: 服务器列表，配置服务器ID、IP和端口
             |__ cluster.conf: 配置服务器分组及数据分组之间的对应关系
             |__ storage.conf: 存储路径及空间分配和回收配置
             |__ server.conf: fs_serverd对应的配置文件
             |__ client.conf: 客户端配置文件
```


### 程序工作目录

```
/opt/fastcfs/
        |__ fcfs
        |    |__ fused.pid: 服务进程fcfs_fused的pid文件
        |    |__ logs: 日志文件目录
        |         |__ fcfs_fused.log: 错误日志
        |
        |__ fdir
        |    |__ serverd.pid: 服务进程fdir_serverd的pid文件
        |    |__ data: 系统数据文件目录
        |    |__ logs: 日志文件目录
        |         |__ fdir_serverd.log: 错误日志
        |         |__ slow.log: 慢查询日志
        |
        |__ fstore
             |__ serverd.pid: 服务进程fs_serverd的pid文件
             |__ data: 系统数据文件目录
             |__ logs: 日志文件目录
                  |__ fs_serverd.log: 错误日志
                  |__ slow.log: 慢查询日志
```

FastCFS集群配置包含如下三部分：

### 1. fastDIR server 配置

配置文件路径：/etc/fastcfs/fdir

fastDIR集群内各个server配置的cluster_servers.conf必须完全一样。

建议配置一次，分发到其他服务器即可。

1.1 把fastDIR集群中的所有服务器配置到cluster_servers.conf中；

  每个fastDIR服务实例包含2个服务端口：cluster 和 service

1.2 配置 server.conf

  * 如果需要修改数据存放路径，修改配置项 data_path 为绝对路径

  * [cluster] 和 [service] 配置的端口（port）必须与cluster_servers.conf中本机的一致，否则启动会报错

  fastDIR重启及查看日志命令：
```
sudo systemctl restart fastdir
tail /opt/fastcfs/fdir/logs/fdir_serverd.log
```

### 2. faststore server 配置

配置文件路径：/etc/fastcfs/fstore

faststore集群各个server配置的servers.conf和cluster.conf必须完全一样。

建议把这两个配置文件分别配置好，然后分发到其他服务器。

2.1 把faststore集群中的所有服务器配置到servers.conf中；

  每个faststore服务实例包含3个服务端口：cluster、replica 和 service

2.2 在cluster.conf中配置服务器分组和数据分组对应关系；

 对于生产环境，为了便于今后扩容，建议数据分组数目至少为256，最好不要超过1024（视业务发展规模而定）

2.3 在storage.conf 中配置存储路径等参数；

2.4 配置 server.conf

  * 如果需要修改数据存放路径，修改配置项 data_path 为绝对路径

  * [cluster]、[replica] 和 [service] 配置的端口（port）必须与servers.conf中本机的一致，否则启动会报错

  faststore重启及查看日志命令：
```
sudo systemctl restart faststore
tail /opt/fastcfs/fstore/logs/fs_serverd.log
```

### 3. FastCFS客户端配置

fused 配置文件路径：/etc/fastcfs/fcfs

3.1 复制faststore server上的如下两个配置文件到 /etc/fastcfs/fstore/
```
/etc/fastcfs/fstore/servers.conf
/etc/fastcfs/fstore/cluster.conf
```

3.2 修改如下两个配置文件的 dir_server 参数
```
/etc/fastcfs/fcfs/fuse.conf
/etc/fastcfs/fdir/client.conf

注：
   * dir_server配置的端口为fastDIR server的service端口；
   * dir_server可以出现多次，一行配置一个dir server IP及其服务端口。
```

3.3 如有必要，修改fuse.conf 中的mountpoint（可选）
```
mountpoint = /opt/fastcfs/fuse
```

  fused 重启及查看日志命令：
```
sudo systemctl restart fastcfs
tail /opt/fastcfs/fcfs/logs/fcfs_fused.log
```

  查看fastDIR集群状态：
```
fdir_cluster_stat
```

  查看单台fastDIR 服务状态：
```
fdir_service_stat $IP:$port

友情提示：可以拷贝fdir_cluster_stat 输出的IP和服务端口
```

  查看faststore集群状态：
```
fs_cluster_stat
```

 查看FastCFS磁盘使用情况：
```
df -h
```
至此，mountpoint（如：/opt/fastcfs/fuse）可以作为本地文件目录访问了。

祝顺利！ have a nice day!
