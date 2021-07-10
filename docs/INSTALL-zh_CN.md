
## 一、fastcfs.sh 脚本统一安装

通过执行fastcfs.sh脚本，可自动从gitee仓库拉取或更新五个仓库代码，按照依赖顺序进行编译、安装，并能根据配置文件模版自动生成集群相关配置文件。

fastcfs.sh 命令参数说明：
```
* install: 安装程序包
* config: 复制配置文件并自动配置为单节点
* start | stop | restart: 服务进程控制
```

或执行如下命令（需要root身份执行）：
```
./fastcfs.sh install
./fastcfs.sh config
./fastcfs.sh restart
```

通过 df 命令查看FastCFS挂载的文件目录：
```
df -h /opt/fastcfs/fuse | grep fuse
```

以上命令执行成功，你就可以通过本地目录 /opt/fastcfs/fuse 访问FastCFS了。


## 二、DIY编译和安装

### 1. Linux下需要安装libaio devel包

对于 CentOS或REHL，执行：
```
yum install libaio-devel -y
```

对于 Unbuntu或Debian，执行：
```
apt install libaio-dev -y
```

其他Linux系统，需要编译安装 libaio库


### 2. libfastcommon

```
git clone https://gitee.com/fastdfs100/libfastcommon.git; cd libfastcommon/
git checkout master
./make.sh clean && ./make.sh && ./make.sh install
```

默认安装目录：
```
/usr/lib64
/usr/lib
/usr/include/fastcommon
```

### 3. libserverframe

```
git clone https://gitee.com/fastdfs100/libserverframe.git; cd libserverframe/
./make.sh clean && ./make.sh && ./make.sh install
```

### 4. Auth client

```
git clone https://gitee.com/fastdfs100/FastCFS.git; cd FastCFS/
./make.sh clean && ./make.sh --module=auth_client && ./make.sh --module=auth_client install
```

### 5. fastDIR

```
git clone https://gitee.com/fastdfs100/fastDIR.git; cd fastDIR/
./make.sh clean && ./make.sh && ./make.sh install
mkdir -p /etc/fastcfs/fdir/
cp conf/*.conf /etc/fastcfs/fdir/
```

### 6. faststore

```
git clone https://gitee.com/fastdfs100/faststore.git; cd faststore/
./make.sh clean && ./make.sh && ./make.sh install
mkdir -p /etc/fastcfs/fstore/
cp conf/*.conf /etc/fastcfs/fstore/
```


### 7. libfuse

libfuse 编译依赖比较复杂，建议使用脚本libfuse_setup.sh一键编译和安装。或者执行如下步骤DIY：

构建libfuse需要先安装meson和ninja。安装meson和ninja需要python3.5及以上版本。

##### python安装

包名：python3  python3-pip

Ubuntu下安装命令：
```
apt install python3 python3-pip -y
```

CentOS下安装命令：
```
yum install python3 python3-pip -y
```

##### meson 和 ninja 安装

```
pip3 install meson
pip3 install ninja
```

##### gcc安装

Ubuntu下安装命令：
```
apt install gcc g++ -y
```

CentOS下安装命令：
```
yum install gcc gcc-c++ -y
```

##### libfuse安装

```
git clone https://gitee.com/mirrors/libfuse.git; cd libfuse/
git checkout fuse-3.10.1
mkdir build/; cd build/
meson ..
meson configure -D prefix=/usr
meson configure -D examples=false
ninja && ninja install
sed -i 's/#user_allow_other/user_allow_other/g' /etc/fuse.conf
```

### 8. FastCFS

进入之前clone下来的FastCFS目录，然后执行：
```
./make.sh --exclude=auth_client clean && ./make.sh --exclude=auth_client && ./make.sh --exclude=auth_client install
mkdir -p /etc/fastcfs/fcfs/
mkdir -p /etc/fastcfs/auth/
cp conf/*.conf /etc/fastcfs/fcfs/
cp -R src/auth/conf/* /etc/fastcfs/auth/
```

配置参见 [配置文档](CONFIGURE-zh_CN.md)
