### 1. libfastcommon

```
git clone https://github.com/happyfish100/libfastcommon.git; cd libfastcommon/
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
git clone https://github.com/happyfish100/libserverframe.git; cd libserverframe/
./make.sh clean && ./make.sh && ./make.sh install
```

### 3. fastDIR

```
git clone https://github.com/happyfish100/fastDIR.git; cd fastDIR/
./make.sh clean && ./make.sh && ./make.sh install
```

### 4. faststore

```
git clone https://github.com/happyfish100/faststore.git; cd faststore/
./make.sh clean && ./make.sh && ./make.sh install
mkdir /etc/fstore/
cp conf/server.conf conf/client.conf conf/servers.conf conf/cluster.conf conf/storage.conf /etc/fstore/
```


### 5. libfuse

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
git clone https://github.com/libfuse/libfuse.git
cd libfuse/
git checkout fuse-3.10.1
mkdir build/; cd build/
meson ..
meson configure -D prefix=/usr
meson configure -D examples=false
ninja && ninja install
sed -i 's/#user_allow_other/user_allow_other/g' /etc/fuse.conf
```

### 6. FastCFS

```
git clone https://github.com/happyfish100/FastCFS.git; cd FastCFS/
./make.sh clean && ./make.sh && ./make.sh install
mkdir /etc/fcfs/
cp conf/fuse.conf /etc/fcfs/
```

配置参见 [配置文档](CONFIGURE-zh_CN.md)
