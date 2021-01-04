
yum 安装方式主要用于测试和生产环境搭建。

### 1. 安装FastOS.repo

先安装FastOS.repo yum源，然后就可以安装FastCFS相关软件包了。

CentOS 7
```
rpm -ivh http://www.fastken.com/yumrepo/el7/x86_64/FastOSrepo-1.0.0-1.el7.centos.x86_64.rpm
```

CentOS 8
```
rpm -ivh http://www.fastken.com/yumrepo/el8/x86_64/FastOSrepo-1.0.0-1.el8.x86_64.rpm
```

yum装包完成后，FastDFS后台程序可通过systemd管理。

### 2. fastDIR server安装

在需要运行 fastDIR server的服务器上执行：
```
yum install fastDIR-server -y
```

### 3. faststore server安装

在需要运行 faststore server的服务器上执行：
```
yum install faststore-server -y
```

### 4. FastCFS客户端安装

在需要使用FastCFS存储服务的机器（即FastCFS客户端）上执行：
```
yum remove fuse -y
yum install FastCFS-fused -y

注：
   * fuse为老版本的包（fuse2.x），需要卸载才可以成功安装FastCFS-fused依赖的fuse3；
   * 第一次安装才需要卸载fuse包，以后就不用执行了。
```

安装完成后，需要修改对应的配置文件，服务才可以正常启动。请参阅[配置指南](CONFIGURE-zh_CN.md)

systemd服务名称如下：
```
  * fastdir： 目录服务，对应程序为 fdir_serverd
  * faststore：存储服务，对应程序为 fs_serverd
  * fastcfs： FUSE后台服务，对应程序为 fcfs_fused
```

可以使用标准的systemd命令对上述3个服务进行控制，例如：
```
sudo systemctl restart fastdir
sudo systemctl restart faststore
sudo systemctl restart fastcfs
```
