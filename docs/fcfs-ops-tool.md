# fcfs.sh -- a ops tool for quickly manage FastCFS clusters

fcfs.sh is a ops tool for quickly deploy and manage FastCFS clusters. It only relying on SSH access to the servers and sudo. It runs fully on your workstation, requiring no servers, databases, or anything like that.

If you set up and tear down FastCFS clusters a lot, and want minimal extra repeating works, this is for you.

***Tip： This shell only support for Centos 7, Centos 8.***

## 1. What this tool can do?

It can quickly setup FastCFS cluster, include install softwares, distribute config files, manage cluster services and query cluster logs.

It can also upgrade or downgrade FastCFS softwares on single node or full cluster.

It can reinstall software or redistribute config files.

## 2. What this tool cannot do?

It is not a generic deployment tool, it is only for FastCFS. It does not handle cluster configuration beyond distribute config files to cluster servers. You must provide usable cluster config files for it.

## 3. Fetch fcfs.sh

Get it from http://fastcfs.net/fastcfs/ops/fcfs.sh and put it in workstation's bin path.

> sudo curl -o /usr/bin/fcfs.sh  http://fastcfs.net/fastcfs/ops/fcfs.sh && sudo chmod +x /usr/bin/fcfs.sh

## 4. Use preconditions

* fcfs.settings -- cluster ops settings, in current working directory
* conf -- cluster config files, in current working directory
* remote server SSH password-free login, see also [SSH password-free login](#SSH-password-free-login)

You must create a independent working directory(exp: ***fastcfs-ops***) for a cluster, put "fcfs.settings" and "conf" into it. And then, all subsequent commands for this cluster must be execute at the same working directory.

### 4.1. fcfs.settings

fcfs.settings include two fields ***fastcfs_version*** and ***fuseclient_ips***, fastcfs_version specify FastCFS main version number, fuseclient_ips specify host to deploy fuse client. When upgrade or downgrade FastCFS, you need change fastcfs_version to version you expect.

fcfs.settings content Example:

```
# Version of FastCFS cluster（exp：2.2.0）
fastcfs_version=2.2.0

# Hosts which fuseclient will install，multiple hosts separated by comma
fuseclient_ips=10.0.1.14
```

***Tip：Use version 2.2.0 and later for deploy.***

### 4.2. conf

All config files of FastCFS cluster must be put into working directory beforehand, conf include four subdirectories：

* fdir -- config files for fdir server
* fstore -- config files for fstore server
* auth -- config files for fauth server
* fcfs -- config files for fuseclient

**Fetch config sample**

```
mkdir fastcfs-ops
cd fastcfs-ops/
curl -o fcfs-config-sample.tar.gz http://fastcfs.net/fastcfs/ops/fcfs-config-sample.tar.gz
tar -xzvf fcfs-config-sample.tar.gz
```

<a name="SSH-password-free-login"></a>
## 5. SSH password-free login

fcfs.sh will connect to cluster hosts via SSH. It requires that the workstation machine from which the shell is being run can connect via SSH as root(or sudo user) without password into cluster nodes.

To enable password-free login, you need generate rsa key pair for ssh:

> ssh-keygen -t rsa -C cfs -f cfs-ssh-key

it will generate key files：

* cfs-ssh-key  -- private key file
* cfs-ssh-key.pub -- public key file

### 5.1. Deploy private key

Copy private key file cfs-ssh-key to workstation machine's current user directory ***"~/.ssh/"***, and add the following phrase to ~/.ssh/config(for each host):

```
Host [node ip]
    HostName [node host name]
    User [ssh user]
    Port 22
    PubkeyAuthentication yes
    IdentityFile ~/.ssh/cfs-ssh-key
```

### 5.2. Deploy public key

Copy public key to file .ssh/authorized_keys in every node server's sudo user home:

> ssh-copy-id -i cfs-ssh-key.pub [user name]@[node server ip]

and ensure that the following lines are in server's sshd config:

```
RSAAuthentication yes
PubkeyAuthentication yes
AuthorizedKeysFile      .ssh/authorized_keys
```

## 6. Tool commands introduction

The tool's command include three parts: command, module and node. Command is required, module and node are optional.

Usage:

> fcfs.sh command [module] [node]

**Commands**:

* setup -- Setup FastCFS softwares, a shortcut command combines install, config, and restart
* install -- Install FastCFS softwares
* reinstall -- Reinstall FastCFS softwares
* remove -- Remove FastCFS softwares, same as erase
* erase -- Erase FastCFS softwares
* config -- Copy cluster config files to target host path
* start -- Start all or one module service in cluster
* restart -- Restart all or one module service in cluster
* stop -- Stop all or one module service in cluster
* tail -- Display the last part of the specified module's log
* help -- Show the detail of commands and examples

**Modules**:

* fdir -- fastDIR server
* fstore -- faststore server
* fauth -- FastCFS auth server
* fuseclient -- FastCFS fuse client

**Node**:

You can specify a single cluster IP, or command will be executed on all nodes.

### 6.1. FastCFS setup

You can use command ***setup*** to quickly setup FastCFS softwares on nodes, then config and start module services.

**Examples**

Setup the full FastCFS cluster:

> fcfs.sh setup

Setup fdir server on all fdir nodes:

> fcfs.sh setup fdir

Setup fdir server on node 10.0.1.11:

> fcfs.sh setup fdir 10.0.1.11

node 10.0.1.11 must belong to fdir cluster, or it will fail.

### 6.2. FastCFS softwares install

You can use command ***install*** to install softwares only. This command is part of setup.

First time, you must install all softwares on all nodes(that is to say you can't specify module and node). After that, you can install a new node with module and node params.

**Examples**

Install all FastCFS softwares on all nodes:

> fcfs.sh install

Install fdir server on all fdir nodes:

> fcfs.sh install fdir

Install fdir server on node 10.0.1.11:

> fcfs.sh install fdir 10.0.1.11

### 6.3. FastCFS softwares upgrade

When you want to upgrade FastCFS's software, you can modify fcfs.settings, change field fastcfs_version to new version, then execute install command:

> fcfs.sh install

### 6.4. FastCFS softwares downgrade

If you realy want to downgrade FastCFS softwares to an old version, you must execute command ***remove*** first:

> fcfs.sh remove

or:

> fcfs.sh erase

and change field fastcfs_version to old version, then execute install command: 

> fcfs.sh install

### 6.5. FastCFS softwares remove

You can use command ***remove*** or ***erase*** to remove FastCFS softwares.

**Examples**

Remove all softwares on all nodes:

> fcfs.sh remove

Remove fdir server on all nodes:

> fcfs.sh remove fdir

Remove fdir server on node 10.0.1.11:

> fcfs.sh remove fdir 10.0.1.11

### 6.6. FastCFS config files distribute

After install FastCFS softwares, you must execute command ***config*** to distribute config files to cluster nodes. If target config file directory does't exist on node, it will create it automatically.

**Exmaples**

Distribute all module config files to all nodes respective：

> fcfs.sh config

Distribute fdir server config files to all fdir nodes：

> fcfs.sh config fdir

Distribute fdir server config files to fdir node 10.0.1.11：

> fcfs.sh config fdir 10.0.1.11

### 6.7. Cluster manage

Manage commands include ***start***, ***stop***, ***restart***. You can operate on all modules or one module's all nodes at same time, or a single node for one module.

If you want to operate on a single node, you must specify node host after module, that is to say you can't operate on all modules on a single node at same time.

Start all service on all nodes:

> fcfs.sh start

Restart fdir service on all fdir nodes:

> fcfs.sh restart fdir

Stop fdir service on fdir node 10.0.1.11:

> fcfs.sh stop fdir 10.0.1.11

### 6.8. Display cluster logs

When you want to query FastCFS services last logs, you can use command ***tail*** to display the last part of the specified module's log.

**Example**

Display the last 100 lines of fdir server log:

> fcfs.sh tail fdir 10.0.1.11 -n 100

or:

> fcfs.sh tail fdir 10.0.1.11 -100

Display the last 10 lines of each fdir server log:

> fcfs.sh tail fdir
