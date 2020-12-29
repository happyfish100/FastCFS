## 配置

为了更好的控制FastCFS的性能，我们通过各种设置参数为FastCFS提供了高度可配置和可调节的行为。

FastCFS的配置由多个子文件组成，其中一个是入口文件，其他文件用于引用。目录/etc/fstore 是FastCFS配置文件的默认存放位置，但是在单个服务器上安装多个FastCFS实例时，必须为每个实例指定不同的位置。

FastCFS有以下几个配置文件：

* server.conf - 服务器全局参数配置
* cluster.conf - 集群参数配置
* servers.conf - 服务器组参数配置
* storage.conf - 存储参数配置
* client.conf - 客户端使用的配置文件，需引用cluster.conf

### 1. server.conf 配置

### 2. cluster.conf 配置

### 3. servers.conf 配置

### 4. storage.conf 配置

### 5. client.conf 配置
