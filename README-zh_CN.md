# FastCFS -- 可以跑数据库的高性能分布式文件系统

## 1. 关于

FastCFS 是一款基于块存储的高性能分布式文件系统，可以作为MySQL、PostgresSQL、Oracle等数据库的后端存储。

## 2. 开发状态

V1.0.0 Beta

## 3. 支持的操作系统

* Linux: Kernel version >= 3.10 （完全支持） 
* MacOS or FreeBSD  （仅支持服务端，不支持FUSE）

## 4. 依赖

* [libfuse](https://github.com/libfuse/libfuse) (版本 3.9.4 或更高版本)
    * [Python](https://python.org/) (版本 3.5 或更高版本)
    * [Ninja](https://ninja-build.org/) (版本 1.7 或更高版本)
    * [gcc](https://www.gnu.org/software/gcc/) (版本 7.5.0 或更高版本)
* [libfastcommon](https://github.com/happyfish100/libfastcommon) (tag: V1.0.44)
* [libserverframe](https://github.com/happyfish100/libserverframe) (tag: V1.1.0)
* [fastDIR](https://github.com/happyfish100/fastDIR) (tag: V1.0.0)
* [faststore](https://github.com/happyfish100/faststore) (tag: V1.0.0)
* [FastCFS](https://github.com/happyfish100/FastCFS) (tag: V1.0.0)

## 5. 安装

libfastcommon、libserverframe、fastDIR、faststore和FastCFS 五个安装包可采用 fastcfs.sh 脚本统一安装配置，也可以按照5.1 - 5.6部分独立安装配置。

*统一安装方式*

```
git clone https://github.com/happyfish100/FastCFS.git && cd FastCFS/
```

通过执行fastcfs.sh脚本，可自动从github仓库拉取或更新五个仓库代码，按照依赖顺序进行编译、安装，并能根据配置文件模版自动生成集群相关配置文件。

fastcfs.sh 命令参数说明：

* pull -- 从github拉取或更新代码库（拉取到本地build目录）
* makeinstall -- 顺序编译、安装代码库（make && make install）
* init -- 初始化集群目录、配置文件（已存在不会重新生成）
* clean -- 清除已编译程序文件（相当于make clean）


一键搭建(包括部署和运行)demo环境：


```
./helloWorld.sh
```

或执行如下命令：

```
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

注：--fuse-mount-point为mount到本地的路径，通过这个mount point实现对FastCFS的文件存取访问。

FCFS_SHELL_PATH=$(pwd)/build/shell
$FCFS_SHELL_PATH/fastdir-cluster.sh restart
$FCFS_SHELL_PATH/faststore-cluster.sh restart
$FCFS_SHELL_PATH/fuse.sh restart

```

### 5.1. libfastcommon

```
git clone https://github.com/happyfish100/libfastcommon.git && cd libfastcommon/
git checkout master
./make.sh clean && ./make.sh && ./make.sh install
```

默认安装目录：
```
/usr/lib64
/usr/lib
/usr/include/fastcommon
```

### 5.2. libserverframe

```
git clone https://github.com/happyfish100/libserverframe.git && cd libserverframe/
./make.sh clean && ./make.sh && ./make.sh install
```

### 5.3. fastDIR

```
git clone https://github.com/happyfish100/fastDIR.git && cd fastDIR/
./make.sh clean && ./make.sh && ./make.sh install
```

编译警告信息：

```
perl: warning: Setting locale failed.
perl: warning: Please check that your locale settings:
	LANGUAGE = (unset),
	LC_ALL = (unset),
	LC_CTYPE = "UTF-8",
	LANG = "en_US.UTF-8"
    are supported and installed on your system.
perl: warning: Falling back to the standard locale ("C").
```

可以修改/etc/profile，增加export LC_ALL=C解决上这个警告（记得刷新当前session：. /etc/profile）
头文件安装成功，其他目录创建失败。

### 5.4. libfuse for CentOS 7.x

构建libfuse需要先安装Meson和Ninja。安装Meson和Ninja需要Python3.5以上。

##### ssl安装

```
yum -y install zlib zlib-devel
yum -y install bzip2 bzip2-devel ncurses openssl openssl-devel openssl-static xz lzma xz-devel sqlite sqlite-devel gdbm gdbm-devel tk tk-devel libffi-devel

wget -c https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-3.2.0.tar.gz
tar -xzvf libressl-3.2.0.tar.gz
cd libressl-3.2.0/
./configure
make
make install
```

新建或修改 /etc/ld.so.conf.d/local.conf 配置文件，添加如下内容：

> /usr/local/lib
  
将 /usr/local/lib 目录加入到库加载目录。

重新加载共享库：

> ldconfig -v

##### Python安装

```
wget -c https://www.python.org/ftp/python/3.8.5/Python-3.8.5.tgz
tar -xzvf Python-3.8.5.tgz
cd Python-3.8.5/
./configure --prefix=/usr/local/py385 --enable-optimizations --with-openssl=/usr/local
make
make test
make install
```

如果找不到ssl库或头文件，需编辑安装文件 Modules/Setup，打开ssl编译注释，并修改SSL目录为：

> SSL=/usr/local

如下所示:

```
# Socket module helper for SSL support; you must comment out the other
# socket line above, and possibly edit the SSL variable:
SSL=/usr/local
_ssl _ssl.c \
        -DUSE_SSL -I$(SSL)/include -I$(SSL)/include/openssl \
        -L$(SSL)/lib -lssl -lcrypto
```

将/usr/local/py385/bin加到PATH中。

升级pip：

> /usr/local/py385/bin/python3 -m pip install --upgrade pip

##### meson和ninja安装

```
python3 -m pip install meson
python3 -m pip install ninja
python3 -m pip install pytest
```

##### gcc安装

安装新版gcc：
(https://mirrors.aliyun.com/gnu/)

```
yum -y install mpfr gmp mpfr-devel gmp-devel glibc-static

wget ftp://ftp.gnu.org/gnu/mpc/mpc-1.0.3.tar.gz
tar -zxvf mpc-1.0.3.tar.gz
cd mpc-1.0.3
./configure
make  && make install
ldconfig -v  # 刷新/etc/ld.so.cache，否则找不到libmpc.so.3

wget -c https://mirrors.aliyun.com/gnu/gcc/gcc-7.5.0/gcc-7.5.0.tar.gz
tar -xzvf gcc-7.5.0.tar.gz
cd gcc-7.5.0/
./configure --disable-multilib
make
make install
```

##### libfuse安装

```
git clone https://github.com/libfuse/libfuse.git
cd libfuse/
git checkout fuse-3.9.4
mkdir build/
cd build/
meson ..
ninja
python3 -m pytest test/
ninja install
```

### 5.5. faststore

```
git clone https://github.com/happyfish100/faststore.git && cd faststore/
./make.sh clean && ./make.sh && ./make.sh install
mkdir /etc/fstore/
cp conf/server.conf conf/client.conf conf/servers.conf conf/cluster.conf conf/storage.conf /etc/fstore/
```

### 5.6. FastCFS

```
git clone https://github.com/happyfish100/FastCFS.git && cd FastCFS/
./make.sh clean && ./make.sh && ./make.sh install
mkdir /etc/fcfs/
cp conf/fuse.conf /etc/fcfs/
```


## 6. 配置

为了更好的控制FastCFS的性能，我们通过各种设置参数为FastCFS提供了高度可配置和可调节的行为。

FastCFS的配置由多个子文件组成，其中一个是入口文件，其他文件用于引用。目录/etc/fstore 是FastCFS配置文件的默认存放位置，但是在单个服务器上安装多个FastCFS实例时，必须为每个实例指定不同的位置。

FastCFS有以下几个配置文件：

* server.conf - 服务器全局参数配置
* cluster.conf - 集群参数配置
* servers.conf - 服务器组参数配置
* storage.conf - 存储参数配置
* client.conf - 客户端使用的配置文件，需引用cluster.conf

### 6.1. server.conf 配置

### 6.2. cluster.conf 配置

### 6.3. servers.conf 配置

### 6.4. storage.conf 配置

### 6.5. client.conf 配置

## 运行

Coming soon.

## License

FastCFS is Open Source software released under the GNU General Public License V3.
