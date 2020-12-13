# FastCFS

## 1. About

FastCFS is a high performance distributed file system.
FastCFS can be used as the database back-end storage.

## 2. Development Status

v1.00 Beta

## 3. Supported Platform

* Linux: Kernel version >= 3.10  (Full support)
* MacOS or FreeBSD (Only server side)

## 4. Dependencies

* [libfuse](https://github.com/libfuse/libfuse) (version 3.9.4 or newer)
    * [Python](https://python.org/) (version 3.5 or newer)
    * [Ninja](https://ninja-build.org/) (version 1.7 or newer)
    * [gcc](https://www.gnu.org/software/gcc/) (version 7.5.0 or newer)
* [libfastcommon](https://github.com/happyfish100/libfastcommon) (master)
* [libserverframe](https://github.com/happyfish100/libserverframe) (master)
* [fastDIR](https://github.com/happyfish100/fastDIR) (master)
* [faststore](https://github.com/happyfish100/faststore) (master)
* [FastCFS](https://github.com/happyfish100/FastCFS) (master)

## 5. Installation

libfastcommon、libserverframe、fastDIR、faststore和FastCFS 五个安装包可采用 fastcfs.sh 脚本统一安装配置，也可以按照5.1-5.5部分独立安装配置。

*统一安装方式*

通过执行fastcfs.sh脚本，可自动从github仓库拉取或更新五个仓库代码，按照依赖顺序进行编译、安装，并能根据配置文件模版自动生成集群相关配置文件。

命令参数说明：

* pull -- 从github拉取或更新代码库（拉取到本地build目录）
* makeinstall -- 顺序编译、安装代码库
* init -- 初始化集群目录、配置文件
* clean -- 清除已编译程序文件

集群初始化参数说明：


```
./fastcfs.sh pull
./fastcfs.sh makeinstall
./fastcfs.sh init \
	--dir-path=/usr/local/fastcfs-test/fastdir \
	--dir-cluster-size=3 \
	--dir-host=127.0.0.1 \
	--dir-cluster-port=11011 \
	--dir-service-port=21011 \
	--dir-bind-addr=127.0.0.1 \
	--store-path=/usr/local/fastcfs-test/faststore \
	--store-cluster-size=3 \
	--store-host=127.0.0.1 \
	--store-cluster-port=31011 \
	--store-service-port=41011 \
	--store-replica-port=51011 \
	--store-bind-addr=127.0.0.1 \
	--fuse-path=/usr/local/fastcfs-test/fuse \
	--fuse-mount-point=/usr/local/fastcfs-test/fuse/fuse1
```


### 5.1. libfastcommon

版本号：version 1.44

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

可以修改/etc/profile，增加以下环境变量解决上这个警告（记得刷新当前session：. /etc/profile）：

```
export LC_CTYPE=zh_CN.UTF-8
export LC_ALL=zh_CN.UTF-8
```

/etc/locale.conf文件修改：

> LANG="zh_CN.UTF-8"

### 5.4. faststore

```
git clone https://github.com/happyfish100/faststore.git
cd faststore/
./make.sh
./make.sh install
cp conf/server.conf /etc/fdir/
cp conf/client.conf /etc/fdir/
mkdir /usr/local/faststore
```

### 5.5. faststore

```
git clone https://github.com/happyfish100/FastCFS.git
cd FastCFS/
./make.sh
./make.sh install
cp conf/fuse.conf /etc/fcfs/
```


## 6. Configuration

In order to control FastCFS's performance, we provide highly configurable and tunable behavior for FastCFS via various settings.

FastCFS's configuration comprise multiple subfiles, one entry file and others use to reference. The directory /etc/fstore is the default location for FastCFS's config files, but when install multiple FastCFS instances on a single server, you must specify distinct location for each one.

FastCFS has the following config files:

* server.conf - Server global properties config
* cluster.conf - Cluster properties config
* servers.conf - Server group properties config
* storage.conf - Storage properties config
* client.conf - Use with client，need to reference cluster.conf

### 6.1. Configure server.conf 

> base_path = /path/to/faststore

base_path设置的目录必须是已存在。

### 6.2. cluster.conf configure

### 6.3. servers.conf configure

各服务器的host参数设置的主机名必须是有效的，能够根据主机名获取到IP地址，或者直接设置成IP地址。

### 6.4. storage.conf configure

> store-path-1 = /path/to/faststore/data1
> store-path-2 = /path/to/faststore/data2
> write-cache-path-1 = /path/to/faststore/cache

### 6.5. fuse.conf configure

fs_fused启动时，可能出现找不到动态库的错误提示：

```
fs_fused: error while loading shared libraries: libfuse3.so.3: cannot open shared object file: No such file or directory
```

libfuse3.so.3默认安装在/usr/local/lib64/目录下，fs_fused编译时从/usr/lib64下连接，导致无法加载动态连接库，可以在该目录下做一个软连接:

> ln -s /usr/local/lib64/libfuse3.so.3 /usr/lib64/libfuse3.so.3  


### 6.6. client.conf configure

## Running

Coming soon.

## License

FastCFS is Open Source software released under the GNU General Public License V3.
