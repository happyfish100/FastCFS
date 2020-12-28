#!/bin/bash

./libfuse_setup.sh

echo "just for FastCFS demo: 1 fdir instance and 1 fstore instance"
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
for arg do
  case "$arg" in
    --prefix=*)
      FASTCFS_BASE=${arg#--prefix=}
    ;;
  esac
done

FUSE_MOUNT_POINT=$FASTCFS_BASE/fuse/fuse1
mkdir -p $FUSE_MOUNT_POINT || exit
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
  --fuse-mount-point=$FUSE_MOUNT_POINT

FCFS_SHELL_PATH=$(pwd)/build/shell
echo "the startup scripts at $FCFS_SHELL_PATH"

echo "$FCFS_SHELL_PATH/fastdir-cluster.sh restart # start the metadata service"
$FCFS_SHELL_PATH/fastdir-cluster.sh restart

echo "$FCFS_SHELL_PATH/faststore-cluster.sh restart # start the store service"
$FCFS_SHELL_PATH/faststore-cluster.sh restart

echo "$FCFS_SHELL_PATH/fuse.sh restart # mount fastCFS to $FUSE_MOUNT_POINT"
$FCFS_SHELL_PATH/fuse.sh restart
