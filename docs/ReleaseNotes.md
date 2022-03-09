## FastCFS V3.1.0 发布，主要改进如下：
### 新特性
1. [fdir] 对核心组件FastDIR进行改进，实现了LRU淘汰算法，以有限内存支持海量文件
   分布式目录服务FastDIR的淘汰算法具有两大特性：
     1. 按目录结构淘汰：先淘汰子节点，然后淘汰父节点；
     2. 按数据线程淘汰：每个数据线程作为一个独立的数据单元，数据存取和淘汰均在其数据线程中以无锁方式完成。

### Bug修复
1. 采用引用计数方式，不再延迟释放 dentry；
2. delay free namespace string for lockless；
3. bugfixed: get trunk_file_size from ini file correctly。

---------------

## FastCFS V3.0.0 发布，主要改进如下：
### 新特性
1. [fdir] 核心组件FastDIR通过插件方式实现数据存储引擎，采用binlog + 存储引擎插件，按需加载inode数据，单机以有限内存（如64GB）支持100亿级的海量文件

### Bug修复
1. [fdir] increase/decrease parent's nlink on rename operation
1. [fdir] set dentry->kv_array->count to 0 correctly
1. [fstore] should init barray->count to 0

---------------

## FastCFS V2.3.0发布，主要改进如下：
### 新特性
1. [fauth] auth server以主备方式支持多节点，避免单点；
1. [fstore&fdir] leader/master选举/切换引入超时机制，选举时长可控；
1. [网络] 网络通信相关改进：
  1）握手失败，server端主动断开连接； 
  2）cluster内部通信server端超时控制； 
  3）调整网络通信超时默认值（连接超时由10秒调整为2秒，收发数据超时由30秒调整为10秒）。

### Bug修复


---------------
## FastCFS V2.2.0 发布，主要改进如下：
### 新特性
1. [fstore] 使用libaio实现异步读，随机读性能提升明显；
1. [fstore] 支持预读机制，顺序读性能提升显著；
1. [csi] k8s驱动（fastcfs-csi）已同步发布，
1. [devOPS] 集群运维工具（fcfs.sh）

### Bug修复
1. [fstore] 修复V2.1.0引入的bug：第一次运行时，一个关键bool变量没有正确赋值；
2. [fuseclient] 修复列举目录导致元数据缓存的一致性问题；
3. [fauth] 修复username和poolname格式修饰符不当导致的乱码问题。


---------------
## FastCFS V2.0.0 发布，为了更好地对接虚拟机和K8s，V2.0主要实现了存储池和访问权限控制，并支持配额：
### 新特性
1. [fauth] 用户和权限体系

### Bug修复

---------------
## FastCFS V1.2.0 发布，主要对数据恢复和master任命机制做了改进，修复了5个稳定性bug，FastCFS的可靠性和稳定性上了一个新台阶：
### 新特性


### Bug修复


---------------
## FastCFS V1.0.0 发布：
FastCFS 3人小团队历经11个月的研发，推出了FastCFS第一个可用版本，MySQL、PostgresSQL和Oracle可以跑通。

FastCFS服务端两个核心组件是 FastStore 和 FastDIR。FastStore是基于块存储的分布式数据存储服务，其文件block大小为4MB，文件 inode + 文件偏移量 (offset)作为block的key，对应的文件内容为 value，FastStore本质是一个分布式Key-Value系统。
### 主要特性
1. [fdir] 高性能分布式目录服务，管理文件元数据，支持命名空间
1. [fstore] FastStore是一个分布式Key-Value系统，提供了文件块操作，基于数据块的无中心设计，（类一致性Hash），理论上可以无限扩容；
1. [fuseclient] 基于FUSE的标准文件接口已经实现，可以使用fused模块mount到本地，为数据库、虚拟机以及其他使用标准文件接口的应用提供存储。

