# FAQ

## 1. 网络连接异常

当日志出现`Connection refused`或者`Transport endpoint is not connected`之类的异常记录。
请检查防火墙设置。FastCFS 共有9个服务端口,这些对应的端口应该打开：

* fdir
  * 默认集群端口 `11011`
  * 默认服务端口 `11012`
* fvote
  * 默认集群端口 `41011`
  * 默认服务端口 `41012`
* fauth
  * 默认集群端口 `31011`
  * 默认服务端口 `31012`
* fstore
  * 默认集群端口 `21014`
  * 默认副本端口 `21015`
  * 默认服务端口 `21016`


## 2. 服务有没有启动顺序？

有。应按顺序启动`fvote`（可选），`fdir`， `fauth`（可选），`fstore`，`fuseclient`。

## 3. FastCFS支持单盘数据恢复吗？

```
支持。
实现机制：从本组的其他服务器上获取数据，恢复指定硬盘（即存储路径）上的所有数据。
使用场景：硬盘坏掉后更换为新盘，或者因某种原因重新格式化硬盘。
单盘故障恢复后，重启fs_serverd时，加上命令行参数 --data-rebuild <store_path>，
其中 store_path为要恢复数据硬盘对应的存储路径。
```

## 4. FastCFS是如何防脑裂的？

```
leader/master选举时采用过半数机制，一个分组的节点数最好为奇数（比如3个）。
配置项为 quorum，默认值为auto，表示节点数为奇数时开启，为偶数时关闭。
详情参见源码目录下的conf/full/cluster.conf

v3.4引入公用选举节点，在偶数节点数（比如双副本）时也可以防脑裂，详情参见配置文档。
```

## 5. fdir和fstore多副本（如3副本）情况下，停机后重启不能写入数据

```
为了保证数据一致性，当停机时间超过配置参数max_shutdown_duration（默认配置300秒），
需要等待本组的其他服务器在线才可以选出master/leader。

以3副本为例，如果3台服务器停机均超过max_shutdown_duration，而有的服务器没有启动，
那么master/leader选举不出来，导致不能写入数据。此时启动fdir_serverd需要加上命令行
参数--force-master-election，启动fs_serverd需要加上命令行参数--force-leader-election
```

## 6. 如何验证多个副本的数据一致性？

```
faststore需要源码编译，修改make.sh，将
CFLAGS='-Wall'
修改为：
CFLAGS='-Wall -DFS_DUMP_SLICE_FOR_DEBUG=32'
启动fs_serverd，数据将导出到 $base_path/data/dump/slice.index，例如：
/opt/fastcfs/fstore/data/dump/slice.index

文件格式：
类型 文件ID block偏移量 slice偏移量 slice长度 数据长度 数据CRC32

可以通过 diff 命令比较同一组服务器下的两个节点导出的slice.index是否相同。

注意：-DFS_DUMP_SLICE_FOR_DEBUG=32 这个编译选项仅供测试环境进行数据一致性验证，请不要在生产环境使用。

```

## 7. fuse客户端通过df看到的空间明显大于存储池配额

这是因为客户端没有启用auth，需要将配置文件/etc/fastcfs/auth/auth.conf中的auth_enabled设置为true，修改后重启fcfs_fused生效。

友情提示：如何配置auth服务请参阅 [认证配置文档](AUTH-zh_CN.md)
