#!/bin/bash
#
# This shell will make and install these four libs in order:
#     libfastcommon
#     libserverframe
#     fastDIR
#     faststore
#
# If no source code in build path, it will git clone from:
#     https://github.com/happyfish100/libfastcommon.git
#     https://github.com/happyfish100/libserverframe.git
#     https://github.com/happyfish100/fastDIR.git
#     https://github.com/happyfish100/faststore.git
#

# FastCFS modules and repositores
COMMON_LIB="libfastcommon"
COMMON_LIB_URL="https://github.com/happyfish100/libfastcommon.git"
FRAME_LIB="libserverframe"
FRAME_LIB_URL="https://github.com/happyfish100/libserverframe.git"
FDIR_LIB="fastDIR"
FDIR_LIB_URL="https://github.com/happyfish100/fastDIR.git"
STORE_LIB="faststore"
STORE_LIB_URL="https://github.com/happyfish100/faststore.git"
FASTCFS_LIB="FastCFS"

DEFAULT_CLUSTER_SIZE=3
DEFAULT_HOST=127.0.0.1
DEFAULT_BIND_ADDR=

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

  if [ $module_name = $FASTCFS_LIB ]; then
    echo "INFO:=====Begin to $make_mode module $module_name...====="
      command="./$make_shell $make_mode"
      echo "INFO:Execute command=$command, path=$PWD"
      if [ $make_mode = "make" ]; then
          result=`./$make_shell`
        else
          result=`./$make_shell $make_mode`
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
  validate_fastdir_params
  validate_faststore_params
}

check_config_file() {
  if ! [ -f $1 ]; then
    echo "ERROR:File $1 does not exist, can't init config from templates!"
    exit 1
  fi
}

init_fastdir_config() {
  # if [[ ${#dir_pathes[*]} -le 0 ]] || [[ $dir_server_count -le 0 ]]; then
  #   echo "Parameters --dir-path and dir-server-count not specified, would not config $FDIR_LIB"
  #   exit 1
  # fi
  # Init config fastDIR to target path.
  echo "INFO:Will initialize $dir_server_count instances for $FDIR_LIB..."

  SERVER_TPL="./template/fastDIR/server.template"
  CLUSTER_TPL="./template/fastDIR/cluster_servers.template"
  CLIENT_TPL="./template/fastDIR/client.template"

  check_config_file $SERVER_TPL
  check_config_file $CLUSTER_TPL
  check_config_file $CLIENT_TPL

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

    t_server_conf=$target_dir/server.conf
    if cp -f $SERVER_TPL $t_server_conf; then
      # Replace placeholders with reality in server template
      echo "INFO:Begin config $t_server_conf..."
      placeholder_replace $t_server_conf BASE_PATH "${dir_pathes[$i]}"
      placeholder_replace $t_server_conf CLUSTER_BIND_ADDR "${dir_bind_addrs[$i]}"
      placeholder_replace $t_server_conf CLUSTER_PORT "${dir_cluster_ports[$i]}"
      placeholder_replace $t_server_conf SERVICE_BIND_ADDR "${dir_bind_addrs[$i]}"
      placeholder_replace $t_server_conf SERVICE_PORT "${dir_service_ports[$i]}"
      
      # Add fdir_serverd command to $dir_cluster_shell
      if [[ -f $dir_cluster_shell ]]; then
        echo "fdir_serverd $t_server_conf \$mode" >> $dir_cluster_shell
      fi
    else
      echo "ERROR:Create server.conf from $SERVER_TPL failed!"
    fi

    t_cluster_conf=$target_dir/cluster_servers.conf
    if cp -f $CLUSTER_TPL $t_cluster_conf; then
      # Replace placeholders with reality in cluster_servers template
      echo "INFO:Begin config $t_cluster_conf..."
      placeholder_replace $t_cluster_conf CLUSTER_PORT "${dir_cluster_ports[0]}"
      placeholder_replace $t_cluster_conf SERVICE_PORT "${dir_service_ports[0]}"
      placeholder_replace $t_cluster_conf SERVER_HOST "${dir_hosts[0]}"
      for (( j=1; j < $dir_server_count; j++ )); do
        #增加服务器section
        #[server-2]
        #cluster-port = 11013
        #service-port = 11014
        #host = myhostname
        echo "[server-$(( $j + 1 ))]" >> $t_cluster_conf
        echo "cluster-port = ${dir_cluster_ports[$j]}" >> $t_cluster_conf
        echo "service-port = ${dir_service_ports[$j]}" >> $t_cluster_conf
        echo "host = ${dir_hosts[$j]}" >> $t_cluster_conf
        echo "" >> $t_cluster_conf
      done
    else
      echo "ERROR:Create cluster_servers.conf from $CLUSTER_TPL failed!"
    fi

    t_client_conf=$target_dir/client.conf
    if cp -f $CLIENT_TPL $t_client_conf; then
      # Replace placeholders with reality in client template
      echo "INFO:Begin config $t_client_conf..."
      placeholder_replace $t_client_conf BASE_PATH "${dir_pathes[$i]}"
      #替换fastDIR服务器占位符
      #dir_server = 192.168.0.196:11012
      t_dir_servers=""
      cr='\
      '
      for (( j=0; j < $dir_server_count; j++ )); do
        t_dir_servers=$t_dir_servers'dir_server = '${dir_hosts[$j]}':'${dir_service_ports[$j]}'\
'
      done
      #placeholder_replace $t_client_conf DIR_SERVERS "$t_dir_servers"
      if [ "$uname" = "FreeBSD" ] || [ "$uname" = "Darwin" ]; then
        sed -i "" "s#\\\${DIR_SERVERS}#${t_dir_servers}#g" $t_client_conf
      else
        sed -i "s#\\\${DIR_SERVERS}#${t_dir_servers}#g" $t_client_conf
      fi
    else
      echo "ERROR:Create client.conf from $CLIENT_TPL failed!"
    fi
  done
}

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
    if cp -f $S_CLUSTER_TPL $t_cluster_conf; then
      # Replace placeholders with reality in cluster template
      echo "INFO:Begin config $t_cluster_conf..."
      placeholder_replace $t_cluster_conf SERVER_GROUP_COUNT "1"
      placeholder_replace $t_cluster_conf DATA_GROUP_COUNT "64"
      placeholder_replace $t_cluster_conf SERVER_GROUP_1_SERVER_IDS "[1, $store_server_count]"

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
      t_fuse_conf=$target_path/fuse.conf
      if cp -f $S_FUSE_TPL $t_fuse_conf; then
        # Replace placeholders with reality in fuse template
        echo "INFO:Begin config $t_fuse_conf..."
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
        placeholder_replace $t_fuse_conf BASE_PATH "$fuse_path"
        placeholder_replace $t_fuse_conf FUSE_MOUNT_POINT "$fuse_mount_point"
        #替换fastDIR服务器占位符
        #dir_server = 192.168.0.196:11012
        t_dir_servers=""
        for (( j=0; j < $dir_server_count; j++ )); do
          t_dir_servers=$t_dir_servers"dir_server = ${dir_hosts[$j]}:${dir_service_ports[$j]}\n"
        done
        placeholder_replace $t_fuse_conf DIR_SERVERS "$t_dir_servers"

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
    make_op $COMMON_LIB make
    make_op $COMMON_LIB install
    make_op $FRAME_LIB make
    make_op $FRAME_LIB install
    make_op $FDIR_LIB make
    make_op $FDIR_LIB install
    make_op $STORE_LIB make
    make_op $STORE_LIB install
    make_op $FASTCFS_LIB make
    make_op $FASTCFS_LIB install
  ;;

  'init')
    # Config FastDIR and faststore cluster.
    if [[ $# -lt 3 ]]; then 
      IP=$(get_first_local_ip)
      echo "Usage for demo: "
      echo "	$0 init \\"
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
      init_fastdir_config
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
