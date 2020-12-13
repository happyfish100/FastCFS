#!/bin/bash
echo "首次编译完成后，搭建简单的测试环境，1节点dir，1节点store"
export IP=`ifconfig -a|grep inet|grep -v 127.0.0.1|grep -v inet6|awk '{print $2}'|tr -d "addr:"`
./fastcfs.sh init \
 --dir-path=/dev/shm/fastdir \
 --dir-cluster-size=1 \
 --dir-host=$IP \
 --dir-cluster-port=11011 \
 --dir-service-port=12011 \
 --store-path=/dev/shm/faststore \
 --store-cluster-size=1 \
 --store-host=$IP \
 --store-cluster-port=21011 \
 --store-service-port=22011 \
 --store-replica-port=23011 \
 --fuse-path=/dev/shm/fuse \
 --fuse-mount-point=/usr/local/cfs
mkdir -p /usr/local/cfs
export CFSDEV=$(pwd)
export PATH=$PATH:$CFSDEV/build/shell
echo "正确初始化完成后，执行如下操作，进行简单测试......."
echo "启动脚本位于：$CFSDEV/build/shell"
echo "fastdir-cluster.sh start #启动元数据服务"
fastdir-cluster.sh start
echo "faststore-cluster.sh start #启动存储服务"
faststore-cluster.sh start
echo "fuse.sh start #挂载fastCFS文件系统到/usr/local/cfs"
fuse.sh start
