#!/bin/bash
#
# This shell will make and install these four libs in order:
#     libfastcommon
#     libserverframe
#     fastDIR
#     faststore
#
# If no source code in build path, it will git clone from:
#     https://gitee.com/fastdfs100/libfastcommon.git
#     https://gitee.com/fastdfs100/libserverframe.git
#     https://gitee.com/fastdfs100/fastDIR.git
#     https://gitee.com/fastdfs100/faststore.git
#

# FastCFS modules and repositores
COMMON_LIB="libfastcommon"
COMMON_LIB_URL="https://gitee.com/fastdfs100/libfastcommon.git"
FRAME_LIB="libserverframe"
FRAME_LIB_URL="https://gitee.com/fastdfs100/libserverframe.git"
FDIR_LIB="fastDIR"
FDIR_LIB_URL="https://gitee.com/fastdfs100/fastDIR.git"
STORE_LIB="faststore"
STORE_LIB_URL="https://gitee.com/fastdfs100/faststore.git"
FASTCFS_LIB="FastCFS"
AUTHCLIENT_LIB="auth_client"

DEFAULT_CLUSTER_SIZE=3
DEFAULT_HOST=127.0.0.1
DEFAULT_BIND_ADDR=

# auth config default properties
AUTH_DEFAULT_BASE_PATH=/usr/local/fastauth
AUTH_DEFAULT_CLUSTER_PORT=61011
AUTH_DEFAULT_SERVICE_PORT=71011

# fastDIR config default properties
FDIR_DEFAULT_BASE_PATH=/usr/local/fastdir
FDIR_DEFAULT_CLUSTER_PORT=11011
FDIR_DEFAULT_SERVICE_PORT=21011

# faststore config default properties
FSTORE_DEFAULT_BASE_PATH=/usr/local/faststore
FSTORE_DEFAULT_CLUSTER_PORT=31011
FSTORE_DEFAULT_SERVICE_PORT=41011
FSTORE_DEFAULT_REPLICA_PORT=51011
FSTORE_DEFAULT_STORE_PATH_COUNT=2

# fuse config default properties
FUSE_DEFAULT_BASE_PATH=/usr/local/fuse

# cluster op shell template
CLUSTER_SHELL_TPL="./template/cluster.sh.template"

BUILD_PATH="build"
mode=$1    # pull, makeinstall, init or clean 
make_shell=make.sh
same_host=false
uname=$(uname)

pull_source_code() {
  if [ $# != 2 ]; then
    echo "ERROR:$0 - pull_source_code() function need repository name and url!"
    exit 1
  fi

  module_name=$1
  module_url=$2
  if ! [ -d $module_name ]; then
    echo "INFO:The $module_name local repository does not exist!"
    echo "INFO:=====Begin to clone $module_name , it will take some time...====="
    git clone $module_url
  else
    echo "INFO:The $module_name local repository have existed."
    cd $module_name
    echo "INFO:=====Begin to pull $module_name, it will take some time...====="
    git pull
    cd ..
  fi
}

make_op() {
  if [ $# != 2 ]; then
    echo "ERROR:$0 - make_op() function need module repository name and mode!"
    exit 1
  fi

  module_name=$1
  make_mode=$2

  if [ $module_name = $AUTHCLIENT_LIB ]; then
      echo "INFO:=====Begin to $make_mode module $module_name...====="
      command="./$make_shell --module=auth_client $make_mode"
      echo "INFO:Execute command=$command, path=$PWD"
      if [ $make_mode = "make" ]; then
          result=`./$make_shell --module=auth_client`
        else
          result=`./$make_shell --module=auth_client $make_mode`
      fi
  elif [ $module_name = $FASTCFS_LIB ]; then
      echo "INFO:=====Begin to $make_mode module $module_name...====="
      command="./$make_shell --exclude=auth_client $make_mode"
      echo "INFO:Execute command=$command, path=$PWD"
      if [ $make_mode = "make" ]; then
          result=`./$make_shell --exclude=auth_client`
        else
          result=`./$make_shell --exclude=auth_client $make_mode`
      fi
  else
    if ! [ -d $BUILD_PATH/$module_name ]; then
      echo "ERROR:$0 - module repository {$module_name} does not exist!"
      echo "ERROR:You must checkout from remote repository first!"
      exit 1
    else  
      cd $BUILD_PATH/$module_name/
      echo "INFO:=====Begin to $make_mode module $module_name...====="
      command="./$make_shell $make_mode"
      echo "INFO:Execute command=$command, path=$PWD"
      if [ $make_mode = "make" ]; then
          result=`./$make_shell`
        else
          result=`./$make_shell $make_mode`
      fi
      cd -
    fi
  fi
}

sed_replace()
{
    sed_cmd=$1
    filename=$2
    if [ "$uname" = "FreeBSD" ] || [ "$uname" = "Darwin" ]; then
       sed -i "" "$sed_cmd" $filename
    else
       sed -i "$sed_cmd" $filename
    fi
}

split_to_array() {
  IFS=',' read -ra $2 <<< "$1"
}

placeholder_replace() {
  filename=$1
  placeholder=$2
  value=$3
  sed_replace "s#\${$placeholder}#$value#g" $filename
}

validate_auth_params() {
  # Validate auth server count
  if [[ $auth_server_count -le 0 ]]; then
    echo "INFO:--auth-server-count not specified, would use default size: $DEFAULT_CLUSTER_SIZE"
    auth_server_count=$DEFAULT_CLUSTER_SIZE
  fi
  # Validate auth base_path
  if [[ ${#auth_pathes[*]} -eq 0 ]]; then
    echo "INFO:--auth-path not specified, would use default path: $AUTH_DEFAULT_BASE_PATH"
    for (( i=0; i < $auth_server_count; i++ )); do
      auth_pathes[$i]="$AUTH_DEFAULT_BASE_PATH/server-$(( $i + 1 ))"
    done
  elif [[ ${#auth_pathes[*]} -eq 1 ]] && [[ ${#auth_pathes[*]} -lt $auth_server_count ]]; then
    tmp_base_path=${auth_pathes[0]}
    for (( i=0; i < $auth_server_count; i++ )); do
      auth_pathes[$i]="$tmp_base_path/server-$(( $i + 1 ))"
    done
  elif [[ ${#auth_pathes[*]} -ne $auth_server_count ]]; then
    echo "ERROR:--auth-path must be one base path, or number of it equal to --auth-server-count!"
    exit 1
  fi
  # Validate auth host
  if [[ ${#auth_hosts[*]} -eq 0 ]]; then
    echo "INFO:--auth-host not specified, would use default host: $DEFAULT_HOST"
    for (( i=0; i < $auth_server_count; i++ )); do
      auth_hosts[$i]=$DEFAULT_HOST
    done
    same_host=true
  elif [[ ${#auth_hosts[*]} -eq 1 ]] && [[ ${#auth_hosts[*]} -lt $auth_server_count ]]; then
    tmp_host=${auth_hosts[0]}
    for (( i=0; i < $auth_server_count; i++ )); do
      auth_hosts[$i]=$tmp_host
    done
    same_host=true
  elif [[ ${#auth_hosts[*]} -ne $auth_server_count ]]; then
    echo "ERROR:--auth-host must be one IP, or number of it equal to --auth-server-count!"
    exit 1
  fi
  # Validate auth cluster port
  if [[ ${#auth_cluster_ports[*]} -eq 0 ]]; then
    echo "INFO:--auth-cluster-port not specified, would use default port: $AUTH_DEFAULT_CLUSTER_PORT,+1,+2..."
    for (( i=0; i < $auth_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        auth_cluster_ports[$i]=$(( $AUTH_DEFAULT_CLUSTER_PORT + $i ))
      else
        auth_cluster_ports[$i]=$AUTH_DEFAULT_CLUSTER_PORT
      fi
    done
  elif [[ ${#auth_cluster_ports[*]} -eq 1 ]] && [[ ${#auth_cluster_ports[*]} -lt $auth_server_count ]]; then
    tmp_port=${auth_cluster_ports[0]}
    for (( i=0; i < $auth_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        auth_cluster_ports[$i]=$(( $tmp_port + $i ))
      else
        auth_cluster_ports[$i]=$tmp_port
      fi
    done
  elif [[ ${#auth_cluster_ports[*]} -ne $auth_server_count ]]; then
    echo "ERROR:--auth-cluster-port must be one port, or number of it equal to --auth-server-count!"
    exit 1
  fi
  # Validate auth service port
  if [[ ${#auth_service_ports[*]} -eq 0 ]]; then
    echo "INFO:--auth-service-port not specified, would use default port: $AUTH_DEFAULT_SERVICE_PORT,+1,+2..."
    for (( i=0; i < $auth_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        auth_service_ports[$i]=$(( $AUTH_DEFAULT_SERVICE_PORT + $i ))
      else
        auth_service_ports[$i]=$AUTH_DEFAULT_SERVICE_PORT
      fi
    done
  elif [[ ${#auth_service_ports[*]} -eq 1 ]] && [[ ${#auth_service_ports[*]} -lt $auth_server_count ]]; then
    tmp_port=${auth_service_ports[0]}
    for (( i=0; i < $auth_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        auth_service_ports[$i]=$(( $tmp_port + $i ))
      else
        auth_service_ports[$i]=$tmp_port
      fi
    done
  elif [[ ${#auth_service_ports[*]} -ne $auth_server_count ]]; then
    echo "ERROR:--auth-service-port must be one port, or number of it equal to --auth-server-count!"
    exit 1
  fi

  for (( i=0; i < $auth_server_count; i++ )); do
    if [[ ${auth_cluster_ports[$i]} -eq ${auth_service_ports[$i]} ]]; then
      echo "ERROR:You must specify different port for --auth-cluster-port and --auth-service-port at same host!"
      exit 1
    fi
  done
  # Validate auth bind_addr
  if [[ ${#auth_bind_addrs[*]} -eq 0 ]]; then
    echo "INFO:--auth-bind-addr not specified, would use default host: $DEFAULT_BIND_ADDR"
    for (( i=0; i < $auth_server_count; i++ )); do
      auth_bind_addrs[$i]=$DEFAULT_BIND_ADDR
    done
  elif [[ ${#auth_bind_addrs[*]} -eq 1 ]] && [[ ${#auth_bind_addrs[*]} -lt $auth_server_count ]]; then
    tmp_bind_addr=${auth_bind_addrs[0]}
    for (( i=0; i < $auth_server_count; i++ )); do
      auth_bind_addrs[$i]=$tmp_host
    done
  elif [[ ${#auth_bind_addrs[*]} -ne $auth_server_count ]]; then
    echo "ERROR:--auth-bind-addr must be one IP, or number of it equal to --auth-server-count!"
    exit 1
  fi
}

validate_fastdir_params() {
  # Validate fastDIR server count
  if [[ $dir_server_count -le 0 ]]; then
    echo "INFO:--dir-server-count not specified, would use default size: $DEFAULT_CLUSTER_SIZE"
    dir_server_count=$DEFAULT_CLUSTER_SIZE
  fi
  # Validate fastDIR base_path
  if [[ ${#dir_pathes[*]} -eq 0 ]]; then
    echo "INFO:--dir-path not specified, would use default path: $FDIR_DEFAULT_BASE_PATH"
    for (( i=0; i < $dir_server_count; i++ )); do
      dir_pathes[$i]="$FDIR_DEFAULT_BASE_PATH/server-$(( $i + 1 ))"
    done
  elif [[ ${#dir_pathes[*]} -eq 1 ]] && [[ ${#dir_pathes[*]} -lt $dir_server_count ]]; then
    tmp_base_path=${dir_pathes[0]}
    for (( i=0; i < $dir_server_count; i++ )); do
      dir_pathes[$i]="$tmp_base_path/server-$(( $i + 1 ))"
    done
  elif [[ ${#dir_pathes[*]} -ne $dir_server_count ]]; then
    echo "ERROR:--dir-path must be one base path, or number of it equal to --dir-server-count!"
    exit 1
  fi
  # Validate fastDIR host
  if [[ ${#dir_hosts[*]} -eq 0 ]]; then
    echo "INFO:--dir-host not specified, would use default host: $DEFAULT_HOST"
    for (( i=0; i < $dir_server_count; i++ )); do
      dir_hosts[$i]=$DEFAULT_HOST
    done
    same_host=true
  elif [[ ${#dir_hosts[*]} -eq 1 ]] && [[ ${#dir_hosts[*]} -lt $dir_server_count ]]; then
    tmp_host=${dir_hosts[0]}
    for (( i=0; i < $dir_server_count; i++ )); do
      dir_hosts[$i]=$tmp_host
    done
    same_host=true
  elif [[ ${#dir_hosts[*]} -ne $dir_server_count ]]; then
    echo "ERROR:--dir-host must be one IP, or number of it equal to --dir-server-count!"
    exit 1
  fi
  # Validate fastDIR cluster port
  if [[ ${#dir_cluster_ports[*]} -eq 0 ]]; then
    echo "INFO:--dir-cluster-port not specified, would use default port: $FDIR_DEFAULT_CLUSTER_PORT,+1,+2..."
    for (( i=0; i < $dir_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        dir_cluster_ports[$i]=$(( $FDIR_DEFAULT_CLUSTER_PORT + $i ))
      else
        dir_cluster_ports[$i]=$FDIR_DEFAULT_CLUSTER_PORT
      fi
    done
  elif [[ ${#dir_cluster_ports[*]} -eq 1 ]] && [[ ${#dir_cluster_ports[*]} -lt $dir_server_count ]]; then
    tmp_port=${dir_cluster_ports[0]}
    for (( i=0; i < $dir_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        dir_cluster_ports[$i]=$(( $tmp_port + $i ))
      else
        dir_cluster_ports[$i]=$tmp_port
      fi
    done
  elif [[ ${#dir_cluster_ports[*]} -ne $dir_server_count ]]; then
    echo "ERROR:--dir-cluster-port must be one port, or number of it equal to --dir-server-count!"
    exit 1
  fi
  # Validate fastDIR service port
  if [[ ${#dir_service_ports[*]} -eq 0 ]]; then
    echo "INFO:--dir-service-port not specified, would use default port: $FDIR_DEFAULT_SERVICE_PORT,+1,+2..."
    for (( i=0; i < $dir_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        dir_service_ports[$i]=$(( $FDIR_DEFAULT_SERVICE_PORT + $i ))
      else
        dir_service_ports[$i]=$FDIR_DEFAULT_SERVICE_PORT
      fi
    done
  elif [[ ${#dir_service_ports[*]} -eq 1 ]] && [[ ${#dir_service_ports[*]} -lt $dir_server_count ]]; then
    tmp_port=${dir_service_ports[0]}
    for (( i=0; i < $dir_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        dir_service_ports[$i]=$(( $tmp_port + $i ))
      else
        dir_service_ports[$i]=$tmp_port
      fi
    done
  elif [[ ${#dir_service_ports[*]} -ne $dir_server_count ]]; then
    echo "ERROR:--dir-service-port must be one port, or number of it equal to --dir-server-count!"
    exit 1
  fi

  for (( i=0; i < $dir_server_count; i++ )); do
    if [[ ${dir_cluster_ports[$i]} -eq ${dir_service_ports[$i]} ]]; then
      echo "ERROR:You must specify different port for --dir-cluster-port and --dir-service-port at same host!"
      exit 1
    fi
  done
  # Validate fastDIR bind_addr
  if [[ ${#dir_bind_addrs[*]} -eq 0 ]]; then
    echo "INFO:--dir-bind-addr not specified, would use default host: $DEFAULT_BIND_ADDR"
    for (( i=0; i < $dir_server_count; i++ )); do
      dir_bind_addrs[$i]=$DEFAULT_BIND_ADDR
    done
  elif [[ ${#dir_bind_addrs[*]} -eq 1 ]] && [[ ${#dir_bind_addrs[*]} -lt $dir_server_count ]]; then
    tmp_bind_addr=${dir_bind_addrs[0]}
    for (( i=0; i < $dir_server_count; i++ )); do
      dir_bind_addrs[$i]=$tmp_host
    done
  elif [[ ${#dir_bind_addrs[*]} -ne $dir_server_count ]]; then
    echo "ERROR:--dir-bind-addr must be one IP, or number of it equal to --dir-server-count!"
    exit 1
  fi
}

validate_faststore_params() {
  # Validate faststore server count
  if [[ $store_server_count -le 0 ]]; then
    echo "INFO:--store-server-count not specified, would use default size: $DEFAULT_CLUSTER_SIZE"
    store_server_count=$DEFAULT_CLUSTER_SIZE
  fi
  # Validate faststore base_path
  if [[ ${#store_pathes[*]} -eq 0 ]]; then
    echo "INFO:--store-path not specified, would use default path: $FSTORE_DEFAULT_BASE_PATH"
    for (( i=0; i < $store_server_count; i++ )); do
      store_pathes[$i]="$FSTORE_DEFAULT_BASE_PATH/server-$(( $i + 1 ))"
    done
  elif [[ ${#store_pathes[*]} -eq 1 ]] && [[ ${#store_pathes[*]} -lt $store_server_count ]]; then
    tmp_base_path=${store_pathes[0]}
    for (( i=0; i < $store_server_count; i++ )); do
      store_pathes[$i]="$tmp_base_path/server-$(( $i + 1 ))"
    done
  elif [[ ${#store_pathes[*]} -ne $store_server_count ]]; then
    echo "ERROR:--store-path must be one base path, or number of it equal to --store-server-count!"
    exit 1
  fi
  # Validate faststore host
  if [[ ${#store_hosts[*]} -eq 0 ]]; then
    echo "INFO:--store-host not specified, would use default host: $DEFAULT_HOST"
    for (( i=0; i < $store_server_count; i++ )); do
      store_hosts[$i]=$DEFAULT_HOST
    done
    same_host=true
  elif [[ ${#store_hosts[*]} -eq 1 ]] && [[ ${#store_hosts[*]} -lt $store_server_count ]]; then
    tmp_host=${store_hosts[0]}
    for (( i=0; i < $store_server_count; i++ )); do
      store_hosts[$i]=$tmp_host
    done
    same_host=true
  elif [[ ${#store_hosts[*]} -ne $store_server_count ]]; then
    echo "ERROR:--store-host must be one IP, or number of it equal to --store-server-count!"
    exit 1
  fi
  # Validate faststore cluster port
  if [[ ${#store_cluster_ports[*]} -eq 0 ]]; then
    echo "INFO:--store-cluster-port not specified, would use default port: $FSTORE_DEFAULT_CLUSTER_PORT,+1,+2..."
    for (( i=0; i < $store_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        store_cluster_ports[$i]=$(( $FSTORE_DEFAULT_CLUSTER_PORT + $i ))
      else
        store_cluster_ports[$i]=$FSTORE_DEFAULT_CLUSTER_PORT
      fi
    done
  elif [[ ${#store_cluster_ports[*]} -eq 1 ]] && [[ ${#store_cluster_ports[*]} -lt $store_server_count ]]; then
    tmp_port=${store_cluster_ports[0]}
    for (( i=0; i < $store_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        store_cluster_ports[$i]=$(( $tmp_port + $i ))
      else
        store_cluster_ports[$i]=$tmp_port
      fi
    done
  elif [[ ${#store_cluster_ports[*]} -ne $store_server_count ]]; then
    echo "ERROR:--store-cluster-port must be one port, or number of it equal to --store-server-count!"
    exit 1
  fi
  # Validate faststore service port
  if [[ ${#store_service_ports[*]} -eq 0 ]]; then
    echo "INFO:--store-service-port not specified, would use default port: $FSTORE_DEFAULT_SERVICE_PORT,+1,+2..."
    for (( i=0; i < $store_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        store_service_ports[$i]=$(( $FSTORE_DEFAULT_SERVICE_PORT + $i ))
      else
        store_service_ports[$i]=$FSTORE_DEFAULT_SERVICE_PORT
      fi
    done
  elif [[ ${#store_service_ports[*]} -eq 1 ]] && [[ ${#store_service_ports[*]} -lt $store_server_count ]]; then
    tmp_port=${store_service_ports[0]}
    for (( i=0; i < $store_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        store_service_ports[$i]=$(( $tmp_port + $i ))
      else
        store_service_ports[$i]=$tmp_port
      fi
    done
  elif [[ ${#store_service_ports[*]} -ne $store_server_count ]]; then
    echo "ERROR:--store-service-port must be one port, or number of it equal to --store-server-count!"
    exit 1
  fi

  for (( i=0; i < $store_server_count; i++ )); do
    if [[ ${store_cluster_ports[$i]} -eq ${store_service_ports[$i]} ]]; then
      echo "ERROR:You must specify different port for --store-cluster-port and --store-service-port at same host!"
      exit 1
    fi
  done
  # Validate faststore replica port
  if [[ ${#store_replica_ports[*]} -eq 0 ]]; then
    echo "INFO:--store-replica-port not specified, would use default port: $FSTORE_DEFAULT_REPLICA_PORT,+1,+2..."
    for (( i=0; i < $store_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        store_replica_ports[$i]=$(( $FSTORE_DEFAULT_REPLICA_PORT + $i ))
      else
        store_replica_ports[$i]=$FSTORE_DEFAULT_REPLICA_PORT
      fi
    done
  elif [[ ${#store_replica_ports[*]} -eq 1 ]] && [[ ${#store_replica_ports[*]} -lt $store_server_count ]]; then
    tmp_port=${store_replica_ports[0]}
    for (( i=0; i < $store_server_count; i++ )); do
      if [[ $same_host = true ]]; then
        store_replica_ports[$i]=$(( $tmp_port + $i ))
      else
        store_replica_ports[$i]=$tmp_port
      fi
    done
  elif [[ ${#store_replica_ports[*]} -ne $store_server_count ]]; then
    echo "ERROR:--store-replica-port must be one port, or number of it equal to --store-server-count!"
    exit 1
  fi

  for (( i=0; i < $store_server_count; i++ )); do
    if [[ ${store_cluster_ports[$i]} -eq ${store_replica_ports[$i]} ]]; then
      echo "ERROR:You must specify different port for --store-cluster-port and --store-replica-port at same host!"
      exit 1
    fi
  done
  # Validate faststore bind_addr
  if [[ ${#store_bind_addrs[*]} -eq 0 ]]; then
    echo "INFO:--store-bind-addr not specified, would use default host: $DEFAULT_BIND_ADDR"
    for (( i=0; i < $store_server_count; i++ )); do
      store_bind_addrs[$i]=$DEFAULT_BIND_ADDR
    done
  elif [[ ${#store_bind_addrs[*]} -eq 1 ]] && [[ ${#store_bind_addrs[*]} -lt $store_server_count ]]; then
    tmp_bind_addr=${store_bind_addrs[0]}
    for (( i=0; i < $store_server_count; i++ )); do
      store_bind_addrs[$i]=$tmp_host
    done
  elif [[ ${#store_bind_addrs[*]} -ne $store_server_count ]]; then
    echo "ERROR:--store-bind-addr must be one IP, or number of it equal to --store-server-count!"
    exit 1
  fi
  # Validate fuse path and mount point
  if [[ -z "$fuse_path" ]]; then
    echo "INFO:--fuse-path not specified, would use default path: $FUSE_DEFAULT_BASE_PATH"
    fuse_path=$FUSE_DEFAULT_BASE_PATH
  fi
  if [[ -z "$fuse_mount_point" ]]; then
    echo "INFO:--fuse-mount-point not specified, would use default point: $FUSE_DEFAULT_BASE_PATH/fuse1"
    fuse_mount_point=$FUSE_DEFAULT_BASE_PATH/fuse1
  fi
}

parse_init_arguments() {
  for arg do
    case "$arg" in
      --auth-path=*)
        split_to_array ${arg#--auth-path=} auth_pathes
      ;;
      --auth-server-count=*)
        auth_server_count=${arg#--auth-server-count=}
      ;;
      --auth-host=*)
        split_to_array ${arg#--auth-host=} auth_hosts
      ;;
      --auth-cluster-port=*)
        split_to_array ${arg#--auth-cluster-port=} auth_cluster_ports
      ;;
      --auth-service-port=*)
        split_to_array ${arg#--auth-service-port=} auth_service_ports
      ;;
      --auth-bind-addr=*)
        split_to_array ${arg#--auth-bind-addr=} auth_bind_addrs
      ;;
      --dir-path=*)
        split_to_array ${arg#--dir-path=} dir_pathes
      ;;
      --dir-server-count=*)
        dir_server_count=${arg#--dir-server-count=}
      ;;
      --dir-host=*)
        split_to_array ${arg#--dir-host=} dir_hosts
      ;;
      --dir-cluster-port=*)
        split_to_array ${arg#--dir-cluster-port=} dir_cluster_ports
      ;;
      --dir-service-port=*)
        split_to_array ${arg#--dir-service-port=} dir_service_ports
      ;;
      --dir-bind-addr=*)
        split_to_array ${arg#--dir-bind-addr=} dir_bind_addrs
      ;;
      --store-path=*)
        split_to_array ${arg#--store-path=} store_pathes
      ;;
      --store-server-count=*)
        store_server_count=${arg#--store-server-count=}
      ;;
      --store-host=*)
        split_to_array ${arg#--store-host=} store_hosts
      ;;
      --store-cluster-port=*)
        split_to_array ${arg#--store-cluster-port=} store_cluster_ports
      ;;
      --store-service-port=*)
        split_to_array ${arg#--store-service-port=} store_service_ports
      ;;
      --store-replica-port=*)
        split_to_array ${arg#--store-replica-port=} store_replica_ports
      ;;
      --store-bind-addr=*)
        split_to_array ${arg#--store-bind-addr=} store_bind_addrs
      ;;
      --fuse-path=*)
        fuse_path=${arg#--fuse-path=}
      ;;
      --fuse-mount-point=*)
        fuse_mount_point=${arg#--fuse-mount-point=}
      ;;
    esac
  done
  validate_auth_params
  validate_fastdir_params
  validate_faststore_params
}

check_config_file() {
  if ! [ -f $1 ]; then
    echo "ERROR:File $1 does not exist, can't init config from templates!"
    exit 1
  fi
}

fastdir_cluster_auth_config_filename=/etc/fastcfs/auth/auth.conf
fastdir_server_session_config_filename=/etc/fastcfs/auth/session.conf
faststore_cluster_auth_config_filename=/etc/fastcfs/auth/auth.conf
faststore_server_session_config_filename=/etc/fastcfs/auth/session.conf

init_auth_config() {
  # if [[ ${#auth_pathes[*]} -le 0 ]] || [[ $auth_server_count -le 0 ]]; then
  #   echo "Parameters --auth-path and auth-server-count not specified, would not config auth"
  #   exit 1
  # fi
  # Init config auth to target path.
  echo "INFO:Will initialize $auth_server_count instances for auth..."

  A_SESSION_VALIDATE_KEY_TPL="./template/auth/keys/session_validate.key"
  A_AUTH_TPL="./template/auth/auth.template"
  A_CLIENT_TPL="./template/auth/client.template"
  A_CLUSTER_TPL="./template/auth/cluster.template"
  A_SERVER_TPL="./template/auth/server.template"
  A_SERVERS_TPL="./template/auth/servers.template"
  A_SESSION_TPL="./template/auth/session.template"

  check_config_file $A_SESSION_VALIDATE_KEY_TPL
  check_config_file $A_AUTH_TPL
  check_config_file $A_CLIENT_TPL
  check_config_file $A_CLUSTER_TPL
  check_config_file $A_SERVER_TPL
  check_config_file $A_SERVERS_TPL
  check_config_file $A_SESSION_TPL

  # Create auth cluster op shell from template
  auth_cluster_shell="$BUILD_PATH/shell/fcfsauth-cluster.sh"
  if [[ -f $CLUSTER_SHELL_TPL ]]; then
    if cp -f $CLUSTER_SHELL_TPL $auth_cluster_shell; then
      chmod u+x $auth_cluster_shell
      placeholder_replace $auth_cluster_shell CLUSTER_NAME "fcfs_auth"
    else
      echo "WARNING:Create $auth_cluster_shell from $CLUSTER_SHELL_TPL failed!"
    fi
  fi

  for (( i=0; i < $auth_server_count; i++ )); do
    target_auth=${auth_pathes[$i]}/conf
    if [ -d $target_auth ]; then
      echo "WARNING:The $(( $i + 1 ))th fcfs_authd instance conf path \"$target_auth\" have existed, skip!"
      echo "WARNING:If you want to reconfig it, you must delete it first."
      continue
    fi

    if ! mkdir -p $target_auth; then
      echo "ERROR:Create target conf path failed:$target_auth!"
      exit 1
    fi

    ta_server_conf=$target_auth/server.conf
    if cp -f $A_SERVER_TPL $ta_server_conf; then
      # Replace placeholders with reality in server template
      echo "INFO:Begin config $ta_server_conf..."
      placeholder_replace $ta_server_conf BASE_PATH "${auth_pathes[$i]}"
      placeholder_replace $ta_server_conf CLUSTER_BIND_ADDR "${auth_bind_addrs[$i]}"
      placeholder_replace $ta_server_conf CLUSTER_PORT "${auth_cluster_ports[$i]}"
      placeholder_replace $ta_server_conf SERVICE_BIND_ADDR "${auth_bind_addrs[$i]}"
      placeholder_replace $ta_server_conf SERVICE_PORT "${auth_service_ports[$i]}"
      
      # Add fcfs_authd command to $auth_cluster_shell
      if [[ -f $auth_cluster_shell ]]; then
        echo "fcfs_authd $t_server_conf \$mode" >> $auth_cluster_shell
      fi
    else
      echo "ERROR:Create server.conf from $A_SERVER_TPL failed!"
    fi

    ta_cluster_conf=$target_auth/cluster.conf
    if cp -f $A_CLUSTER_TPL $ta_cluster_conf; then
      # Replace placeholders with reality in auth cluster template
      echo "INFO:Begin config $ta_cluster_conf..."
    else
      echo "ERROR:Create cluster.conf from $A_CLUSTER_TPL for auth failed!"
    fi

    ta_servers_conf=$target_auth/servers.conf
    if cp -f $A_SERVERS_TPL $ta_servers_conf; then
      # Replace placeholders with reality in auth servers template
      echo "INFO:Begin config $ta_servers_conf..."
      placeholder_replace $ta_servers_conf CLUSTER_PORT "${auth_cluster_ports[0]}"
      placeholder_replace $ta_servers_conf SERVICE_PORT "${auth_service_ports[0]}"
      placeholder_replace $ta_servers_conf SERVER_HOST "${auth_hosts[0]}"
      for (( j=1; j < $auth_server_count; j++ )); do
        #增加服务器section
        #[server-2]
        #cluster-port = 11013
        #service-port = 11014
        #host = myhostname
        echo "[server-$(( $j + 1 ))]" >> $ta_servers_conf
        echo "cluster-port = ${auth_cluster_ports[$j]}" >> $ta_servers_conf
        echo "service-port = ${auth_service_ports[$j]}" >> $ta_servers_conf
        echo "host = ${auth_hosts[$j]}" >> $ta_servers_conf
        echo "" >> $ta_servers_conf
      done
    else
      echo "ERROR:Create servers.conf from $A_SERVERS_TPL for auth failed!"
    fi

    ta_client_conf=$target_auth/client.conf
    if cp -f $A_CLIENT_TPL $ta_client_conf; then
      # Replace placeholders with reality in client template
      echo "INFO:Begin config $ta_client_conf..."
      placeholder_replace $ta_client_conf BASE_PATH "${auth_pathes[$i]}"
    else
      echo "ERROR:Create client.conf from $A_CLIENT_TPL for auth failed!"
    fi

    ta_keys_path=$target_auth/keys
    if ! mkdir -p $ta_keys_path; then
      echo "ERROR:Create target conf/keys path failed:$ta_keys_path!"
      exit 1
    fi
    ta_session_validate_key=$ta_keys_path/session_validate.key
    if cp -f $A_SESSION_VALIDATE_KEY_TPL $ta_session_validate_key; then
      # Replace placeholders with reality in auth cluster template
      echo "INFO:Copy $ta_session_validate_key success."
    else
      echo "ERROR:Copy session_validate.key from $A_SESSION_VALIDATE_KEY_TPL for validate key failed!"
    fi

    ta_auth_conf=$target_auth/auth.conf
    if cp -f $A_AUTH_TPL $ta_client_conf; then
      # Replace placeholders with reality in auth template
      echo "INFO:Begin config $ta_auth_conf..."
      placeholder_replace $ta_auth_conf CLIENT_CONFIG_FILENAME "${ta_client_conf}"
      placeholder_replace $ta_auth_conf SECRET_KEY_PATH "${ta_keys_path}"
      placeholder_replace $ta_auth_conf SESSION_VALIDATE_KEY_FILENAME "${ta_session_validate_key}"
    else
      echo "ERROR:Create auth.conf from $A_AUTH_TPL for auth failed!"
    fi

    ta_session_conf=$target_auth/session.conf
    if cp -f $A_SESSION_TPL $ta_session_conf; then
      # Replace placeholders with reality in session template
      echo "INFO:Begin config $ta_session_conf..."
      placeholder_replace $ta_session_conf SESSION_VALIDATE_KEY_FILENAME "${ta_session_validate_key}"
    else
      echo "ERROR:Create session.conf from $A_SESSION_TPL for auth failed!"
    fi

    if [[ $i -eq 0 ]]; then
      fastdir_cluster_auth_config_filename=$ta_auth_conf
      fastdir_server_session_config_filename=$ta_session_conf
      faststore_cluster_auth_config_filename=$ta_auth_conf
      faststore_server_session_config_filename=$ta_session_conf
    fi
  done
}

fuse_dir_cluster_config_filename="cluster.conf"
auth_server_client_config_filename="../fdir/client.conf"

init_fastdir_config() {
  # if [[ ${#dir_pathes[*]} -le 0 ]] || [[ $dir_server_count -le 0 ]]; then
  #   echo "Parameters --dir-path and dir-server-count not specified, would not config $FDIR_LIB"
  #   exit 1
  # fi
  # Init config fastDIR to target path.
  echo "INFO:Will initialize $dir_server_count instances for $FDIR_LIB..."

  D_SERVER_TPL="./template/fastDIR/server.template"
  D_CLUSTER_TPL="./template/fastDIR/cluster.template"
  D_SERVERS_TPL="./template/fastDIR/servers.template"
  D_CLIENT_TPL="./template/fastDIR/client.template"

  check_config_file $D_SERVER_TPL
  check_config_file $D_CLUSTER_TPL
  check_config_file $D_SERVERS_TPL
  check_config_file $D_CLIENT_TPL

  # Create fastDIR cluster op shell from template
  dir_cluster_shell="$BUILD_PATH/shell/fastdir-cluster.sh"
  if [[ -f $CLUSTER_SHELL_TPL ]]; then
    if cp -f $CLUSTER_SHELL_TPL $dir_cluster_shell; then
      chmod u+x $dir_cluster_shell
      placeholder_replace $dir_cluster_shell CLUSTER_NAME "$FDIR_LIB"
    else
      echo "WARNING:Create $dir_cluster_shell from $CLUSTER_SHELL_TPL failed!"
    fi
  fi

  for (( i=0; i < $dir_server_count; i++ )); do
    target_dir=${dir_pathes[$i]}/conf
    if [ -d $target_dir ]; then
      echo "WARNING:The $(( $i + 1 ))th $FDIR_LIB instance conf path \"$target_dir\" have existed, skip!"
      echo "WARNING:If you want to reconfig it, you must delete it first."
      continue
    fi

    if ! mkdir -p $target_dir; then
      echo "ERROR:Create target conf path failed:$target_dir!"
      exit 1
    fi

    td_server_conf=$target_dir/server.conf
    if cp -f $D_SERVER_TPL $td_server_conf; then
      # Replace placeholders with reality in server template
      echo "INFO:Begin config $td_server_conf..."
      placeholder_replace $td_server_conf BASE_PATH "${dir_pathes[$i]}"
      placeholder_replace $td_server_conf CLUSTER_BIND_ADDR "${dir_bind_addrs[$i]}"
      placeholder_replace $td_server_conf CLUSTER_PORT "${dir_cluster_ports[$i]}"
      placeholder_replace $td_server_conf SERVICE_BIND_ADDR "${dir_bind_addrs[$i]}"
      placeholder_replace $td_server_conf SERVICE_PORT "${dir_service_ports[$i]}"
      placeholder_replace $td_server_conf SESSION_CONFIG_FILENAME "${fastdir_server_session_config_filename}"
      
      # Add fdir_serverd command to $dir_cluster_shell
      if [[ -f $dir_cluster_shell ]]; then
        echo "fdir_serverd $td_server_conf \$mode" >> $dir_cluster_shell
      fi
    else
      echo "ERROR:Create server.conf from $D_SERVER_TPL failed!"
    fi

    td_cluster_conf=$target_dir/cluster.conf
    if [[ $i -eq 0 ]]; then
      fuse_dir_cluster_config_filename=$td_cluster_conf
    fi

    if cp -f $D_CLUSTER_TPL $td_cluster_conf; then
      # Replace placeholders with reality in fastDIR cluster template
      # config the auth config filename
      # auth_config_filename = /etc/fastcfs/auth/auth.conf
      echo "INFO:Begin config $td_cluster_conf..."
      placeholder_replace $td_cluster_conf AUTH_CONFIG_FILENAME "${fastdir_cluster_auth_config_filename}"
    else
      echo "ERROR:Create cluster.conf from $D_CLUSTER_TPL for fastDIR failed!"
    fi

    td_servers_conf=$target_dir/servers.conf
    if cp -f $D_SERVERS_TPL $td_servers_conf; then
      # Replace placeholders with reality in fastDIR servers template
      echo "INFO:Begin config $td_servers_conf..."
      placeholder_replace $td_servers_conf CLUSTER_PORT "${dir_cluster_ports[0]}"
      placeholder_replace $td_servers_conf SERVICE_PORT "${dir_service_ports[0]}"
      placeholder_replace $td_servers_conf SERVER_HOST "${dir_hosts[0]}"
      for (( j=1; j < $dir_server_count; j++ )); do
        #增加服务器section
        #[server-2]
        #cluster-port = 11013
        #service-port = 11014
        #host = myhostname
        echo "[server-$(( $j + 1 ))]" >> $td_servers_conf
        echo "cluster-port = ${dir_cluster_ports[$j]}" >> $td_servers_conf
        echo "service-port = ${dir_service_ports[$j]}" >> $td_servers_conf
        echo "host = ${dir_hosts[$j]}" >> $td_servers_conf
        echo "" >> $td_servers_conf
      done
    else
      echo "ERROR:Create servers.conf from $SERVERS_TPL for fastDIR failed!"
    fi

    td_client_conf=$target_dir/client.conf
    if cp -f $D_CLIENT_TPL $td_client_conf; then
      # Replace placeholders with reality in client template
      echo "INFO:Begin config $td_client_conf..."
      placeholder_replace $td_client_conf BASE_PATH "${dir_pathes[$i]}"
    else
      echo "ERROR:Create client.conf from $D_CLIENT_TPL for fastDIR failed!"
    fi
  
    if [[ $i -eq 0 ]]; then
      auth_server_client_config_filename=$td_client_conf
    fi
  done
}

reinit_auth_config() {
  # 替换server.conf中引用的fdir/client.conf.
  echo "INFO:Will reinitialize server.conf for auth..."

  for (( i=0; i < $auth_server_count; i++ )); do
    target_auth=${auth_pathes[$i]}/conf
    ta_server_conf=$target_auth/server.conf
    echo "INFO:Begin reconfig $ta_server_conf..."
    placeholder_replace $ta_server_conf CLIENT_CONFIG_FILENAME "${auth_server_client_config_filename}"
  done
}

fuse_store_cluster_config_filename="cluster.conf"

init_faststore_config() {
  # if [[ ${#store_pathes[*]} -le 0 ]] || [[ $store_server_count -le 0 ]]; then
  #   echo "Parameters --store-path and store-server-count not specified, would not config $STORE_LIB"
  #   exit 1
  # fi
  # Init config faststore to target path.
  echo "INFO:Will initialize $store_server_count instances for $STORE_LIB..."

  S_SERVER_TPL="./template/faststore/server.template"
  S_CLUSTER_TPL="./template/faststore/cluster.template"
  S_CLIENT_TPL="./template/faststore/client.template"
  S_SERVERS_TPL="./template/faststore/servers.template"
  S_STORAGE_TPL="./template/faststore/storage.template"
  S_FUSE_TPL="./template/fastCFS/fuse.template"

  check_config_file $S_SERVER_TPL
  check_config_file $S_CLUSTER_TPL
  check_config_file $S_CLIENT_TPL
  check_config_file $S_SERVERS_TPL
  check_config_file $S_STORAGE_TPL
  check_config_file $S_FUSE_TPL

  # Create faststore cluster op shell from template
  store_cluster_shell="$BUILD_PATH/shell/faststore-cluster.sh"
  if [[ -f $CLUSTER_SHELL_TPL ]]; then
    if cp -f $CLUSTER_SHELL_TPL $store_cluster_shell; then
      chmod u+x $store_cluster_shell
      placeholder_replace $store_cluster_shell CLUSTER_NAME "$STORE_LIB"
    else
      echo "WARNING:Create $store_cluster_shell from $CLUSTER_SHELL_TPL failed!"
    fi
  fi

  for (( i=0; i < $store_server_count; i++ )); do
    target_path=${store_pathes[$i]}/conf
    if [ -d $target_path ]; then
      echo "WARNING:The $(( $i + 1 ))th $STORE_LIB instance conf path \"$target_path\" have existed, skip!"
      echo "WARNING:If you want to reconfig it, you must delete it first."
      continue
    fi

    if ! mkdir -p $target_path; then
      echo "ERROR:Create target conf path failed:$target_path!"
      exit 1
    fi

    t_server_conf=$target_path/server.conf
    if cp -f $S_SERVER_TPL $t_server_conf; then
      # Replace placeholders with reality in server template
      echo "INFO:Begin config $t_server_conf..."
      placeholder_replace $t_server_conf BASE_PATH "${store_pathes[$i]}"
      placeholder_replace $t_server_conf CLUSTER_BIND_ADDR "${store_bind_addrs[$i]}"
      placeholder_replace $t_server_conf CLUSTER_PORT "${store_cluster_ports[$i]}"
      placeholder_replace $t_server_conf SERVICE_BIND_ADDR "${store_bind_addrs[$i]}"
      placeholder_replace $t_server_conf SERVICE_PORT "${store_service_ports[$i]}"
      placeholder_replace $t_server_conf REPLICA_BIND_ADDR "${store_bind_addrs[$i]}"
      placeholder_replace $t_server_conf REPLICA_PORT "${store_replica_ports[$i]}"
      placeholder_replace $t_server_conf SESSION_CONFIG_FILENAME "${faststore_server_session_config_filename}"

      # Add fs_serverd command to $store_cluster_shell
      if [[ -f $store_cluster_shell ]]; then
        echo "fs_serverd $t_server_conf \$mode" >> $store_cluster_shell
      fi
    else
      echo "ERROR:Create server.conf from $S_SERVER_TPL failed!"
    fi

    t_servers_conf=$target_path/servers.conf
    if cp -f $S_SERVERS_TPL $t_servers_conf; then
      # Replace placeholders with reality in servers template
      echo "INFO:Begin config $t_servers_conf..."
      placeholder_replace $t_servers_conf CLUSTER_PORT "${store_cluster_ports[0]}"
      placeholder_replace $t_servers_conf REPLICA_PORT "${store_replica_ports[0]}"
      placeholder_replace $t_servers_conf SERVICE_PORT "${store_service_ports[0]}"
      placeholder_replace $t_servers_conf SERVER_HOST "${store_hosts[0]}"
      for (( j=1; j < $store_server_count; j++ )); do
        #增加服务器section
        #[server-2]
        #cluster-port = 11013
        #replica-port = 21017
        #service-port = 11014
        #host = myhostname
        echo "[server-$(( $j + 1 ))]" >> $t_servers_conf
        echo "cluster-port = ${store_cluster_ports[$j]}" >> $t_servers_conf
        echo "replica-port = ${store_replica_ports[$j]}" >> $t_servers_conf
        echo "service-port = ${store_service_ports[$j]}" >> $t_servers_conf
        echo "host = ${store_hosts[$j]}" >> $t_servers_conf
        echo "" >> $t_servers_conf
      done
    else
      echo "ERROR:Create servers.conf from $S_SERVERS_TPL failed!"
    fi

    t_client_conf=$target_path/client.conf
    if cp -f $S_CLIENT_TPL $t_client_conf; then
      # Replace placeholders with reality in client template
      echo "INFO:Begin config $t_client_conf..."
      placeholder_replace $t_client_conf BASE_PATH "${store_pathes[$i]}"
    else
      echo "ERROR:Create client.conf from $S_CLIENT_TPL failed!"
    fi

    t_cluster_conf=$target_path/cluster.conf
    
    if [[ $i -eq 0 ]]; then
      fuse_store_cluster_config_filename=$t_cluster_conf
    fi

    if cp -f $S_CLUSTER_TPL $t_cluster_conf; then
      # Replace placeholders with reality in cluster template
      echo "INFO:Begin config $t_cluster_conf..."
      placeholder_replace $t_cluster_conf SERVER_GROUP_COUNT "1"
      placeholder_replace $t_cluster_conf DATA_GROUP_COUNT "64"
      placeholder_replace $t_cluster_conf SERVER_GROUP_1_SERVER_IDS "[1, $store_server_count]"
      placeholder_replace $t_cluster_conf AUTH_CONFIG_FILENAME "$faststore_cluster_auth_config_filename"

      data_group_ids='data_group_ids = [1, 32]\
data_group_ids = [33, 64]'
      if [ "$uname" = "FreeBSD" ] || [ "$uname" = "Darwin" ]; then
        sed -i "" "s#\\\${DATA_GROUP_IDS}#${data_group_ids}#g" $t_cluster_conf
      else
        sed -i "s#\\\${DATA_GROUP_IDS}#${data_group_ids}#g" $t_cluster_conf
      fi
    fi

    t_storage_conf=$target_path/storage.conf
    if cp -f $S_STORAGE_TPL $t_storage_conf; then
      # Replace placeholders with reality in storage template
      echo "INFO:Begin config $t_storage_conf..."
      placeholder_replace $t_storage_conf DATA1_PATH "${store_pathes[$i]}/storage_data1"
      #placeholder_replace $t_storage_conf DATA2_PATH "${store_pathes[$i]}/storage_data2"
    fi

    if [[ $same_host = false ]] || [[ $i -eq 0 ]]; then
      if ! [[ -d $fuse_path ]]; then
        if ! mkdir -p $fuse_path; then
          echo "WARNING:Create fuse base_path $fuse_path failed!"
        fi
      fi
      if ! [[ -d $fuse_mount_point ]]; then
        if ! mkdir -p $fuse_mount_point; then
          echo "WARNING:Create fuse mount point $fuse_mount_point failed!"
        fi
      fi
      fuse_conf_path=$fuse_path/conf
      if ! [[ -d $fuse_conf_path ]]; then
        if ! mkdir -p $fuse_conf_path; then
          echo "WARNING:Create fuse conf path $fuse_conf_path failed!"
        fi
      fi
      t_fuse_conf=$fuse_conf_path/fuse.conf
      if cp -f $S_FUSE_TPL $t_fuse_conf; then
        # Replace placeholders with reality in fuse template
        echo "INFO:Begin config $t_fuse_conf..."
        placeholder_replace $t_fuse_conf BASE_PATH "$fuse_path"
        placeholder_replace $t_fuse_conf FUSE_MOUNT_POINT "$fuse_mount_point"
        
        #替换fastDIR、faststore服务器集群配置文件占位符
        placeholder_replace $t_fuse_conf DIR_CLUSTER_CONFIG_FILENAME "$fuse_dir_cluster_config_filename"
        placeholder_replace $t_fuse_conf STORE_CLUSTER_CONFIG_FILENAME "$fuse_store_cluster_config_filename"

        # Create fuse op shell from template
        fuse_shell="$BUILD_PATH/shell/fuse.sh"
        if [[ -f $CLUSTER_SHELL_TPL ]]; then
          if cp -f $CLUSTER_SHELL_TPL $fuse_shell; then
            chmod u+x $fuse_shell
            placeholder_replace $fuse_shell CLUSTER_NAME "fuse"
            # Add fcfs_fused command to $fuse_shell
            echo "fcfs_fused $t_fuse_conf \$mode" >> $fuse_shell
          else
            echo "WARNING:Create $fuse_shell from $CLUSTER_SHELL_TPL failed!"
          fi
        fi
      else
        echo "ERROR:Create fuse.conf from $S_FUSE_TPL failed!"
      fi
    fi
  done
}

get_first_local_ip() {
  if [ -f /usr/sbin/ifconfig ]; then
    IFCONFIG=/usr/sbin/ifconfig
  elif [ -f /sbin/ifconfig ]; then
    IFCONFIG=/sbin/ifconfig
  else
    echo "can't find ifconfig command" 1>&2
    exit
  fi

  CMD="$IFCONFIG -a | grep -w inet | grep -v 127.0.0.1 | awk '{print \$2}' | tr -d 'addr:' | head -n 1"
  sh -c "$CMD"
}

case "$mode" in
  'pull')
    # Clone or pull source code from github.com if not exists.

    echo "INFO:Begin to pull source codes..."
    # Pull fastCFS self.
    git pull
    # Create build path if not exists.
    if ! [ -d $BUILD_PATH ]; then
      mkdir -m 775 $BUILD_PATH
      echo "INFO:Build path {$BUILD_PATH} not exist, created."
    fi
    cd $BUILD_PATH
    
    # Pull libfastcommon
    pull_source_code $COMMON_LIB $COMMON_LIB_URL 

    # Pull libserverframe
    pull_source_code $FRAME_LIB $FRAME_LIB_URL

    # Pull fastDIR
    pull_source_code $FDIR_LIB $FDIR_LIB_URL

    # Pull faststore
    pull_source_code $STORE_LIB $STORE_LIB_URL
    cd ..
  ;;

  'makeinstall')
    # Make and install all lib repositories sequentially.
    make_op $COMMON_LIB clean
    make_op $COMMON_LIB make
    make_op $COMMON_LIB install

    make_op $FRAME_LIB clean
    make_op $FRAME_LIB make
    make_op $FRAME_LIB install

    make_op $AUTHCLIENT_LIB clean
    make_op $AUTHCLIENT_LIB make
    make_op $AUTHCLIENT_LIB install

    make_op $FDIR_LIB clean
    make_op $FDIR_LIB make
    make_op $FDIR_LIB install

    make_op $STORE_LIB clean
    make_op $STORE_LIB make
    make_op $STORE_LIB install

    make_op $FASTCFS_LIB clean
    make_op $FASTCFS_LIB make
    make_op $FASTCFS_LIB install
  ;;

  'init')
    # Config FastDIR and faststore cluster.
    if [[ $# -lt 3 ]]; then 
      IP=$(get_first_local_ip)
      echo "Usage for demo: "
      echo "	$0 init \\"
      echo "	--auth-path=/usr/local/fastcfs-test/auth \\"
      echo "	--auth-server-count=1 \\"
      echo "	--auth-host=$IP \\"
      echo "	--auth-cluster-port=61011 \\"
      echo "	--auth-service-port=71011 \\"
      echo "	--auth-bind-addr= \\"
      echo "	--dir-path=/usr/local/fastcfs-test/fastdir \\"
      echo "	--dir-server-count=1 \\"
      echo "	--dir-host=$IP \\"
      echo "	--dir-cluster-port=11011 \\"
      echo "	--dir-service-port=21011 \\"
      echo "	--dir-bind-addr= \\"
      echo "	--store-path=/usr/local/fastcfs-test/faststore \\"
      echo "	--store-server-count=1 \\"
      echo "	--store-host=$IP \\"
      echo "	--store-cluster-port=31011 \\"
      echo "	--store-service-port=41011 \\"
      echo "	--store-replica-port=51011 \\"
      echo "	--store-bind-addr= \\"
      echo "	--fuse-path=/usr/local/fastcfs-test/fuse \\"
      echo "	--fuse-mount-point=/usr/local/fastcfs-test/fuse/fuse1"
    else
      parse_init_arguments $*
      if ! [ -d $BUILD_PATH/shell ]; then
        mkdir -m 775 $BUILD_PATH/shell
        echo "INFO:Build path {$BUILD_PATH/shell} not exist, created."
      fi
      init_auth_config
      init_fastdir_config
      reinit_auth_config
      init_faststore_config
    fi
  ;;

  'clean')
    # Clean all lib repositories
    make_op $COMMON_LIB clean
    make_op $FRAME_LIB clean
    make_op $FDIR_LIB clean
    make_op $STORE_LIB clean
    make_op $FASTCFS_LIB clean
  ;;

  *)
    # usage
    echo "Usage: $0 {pull|makeinstall|init|clean} [options]"
    exit 1
  ;;
esac

exit 0
