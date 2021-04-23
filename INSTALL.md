### 1. libfastcommon

```
git clone git@github.com:happyfish100/libfastcommon.git || git clone https://github.com/happyfish100/libfastcommon.git
cd libfastcommon/
git checkout master
./make.sh clean && ./make.sh && ./make.sh install
```

default install directories:
```
/usr/lib64
/usr/lib
/usr/include/fastcommon
```

### 2. libserverframe

```
git clone git@github.com:happyfish100/libserverframe.git || git clone https://github.com/happyfish100/libserverframe.git
cd libserverframe/
./make.sh clean && ./make.sh && ./make.sh install
```

### 3. Auth client

```
git clone git@github.com:happyfish100/FastCFS.git || git clone https://github.com/happyfish100/FastCFS.git
cd FastCFS/
./make.sh clean && ./make.sh --module=auth_client && ./make.sh --module=auth_client install
```

### 4. fastDIR

```
git clone git@github.com:happyfish100/fastDIR.git || git clone https://github.com/happyfish100/fastDIR.git
cd fastDIR/
./make.sh clean && ./make.sh && ./make.sh install
mkdir -p /etc/fastcfs/fdir/
cp conf/*.conf /etc/fastcfs/fdir/
```

### 5. faststore

```
git clone git@github.com:happyfish100/faststore.git || git clone https://github.com/happyfish100/faststore.git
cd faststore/
./make.sh clean && ./make.sh && ./make.sh install
mkdir -p /etc/fastcfs/fstore/
cp conf/*.conf /etc/fastcfs/fstore/
```


### 6. libfuse

libfuse requires meson and ninja which require python3.5 or higher version

##### python

packages: python3  python3-pip

Ubuntu:
```
apt install python3 python3-pip -y
```

CentOS:
```
yum install python3 python3-pip -y
```

##### meson and ninja

```
pip3 install meson
pip3 install ninja
```

##### gcc

Ubuntu:
```
apt install gcc g++ -y
```

CentOS:
```
yum install gcc gcc-c++ -y
```

##### libfuse

```
git clone git@github.com:libfuse/libfuse.git || git clone https://github.com/libfuse/libfuse.git
cd libfuse/
git checkout fuse-3.10.1
mkdir build/; cd build/
meson ..
meson configure -D prefix=/usr
meson configure -D examples=false
ninja && ninja install
sed -i 's/#user_allow_other/user_allow_other/g' /etc/fuse.conf
```

### 7. FastCFS

```
cd FastCFS/
./make.sh clean && ./make.sh && ./make.sh install
mkdir -p /etc/fastcfs/fcfs/
mkdir -p /etc/fastcfs/auth/
cp conf/*.conf /etc/fastcfs/fcfs/
cp -R src/auth/conf/* /etc/fastcfs/auth/
```
