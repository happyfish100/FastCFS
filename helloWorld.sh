#!/bin/bash

echo "just for FastCFS demo: 1 fdir instance and 1 fstore instance"

mounted_path=/opt/fastcfs/fuse

./fastcfs.sh install || exit 1
./fastcfs.sh config --force
./fastcfs.sh restart

echo "waiting services ready ..."

sleep 3
if [ -d $mounted_path ]; then
  df -h $mounted_path | grep fuse
  if [ $? -eq 0 ]; then
    echo ""
    echo "the mounted path is: $mounted_path"
    echo "have a nice day!"
    exit 0
  fi
fi

tail_cmd='tail -n 20 /opt/fastcfs/fcfs/logs/fcfs_fused.log'
echo 'some mistake happen!'
echo $tail_cmd
sh -c "$tail_cmd"
exit 1
