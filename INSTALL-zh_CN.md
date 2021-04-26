
## 一、fastcfs.sh 脚本统一安装

通过执行fastcfs.sh脚本，可自动从gitee仓库拉取或更新五个仓库代码，按照依赖顺序进行编译、安装，并能根据配置文件模版自动生成集群相关配置文件。

fastcfs.sh 命令参数说明：

* pull -- 从gitee拉取或更新代码库（拉取到本地build目录）
* makeinstall -- 顺序编译、安装代码库（make clean && make && make install）
* init -- 初始化集群目录、配置文件（已存在不会重新生成）
* clean -- 清除已编译程序文件（相当于make clean）



或执行如下命令（需要root身份执行）：

```
./libfuse_setup.sh
./fastcfs.sh pull
./fastcfs.sh makeinstall
IP=$(ifconfig -a | grep -w inet | grep -v 127.0.0.1 | awk '{print $2}' | tr -d 'addr:' | head -n 1)
./fastcfs.sh init \
	--auth-path=/usr/local/fastcfs-test/auth \
	--auth-server-count=1 \
	--auth-host=$IP  \
	--auth-cluster-port=61011 \
	--auth-service-port=71011 \
	--auth-bind-addr=  \
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

注：
   * 当本机有多个IP地址时，需要手动设置IP 变量为其中的一个IP地址。查看本机IP列表：
     ifconfig -a | grep -w inet | grep -v 127.0.0.1 | awk '{print $2}' | tr -d 'addr:'
   * --fuse-mount-point为mount到本地的路径，通过这个mount point对FastCFS进行文件存取访问。

FCFS_SHELL_PATH=$(pwd)/build/shell
$FCFS_SHELL_PATH/fastdir-cluster.sh restart
$FCFS_SHELL_PATH/faststore-cluster.sh restart
$FCFS_SHELL_PATH/fuse.sh restart

```

## 二、DIY编译和安装

### 1. libfastcommon

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

### 2. libserverframe

```
git clone https://gitee.com/fastdfs100/libserverframe.git; cd libserverframe/
./make.sh clean && ./make.sh && ./make.sh install
```

### 3. Auth client

```
git clone https://gitee.com/fastdfs100/FastCFS.git; cd FastCFS/
./make.sh clean && ./make.sh --module=auth_client && ./make.sh --module=auth_client install
```

### 4. fastDIR

```
git clone https://gitee.com/fastdfs100/fastDIR.git; cd fastDIR/
./make.sh clean && ./make.sh && ./make.sh install
mkdir -p /etc/fastcfs/fdir/
cp conf/*.conf /etc/fastcfs/fdir/
```

### 5. faststore

```
git clone https://gitee.com/fastdfs100/faststore.git; cd faststore/
./make.sh clean && ./make.sh && ./make.sh install
mkdir -p /etc/fastcfs/fstore/
cp conf/*.conf /etc/fastcfs/fstore/
```


### 6. libfuse

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

### 7. FastCFS

进入之前clone下来的FastCFS目录，然后执行：
```
./make.sh --exclude=auth_client clean && ./make.sh --exclude=auth_client && ./make.sh --exclude=auth_client install
mkdir -p /etc/fastcfs/fcfs/
mkdir -p /etc/fastcfs/auth/
cp conf/*.conf /etc/fastcfs/fcfs/
cp -R src/auth/conf/* /etc/fastcfs/auth/
```

配置参见 [配置文档](CONFIGURE-zh_CN.md)
