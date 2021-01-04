# FastCFS -- a high performance cloud native distributed file system for databases

## 1. About

FastCFS is a block based standard file system which can be used as the back-end storage of databases, such as MySQL, PostgresSQL, Oracle etc.

## 2. Current Version

V1.1.1

## 3. Supported Platforms

* Linux: Kernel version >= 3.10  (Full support)
* MacOS or FreeBSD (Only server side)

## 4. Dependencies

* [libfuse](https://github.com/libfuse/libfuse) (version 3.9.4 or newer)
    * [Python](https://python.org/) (version 3.5 or newer)
    * [Ninja](https://ninja-build.org/) (version 1.7 or newer)
    * [gcc](https://www.gnu.org/software/gcc/) (version 4.7.0 or newer)
* [libfastcommon](https://github.com/happyfish100/libfastcommon) (tag: V1.0.46)
* [libserverframe](https://github.com/happyfish100/libserverframe) (tag: V1.1.2)
* [fastDIR](https://github.com/happyfish100/fastDIR) (tag: V1.1.1)
* [faststore](https://github.com/happyfish100/faststore) (tag: V1.1.1)
* [FastCFS](https://github.com/happyfish100/FastCFS) (tag: V1.1.1)

## 5. Installation

### 5.1 DIY installation (step by step)

please see [INSTALL](INSTALL.md)

recommend to execute libfuse_setup.sh for compiling and installing libfuse

### 5.2 easy installation

libfastcommon, libserverframe, fastDIR, faststore and FastCFS can be compiled, installed and auto configurated by fastcfs.sh

fastcfs.sh can automatically pull or update above five projects codes from GitHub, compile and install according to dependency orders, and automatically generate cluster related configuration files according to the config templates.

```
git clone https://github.com/happyfish100/FastCFS.git; cd FastCFS/
```

fastcfs.sh usage:

* pull -- pull or update codes from GitHub to subdir named "build"
* makeinstall -- compile and install in order (make clean && make && make install)
* init -- initialize the cluster directory and config files (NOT regenerated when already exist)
* clean -- clean the compiled program files (make clean)


one click to build (deploy and run) demo environment (MUST by root):

```
./helloWorld.sh --prefix=/usr/local/fastcfs-test
```

or execute following commands (MUST by root):

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

Note:
   * you should set IP variable manually with multi local ip addresses. show local ip list:
     ifconfig -a | grep -w inet | grep -v 127.0.0.1 | awk '{print $2}' | tr -d 'addr:'
   * --fuse-mount-point is the path mounted to local, the files in FastCFS can be accessed by this local path

FCFS_SHELL_PATH=$(pwd)/build/shell
$FCFS_SHELL_PATH/fastdir-cluster.sh restart
$FCFS_SHELL_PATH/faststore-cluster.sh restart
$FCFS_SHELL_PATH/fuse.sh restart

```

now you can see the mounted path of FastCFS by the command:

```
df -h
```

## 6. Contact us

email: 384681(at)qq(dot)com

WeChat subscription: search "fastdfs" for the related articles (Chinese Only)
