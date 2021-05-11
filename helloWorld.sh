#!/bin/bash

echo "just for FastCFS demo: 1 fdir instance and 1 fstore instance"

mounted_path=/opt/fastcfs/fuse

./fastcfs.sh setup
./fastcfs.sh config $1
./fastcfs.sh restart

echo "waiting services ready ..."

sleep 3
if [ -d $mounted_path ]; then
  df -h $mounted_path
  echo ""
  echo "the mounted path is: $mounted_path"
fi

echo "have a nice day!"
