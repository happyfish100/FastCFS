#!/bin/bash

echo "搭建简单的测试环境，1节点dir，1节点store"
./fastcfs.sh pull
./fastcfs.sh makeinstall

if [ -f /usr/sbin/ifconfig ]; then
  IFCONFIG=/usr/sbin/ifconfig
elif [ -f /sbin/ifconfig ]; then
  IFCONFIG=/sbin/ifconfig
else
  echo "can't find ifconfig command"
  exit
fi

FASTCFS_BASE=/usr/local/fastcfs-test
mkdir -p $FASTCFS_BASE
CMD="$IFCONFIG -a | grep -w inet | grep -v 127.0.0.1 | awk '{print \$2}' | tr -d 'addr:' | head -n 1"
IP=$(sh -c "$CMD")
./fastcfs.sh init \
 --dir-path=$FASTCFS_BASE/fastdir \
 --dir-server-count=1 \
 --dir-host=$IP \
 --dir-cluster-port=11011 \
 --dir-service-port=12011 \
 --store-path=$FASTCFS_BASE/faststore \
 --store-server-count=1 \
 --store-host=$IP \
 --store-cluster-port=21011 \
 --store-service-port=22011 \
 --store-replica-port=23011 \
 --fuse-path=$FASTCFS_BASE/fuse \
 --fuse-mount-point=$FASTCFS_BASE/fuse/fuse1

FCFS_SHELL_PATH=$(pwd)/build/shell
echo "正确初始化完成后，执行如下操作，进行简单测试......."
echo "启动脚本位于：$FCFS_SHELL_PATH"

echo "$FCFS_SHELL_PATH/fastdir-cluster.sh restart #启动元数据服务"
$FCFS_SHELL_PATH/fastdir-cluster.sh restart

echo "$FCFS_SHELL_PATH/faststore-cluster.sh restart #启动存储服务"
$FCFS_SHELL_PATH/faststore-cluster.sh restart

echo "$FCFS_SHELL_PATH/fuse.sh restart #挂载fastCFS文件系统"
$FCFS_SHELL_PATH/fuse.sh restart
