# 快速体验FastCFS

以CentOS 8为例，用单个服务节点，以一键安装的方式体验下FastCFS。 此方式仅供体验使用，不适合在生产环境使用. 

## 1. 获得最新的代码

```
git clone https://gitee.com/fastdfs100/FastCFS.git
```

## 2. 安装

涉及到libfuse的安装更新以及在/usr/local下创建目录，请务必使用sudo或者root的方式安装。

```
cd FastCFS/
sudo ./helloWorld.sh
```

安装过程中会有进度提示信息，全自动进行，当看到以下提示时说明安装成功了。

```
INFO: Copy file fuse.conf to /etc/fastcfs/fcfs/
waiting services ready ...
/dev/fuse        38G     0   38G   0% /opt/fastcfs/fuse

the mounted path is: /opt/fastcfs/fuse
have a nice day!
```

## 3. 验证：查看安装的文件目录

安装成功后，通过df -h 可以看到新增一个/dev/fuse的设备，默认容量与你的磁盘容量有关，非固定值。比如我的是38G. 

```
Filesystem           Size  Used Avail Use% Mounted on
devtmpfs             189M     0  189M   0% /dev
tmpfs                219M     0  219M   0% /dev/shm
tmpfs                219M  6.6M  212M   4% /run
tmpfs                219M     0  219M   0% /sys/fs/cgroup
/dev/mapper/cs-root   47G  5.9G   42G  13% /
/dev/sda1           1014M  242M  773M  24% /boot
tmpfs                 44M  4.0K   44M   1% /run/user/1000
/dev/fuse             38G     0   38G   0% /opt/fastcfs/fuse
```

## 4. 使用：在新挂载的目录/opt/fastcfs/fuse 中，可以进行标准的文件创建/编辑/删除操作

```
[first@192.168.126.50 /opt/fastcfs/fuse]
$ pwd
/opt/fastcfs/fuse
[first@192.168.126.50 /opt/fastcfs/fuse]
$ touch abc  // 创建文件
[first@192.168.126.50 /opt/fastcfs/fuse]
$ ll
total 0
-rw-rw-r--. 1 first first 0 May 14 17:06 abc
[first@192.168.126.50 /opt/fastcfs/fuse]
$ rm abc  // 删除文件
[first@192.168.126.50 /opt/fastcfs/fuse]
$ mkdir -p d1/d2/d3  // 创建目录
[first@192.168.126.50 /opt/fastcfs/fuse]
$ ll
total 0
drwxrwxr-x. 2 first first 0 May 14 17:06 d1
[first@192.168.126.50 /opt/fastcfs/fuse]
$ touch d1/d2/d3/123 // 创建文件
[first@192.168.126.50 /opt/fastcfs/fuse]
$ ll d1/d2/d3/123 
-rw-rw-r--. 1 first first 0 May 14 17:06 d1/d2/d3/123
[first@192.168.126.50 /opt/fastcfs/fuse]
$ rm -rf d1 // 删除目录
[first@192.168.126.50 /opt/fastcfs/fuse]
$ ll
total 0
```
 
## 5. 停止服务

使用fashcfs.sh为管理脚本，可以停止、重启服务:

```
[first@192.168.126.50 ~/FastCFS]
$ sudo ./fastcfs.sh stop
[sudo] password for first:
waiting for pid [2364] exit ...
pid [2364] exit.
waiting for pid [2379] exit ...
pid [2379] exit.
waiting for pid [2383] exit ...
pid [2383] exit.
waiting for pid [2400] exit ...
pid [2400] exit.
```

## 6. 移除安装包恢复初始环境

```
sudo yum remove FastCFS-auth-server FastCFS-fused fastDIR-server faststore-server -y
sudo yum remove fastDIR-config faststore-config FastCFS-auth-config FastCFS-fuse-config -y
sudo rm -rf /etc/fastcfs /opt/fastcfs /opt/faststore
```
