#!/bin/bash
echo "首次编译完成后，搭建简单的测试环境，1节点dir，1节点store"
if [ -f /usr/sbin/ifconfig ]; then
  IFCONFIG=/usr/sbin/ifconfig
elif [ -f /sbin/ifconfig ]; then
  IFCONFIG=/sbin/ifconfig
else
  echo "can't find ifconfig command"
  exit
fi

CMD="$IFCONFIG -a | grep -w inet | grep -v 127.0.0.1 | awk '{print \$2}' | tr -d 'addr:' | head -n 1"
echo $CMD
IP=$(sh -c "$CMD")
./fastcfs.sh init \
 --dir-path=/usr/local/fastdir \
 --dir-cluster-size=1 \
 --dir-host=$IP \
 --dir-cluster-port=11011 \
 --dir-service-port=12011 \
 --store-path=/usr/local/faststore \
 --store-cluster-size=1 \
 --store-host=$IP \
 --store-cluster-port=21011 \
 --store-service-port=22011 \
 --store-replica-port=23011 \
 --fuse-path=/usr/local/fuse \
 --fuse-mount-point=/usr/local/fastcfs
mkdir -p /usr/local/fastcfs
export CFSDEV=$(pwd)
export PATH=$PATH:$CFSDEV/build/shell
echo "正确初始化完成后，执行如下操作，进行简单测试......."
echo "启动脚本位于：$CFSDEV/build/shell"
echo "fastdir-cluster.sh start #启动元数据服务"
fastdir-cluster.sh start
echo "faststore-cluster.sh start #启动存储服务"
faststore-cluster.sh start
echo "fuse.sh start #挂载fastCFS文件系统到/usr/local/fastcfs"
fuse.sh start
