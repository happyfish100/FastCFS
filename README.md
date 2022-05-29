# FastCFS -- a high performance general distributed file system for databases, K8s and KVM etc.

English | [Chinese](README-zh_CN.md)

## 1. About

FastCFS is a general distributed file system with strong consistency, high performance, high availability and supporting 10 billion massive files.
FastCFS can be used as the back-end storage of databases (MySQL, PostgresSQL, Oracle etc.), K8s, KVM, FTP, SMB and NFS.

### Main Features

* High performance on the premise of strong data consistency: performance is more better than ceph
* Fully compatible with POSIX, supporting file lock and 10 billion massive files
* High availability: no single point & automatic failover
* Easy to use:
    * simple and efficient architecture and native implementation
    * independent of third-party components
    * built-in operation and maintenance tools
* Strong random write performance: FCFS allocates space based on the trunk file, converts random writes from the client to sequential writes

### Classical Application Scene

* **Database**: supports two storage methods (conventional exclusive data and high-level shared data) and database cloudification
* **File Storage**: documents, pictures, videos, etc., FastCFS is easier to integrate with general software than the object storage
* **Unified Storage**: database and file storage share a storage cluster, which significantly improves the utilization of storage resources
* **High Performance Computing**: FastCFS with high reliability and high performance is naturally suitable for the HPC scene
* **Video monitoring**: smooth writing for multi-channel videos with HDD such as SATA because FastCFS uses sequential writing approach


## 2. Current Version

V3.3.0

## 3. Supported Platforms

* Linux: Kernel version >= 3.10  (Full support, >=4.18 is recommended)
* MacOS or FreeBSD (Only server side)

## 4. Dependencies

* [libfuse](https://github.com/libfuse/libfuse) (version 3.9.4 or newer)
    * [Python](https://python.org/) (version 3.5 or newer)
    * [Ninja](https://ninja-build.org/) (version 1.7 or newer)
    * [gcc](https://www.gnu.org/software/gcc/) (version 4.7.0 or newer)
* [libfastcommon](https://github.com/happyfish100/libfastcommon) (tag: V1.0.57)
* [libserverframe](https://github.com/happyfish100/libserverframe) (tag: V1.1.14)
* [libdiskallocator](https://github.com/happyfish100/libdiskallocator) (tag: V1.0.3)
* [fastDIR](https://github.com/happyfish100/fastDIR) (tag: V3.3.0)
* [faststore](https://github.com/happyfish100/faststore) (tag: V3.3.0)
* [FastCFS](https://github.com/happyfish100/FastCFS) (tag: V3.3.0)

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

*  [fstore] data recovery after single disk fault (done)
*  [fstore] after the machine recovery, the data masters should be rebalanced (done)
*  [fauth & fdir & fstore] leader election uses  more than half principle to prevent brain-split (done)
*  [fauth & fdir & fstore] import public vote node under 2 copies scene to prevent brain-split (not released)
*  [fdir & fstore] binlog deduplication and historical data deletion (doing)
*  [fstore] hierarchical storage & slice merging: supporting two-level storage, such as SSD + HDD
*  [all] cluster online expansion

## 10. Contact us

email: 384681(at)qq(dot)com

WeChat subscription: search "fastdfs" for the related articles (Chinese Only)
