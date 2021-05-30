#!/bin/bash

fcfs_settings_file="fcfs.settings"
#fcfs_dependency_file_server="http://www.fastken.com"
fcfs_dependency_file_server="http://localhost:8082"
module_names=(fdir fstore fauth fuseclient)
module_programs=([fauth]=FastCFS-auth-server [fdir]=fastDIR-server [fauth]=faststore-server [fuseclient]=FastCFS-fused)
shell_name=$0
shell_command=$1
uname=$(uname)

# View last N lines log of the cluster with system command tail;
tail_log() {
  echo ""
}

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
  echo ""
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
  load_fcfs_settings
  if [ -z $fastcfs_version ]; then
    echo "Error: fastcfs_version in $fcfs_settings_file cannot be empty"
    exit 1
  fi
  load_dependency_settings $fastcfs_version

  if [ $fdir_need_execute -eq 1 ]; then
    for fdir_server_ip in ${fdir_group[@]}; do
      if [ -z $node_host_need_execute ] || [ $fdir_server_ip = $node_host_need_execute ]; then
        echo "Info: begin install fastDIR-server on server $fdir_server_ip"
        install_program_on_remote $fdir_server_ip fastDIR-server
      fi
    done
  fi
  if [ $fstore_need_execute -eq 1 ]; then
    for (( i=1 ; i <= $fstore_group_count ; i++ )); do
      fstore_group="fstore_group_$i[@]"
      for fstore_server_ip in ${!fstore_group}; do
        if [ -z $node_host_need_execute ] || [ $fstore_server_ip = $node_host_need_execute ]; then
          echo "Info: begin install faststore-server on server $fstore_server_ip"
          install_program_on_remote $fstore_server_ip faststore-server
        fi
      done
    done
  fi
  if [ $fauth_need_execute -eq 1 ]; then
    for fauth_server_ip in ${fauth_group[@]}; do
      if [ -z $node_host_need_execute ] || [ $fauth_server_ip = $node_host_need_execute ]; then
        echo "Info: begin install FastCFS-auth-server on server $fauth_server_ip"
        install_program_on_remote $fauth_server_ip FastCFS-auth-server
      fi
    done
  fi
  if [ $fuseclient_need_execute -eq 1 ]; then
    if [ ${#fuseclient_ip_array[@]} -eq 0 ]; then
      echo "WARN: param fuseclient_ips has no value in $fcfs_settings_file"
    else
      node_match_setting=0
      for fuseclient_server_ip in ${fuseclient_ip_array[@]}; do
        if [ -z $node_host_need_execute ] || [ $fuseclient_server_ip = $node_host_need_execute ]; then
          echo "Info: begin install FastCFS-fused on server $fuseclient_server_ip"
          install_program_on_remote $fuseclient_server_ip FastCFS-fused
        fi
        if [ $fuseclient_server_ip = $node_host_need_execute ]; then
          node_match_setting=1
        fi
      done
      if ! [ -z $node_host_need_execute ] && [ $node_match_setting -eq 0 ]; then
        echo "Error: the node $node_host_need_execute not match param fuseclient_ips in $fcfs_settings_file"
      fi
    fi
  fi
}

check_module_param $*
check_node_param $*
load_cluster_groups
# Check if node_host_need_execute match cluster settings or config

case "$shell_command" in
  'setup')
    install_dependent_libs $*
    deploy_config_files $*
    restart_servers $*
  ;;
  'install')
    install_dependent_libs $*
  ;;
  'config')
    deploy_config_files $*
  ;;
  'start')
    # Start services.
    service_op start $*
  ;;
  'restart')
    # Restart services.
    service_op restart $*
  ;;
  'stop')
    # Start services.
    service_op stop $*
  ;;
  'tail')
    # Start services.
    tail_log $*
  ;;
  *)
    # usage
    print_usage
    exit 1
  ;;
esac

exit 0