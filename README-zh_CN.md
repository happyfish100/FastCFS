# FastCFS -- 可以跑数据库的高性能云原生分布式文件系统

## 1. 关于

FastCFS 是一款基于块存储的通用分布式文件系统，可以作为MySQL、PostgresSQL、Oracle等数据库和云平台的后端存储。

## 2. 当前版本

V1.1.0

## 3. 支持的操作系统

* Linux: Kernel version >= 3.10 （完全支持） 
* MacOS or FreeBSD  （仅支持服务端，不支持FUSE）

## 4. 依赖

* [libfuse](https://github.com/libfuse/libfuse) (版本 3.9.4 或更高版本)
    * [Python](https://python.org/) (版本 3.5 或更高版本)
    * [Ninja](https://ninja-build.org/) (版本 1.7 或更高版本)
    * [gcc](https://www.gnu.org/software/gcc/) (版本 4.7.0 或更高版本)
* [libfastcommon](https://github.com/happyfish100/libfastcommon) (tag: V1.0.45)
* [libserverframe](https://github.com/happyfish100/libserverframe) (tag: V1.1.1)
* [fastDIR](https://github.com/happyfish100/fastDIR) (tag: V1.1.0)
* [faststore](https://github.com/happyfish100/faststore) (tag: V1.1.0)
* [FastCFS](https://github.com/happyfish100/FastCFS) (tag: V1.1.0)

## 5. 安装

### 5.1 DIY安装

参阅 INSTALL.md

libfuse 编译依赖比较复杂，建议使用脚本libfuse_setup.sh一键编译和安装。

### 5.2 一键安装

libfastcommon、libserverframe、fastDIR、faststore和FastCFS 五个安装包可采用 fastcfs.sh 脚本统一安装配置，也可以参照INSTALL.md独立安装。

```
git clone https://github.com/happyfish100/FastCFS.git; cd FastCFS/
```

通过执行fastcfs.sh脚本，可自动从github仓库拉取或更新五个仓库代码，按照依赖顺序进行编译、安装，并能根据配置文件模版自动生成集群相关配置文件。

fastcfs.sh 命令参数说明：

* pull -- 从github拉取或更新代码库（拉取到本地build目录）
* makeinstall -- 顺序编译、安装代码库（make clean && make && make install）
* init -- 初始化集群目录、配置文件（已存在不会重新生成）
* clean -- 清除已编译程序文件（相当于make clean）


一键搭建(包括部署和运行)demo环境（需要root身份执行）：

```
./helloWorld.sh --prefix=/usr/local/fastcfs-test
```

或执行如下命令（需要root身份执行）：

```
./libfuse_setup.sh
./fastcfs.sh pull
./fastcfs.sh makeinstall
IP=$(ifconfig -a | grep -w inet | grep -v 127.0.0.1 | awk '{print $2}' | tr -d 'addr:' | head -n 1)
./fastcfs.sh init \
	--dir-path=/usr/local/fastcfs-test/fastdir \
	--dir-server-count=1 \
	--dir-host=$IP  \
	--dir-cluster-port=11011 \
	--dir-service-port=21011 \
	--dir-bind-addr=  \
	--store-path=/usr/local/fastcfs-test/faststore \
	--store-server-count=1 \
	--store-host=$IP  \
	--store-cluster-port=31011 \
	--store-service-port=41011 \
	--store-replica-port=51011 \
	--store-bind-addr= \
	--fuse-path=/usr/local/fastcfs-test/fuse \
	--fuse-mount-point=/usr/local/fastcfs-test/fuse/fuse1

注：--fuse-mount-point为mount到本地的路径，通过这个mount point对FastCFS进行文件存取访问。

FCFS_SHELL_PATH=$(pwd)/build/shell
$FCFS_SHELL_PATH/fastdir-cluster.sh restart
$FCFS_SHELL_PATH/faststore-cluster.sh restart
$FCFS_SHELL_PATH/fuse.sh restart

```

上述操作完成后，通过命令 df -h  可以看到FastCFS挂载的文件目录。

## 6. 联系我们

查看FastCFS相关技术文章，请关注微信公众号：

![微信公众号](images/wechat_subscribe.jpg)


微信交流群：
![微信交流群](images/wechat_group.png)
