# FastCFS -- a high performance general distributed file system for databases, K8s and VM etc.

English | [Chinese](./README-zh_CN.md)

## 1. About

FastCFS is a general distributed file system with strong consistency, high performance, high availability and supporting 10 billion massive files.
FastCFS can be used as the back-end storage of databases (MySQL, PostgresSQL, Oracle etc.), K8s and NAS.

## 2. Current Version

V3.1.0

## 3. Supported Platforms

* Linux: Kernel version >= 3.10  (Full support)
* MacOS or FreeBSD (Only server side)

## 4. Dependencies

* [libfuse](https://github.com/libfuse/libfuse) (version 3.9.4 or newer)
    * [Python](https://python.org/) (version 3.5 or newer)
    * [Ninja](https://ninja-build.org/) (version 1.7 or newer)
    * [gcc](https://www.gnu.org/software/gcc/) (version 4.7.0 or newer)
* [libfastcommon](https://github.com/happyfish100/libfastcommon) (tag: V1.0.55)
* [libserverframe](https://github.com/happyfish100/libserverframe) (tag: V1.1.12)
* [libdiskallocator](https://github.com/happyfish100/libdiskallocator) (tag: V1.0.1)
* [fastDIR](https://github.com/happyfish100/fastDIR) (tag: V3.1.0)
* [faststore](https://github.com/happyfish100/faststore) (tag: V3.1.0)
* [FastCFS](https://github.com/happyfish100/FastCFS) (tag: V3.1.0)

## 5. Installation

### 5.1 DIY installation

you can use [Cluster Operation Tool](docs/fcfs-ops-tool.md) to deploy FastCFS

step by step please see [INSTALL](docs/INSTALL.md)

recommend to execute libfuse_setup.sh for compiling and installing libfuse

### 5.2 easy installation

libfastcommon, libserverframe, fastDIR, faststore and FastCFS can be compiled, installed and auto configurated by fastcfs.sh

fastcfs.sh can automatically pull or update above six projects codes from GitHub, compile and install according to dependency orders, and automatically generate cluster related configuration files according to the config templates.

```
git clone https://github.com/happyfish100/FastCFS.git; cd FastCFS/
```

fastcfs.sh usage:

```
* install: pull/update codes from gitee, then make and install
* config: copy config files and configure them with local ip
* start | stop | restart: for service processes control
```

one click to build (deploy and run) single node demo environment (MUST run by root):

```
./helloWorld.sh
```

or execute following commands (MUST run by root):

```
./fastcfs.sh install
./fastcfs.sh config --force
./fastcfs.sh restart
```

now you can see the mounted path of FastCFS by the command:

```
df -h /opt/fastcfs/fuse | grep fuse
```

## 6. Benchmark

FastCFS has huge better performance than Ceph: the IOPS ratio of sequential write is 6.x, sequential read is 2.x, random write is about 2.0.

* [FastCFS vs. Ceph benchmark](docs/benchmark.md)

## 7. K8s CSI Driver

[fastcfs-csi](https://github.com/happyfish100/fastcfs-csi)

## 8. Chinese Relative articles

<a href="https://blog.csdn.net/happy_fish100/" target="_blank">CSDN blog</a>

## 9. TODO List

*  [fstore] provide cluster tools for automatic expansion
*  [fstore] hierarchical storage & slice merging: supporting two-level storage, such as SSD + HDD
*  [fdir & fstore] binlog deduplication (fdir binlog, fstore replica & slice binlog)
*  [fstore] after the machine recovery, the data masters should be rebalanced

## 10. Contact us

email: 384681(at)qq(dot)com

WeChat subscription: search "fastdfs" for the related articles (Chinese Only)
