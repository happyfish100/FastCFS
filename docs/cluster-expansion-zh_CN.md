
# FastCFS集群扩容手册

FastCFS集群三个服务组件：fauth、fdir 和fstore，下面将分别介绍这三个服务组件的扩容方法和步骤。

## 1. fauth（认证服务）

认证服务的用户和权限等数据保存在fdir中，服务采用热备模式，建议部署2 ~ 3个服务器（节点），主节点失效后会自动切换到备用节点。
因认证服务器自身不保存数据，因此可以根据实际需要随时增加或减少认证服务器。

## 2. fdir（目录服务）

FastCFS目录服务用于文件元数据管理，目前只支持一组服务器，使用常规服务器（如64GB内存 + SSD）可以支持百亿级文件。
详情参见[高性能大容量分布式目录服务FastDIR简介](https://my.oschina.net/u/3334339/blog/5405816)

建议将fdir部署在3台服务器上，可以根据实际需要随时增加或减少fdir服务器，fdir将基于binlog自动完成数据同步。

## 3. fstore（数据存储）

fstore 采用分组方式保存文件内容。为了便于扩容，FastCFS引入了数据分组（DG）的概念。

按4MB分块的文件内容（文件块），根据其hash code（文件ID + 文件块偏移量），路由到对应的数据分组。计算公式如下：
```
DGI = HashCode % DGC

DGI：数据分组索引号
HashCode：文件分块哈希值
DGC：数据分组总数
```

数据分组是逻辑或虚拟概念，映射到的物理或实体概念是服务器分组（SG），二者是多对一关系，即：一个服务器分组（SG）可以容纳多个数据分组（DG）。

fstore集群的服务器分组总数，英文缩写为SGC。

DG 和SG映射关系配置在fstore的cluster.conf中，配置示例片段（简洁起见，只配置了一个3副本的SG）：
```
# SGC
server_group_count = 1

# DGC
data_group_count = 256

# 配置SG1
[server-group-1]
server_ids = [1-3]
data_group_ids = [1, 256]

```

fstore集群扩容，可以一次增加一个SG。当集群规模较小（比如SGC小于等于4）时，建议一次扩容一倍（SGC翻番）。

DGC一旦确定就不可更改，除非建立新集群。因此在初始化集群时，需要确定好数据分组总数（DGC），可以根据业务发展规划，充分预估出DGC。

比如根据存储量预估5年后需要的服务器分组数（SGC）为20，为了充分发挥多核性能，每台服务器上跑32个数据分组（DG），DGC为20 * 32 = 640，按2次幂向上对齐，最终配置为1024。

* 友情提示：建议生产环境DGC至少配置为256。

fstore在线扩容分为两个步骤修改cluster.conf：
1. 增加扩容的SG 和迁移过去的DG映射；
2. 新增的SG自动同步完成后，将原有SG迁移出去的DG映射删除。

上述两个步骤将cluster.conf修改完成后，都需要将cluster.conf分发到fstore集群和fuseclient，然后重启fstore集群和fuseclient。推荐重启步骤如下：
```
 1. 停止fuseclient
 2. 重启fstore集群
 3. 启动fuseclient
```

将上述配置示例的1个SG扩容为2个SG（均采用3副本），我们如何调整cluster.conf：

### 步骤1
修改后的cluster.conf内容片段如下：

```
# SGC，由1扩容为2
server_group_count = 2

# DGC
data_group_count = 256

# 修改DG映射，将 [129, 256]迁移至SG2
[server-group-1]
server_ids = [1-3]
data_group_ids = [1, 128]

# 配置SG2
# 必须加上server [1-3]用于同步已有数据
[server-group-2]
server_ids = [1-6]
data_group_ids = [129, 256]
```

将cluster.conf分发到fstore集群所有服务器以及所有fuseclient后，重启fstore集群和fuseclient。

等待新增的SG同步完成，然后进入步骤2。

* 友情提示：
   * 数据同步过程中fstore集群正常提供服务；
   * 可以使用工具 fs_cluster_stat 查看fstore集群状态，比如：
```
# 查看ACTIVE列表，确保所有服务器状态为ACTIVE，确认一下最后一行输出的总数
fs_cluster_stat -A

# 查看非ACTIVE列表，确保输出为空
fs_cluster_stat -N

# 抽查某个数据分组（如ID为256）的ACTIVE状态，确保全部为ACTIVE
fs_cluster_stat -g 256

# 查看帮助
fs_cluster_stat -h
```

### 步骤2
修改后的cluster.conf内容片段如下：

```
# SGC
server_group_count = 2

# DGC
data_group_count = 256

[server-group-1]
server_ids = [1-3]
data_group_ids = [1, 128]

# 去掉server [1-3]
[server-group-2]
server_ids = [4-6]
data_group_ids = [129, 256]
```

将cluster.conf分发到fstore集群所有服务器以及所有fuseclient后，重启fstore集群和fuseclient

### 清除已迁走的数据
数据迁移完成后，为了清除迁移出去的数据占用空间，V3.2支持清除迁移出去的replica和slice binlog，启动 fs_serverd时带上参数 --migrate-clean 即可，示例如下：
```
/usr/bin/fs_serverd  /etc/fastcfs/fstore/server.conf restart --migrate-clean
```

**注意：** 为了防止误操作，正常情况下启动fs_serverd **不要** 使用--migrate-clean！

* 友情提示：
   * 配置文件分发和程序启停可以使用fcfs.sh，使用说明参见[FastCFS集群运维工具](fcfs-ops-tool-zh_CN.md)
   * 步骤1和2重启fstore集群和fuseclient的过程，会导致服务不可用，建议在业务低峰期进行
