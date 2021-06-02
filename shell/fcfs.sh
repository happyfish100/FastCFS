#!/bin/bash

fcfs_settings_file="fcfs.settings"
#fcfs_dependency_file_server="http://www.fastken.com"
fcfs_dependency_file_server="http://localhost:8082"

STORE_CONF_FILES=(client.conf server.conf cluster.conf storage.conf)
STORE_CONF_PATH="/etc/fastcfs/fstore/"

FDIR_CONF_FILES=(client.conf cluster.conf server.conf)
FDIR_CONF_PATH="/etc/fastcfs/fdir/"

AUTH_CONF_FILES=(auth.conf client.conf cluster.conf server.conf session.conf)
AUTH_CONF_PATH="/etc/fastcfs/auth/"
AUTH_KEYS_FILES=(session_validate.key)
AUTH_KEYS_PATH="/etc/fastcfs/auth/keys/"

FUSE_CONF_FILES=(fuse.conf)
FUSE_CONF_PATH="/etc/fastcfs/fcfs/"

module_names=(fdir fstore fauth fuseclient)

shell_name=$0
shell_command=$1
uname=$(uname)

#---Usage info section begin---#
print_usage() {
  # Print usage to console.
  echo ""
  echo "Usage: $shell_name <command> [module] [node]"
  echo ""
  echo "A self-sufficient operation shell for FastCFS cluster"
  echo ""
  echo "Commands:"
  echo "  setup      This command setup FastCFS cluster, a Shortcut command combines install, config, and restart"
  echo "  install    This command install FastCFS cluster's dependent libs with yum"
  echo "  config     This command copy cluster config file to target host path"
  echo "  start      This command start all or one module service in cluster"
  echo "  stop       This command stop  all or one module service in cluster"
  echo "  restart    This command restart all or one module service in cluster"
  echo "  tail       This command view the last N lines of the specified module's log"
  echo "  help       This command show the detail of commands and examples"
  echo ""
}

print_detail_usage() {
  # Print usage to console.
  print_usage
  echo "Modules:"
  echo "  fdir       fastDIR"
  echo "  fstore     faststore"
  echo "  fauth      FastCFS auth client module"
  echo "  fuseclient   FastCFS client use with fuse"
  echo ""
  echo "Node:"
  echo "  You can specify a single cluster IP, or command will be executed on all nodes."
  echo ""
  echo "Examples:"
  echo "  $shell_name setup"
  echo "      -- Setup FastCFS cluster's all modules on all nodes, and then config and start module services."
  echo ""
  echo "  $shell_name install"
  echo "  $shell_name config fdir 172.16.168.128"
  echo "  $shell_name start fdir 172.16.168.128"
  echo "  $shell_name stop fdir 172.16.168.128"
  echo "  $shell_name restart fdir 172.16.168.128"
  echo "  $shell_name tail fdir 172.16.168.128"
  echo ""
}

case "$shell_command" in
  'setup' | 'install' | 'config' | 'start' | 'restart' | 'stop' | 'tail')
  ;;
  'help')
    print_detail_usage
    exit 1
  ;;
  *)
    print_usage
    exit 1
  ;;
esac
#---Usage info section end---#

#---Settings and cluster info section begin---#
split_to_array() {
  if ! [ -z $2 ]; then
    IFS=',' read -ra $2 <<< "$1"
  fi
}

# Load cluster settings from file fcfs.settings
load_fcfs_settings() {
  if ! [ -f $fcfs_settings_file ]; then
    echo "ERROR: file $fcfs_settings_file not exist"
    exit 1
  else
    fcfs_settings=`sed -e 's/#.*//' -e '/^$/ d' $fcfs_settings_file`
    eval $fcfs_settings
    split_to_array $fuseclient_ips fuseclient_ip_array
    # if [ -z $fastcfs_version ]; then
    #   echo "WARN: param fastcfs_version has no value in $fcfs_settings_file"
    # fi
    # if [ -z $fuseclient_ips ]; then
    #   echo "WARN: param fuseclient_ips has no value in $fcfs_settings_file"
    # fi 
  fi
}

# Load cluster dependent lib version settings from file dependency.[FastCFS-version].settings
load_dependency_settings() {
  dependency_file_version=$1
  if [ -z $dependency_file_version ]; then
    echo "Error: dependency file version cannot be empty"
    exit 1
  fi
  dependency_settings_file="dependency.$dependency_file_version.settings"
  if ! [ -f $dependency_settings_file ]; then
    # File not exist in local path, will get it from remote server match the version
    echo "WARN: file $dependency_settings_file not exist, getting it from remote server $fcfs_dependency_file_server"
    remote_file_url="$fcfs_dependency_file_server/$dependency_settings_file"
    download_res=`curl -f -o $dependency_settings_file $remote_file_url`

    if ! [ -f $dependency_settings_file ]; then
      echo "Error: cannot download file $dependency_settings_file from $remote_file_url"
      echo "Please make sure that remote file exists and network is accessible"
      exit 1
    fi
  fi
  dependency_settings=`sed -e 's/#.*//' -e '/^$/ d' $dependency_settings_file`
  eval $dependency_settings
  if [ -z $libfastcommon ]; then
    echo "WARN: dependency libfastcommon has no version value in $dependency_settings_file"
  fi
  if [ -z $libserverframe ]; then
    echo "WARN: dependency libserverframe has no version value in $dependency_settings_file"
  fi
  if [ -z $fauth ]; then
    echo "WARN: dependency fauth has no version value in $dependency_settings_file"
  fi
  if [ -z $fdir ]; then
    echo "WARN: dependency fdir has no version value in $dependency_settings_file"
  fi
  if [ -z $fstore ]; then
    echo "WARN: dependency fstore has no version value in $dependency_settings_file"
  fi
}

# Get module name from command args, if not specify, will handle all modules.
fdir_need_execute=0
fstore_need_execute=0
fauth_need_execute=0
fuseclient_need_execute=0

check_module_param() {
  if [ $# -gt 1 ]; then
    module_name=$2
    case "$module_name" in
      'fdir')
        fdir_need_execute=1
      ;;
      'fstore')
        fstore_need_execute=1
      ;;
      'fauth')
        fauth_need_execute=1
      ;;
      'fuseclient')
        fuseclient_need_execute=1
      ;;
      *)
        echo "Error: module name invalid:$module_name"
        echo "Allowed module names:fdir, fstore, fauth, fuseclient"
        exit 1
      ;;
    esac
  else
    fdir_need_execute=1
    fstore_need_execute=1
    fauth_need_execute=1
    fuseclient_need_execute=1
  fi
}

# Get node host from command args, if not specify, will handle all nodes.
# It must specify after module name.
check_node_param() {
  if [ $# -gt 2 ]; then
    node_host_need_execute=$3
  fi
}

list_servers_in_config() {
  list_program=$1
  list_config_file=$2
  cmd_exist=`which $list_program`
  if [ -z "$cmd_exist" ]; then
    echo "Error: program $list_program not found, you must install it first."
    exit 1
  else
    if ! [ -f $list_config_file ]; then
      echo "Error: cluster config file $list_config_file not exist"
      exit 1
    else
      list_result=$($list_program $list_config_file)
      echo "Info: get servers from $list_config_file:"
      echo "$list_result"
      eval $list_result
    fi
  fi
}

# Load cluster groups from cluster config files.
#   fdir_group=(172.16.168.128)
#   fstore_group_count=1
#   fstore_group_1=(172.16.168.128)
#   fauth_group=(172.16.168.128)
load_cluster_groups() {
  if [ $fdir_need_execute -eq 1 ]; then
    list_servers_in_config fdir_list_servers conf/fdir/cluster.conf
  fi
  if [ $fstore_need_execute -eq 1 ]; then
    list_servers_in_config fstore_list_servers conf/fstore/cluster.conf
  fi
  if [ $fauth_need_execute -eq 1 ]; then
    list_servers_in_config fauth_list_servers conf/fauth/cluster.conf
  fi
}
#---Settings and cluster info section end---#

#---Iterate hosts for execute command section begin---#
execute_command_on_fdir_servers() {
  command_name=$1
  function_name=$2
  module_name=$3
  if [ $fdir_need_execute -eq 1 ]; then
    fdir_node_match_setting=0
    for fdir_server_ip in ${fdir_group[@]}; do
      if [ -z $node_host_need_execute ] || [ $fdir_server_ip = "$node_host_need_execute" ]; then
        echo "Info: begin $command_name $module_name on server $fdir_server_ip"
        $function_name $fdir_server_ip $module_name $command_name
      fi
      if [ $fdir_server_ip = "$node_host_need_execute" ]; then
        fdir_node_match_setting=1
      fi
    done
    if ! [ -z $node_host_need_execute ] && [ $fdir_node_match_setting -eq 0 ]; then
      echo "Error: the node $node_host_need_execute not match host in fdir cluster.conf"
    fi
  fi
}

execute_command_on_fstore_servers() {
  command_name=$1
  function_name=$2
  module_name=$3
  if [ $fstore_need_execute -eq 1 ]; then
    fstore_node_match_setting=0
    for (( i=1 ; i <= $fstore_group_count ; i++ )); do
      fstore_group="fstore_group_$i[@]"
      for fstore_server_ip in ${!fstore_group}; do
        if [ -z $node_host_need_execute ] || [ $fstore_server_ip = "$node_host_need_execute" ]; then
          echo "Info: begin $command_name $module_name on server $fstore_server_ip"
          $function_name $fstore_server_ip $module_name $command_name
        fi
        if [ $fstore_server_ip = "$node_host_need_execute" ]; then
          fstore_node_match_setting=1
        fi
      done
    done
    if ! [ -z $node_host_need_execute ] && [ $fstore_node_match_setting -eq 0 ]; then
      echo "Error: the node $node_host_need_execute not match host in fstore cluster.conf"
    fi
  fi
}

execute_command_on_fauth_servers() {
  command_name=$1
  function_name=$2
  module_name=$3
  if [ $fauth_need_execute -eq 1 ]; then
    fauth_node_match_setting=0
    for fauth_server_ip in ${fauth_group[@]}; do
      if [ -z $node_host_need_execute ] || [ $fauth_server_ip = "$node_host_need_execute" ]; then
        echo "Info: begin $command_name $module_name on server $fauth_server_ip"
        $function_name $fauth_server_ip $module_name $command_name
      fi
      if [ $fauth_server_ip = "$node_host_need_execute" ]; then
        fauth_node_match_setting=1
      fi
    done
    if ! [ -z $node_host_need_execute ] && [ $fauth_node_match_setting -eq 0 ]; then
      echo "Error: the node $node_host_need_execute not match host in fauth cluster.conf"
    fi
  fi
}

execute_command_on_fuseclient_servers() {
  command_name=$1
  function_name=$2
  module_name=$3
  if [ $fuseclient_need_execute -eq 1 ]; then
    if [ ${#fuseclient_ip_array[@]} -eq 0 ]; then
      echo "WARN: param fuseclient_ips has no value in $fcfs_settings_file"
    else
      fuseclient_node_match_setting=0
      for fuseclient_server_ip in ${fuseclient_ip_array[@]}; do
        if [ -z $node_host_need_execute ] || [ $fuseclient_server_ip = "$node_host_need_execute" ]; then
          echo "Info: begin $command_name $module_name on server $fuseclient_server_ip"
          $function_name $fuseclient_server_ip $module_name $command_name
        fi
        if [ $fuseclient_server_ip = "$node_host_need_execute" ]; then
          fuseclient_node_match_setting=1
        fi
      done
      if ! [ -z $node_host_need_execute ] && [ $fuseclient_node_match_setting -eq 0 ]; then
        echo "Error: the node $node_host_need_execute not match param fuseclient_ips in $fcfs_settings_file"
      fi
    fi
  fi
}
#---Iterate hosts for execute command section end---#

#---Install section begin---#
check_remote_osname() {
  uname=$(uname)
  if [ $uname = 'Linux' ]; then
    osname=$(cat /etc/os-release | grep -w NAME | awk -F '=' '{print $2;}' | \
            awk -F '"' '{if (NF==3) {print $2} else {print $1}}' | awk '{print $1}')
    if [ $osname = 'CentOS' ]; then
      osversion=$(cat /etc/system-release | awk '{print $4}')
    fi
    if [ -z $osversion ]; then
      osversion=$(cat /etc/os-release | grep -w VERSION_ID | awk -F '=' '{print $2;}' | \
              awk -F '"' '{if (NF==3) {print $2} else {print $1}}')
    fi
    os_major_version=$(echo $osversion | awk -F '.' '{print $1}')
  else
    echo "Unsupport OS: $uname" 1>&2
    exit 1
  fi
}

check_install_fastos_repo() {
  repo=$(rpm -q FastOSrepo 2>/dev/null)
  if [ $? -ne 0 ]; then
    if [ $os_major_version -eq 7 ]; then
      rpm -ivh http://www.fastken.com/yumrepo/el7/x86_64/FastOSrepo-1.0.0-1.el7.centos.x86_64.rpm
    else
      rpm -ivh http://www.fastken.com/yumrepo/el8/x86_64/FastOSrepo-1.0.0-1.el8.x86_64.rpm
    fi
  fi
}

install_program() {
  program_name=$1
  if [ $osname = 'CentOS' ] && [ $os_major_version -eq 7 -o $os_major_version -eq 8 ]; then
    check_install_fastos_repo
    if [ $program_name = 'FastCFS-fused' ]; then
      rpm -q fuse >/dev/null && yum remove fuse -y
    fi
    # yum install FastCFS-auth-server fastDIR-server faststore-server FastCFS-fused -y
    yum install $program_name -y
  else
    echo "Unsupport OS: $uname" 1>&2
    echo "Command setup and install can only be used for CentOS 7 or 8."
    exit 1
  fi
}

install_program_on_remote() {
  remote_host=$1
  program_name=$2
  ssh -Tq $remote_host <<EOF
$(typeset -f check_remote_osname)
check_remote_osname
$(typeset -f check_install_fastos_repo)
$(typeset -f install_program)
install_program $program_name
EOF
}

# Install libs to target nodes.
install_dependent_libs() {
  load_dependency_settings $fastcfs_version
  execute_command_on_fdir_servers install install_program_on_remote fastDIR-server
  execute_command_on_fstore_servers install install_program_on_remote faststore-server
  execute_command_on_fauth_servers install install_program_on_remote FastCFS-auth-server
  execute_command_on_fuseclient_servers install install_program_on_remote FastCFS-fused
}
#---Install section end---#

#---Config section begin---#
check_remote_path() {
  dest_path=$1
  if ! [ -d $dest_path ]; then
    if ! mkdir -p $dest_path; then
      echo "ERROR: Create target conf path failed:$dest_path!"
      exit 1
    fi
  fi
}

check_path_on_remote() {
  remote_host=$1
  remote_target_path=$2
  ssh -Tq $remote_host <<EOF
$(typeset -f check_remote_path)
check_remote_path $remote_target_path
EOF
}

check_paths_infile_remote() {
  # Check all paths in this config file, if not exist, will create it.
  check_conf_file=$1
  check_result=`sed -n \
-e '/^mountpoint *=/ p' \
-e '/^base_path *=/ p' \
-e '/^path *=/ p' \
$check_conf_file|sed 's/\([^=]*\)=\([^=]\)/\2/g'`

  for check_path in ${check_result[@]}; do
    if ! [ -d $check_path ]; then
      if ! mkdir -p $check_path; then
        echo "ERROR: Create target path in file $check_conf_file failed:$check_path!"
        exit 1
      else
        echo "INFO: Created target path in file $check_conf_file successfully:$check_path!"
      fi
    fi
  done
}

check_paths_infile() {
  remote_host=$1
  remote_target_file=$2$3
  ssh -Tq $remote_host <<EOF
$(typeset -f check_paths_infile_remote)
check_paths_infile_remote $remote_target_file
EOF
}

copy_config_to_remote() {
  target_server_ip=$1
  target_module=$2
  src_path="conf/$target_module"

  case "$target_module" in
    'fdir')
      config_file_array="${FDIR_CONF_FILES[*]}"
      dest_path=$FDIR_CONF_PATH
    ;;
    'fstore')
      config_file_array="${STORE_CONF_FILES[*]}"
      dest_path=$STORE_CONF_PATH
    ;;
    'fauth')
      config_file_array="${AUTH_CONF_FILES[*]}"
      dest_path=$AUTH_CONF_PATH
    ;;
    'keys')
      config_file_array="${AUTH_KEYS_FILES[*]}"
      dest_path=$AUTH_KEYS_PATH
      src_path="conf/fauth/keys"
    ;;
    'fuseclient')
      config_file_array="${FUSE_CONF_FILES[*]}"
      dest_path=$FUSE_CONF_PATH
    ;;
    *)
      echo "ERROR: target module name is invalid - $target_module"
      exit 1
    ;;
  esac

  check_path_on_remote $target_server_ip $dest_path

  for CONF_FILE in ${config_file_array[@]}; do
    tmp_src_file=$src_path/$CONF_FILE
    if [ -f $tmp_src_file ]; then
      echo "INFO: Copy file $CONF_FILE to $dest_path of server $target_server_ip"
      scp $tmp_src_file $target_server_ip:$dest_path
      file_extension="${CONF_FILE##*.}"
      if [ $file_extension = "conf" ]; then
        check_paths_infile $target_server_ip $dest_path $CONF_FILE
      fi
    else
      echo "WARN: file $tmp_src_file not exist"
    fi
  done
}

deploy_config_files() {
  execute_command_on_fdir_servers config copy_config_to_remote fdir
  execute_command_on_fstore_servers config copy_config_to_remote fstore
  execute_command_on_fauth_servers config copy_config_to_remote fauth
  execute_command_on_fauth_servers config copy_config_to_remote keys
  execute_command_on_fuseclient_servers config copy_config_to_remote fuseclient
}
#---Config section end---#

#---Service op section begin---#
service_op_on_remote() {
  service_name=$1
  operate_mode=$2
  conf_file=$3
  $service_name $conf_file $operate_mode
}

service_op() {
  target_server_ip=$1
  target_module=$2
  operate_mode=$3
  case "$target_module" in
    'fdir')
      service_name="fdir_serverd"
      conf_file="${FDIR_CONF_PATH}server.conf"
    ;;
    'fstore')
      service_name="fs_serverd"
      conf_file="${STORE_CONF_PATH}server.conf"
    ;;
    'fauth')
      service_name="fcfs_authd"
      conf_file="${AUTH_CONF_PATH}server.conf"
    ;;
    'fuseclient')
      service_name="fcfs_fused"
      conf_file="${FUSE_CONF_PATH}server.conf"
    ;;
    *)
      echo "ERROR: target module name is invalid - $target_module"
      exit 1
    ;;
  esac
  ssh -Tq $target_server_ip <<EOF
$(typeset -f service_op_on_remote)
service_op_on_remote $service_name $operate_mode $conf_file
EOF
}

cluster_service_op() {
  operate_mode=$1
  execute_command_on_fdir_servers $operate_mode service_op fdir
  execute_command_on_fstore_servers $operate_mode service_op fstore
  execute_command_on_fauth_servers $operate_mode service_op fauth
  execute_command_on_fuseclient_servers $operate_mode service_op fuseclient
}
#---Service op section end---#

# View last N lines log of the cluster with system command tail;
tail_log() {
  echo ""
}


check_module_param $*
check_node_param $*
load_cluster_groups
load_fcfs_settings
if [ -z $fastcfs_version ]; then
  echo "Error: fastcfs_version in $fcfs_settings_file cannot be empty"
  exit 1
fi

case "$shell_command" in
  'setup')
    install_dependent_libs
    deploy_config_files
    cluster_service_op restart
  ;;
  'install')
    install_dependent_libs
  ;;
  'config')
    deploy_config_files
  ;;
  'start')
    cluster_service_op start
  ;;
  'restart')
    cluster_service_op restart
  ;;
  'stop')
    cluster_service_op stop
  ;;
  'tail')
    tail_log $*
  ;;
  *)
    print_usage
    exit 1
  ;;
esac

exit 0