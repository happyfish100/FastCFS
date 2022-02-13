
# Auth (认证模块)配置及运行

如果你不需要使用存储池或访问权限控制，可以跳过本文档。

本文档以FastCFS RPM包设定的路径（配置文件目录和程序工作目录等）进行说明，如果你采用自助编译安装方式的话，请自行对应。


## 1. Auth配置文件目录结构

```
/etc/fastcfs/
        |
        |__ auth: 认证中心
             |__ keys: 存放用户密钥文件，每个用户对应一个密钥文件，例如 admin.key
             |    |__ session_validate.key: 用于FastDIR和FastStore请求auth服务验证session和权限
             |__ cluster.conf: 服务器列表，配置服务器ID、IP和端口
             |__ server.conf: fcfs_authd对应的配置文件
             |__ client.conf: 客户端配置文件
             |__ auth.conf: 认证相关的公共配置文件，在FastDIR和FastStore的cluster.conf中引用
             |__ session.conf: session相关配置文件，在Auth、FastDIR和FastStore的server.conf中引用
```


## 2. authd程序工作目录

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

## 3. 认证中心（authd server）配置

配置文件路径：/etc/fastcfs/auth

Auth集群内各个server配置的cluster.conf必须完全一样。

建议配置一次，分发到其他服务器即可。

### 3.1 把Auth集群中的所有服务实例配置到cluster.conf中

每个Auth服务实例包含2个服务端口：cluster 和 service

一个Auth服务实例需要配置一个[server-$id]的section，其中$id为实例ID。

### 3.2 配置 server.conf

* [cluster] 和 [service] 配置的端口（port）必须与cluster.conf中本机的一致，否则启动会报错

### 3.3 配置 auth.conf

将 auth_enabled 设置为 true 以开启认证功能

### 3.4 复制FastDIR server上的如下配置文件到 /etc/fastcfs/fdir/

```
/etc/fastcfs/fdir/client.conf
/etc/fastcfs/fdir/cluster.conf
```

### 3.5 启动authd

authd命令直接重启：

```
/usr/bin/fcfs_authd /etc/fastcfs/auth/server.conf restart
```

或者系统服务命令启动：

```
sudo systemctl restart fastauth
```

查看日志：

```
tail /opt/fastcfs/auth/logs/fcfs_authd.log
```

* 第一次运行将自动创建 admin 用户，默认生成的密钥文件名为 **/etc/fastcfs/auth/keys/admin.key**

### 3.6 创建存储池

创建名为 fs 的存储池，配额无限制：

```
fcfs_pool create fs unlimited
```

**_注：存储池名称必须和FastCFS fuse客户端配置文件fuse.conf中的命名空间一致（缺省配置为fs，可按需修改）_**

## 4. FastDIR server

### 4.1 复制auth server上的如下配置文件到 /etc/fastcfs/auth/

```
/etc/fastcfs/auth/auth.conf
/etc/fastcfs/auth/session.conf
/etc/fastcfs/auth/cluster.conf
/etc/fastcfs/auth/client.conf
```

### 4.2 复制auth server上的如下密钥文件到 /etc/fastcfs/auth/keys/

```
/etc/fastcfs/auth/keys/admin.key
/etc/fastcfs/auth/keys/session_validate.key
```

拷贝完成后重启FastDIR服务（fdir_serverd）

## 5. FastStore server

* 参见 4. FastDIR server 部分


拷贝完成后重启FastStore服务（fs_serverd）

## 6. FastCFS客户端（fused）

* 参见 4. FastDIR server 部分

拷贝完成后重启fuse服务（fcfs_fused）

## 友情提示

* 4 ~ 6部分的配置及启动参见 [配置指南](CONFIGURE-zh_CN.md)


## 7. 命令行工具

* fcfs_user：用户管理，主要包括创建用户、删除用户、设置用户权限（权限包括用户管理、创建存储池等等）

* fcfs_pool：储存池管理，主要包括创建pool、删除pool、把pool读写权限授权给其他用户

## 8. 注意事项

* Auth server依赖FastDIR server，需要先启动FastDIR server，然后启动Auth server。
