#!/bin/bash

fcfs_settings_file="fcfs.settings"
fcfs_dependency_file_server="http://www.fastken.com/"
shell_name=$0
shell_command=$1
uname=$(uname)

get_osname() {
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

install_dependent_libs() {
  if [ $osname = 'CentOS' ] && [ $os_major_version -eq 7 -o $os_major_version -eq 8 ]; then
    check_install_fastos_repo
    rpm -q fuse >/dev/null && yum remove fuse -y
    yum install FastCFS-auth-server fastDIR-server faststore-server FastCFS-fused -y
  else
    echo "Unsupport OS: $uname" 1>&2
    echo "Command setup and install can only be used for CentOS 7 or 8."
    exit 1
  fi
}

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
  echo "  fsclient   FastCFS client use with fuse"
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

# Load cluster settings from file fcfs.settings
load_fcfs_settings() {
  if ! [ -f $fcfs_settings_file ]; then
    echo "ERROR: file $fcfs_settings_file not exist"
    exit 1
  else
    fcfs_settings=`sed -e 's/#.*//' -e '/^$/ d' $fcfs_settings_file`
    eval $fcfs_settings
    if [ -z $fastcfs_version ]; then
      echo "WARN: param fastcfs_version has no value in $fcfs_settings_file"
    fi
    if [ -z $fsclient_ips ]; then
      echo "WARN: param fsclient_ips has no value in $fcfs_settings_file"
    fi 
  fi
}

# Load cluster dependent libs settings from file dependency.[FastCFS-version].settings
load_dependency_settings() {
  dependency_file_version=$1
  if [ -z $dependency_file_version ]; then
    echo "Error: dependency file version cannot be empty"
    exit 1
  fi
  dependency_settings_file="dependency.$dependency_file_version.settings"
  if ! [ -f $dependency_settings_file ]; then
    # File not exist in local path, will get it from remote server match the version
    echo "WARN: file $dependency_settings_file not exist, getting it from remote server"
    remote_file_url="$fcfs_dependency_file_server/$dependency_settings_file"
  else
    fcfs_settings=`sed -e 's/#.*//' -e '/^$/ d' $fcfs_settings_file`
    eval $fcfs_settings
    if [ -z $fastcfs_version ]; then
      echo "WARN: param fastcfs_version has no value in $fcfs_settings_file"
    fi
    if [ -z $fsclient_ips ]; then
      echo "WARN: param fsclient_ips has no value in $fcfs_settings_file"
    fi 
  fi
}

load_fcfs_settings


case "$shell_command" in
  'setup')
    install_dependent_libs
    deploy_config_files
    restart_servers
  ;;
  'install')
    install_dependent_libs
  ;;
  'config')
    deploy_config_files
  ;;
  'start')
    # Start services.
    service_op start
  ;;
  'restart')
    # Restart services.
    service_op restart
  ;;
  'stop')
    # Start services.
    service_op stop
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