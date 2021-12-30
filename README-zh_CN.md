# FastCFS -- 可以跑数据库的高性能通用分布式文件系统

[English](./README.md) | 简体中文

## 1. 关于

FastCFS 是一款强一致性、高性能、高可用、支持百亿级海量文件的通用分布式文件系统，可以作为MySQL、PostgresSQL、Oracle等数据库和云平台的后端存储。

## 2. 当前版本

V3.0.0

## 3. 支持的操作系统

* Linux: Kernel version >= 3.10 （完全支持） 
* MacOS or FreeBSD  （仅支持服务端，不支持FUSE）

## 4. 依赖

* [libfuse](https://gitee.com/mirrors/libfuse) (版本 3.9.4 或更高版本)
    * [Python](https://python.org/) (版本 3.5 或更高版本)
    * [Ninja](https://ninja-build.org/) (版本 1.7 或更高版本)
    * [gcc](https://www.gnu.org/software/gcc/) (版本 4.7.0 或更高版本)
* [libfastcommon](https://gitee.com/fastdfs100/libfastcommon) (tag: V1.0.54)
* [libserverframe](https://gitee.com/fastdfs100/libserverframe) (tag: V1.1.11)
* [fastDIR](https://gitee.com/fastdfs100/fastDIR) (tag: V3.0.0)
* [faststore](https://gitee.com/fastdfs100/faststore) (tag: V3.0.0)
* [FastCFS](https://gitee.com/fastdfs100/FastCFS) (tag: V3.0.0)

## 5. 安装

FastCFS包含 libfastcommon、libserverframe、libdiskallocator、fastDIR、faststore和FastCFS 六个安装包。

### 一键部署

如果你打算快速体验一下FastCFS，可以一键搭建(包括部署和运行)单节点（需要root身份执行）：
```
git clone https://gitee.com/fastdfs100/FastCFS.git; cd FastCFS/
./helloWorld.sh

# 注意：helloWorld.sh将更改FastCFS相关配置文件，请不要在多节点集群上执行！
```

上述操作完成后，执行命令：
```
df -h /opt/fastcfs/fuse | grep fuse
```
可以看到FastCFS挂载的文件目录，你可以当作本地文件系统访问该目录。

一键部署的详细说明，请参见这里[一键部署详细说明](docs/Easy-install-detail-zh_CN.md)

### 集群部署工具

推荐使用 [FastCFS集群运维工具](docs/fcfs-ops-tool-zh_CN.md)

### DIY安装

如果你要自己搭建FastCFS环境，可以采用如下三种安装方式之一：

* yum安装（仅支持CentOS 7.x 和 8.x），参阅 [YUM安装文档](docs/YUMINSTALL-zh_CN.md)
* apt安装，参阅 [apt 安装文档](docs/APT-INSTALL-zh_CN.md)
* 源码编译安装，参阅 [安装文档](docs/INSTALL-zh_CN.md)

### 配置指南

FastCFS安装完成后，请参阅[配置指南](docs/CONFIGURE-zh_CN.md)


## 6. 性能测试

FastCFS性能明显优于Ceph：顺序写是Ceph的6.x倍，顺序读是Ceph的2.x倍，随机写大约是Ceph的2倍。

* [FastCFS与Ceph性能对比测试结果概要](docs/benchmark.md)

* 详情参见 [完整PDF文档](docs/benchmark-20210621.pdf)

## 7. K8s CSI驱动

参见项目 [fastcfs-csi](https://gitee.com/fastdfs100/fastcfs-csi)

## 8. 技术文章

参见 <a href="https://my.oschina.net/u/3334339" target="_blank">技术博客</a>

## 9. 常见问题

参见 [FastCFS常见问题](docs/FAQ-zh_CN.md)

## 10. 待完成工作

*  [fstore] 提供集群扩容工具，实现自动化扩容
*  [fstore] 分级存储 & slice数据合并：支持两级存储（如SSD + HDD）
*  [fdir & fstore] binlog去重（fdir binlog、fstore replica & slice binlog）
*  [fstore] 机器故障恢复后，master需重新均衡分配

## 11. 联系我们

查看FastCFS相关技术文章，请关注微信公众号：

<img src="images/wechat_subscribe.jpg" width="200" alt="微信公众号">

微信交流群：

<img src="images/wechat_group.jpg" width="200" alt="微信交流群">
