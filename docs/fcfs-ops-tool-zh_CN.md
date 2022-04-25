# FastCFS 运维工具介绍

* [fcfs.sh](#fcfs.sh) -- 用于管理 FastCFS 集群的快捷运维工具
* [fcfs_conf.sh](#fcfs_conf.sh) -- 用于快速创建集群配置文件的工具

## 1. fcfs.sh 介绍

fcfs.sh 是一个用于快速部署和管理 FastCFS 集群的脚本工具。它基于 SSH 访问协议连接远程节点服务器，并使用服务器上带有 sudo 功能的账号（也可以使用root）进行相关命令的执行。它只需在你的工作站机器上运行，不需要服务器、数据库或者其他额外资源。

如果你需要经常搭建、卸载或配置 FastCFS集群，又想节省大量的重复工作，那么它很适合你。

***注： 该脚本工具目前仅适用于 Centos 7, Centos 8。***

<a name="fcfs.sh"></a>
### 1.1. 这个工具能做什么？

它能快速构建 FastCFS 集群，包括安装软件、分发配置文件，以及管理集群服务和查看集群日志。

它也能对 FastCFS 单节点或整个集群的相关软件进行升级或降级。

它能重新安装软件或者重新分发配置文件。

### 1.2. 这个工具不能做什么？

它不是一个通用的部署工具， 仅适用于 FastCFS 集群。除了将配置文件分发到集群的各个服务器节点以外，它不对集群配置做任何处理。你必须为它提供可用的集群配置文件。

### 1.3. 获取 fcfs.sh

从以下地址下载 fcfs.sh，并将其放置在工作站机器的bin目录下，以便命令行能够找到：

> http://fastcfs.net/fastcfs/ops/fcfs.sh

获取命令：

> sudo curl -o /usr/bin/fcfs.sh  http://fastcfs.net/fastcfs/ops/fcfs.sh && sudo chmod +x /usr/bin/fcfs.sh

下载完成后可通过命令 ***which fcfs.sh*** 测试工具是否生效。

### 1.4. 使用工具的前置条件

* fcfs.settings -- 集群运维配置，放置在当前工作目录中
* conf -- 集群配置文件目录，放置在当前工作目录中
* 远程服务器 SSH 免密登录，参见 [5. SSH 免密登录](#SSH-password-free-login)

你必须为每个集群创建一个独立的工作目录（例如：***fastcfs-ops***），将 ***fcfs.settings*** 和 ***conf*** 放入其中。然后所有针对该集群的后续命令操作，都必须在该工作目录中执行。

#### 1.4.1. fcfs.settings

fcfs.settings 包括两个字段 ***fastcfs_version*** 和 ***fuseclient_ips***，fastcfs_version 用于指定 FastCFS 主版本号，fuseclient_ips 用于指定要部署 fuse client 的服务器主机。当需要对 FastCFS 集群软件进行升级或降级操作时，你需要先修改 fastcfs_version 对应的版本号为你期望的版本。

fcfs.settings 配置文件内容举例:

```
# 要安装的集群版本号（例如：3.3.0）
fastcfs_version=3.3.0

# 要安装 fuseclient 客户端的IP列表，多个ip以英文逗号分隔
fuseclient_ips=10.0.1.14
```

***注：推荐使用 3.3.0 及以后的版本用于集群部署。***

#### 1.4.2. conf

FastCFS集群的所有配置文件必须提前放置在工作目录中的 ***conf*** 目录，conf 包含以下四个子目录：

* fdir -- fdir 服务器的配置文件
* fstore -- fstore 服务器的配置文件
* auth -- fauth 服务器的配置文件
* fcfs -- fuseclient 客户端的配置文件

**获取配置文件样例**

```
mkdir fastcfs-ops
cd fastcfs-ops/
curl -o fcfs-config-sample.tar.gz http://fastcfs.net/fastcfs/ops/fcfs-config-sample.tar.gz
tar -xzvf fcfs-config-sample.tar.gz
```

**使用fcfs_conf.sh创建集群配置文件**

也可以根据自己的需要，使用脚本工具 fcfs_conf.sh 创建 FastCFS集群配置文件。参见：[fcfs_conf.sh](#fcfs_conf.sh)

<a name="SSH-password-free-login"></a>
### 1.5. SSH 免密登录

fcfs.sh 将通过 SSH 连接所有集群主机。它需要工作站机器可以通过 SSH 在远程主机上执行脚本，执行命令的用户身份可以是 root 或带 sudo的用户，并且必须支持免密登录。

为了开启免密登录，你需要先为 SSH 生成 rsa 公钥/私钥对：

> ssh-keygen -t rsa -C cfs -f cfs-ssh-key

将生成密钥文件：

* cfs-ssh-key  -- 私钥文件
* cfs-ssh-key.pub -- 公钥文件

#### 1.5.1. 部署私钥

将私钥文件 cfs-ssh-key 拷贝到工作站机器的当前用户home目录 ***"~/.ssh/"***，然后将下面的配置片段加入配置文件 ~/.ssh/config（每个集群主机一份）：

```
Host [node ip]
    HostName [node host name]
    User [ssh user]
    Port 22
    PubkeyAuthentication yes
    IdentityFile ~/.ssh/cfs-ssh-key
```

#### 1.5.2. 部署公钥

将公钥文件内容加入到每个节点服务器sudo 用户的home目录下 ***.ssh/authorized_keys*** 文件中：

> ssh-copy-id -i cfs-ssh-key.pub [user name]@[node server ip]

并确保服务器上sshd的配置文件中开启了以下配置参数：

```
RSAAuthentication yes
PubkeyAuthentication yes
AuthorizedKeysFile      .ssh/authorized_keys
```

### 1.6. 工具命令介绍

fcfs.sh工具的命令包括三部分：command，module 和 node。Command 是必须的，module 和 node 是可选的。

用法:

> fcfs.sh command [module] [node]

**Commands**:

* setup -- 安装并运行 FastCFS 软件的快捷命令，组合了 install、config 和 restart三个命令
* install -- 安装 FastCFS 软件
* reinstall -- 重新安装 FastCFS 软件
* remove -- 移除 FastCFS 软件，功能与 erase 命令相同
* erase -- 移除 FastCFS 软件
* config -- 将集群配置文件拷贝到目标主机目录
* start -- 启动集群中的所有或单个模块服务
* restart -- 重启集群中的所有或单个模块服务
* stop -- 停止集群中的所有或单个模块服务
* tail -- 查看指定模块日志的最后一部分内容（等同于远程服务器的tail命令）
* status -- 查看指定模块服务进程状态
* help -- 查看命令的详细说明和样例

**Modules**:

* fdir -- fastDIR 服务器
* fstore -- faststore 服务器
* fauth -- FastCFS auth 服务器
* fuseclient -- FastCFS fuse 客户端

**Node**:

可以用于指定要执行命令的单个集群节点主机名或IP，如果不指定，命令将在所有节点上执行。node 参数必须在使用了 module 参数的情况下使用，不能单独直接使用。

#### 1.6.1. FastCFS 一键安装

你可以使用命令 ***setup*** 在集群节点上快速安装 FastCFS 软件，并自动配置和启动模块服务.

**样例**

安装并启动整个 FastCFS 集群:

> fcfs.sh setup

在所有 fdir 节点上安装并启动 fdir 服务器:

> fcfs.sh setup fdir

在节点 10.0.1.11 上安装 fdir 服务器：

> fcfs.sh setup fdir 10.0.1.11

节点 10.0.1.11 必须属于 fdir 集群，否则命令将失败。

#### 1.6.2. FastCFS 软件安装

你可以使用命令 ***install*** 只安装 FastCFS 软件。这个命令是 setup 命令的一部分。

第一次执行该命令时，必须在所有节点上安装所有软件（也就是说，此时不能指定module和node）。之后，当需要增加新节点时，可以携带 module 和 node 参数使用。

**样例**

在所有节点安装所需的 FastCFS 软件:

> fcfs.sh install

在所有 fdir 节点上安装 fdir 服务器：

> fcfs.sh install fdir

在 10.0.1.11 节点上安装 fdir 服务器：

> fcfs.sh install fdir 10.0.1.11

#### 1.6.3. FastCFS 软件升级

当你想升级 FastCFS 软件时，你需要先将配置文件 fcfs.settings 中的 fastcfs_version 字段的值修改为新的版本号，然后执行 install 命令：

> fcfs.sh install

#### 1.6.4. FastCFS 软件降级

如果你确实需要将 FastCFS 软件降级为旧的版本，你必须先执行 ***remove*** 命令：

> fcfs.sh remove

或:

> fcfs.sh erase

并将字段 fastcfs_version 的值修改为旧的版本号，然后执行 install 命令：

> fcfs.sh install

#### 1.6.5. FastCFS 软件卸载

可以使用命令 ***remove*** 或 ***erase*** 卸载 FastCFS 软件。

**样例**

移除所有节点上的所有软件：

> fcfs.sh remove

移除所有 fdir 节点上的 fdir 服务器：

> fcfs.sh remove fdir

移除节点10.0.1.11上的 fdir 服务器：

> fcfs.sh remove fdir 10.0.1.11

#### 1.6.6. FastCFS 配置文件分发

成功安装 FastCFS 软件之后，你必须通过执行命令 ***config*** 来将配置文件分发到各个集群节点。如果节点上的目标配置文件目录不存在，它将会自动创建它们。

**样例**

将所有模块的配置文件分发到所有相应的节点上：

> fcfs.sh config

将 fdir 服务器的配置文件分发到所有 fdir 节点上：

> fcfs.sh config fdir

将 fdir 服务器的配置文件分发到 fdir 节点 10.0.1.11 上：

> fcfs.sh config fdir 10.0.1.11

#### 1.6.7. 集群管理

管理集群的命令包括 ***start***, ***stop***, ***restart***。你可以同时操作所有模块的所有节点，也可以同时操作某一模块的所有节点，也可以单独操作某一模块的某一个节点。

如果想操作单个节点，你必须在 module 参数后指定 node（host）参数（也就是说，你不能同时操作某一节点上的所有模块）。

**样例**

启动所有节点上的所有服务：

> fcfs.sh start

重启所有 fdir 节点上的 fdir 服务：

> fcfs.sh restart fdir

停止 节点10.0.1.11上的 fdir 服务：

> fcfs.sh stop fdir 10.0.1.11

#### 1.6.8. 查看集群日志

当你想查看 FastCFS 服务的最新日志时，你可以使用命令 ***tail*** 来显示指定模块日志文件的最后一部分。

**样例**

显示 10.0.1.11节点上 fdir 服务日志文件的最后 100 行：

> fcfs.sh tail fdir 10.0.1.11 -n 100

或:

> fcfs.sh tail fdir 10.0.1.11 -100

显示每个 fdir 服务器日志的最后 10 行：

> fcfs.sh tail fdir

#### 1.6.9. 查看集群服务进程状态

当你想查看 FastCFS 服务的进程状态时，你可以使用命令 ***status*** 来显示指定模块的服务进程状态。

**样例**

显示 10.0.1.11 节点上 fdir 服务进程状态：

> fcfs.sh status fdir 10.0.1.11

显示每个 fdir 服务器进程状态：

> fcfs.sh status fdir

显示所有服务进程状态：

> fcfs.sh status

<a name="fcfs_conf.sh"></a>
## 2. fcfs_conf.sh 介绍

fcfs_conf.sh 是一个快速创建 FastCFS集群配置文件的运维工具。它通过 bash shell 访问本地配置文件 fcfs_conf.settings。它只需在你的工作站机器上运行，不需要服务器、数据库或者其他额外资源。

如果你需要在指定的服务器IP列表上快速生成集群配置文件，那么它很适合你。

### 2.1. 获取 fcfs_conf.sh

从以下地址下载 fcfs_conf.sh，并将其放置在工作站机器的bin目录下，以便命令行能够找到：

> http://fastcfs.net/fastcfs/ops/fcfs_conf.sh

获取命令：

> sudo curl -o /usr/bin/fcfs_conf.sh  http://fastcfs.net/fastcfs/ops/fcfs_conf.sh && sudo chmod +x /usr/bin/fcfs_conf.sh

下载完成后可通过命令 ***which fcfs_conf.sh*** 测试工具是否生效。

### 2.2. 使用fcfs_conf.sh的前置条件

* fcfs_conf.settings -- 生成集群配置文件的参数配置，放置在当前工作目录中

你必须为每个集群创建一个独立的工作目录（例如：***fastcfs-ops***），将 ***fcfs_conf.settings***放入其中。然后在该工作目录中执行fcfs_conf.sh命令。

#### 2.2.1. fcfs_conf.settings

fcfs_conf.settings 包括以下六种字段：

* fastcfs_version -- 要生成哪个版本集群的配置文件
* auth_ips -- FastCFS auth 服务器IP地址列表，多个IP以英文半角逗号分隔
* fdir_ips --  fastDIR服务器IP地址列表，多个IP以英文半角逗号分隔
* fstore_group_count -- faststore服务器分组数量
* fstore_group_[N] -- faststore服务器分组IP地址列表，多个IP以英文半角逗号分隔
* data_group_count -- faststore服务区数据分组数（推荐64的整数倍）

参数 fstore_group_count 设置的数量必须与 fstore_group_[N] 数量相匹配，并且编号从1开始递增。

fcfs_conf.settings 配置文件内容举例:

```
# 要生成配置的集群版本号（例如：3.3.0）
fastcfs_version=3.3.0

# 集群主机列表和分组数
auth_ips=10.0.1.11,10.0.1.12,10.0.1.13
fdir_ips=10.0.1.11,10.0.1.12,10.0.1.13
fstore_group_count=2
fstore_group_1=10.0.1.11,10.0.1.12,10.0.1.13
fstore_group_2=10.0.2.11,10.0.2.12,10.0.2.13
data_group_count=128
```

***注：推荐使用 3.3.0 及以后的版本用于生成集群配置文件。***

### 2.3. fcfs_conf.sh工具命令介绍

fcfs_conf.sh工具的命令包括两部分：command，module。Command 是必须的，module 是可选的。

用法:

> fcfs_conf.sh command [module]

**Commands**:

* create -- 创建配置文件
* help -- 查看命令的详细说明和样例

**Modules**:

* fdir -- fastDIR 服务器
* fstore -- faststore 服务器
* fauth -- FastCFS auth 服务器
* fuseclient -- FastCFS fuse 客户端

## 3. conf_tpl_tar.sh 介绍

一个简单的集群配置文件模版打包工具，仅供FastCFS开发者使用。

它将从FastCFS, fastDIR, faststore的源代码仓库将FastCFS 集群配置文件模版打包成tar包。

这三个仓库必须在同一个目录，并且 conf_tpl_tar.sh 必须在子目录 FastCFS/shell/ 中执行。

命令格式:

```
conf_tpl_tar.sh <version> [update]
```

* version -- 要创建模版压缩包的集群版本号.
* update -- 在创建之前获取最新的源代码，可选。

举例:

> ./conf_tpl_tar.sh 3.3.0 update

将在当前目录创建配置模版压缩包文件 **conf.3.3.0.tpl.tar.gz**。
