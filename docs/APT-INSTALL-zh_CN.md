
## 安装基于 Debian 10及以上版本 和 Ubuntu 18.04及以上版本


### 1. 配置 apt 存储库

配置 apt 存储库和签名密钥，以使系统的包管理器启用自动更新。

```shell
sudo apt-get install curl gpg
curl http://www.fastken.com/aptrepo/packages.fastos.pub | gpg --dearmor > fastos-archive-keyring.gpg
sudo install -D -o root -g root -m 644 fastos-archive-keyring.gpg /usr/share/keyrings/fastos-archive-keyring.gpg
sudo sh -c 'echo "deb [signed-by=/usr/share/keyrings/fastos-archive-keyring.gpg] http://www.fastken.com/aptrepo/fastos/ fastos main" > /etc/apt/sources.list.d/fastos.list'
rm -f fastos-archive-keyring.gpg
```

如果需要安装调试包，执行：
```shell
sudo sh -c 'echo "deb [signed-by=/usr/share/keyrings/fastos-archive-keyring.gpg] http://www.fastken.com/aptrepo/fastos-debug/ fastos-debug main" > /etc/apt/sources.list.d/fastos-debug.list'
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

### 5. Vote server安装（可选）

Vote server作为FastCFS多个服务模块共用的选举节点，主要作用是实现双副本防脑裂（即双活互备防脑裂）。

在需要运行选举节点的服务器上执行：
```shell
sudo apt install fastcfs-vote-server -y
```

### 6. Auth server安装（可选）

如果需要存储池或访问控制，则需要安装本模块。

在需要运行 Auth server的服务器上执行：
```shell
sudo apt install fastcfs-auth-server -y
```

### 7. 集群配置（必须）

安装完成后，需要修改对应的配置文件，服务才可以正常启动。请参阅[配置指南](CONFIGURE-zh_CN.md)


### 8. 启动

FastCFS后台程序可通过systemd管理。systemd服务名称如下：

  * fastdir： 目录服务，对应程序为 fdir_serverd
  * faststore：存储服务，对应程序为 fs_serverd
  * fastcfs： FUSE后台服务，对应程序为 fcfs_fused
  * fastvote： 选举节点，对应程序为 fcfs_voted
  * fastauth： 认证服务，对应程序为 fcfs_authd

可以使用标准的systemd命令对上述5个服务进行管理，例如：
```shell
sudo systemctl restart fastdir
sudo systemctl restart faststore
sudo systemctl restart fastcfs
sudo systemctl restart fastvote
sudo systemctl restart fastauth
```
