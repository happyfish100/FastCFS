# FastCFS -- a high performance distributed file system for databases, K8s and VM etc.

Chinese language please click: [README in Chinese](README-zh_CN.md)

## 1. About

FastCFS is a block based standard distributed file system which can be used as the back-end storage of databases (MySQL, PostgresSQL, Oracle etc.), K8s and virtual machines such as KVM.

## 2. Current Version

V2.0.1

## 3. Supported Platforms

* Linux: Kernel version >= 3.10  (Full support)
* MacOS or FreeBSD (Only server side)

## 4. Dependencies

* [libfuse](https://github.com/libfuse/libfuse) (version 3.9.4 or newer)
    * [Python](https://python.org/) (version 3.5 or newer)
    * [Ninja](https://ninja-build.org/) (version 1.7 or newer)
    * [gcc](https://www.gnu.org/software/gcc/) (version 4.7.0 or newer)
* [libfastcommon](https://github.com/happyfish100/libfastcommon) (tag: V1.0.50)
* [libserverframe](https://github.com/happyfish100/libserverframe) (tag: V1.1.7)
* [fastDIR](https://github.com/happyfish100/fastDIR) (tag: V2.0.1)
* [faststore](https://github.com/happyfish100/faststore) (tag: V2.0.1)
* [FastCFS](https://github.com/happyfish100/FastCFS) (tag: V2.0.1)

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

```
* setup: pull/update codes from gitee, then make and install
* config: copy config files and configure them with local ip
* start | stop | restart: for service processes control
```

one click to build (deploy and run) demo environment (MUST by root):

```
./helloWorld.sh
```

or execute following commands (MUST by root):

```
./fastcfs.sh setup
./fastcfs.sh config
./fastcfs.sh restart
```

now you can see the mounted path of FastCFS by the command:

```
df -h /opt/fastcfs/fuse | grep fuse
```

## 6. Contact us

email: 384681(at)qq(dot)com

WeChat subscription: search "fastdfs" for the related articles (Chinese Only)
