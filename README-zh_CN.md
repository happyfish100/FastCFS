# FastCFS

## 1. 关于

FastCFS 是一个高性能分布式存储系统.

## 2. 开发状态

开发中

## 3. 支持的操作系统

* CentOS (版本7.8 或更新)
* Ubuntu (测试中)
* BSD (测试中)

## 4. 依赖

* [libfuse](https://github.com/libfuse/libfuse) (版本 3.9.4 或更新)
    * [Python](https://python.org/) (版本 3.5 或更新)
    * [Ninja](https://ninja-build.org/) (版本 1.7 或更新)
    * [gcc](https://www.gnu.org/software/gcc/) (版本 7.5.0 或更新)
* [libfastcommon](https://github.com/happyfish100/libfastcommon) (版本 commit-c2d8faa)
* [libserverframe](https://github.com/happyfish100/libserverframe) (版本 commit-02adaac)
* [fastDIR](https://github.com/happyfish100/fastDIR) (版本 commit-62cab21)

## 5. 安装

### 5.1. libfastcommon

版本号：version 1.43

```
git clone https://github.com/happyfish100/libfastcommon.git
cd libfastcommon/
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
git clone https://github.com/happyfish100/libserverframe.git
./make.sh
./make.sh install
```

### 5.3. fastDIR

```
git clone https://github.com/happyfish100/fastDIR.git
./make.sh
./make.sh install
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

### 5.4. libfuse for CentOS 7.8

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
git clone https://github.com/happyfish100/faststore.git
cd faststore/
./make.sh
./make.sh install
cp conf/server.conf /etc/fdir/
cp conf/client.conf /etc/fdir/
mkdir /usr/local/faststore
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
