
## 安装基于 Debian 和 Ubuntu 的发行版


### 1. 配置 apt 存储库

配置 apt 存储库和签名密钥，以使系统的包管理器启用自动更新。

```shell
curl http://www.fastken.com/aptrepo/packages.fastos.pub | sudo apt-key add
sudo sh -c 'echo "deb http://www.fastken.com/aptrepo/fastos/ fastos main" >> /etc/apt/sources.list'
```

然后更新包缓存
```
sudo apt update
```


### 2. fastDIR server安装

在需要运行 fastDIR server的服务器上执行：
```shell
sudo apt install fastdir-server -y
```


### 3. faststore server安装

在需要运行 faststore server的服务器上执行：
```shell
sudo apt install faststore-server -y
```


### 4. FastCFS客户端安装

在需要使用FastCFS存储服务的机器（即FastCFS客户端）上执行：
```shell
# 注：fastcfs-fused依赖fuse3
sudo apt install fastcfs-fused -y
```


### 5. Auth server安装（可选）

如果需要存储池或访问控制，则需要安装本模块。

在需要运行 Auth server的服务器上执行：
```shell
sudo apt install fastcfs-auth-server -y
```

### 6. 集群配置（必须）

安装完成后，需要修改对应的配置文件，服务才可以正常启动。请参阅[配置指南](CONFIGURE-zh_CN.md)


### 7. 启动

FastCFS后台程序可通过systemd管理。systemd服务名称如下：

  * fastdir： 目录服务，对应程序为 fdir_serverd
  * faststore：存储服务，对应程序为 fs_serverd
  * fastcfs： FUSE后台服务，对应程序为 fcfs_fused
  * fastauth： 认证服务，对应程序为 fcfs_authd

可以使用标准的systemd命令对上述4个服务进行管理，例如：
```shell
sudo systemctl restart fastdir
sudo systemctl restart faststore
sudo systemctl restart fastcfs
sudo systemctl restart fastauth
```

