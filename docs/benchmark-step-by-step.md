
## FastCFS压测参考文档

服务器之间需要实现免密登录，详情参见：[FastCFS集群部署运维工具介绍](fcfs-ops-tool-zh_CN.md)

### 集群快速部署脚本

在施压服务器（即client机器）上执行如下命令：

```
curl -o /usr/bin/fcfs.sh http://fastcfs.net/fastcfs/ops/fcfs.sh && sudo chmod +x /usr/bin/fcfs.sh;
curl -o /usr/bin/fcfs_conf.sh http://fastcfs.net/fastcfs/ops/fcfs_conf.sh && sudo chmod +x /usr/bin/fcfs_conf.sh;

mkdir -p fastcfs-ops && cd fastcfs-ops;
server_ips='用西文逗号分隔的服务器列表';
client_ip=$(ip addr | grep -w inet | grep -v 127.0.0.1 | awk '{print $2}' | tr -d 'addr:' | awk -F '/' '{print $1}' | head -n 1);
cat > fcfs_conf.settings <<EOF
fastcfs_version=5.0.0

vote_ips=$server_ips
auth_ips=$server_ips
fdir_ips=$server_ips
fstore_group_count=1
fstore_group_1=$server_ips
data_group_count=64
EOF


cat > fcfs.settings <<EOF
fastcfs_version=5.0.0

fuseclient_ips=$client_ip
EOF

fcfs_conf.sh create;
```

修改conf/fdir/cluster.conf 和 conf/fstore/cluster.conf 的配置项 communication，配置示例如下：

```
# config the auth config filename
auth_config_filename = ../auth/auth.conf

# the communication value list:
##  socket: TCP over ethernet or RDMA network
##  rdma: RDMA network only
# default value by the global config with same name
communication = socket
```

修改conf/fcfs/fuse.conf的配置项，配置示例如下：

```
# the mount point (local path) for FUSE
# the local path must exist
mountpoint = /opt/fastcfs/fuse

# if use busy polling for RDMA network
# should set to true for HPC
# default value is false
busy_polling = true


[read-ahead]
# if enable read ahead feature for FastStore
# default value is true
enabled = false
```

首次部署：
```
fcfs.sh setup;
```

修改配置后重新部署：
```
fcfs.sh config;
fcfs.sh restart;
```


### 安装压测工具

CentOS 或者Alibaba Cloud Linux：
```
yum install fio FastCFS-api-tests -y
```

Ubuntu或者Debian：
```
apt install fio fastcfs-api-tests -y
```

### fio压测

```
cd /opt/fastcfs/fuse;
jobnum=2; rw=randread;
out_path=FastCFS-fio/$rw-$jobnum;
log_file=$out_path/fio;
mkdir -p $out_path;

fio --filename_format=test_file.\$jobnum  --direct=1 --rw=$rw --thread --numjobs=$jobnum --iodepth=16 --ioengine=psync  --bs=4k --group_reporting --name=FastCFS --loops=1000 --log_avg_msec=500 --write_bw_log=$log_file --write_lat_log=$log_file --write_iops_log=$log_file --runtime=300 --size=256M
```

### 自带fcfs_beachmark压测
```
fcfs_beachmark -m randread -s 256M -T 4  -t 300 -f  /opt/fastcfs/fuse/test_file
```
