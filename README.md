# FastCFS

## 1. About

FastCFS is a high performance distributed file storage system.

## 2. Development Status

Developing

## 3. Supported Platform

* CentOS (version 7.8 or newer)
* Ubuntu (Testing)
* BSD (Testing)

## 4. Dependencies

* [libfuse](https://github.com/libfuse/libfuse) (version 3.9.4 or newer)
    * [Python](https://python.org/) (version 3.5 or newer)
    * [Ninja](https://ninja-build.org/) (version 1.7 or newer)
    * [gcc](https://www.gnu.org/software/gcc/) (version 7.5.0 or newer)
* [libfastcommon](https://github.com/happyfish100/libfastcommon) (version commit-c2d8faa)
* [libserverframe](https://github.com/happyfish100/libserverframe) (version commit-02adaac)
* [fastDIR](https://github.com/happyfish100/fastDIR) (version commit-62cab21)

## 5. Installation

### 5.1. libfastcommon

版本号：version 1.43

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

可以修改/etc/profile，增加export LC_ALL=C解决上这个警告（记得刷新当前session：. /etc/profile）
头文件安装成功，其他目录创建失败。

```
git clone https://github.com/happyfish100/faststore.git
cd faststore/
./make.sh
./make.sh install
cp conf/server.conf /etc/fdir/
cp conf/client.conf /etc/fdir/
mkdir /usr/local/faststore
```

## Configuration

Coming soon.

## Running

Coming soon.

## License

FastCFS is Open Source software released under the GNU General Public License V3.
