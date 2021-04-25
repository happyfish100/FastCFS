#!/bin/bash

if [ -f /usr/sbin/ifconfig ]; then
  IFCONFIG=/usr/sbin/ifconfig
elif [ -f /sbin/ifconfig ]; then
  IFCONFIG=/sbin/ifconfig
else
  echo "can't find ifconfig command"
  exit
fi

FASTCFS_BASE=/usr/local/fastcfs-test
IP=''
for arg do
  case "$arg" in
    --prefix=*)
      FASTCFS_BASE=${arg#--prefix=}
    ;;
   --ip=*)
      IP=${arg#--ip=}
    ;;
  esac
done

FUSE_MOUNT_POINT=$FASTCFS_BASE/fuse/fuse1
mkdir -p $FUSE_MOUNT_POINT || exit

if [ -z "$IP" ]; then
  CMD="$IFCONFIG -a | grep -w inet | grep -v 127.0.0.1 | awk '{print \$2}' | tr -d 'addr:'"
  IPS=$(sh -c "$CMD")
  if [ -z "$IPS" ]; then
    echo "can't find ip address"
    exit
  fi

  IP_COUNT=$(echo "$IPS" | wc -l)
  if [ $IP_COUNT -eq 1 ]; then
    IP=$(echo "$IPS" | head -n 1)
  else
    while true; do
      echo ''
      i=0
      for IP in $IPS; do
        ((i++))
        echo "${i}. $IP"
      done
      printf "please choice IP number [1 - %d], q for quit: " $IP_COUNT
      read index
      if [ "x$index" = 'xq' ]; then
        exit
      fi
      IP=$(echo "$IPS" | head -n $index | tail -n 1)
      if [ ! -z "$IP" ]; then
         echo "use local IP: $IP"
         echo ''
         break
      fi
    done
  fi
fi

./libfuse_setup.sh

echo "just for FastCFS demo: 1 fdir instance and 1 fstore instance"
./fastcfs.sh pull
./fastcfs.sh makeinstall
./fastcfs.sh init \
  --auth-path=$FASTCFS_BASE/auth \
	--auth-server-count=1 \
	--auth-host=$IP  \
	--auth-cluster-port=61011 \
	--auth-service-port=71011 \
	--auth-bind-addr=  \
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
