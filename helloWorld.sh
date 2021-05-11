#!/bin/bash

echo "just for FastCFS demo: 1 fdir instance and 1 fstore instance"
./fastcfs.sh setup
./fastcfs.sh config
./fastcfs.sh restart
