
## Auth (认证模块)配置及运行

如果你不需要使用存储池或访问权限控制，可以跳过本文档。

本文档以FastCFS RPM包设定的路径（配置文件目录和程序工作目录等）进行说明，如果你采用自助编译安装方式的话，请自行对应。


### Auth配置文件目录结构

```
/etc/fastcfs/
        |
        |__ auth: 认证中心
             |__ keys: 存放用户密钥文件，每个用户对应一个密钥文件，例如 admin.key
             |    |__ session_validate.key: 用于FastDIR和FastStore请求auth服务验证session和权限
             |__ servers.conf: 服务器列表，配置服务器ID、IP和端口
             |__ cluster.conf: 集群配置
             |__ server.conf: fcfs_authd对应的配置文件
             |__ client.conf: 客户端配置文件
             |__ auth.conf: 认证相关的公共配置文件，在FastDIR和FastStore的cluster.conf中引用
             |__ session.conf: session相关配置文件，在Auth、FastDIR和FastStore的server.conf中引用
```


### authd程序工作目录

```
/opt/fastcfs/
        |
        |__ auth
             |__ authd.pid: 服务进程fcfs_authd的pid文件
             |__ logs: 日志文件目录
                  |__ fcfs_authd.log: 错误日志
                  |__ slow.log: 慢查询日志
```

开启认证功能需要设置认证中心、FastDIR server、FastStore server和FastCFS客户端。

### 1. 认证中心（authd server）配置

配置文件路径：/etc/fastcfs/auth

Auth集群内各个server配置的servers.conf和cluster.conf必须完全一样。

建议配置一次，分发到其他服务器即可。

1.1 把Auth集群中的所有服务实例配置到servers.conf中；

  每个Auth服务实例包含2个服务端口：cluster 和 service

  一个Auth服务实例需要配置一个[server-$id]的section，其中$id为实例ID。

  * 注：目前仅支持一个服务实例，后续版本将支持多服务实例。

1.2 配置 server.conf

  * [cluster] 和 [service] 配置的端口（port）必须与servers.conf中本机的一致，否则启动会报错

1.3 配置 auth.conf

将 auth_enabled 设置为 true 以开启认证功能

1.4 复制FastDIR server上的如下配置文件到 /etc/fastcfs/fdir/

```
/etc/fastcfs/fdir/client.conf
/etc/fastcfs/fdir/cluster.conf
/etc/fastcfs/fdir/servers.conf
```

1.5 启动authd

  authd重启：
```
/usr/bin/fcfs_authd /etc/fastcfs/auth/server.conf restart
```
或者：
```
sudo systemctl restart fcfs_authd
```

查看日志：
```
tail /opt/fastcfs/auth/logs/fcfs_authd.log
```

* 第一次运行将自动创建 admin 用户，默认生成的密钥文件名为 /etc/fastcfs/auth/keys/admin.key

1.6 创建存储池

创建名为 fs 的存储池，配额无限制：
```
fcfs_pool create fs unlimited
```

### 2. FastDIR server

2.1 复制auth server上的如下配置文件到 /etc/fastcfs/auth/
```
/etc/fastcfs/auth/auth.conf
/etc/fastcfs/auth/session.conf
/etc/fastcfs/auth/cluster.conf
/etc/fastcfs/auth/servers.conf
/etc/fastcfs/auth/client.conf
```

2.2 复制auth server上的如下密钥文件到 /etc/fastcfs/auth/keys
```
/etc/fastcfs/auth/keys/admin.key
/etc/fastcfs/auth/keys/session_validate.key
```

### 3. FastStore server

参见 2. FastDIR server 部分

### 4. FastCFS客户端（used）

参见 2. FastDIR server 部分
