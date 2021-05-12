# FastCFS -- 可以跑数据库的高性能分布式文件系统

## 1. 关于

FastCFS 是一款基于块存储的通用分布式文件系统，可以作为MySQL、PostgresSQL、Oracle等数据库和云平台的后端存储。

## 2. 当前版本

V2.0.1

## 3. 支持的操作系统

* Linux: Kernel version >= 3.10 （完全支持） 
* MacOS or FreeBSD  （仅支持服务端，不支持FUSE）

## 4. 依赖

* [libfuse](https://gitee.com/mirrors/libfuse) (版本 3.9.4 或更高版本)
    * [Python](https://python.org/) (版本 3.5 或更高版本)
    * [Ninja](https://ninja-build.org/) (版本 1.7 或更高版本)
    * [gcc](https://www.gnu.org/software/gcc/) (版本 4.7.0 或更高版本)
* [libfastcommon](https://gitee.com/fastdfs100/libfastcommon) (tag: V1.0.50)
* [libserverframe](https://gitee.com/fastdfs100/libserverframe) (tag: V1.1.7)
* [fastDIR](https://gitee.com/fastdfs100/fastDIR) (tag: V2.0.1)
* [faststore](https://gitee.com/fastdfs100/faststore) (tag: V2.0.1)
* [FastCFS](https://gitee.com/fastdfs100/FastCFS) (tag: V2.0.1)

## 5. 安装

FastCFS包含 libfastcommon、libserverframe、fastDIR、faststore和FastCFS 五个安装包。

### 一键部署

如果你打算快速体验一下FastCFS，可以一键搭建(包括部署和运行)单节点（需要root身份执行）：
```
git clone https://gitee.com/fastdfs100/FastCFS.git; cd FastCFS/
./helloWorld.sh
```

上述操作完成后，执行命令：
```
df -h /opt/fastcfs/fuse
```
可以看到FastCFS挂载的文件目录，你可以当作本地文件系统访问该目录。


### DIY安装

如果你要自己搭建FastCFS环境，可以采用如下两种安装方式之一：

* yum安装（仅支持CentOS 7.x 和 8.x），参阅 [YUM安装文档](YUMINSTALL-zh_CN.md)

* 源码编译安装，参阅 [安装文档](INSTALL-zh_CN.md)

### 配置指南

FastCFS安装完成后，请参阅[配置指南](CONFIGURE-zh_CN.md)


## 6. 联系我们

查看FastCFS相关技术文章，请关注微信公众号：

<img src="images/wechat_subscribe.jpg" width="200" alt="微信公众号">

微信交流群：

<img src="images/wechat_group.jpg" width="200" alt="微信交流群">
