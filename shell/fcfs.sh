#!/bin/bash
#
# fcfs.sh is a ops tool for quickly deploy FastCFS clusters.
# It only relying on SSH access to the servers and sudo.
# It runs fully on your workstation, requiring no servers, databases, or anything like that.
#
# If you set up and tear down FastCFS clusters a lot, and want minimal extra repeating works,
# this is for you.
#
#---1. Config path section begin---#
declare -ir MIN_VERSION_OF_Ubuntu=18
declare -ir MIN_VERSION_OF_Debian=10
declare -ir MIN_VERSION_OF_CentOS=7
declare -ir MIN_VERSION_OF_Red=8
declare -ir MIN_VERSION_OF_Rocky=8
declare -ir MIN_VERSION_OF_Oracle=8
declare -ir MIN_VERSION_OF_Fedora=20
declare -ir MIN_VERSION_OF_AlmaLinux=7
declare -ir MIN_VERSION_OF_Alibaba=2
declare -ir MIN_VERSION_OF_Anolis=7
declare -ir MIN_VERSION_OF_Amazon=2
YUM_OS_ARRAY=(Red Rocky Oracle Fedora CentOS AlmaLinux Alibaba Anolis Amazon)
APT_OS_ARRAY=(Ubuntu Debian)

fcfs_settings_file="fcfs.settings"
fcfs_dependency_file_server="http://fastcfs.cn/fastcfs/ops/dependency"
fcfs_cache_path=".fcfs"
fcfs_installed_file="installed.settings"

STORE_CONF_FILES=(client.conf server.conf cluster.conf storage.conf)
STORE_CONF_PATH="/etc/fastcfs/fstore/"
STORE_LOG_FILE="/opt/fastcfs/fstore/logs/fs_serverd.log"

FDIR_CONF_FILES=(client.conf cluster.conf server.conf storage.conf)
FDIR_CONF_PATH="/etc/fastcfs/fdir/"
FDIR_LOG_FILE="/opt/fastcfs/fdir/logs/fdir_serverd.log "

VOTE_CONF_FILES=(client.conf cluster.conf server.conf)
VOTE_CONF_PATH="/etc/fastcfs/vote/"
VOTE_LOG_FILE="/opt/fastcfs/vote/logs/fcfs_voted.log "

AUTH_CONF_FILES=(auth.conf client.conf cluster.conf server.conf session.conf)
AUTH_CONF_PATH="/etc/fastcfs/auth/"
AUTH_KEYS_FILES=(session_validate.key)
AUTH_KEYS_PATH="/etc/fastcfs/auth/keys/"
AUTH_LOG_FILE="/opt/fastcfs/auth/logs/fcfs_authd.log"

FUSE_CONF_FILES=(fuse.conf)
FUSE_CONF_PATH="/etc/fastcfs/fcfs/"
FUSE_LOG_FILE="/opt/fastcfs/fcfs/logs/fcfs_fused.log"
#---1. Config path section end---#

#---2. Get shell and command section begin---#
shell_name=$0
shell_command=$1
local_os_uname=$(uname)
#---2. Get shell and command section end---#

#---3. Usage info section begin---#
print_usage() {
  # Print usage to console.
  echo ""
  echo "Usage: $shell_name <command> [module] [node]"
  echo ""
  echo "A self-sufficient operation shell for FastCFS cluster"
  echo ""
  echo "Commands:"
  echo "  setup      Setup FastCFS softwares, a shortcut command combines install, config, and restart"
  echo "  install    Install FastCFS softwares"
  echo "  reinstall  Reinstall FastCFS softwares, only used for yum"
  echo "  erase      Erase FastCFS softwares"
  echo "  remove     Remove FastCFS softwares, same as erase"
  echo "  config     Copy cluster config files to target host path"
  echo "  start      Start all or one module service in cluster"
  echo "  stop       Stop all or one module service in cluster"
  echo "  restart    Restart all or one module service in cluster"
  echo "  tail       Display the last part of the specified module's log"
  echo "  status     Display the service processes status"
  echo "  help       Show the detail of commands and examples"
  echo ""
}

print_detail_usage() {
  # Print usage to console.
  print_usage
  echo "Modules:"
  echo "  fvote        FastCFS vote server"
  echo "  fdir         fastDIR server"
  echo "  fstore       faststore server"
  echo "  fauth        FastCFS auth server"
  echo "  fuseclient   FastCFS fuse client"
  echo ""
  echo "Node:"
  echo "  You can specify a single cluster IP, or command will be executed on all nodes."
  echo ""
  echo "Tail command options:"
  echo "  -n number"
  echo "          The location is number lines"
  echo ""
  echo "  -number"
  echo "          The location is number lines"
  echo ""
  echo "Examples:"
  echo "  $shell_name setup"
  echo "          Setup all FastCFS softwares on all nodes, then config and start module services."
  echo ""
  echo "  $shell_name install"
  echo "          Install all FastCFS softwares on all nodes."
  echo ""
  echo "  $shell_name erase"
  echo "          Erase all FastCFS softwares on all nodes."
  echo ""
  echo "  $shell_name config fdir 172.16.168.128"
  echo "          Copy fdir config file to host 172.16.168.128."
  echo ""
  echo "  $shell_name start fdir"
  echo "          Start all fdir servers."
  echo ""
  echo "  $shell_name start fdir 172.16.168.128"
  echo "          Start fdir server on host 172.16.168.128."
  echo ""
  echo "  $shell_name tail fdir 172.16.168.128 -n 100"
  echo "          Display the last 100 lines of fdir server log."
  echo ""
  echo "  $shell_name status"
  echo "          Display all modules service process status."
  echo ""
  echo "  $shell_name status fdir"
  echo "          Display all the status of fdir service processes status."
  echo ""
  echo ""
}

case "$shell_command" in
  'setup' | 'install' | 'reinstall' | 'erase' | 'remove' | 'config' | 'start' | 'restart' | 'stop' | 'tail' | 'status')
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
#---3. Usage info section end---#

#---4. Tool functions section begin---#
sed_replace()
{
    local sed_cmd=$1
    local file_name=$2
    local current_uname=$(uname)
    if [ "$current_uname" = "FreeBSD" ] || [ "$current_uname" = "Darwin" ]; then
       sed -i "" "$sed_cmd" $file_name
    else
       sed -i "$sed_cmd" $file_name
    fi
}

split_to_array() {
  if ! [ -z $2 ]; then
    IFS=',' read -ra $2 <<< "$1"
  fi
}

# Check preceding version less than or equals following.
function version_le() { test "$(echo "$@" | tr " " "\n" | sort -V | head -n 1)" == "$1"; }

# Execute command on a host over SSH
execute_remote_command() {
  local remote_host=$1
  local remote_command=$2
  local result=$(ssh -q -o StrictHostKeyChecking=no -o ConnectTimeout=5 -T $remote_host $remote_command)
  echo $result
}

# Execute command on a host over SSH without return
execute_remote_command_no_return() {
  local remote_host=$1
  local remote_command=$2
  ssh -q -o StrictHostKeyChecking=no -o ConnectTimeout=5 -T $remote_host $remote_command
}

# Get remote server's uname
get_remote_uname() {
  local remote_host=$1
  local remote_uname=$(execute_remote_command $remote_host "uname")
  echo $remote_uname
}

# Get remote server's os name
get_remote_osname() {
  local remote_host=$1
  local osname_command='cat /etc/os-release 
| grep -w NAME 
| awk -F '\''='\'' '\''{print $2;}'\'' 
| awk -F '\''"'\'' '\''{if (NF==3) {print $2} else {print $1}}'\'' 
| awk '\''{print $1}'\'''
  local remote_osname=$(execute_remote_command $remote_host "$osname_command")
  echo "$remote_osname"
}

# Get remote Ubuntu server's version
get_os_version() {
  local remote_host=$1
  local os_version_command='cat /etc/os-release 
| grep -w VERSION_ID 
| awk -F '\''='\'' '\''{print $2;}'\'' 
| awk -F '\''"'\'' '\''{if (NF==3) {print $2} else {print $1}}'\'''
  local remote_os_version=$(execute_remote_command $remote_host "$os_version_command")
  echo "$remote_os_version"
}

# Get remote CentOS server's version
get_centos_version() {
  local remote_host=$1
  local centos_version_command='cat /etc/system-release | awk '\''{print $4}'\'''
  local remote_centos_version=$(execute_remote_command $remote_host "$centos_version_command")
  echo "$remote_centos_version"
}
#---4. Tool functions section end---#

#---5. Parse cluster servers section begin---#
INT_REGEXP='^[0-9]+$'
RANGE_REGEXP='^\[[0-9]*, *[0-9]*\]$'

parse_value_in_section() {
  local conf_file=$1
  local section_name=$2
  local field_name=$3
  local value=$(\
sed -n "/^\\[$section_name\\]/,/^\[/ p" $conf_file | \
sed -n "/^$field_name *=/ p" | \
sed 's/\([^=]*\)=\([^=]\)/\2/g' | \
sed 's/ *^//g' | sed 's/ *$//g')
  echo $value
}

parse_fstore_servers() {
  local conf_file=$1
  if ! [ -f $conf_file ]; then
    echo "ERROR: fstore cluster config file $conf_file does not exist."
    exit 1
  fi
  local server_groups=`sed -n '/^\[server-group/ p' $conf_file | sed 's/\[//' | sed 's/\]//'`
  for server_group in ${server_groups[@]}; do
    let fstore_group_count=$fstore_group_count+1
    local fstore_server_hosts=""
    local server_ids=$(parse_value_in_section $conf_file $server_group "server_ids")
    # echo "server_ids in $server_group is:==$server_ids=="
    if [[ $server_ids =~ $INT_REGEXP ]]; then
      #单个整数，只有一个节点
      # echo "$server_ids is integer"
      local server_host=$(parse_value_in_section $conf_file "server-$server_ids" "host")
      server_host=${server_host%%:*}
      if [ -n server_host ]; then
        fstore_server_hosts=$server_host
      fi
    elif [[ $server_ids =~ $RANGE_REGEXP ]]; then
      # echo "[$server_ids] is range"
      local src_str=`echo $server_ids | sed 's/\[//' | sed 's/\]//' | sed 's/, */../'`
      local dest="echo {$src_str}"
      for server_id in $(eval $dest); do
        local server_host=$(parse_value_in_section $conf_file "server-$server_id" "host")
        server_host=${server_host%%:*}
        if [ -n server_host ]; then
          fstore_server_hosts="$fstore_server_hosts $server_host"
        fi
      done 
    else
      echo "ERROR: Invalid server_ids format:[$server_ids] in config file $conf_file"
      exit 1
    fi
    fstore_server_hosts=`echo $fstore_server_hosts | sed 's/ *^//g'`
    if [[ -n $fstore_server_hosts ]]; then
      local array_str="fstore_group_$fstore_group_count=($fstore_server_hosts)"
      eval $array_str
    else
      echo "ERROR: Parse fstore server hosts failed, $conf_file must conform to format below:"
      echo "  [server-group-1]"
      echo "  server_ids = [1, 3]"
      echo "  # Or server_ids = 1"
      echo "  [server-1]"
      echo "  host = 10.0.1.11"
      exit 1
    fi
  done
}

parse_fdir_servers() {
  local conf_file=$1
  if ! [ -f $conf_file ]; then
    echo "ERROR: fdir cluster config file $conf_file does not exist."
    exit 1
  fi
  local fdir_servers=`sed -n '/^\[server-/ p' $conf_file | sed 's/\[//' | sed 's/\]//'`
  local fdir_server_hosts=""
  for fdir_server in ${fdir_servers[@]}; do
    local server_host=$(parse_value_in_section $conf_file $fdir_server "host")
    server_host=${server_host%%:*}
    if [ -n server_host ]; then
      fdir_server_hosts="$fdir_server_hosts $server_host"
    fi
  done
  fdir_server_hosts=`echo $fdir_server_hosts | sed 's/ *^//g'`
  if [[ -n $fdir_server_hosts ]]; then
    local array_str="fdir_group=($fdir_server_hosts)"
    eval $array_str
  else
    echo "ERROR: Parse fdir server hosts failed, $conf_file must conform to format below:"
    echo "  [server-1]"
    echo "  host = 10.0.1.11"
    echo "  ..."
    exit 1
  fi
}

parse_fauth_servers() {
  local conf_file=$1
  if ! [ -f $conf_file ]; then
    echo "ERROR: fauth cluster config file $conf_file does not exist."
    exit 1
  fi
  local fauth_servers=`sed -n '/^\[server-/ p' $conf_file | sed 's/\[//' | sed 's/\]//'`
  local fauth_server_hosts=""
  for fauth_server in ${fauth_servers[@]}; do
    local server_host=$(parse_value_in_section $conf_file $fauth_server "host")
    server_host=${server_host%%:*}
    if [ -n server_host ]; then
      fauth_server_hosts="$fauth_server_hosts $server_host"
    fi
  done
  fauth_server_hosts=`echo $fauth_server_hosts | sed 's/ *^//g'`
  if [[ -n $fauth_server_hosts ]]; then
    local array_str="fauth_group=($fauth_server_hosts)"
    eval $array_str
  else
    echo "ERROR: Parse fauth server hosts failed, $conf_file must conform to format below:"
    echo "  [server-1]"
    echo "  host = 10.0.1.11"
    echo "  ..."
    exit 1
  fi
}

parse_fvote_servers() {
  local conf_file=$1
  if ! [ -f $conf_file ]; then
    echo "ERROR: fvote cluster config file $conf_file does not exist."
    exit 1
  fi
  local fvote_servers=`sed -n '/^\[server-/ p' $conf_file | sed 's/\[//' | sed 's/\]//'`
  local fvote_server_hosts=""
  for fvote_server in ${fvote_servers[@]}; do
    local server_host=$(parse_value_in_section $conf_file $fvote_server "host")
    server_host=${server_host%%:*}
    if [ -n server_host ]; then
      fvote_server_hosts="$fvote_server_hosts $server_host"
    fi
  done
  fvote_server_hosts=`echo $fvote_server_hosts | sed 's/ *^//g'`
  if [[ -n $fvote_server_hosts ]]; then
    local array_str="fvote_group=($fvote_server_hosts)"
    eval $array_str
  else
    echo "ERROR: Parse fvote server hosts failed, $conf_file must conform to format below:"
    echo "  [server-1]"
    echo "  host = 10.0.1.11"
    echo "  ..."
    exit 1
  fi
}
#---5. Parse cluster servers section end---#

#---6. Settings and cluster info section begin---#
check_fcfs_cache_path() {
  if ! [ -d $fcfs_cache_path ]; then
    if ! mkdir -p $fcfs_cache_path; then
      echo "ERROR: Create fcfs cache path failed, $fcfs_cache_path!"
      exit 1
    else
      echo "INFO: Created fcfs cache path successfully, $fcfs_cache_path."
    fi
  fi
}

# Load cluster settings from file fcfs.settings
load_fcfs_settings() {
  if ! [ -f $fcfs_settings_file ]; then
    echo "ERROR: File $fcfs_settings_file does not exist"
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

# Load installed settings from file .fcfs/installed.settings
load_installed_settings() {
  local installed_mark_file=$fcfs_cache_path/$fcfs_installed_file
  if [ -f $installed_mark_file ]; then
    local installed_marks=`sed -e 's/#.*//' -e '/^$/ d' $installed_mark_file`
    eval $installed_marks
  fi
}

# Load cluster dependent lib version settings from file dependency.[FastCFS-version].settings
load_dependency_settings() {
  dependency_file_version=$1
  if [ -z $dependency_file_version ]; then
    echo "ERROR: Dependency file version cannot be empty."
    exit 1
  fi
  dependency_settings_file="$fcfs_cache_path/dependency.$dependency_file_version.settings"
  if ! [ -f $dependency_settings_file ]; then
    # File not exist in local path, will get it from remote server match the version
    echo "WARN: File $dependency_settings_file does not exist, getting it from remote server $fcfs_dependency_file_server."
    check_fcfs_cache_path
    remote_file_url="$fcfs_dependency_file_server/$dependency_file_version"
    download_res=`curl -f -o $dependency_settings_file $remote_file_url`

    if ! [ -f $dependency_settings_file ]; then
      echo "ERROR: Cannot download file $dependency_settings_file from $remote_file_url."
      echo "       Please make sure that remote file exists and network is accessible."
      exit 1
    fi
  fi
  dependency_settings=`sed -e 's/#.*//' -e '/^$/ d' $dependency_settings_file`
  eval $dependency_settings
  if [ -z $libfastcommon ]; then
    echo "WARN: Dependency libfastcommon has no version value in $dependency_settings_file."
  fi
  if [ -z $libserverframe ]; then
    echo "WARN: Dependency libserverframe has no version value in $dependency_settings_file."
  fi
  if [ -z $fvote ]; then
    echo "WARN: Dependency fvote has no version value in $dependency_settings_file."
  fi
  if [ -z $fauth ]; then
    echo "WARN: Dependency fauth has no version value in $dependency_settings_file."
  fi
  if [ -z $fdir ]; then
    echo "WARN: Dependency fdir has no version value in $dependency_settings_file."
  fi
  if [ -z $fstore ]; then
    echo "WARN: Dependency fstore has no version value in $dependency_settings_file."
  fi
}

# Get module name from command args, if not specify, will handle all modules.
fdir_need_execute=0
fstore_need_execute=0
fauth_need_execute=0
fvote_need_execute=0
fuseclient_need_execute=0

has_module_param=1
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
      'fvote')
        fvote_need_execute=1
      ;;
      'fuseclient')
        fuseclient_need_execute=1
      ;;
      *)
        has_module_param=0
        if ! [ $shell_command = 'tail' ] || ! [[ $module_name =~ ^- ]]; then
          echo "ERROR: Module name invalid, $module_name."
          echo "       Allowed module names: fdir, fstore, fauth, fvote, fuseclient."
          exit 1
        else
          fdir_need_execute=1
          fstore_need_execute=1
          fauth_need_execute=1
          fvote_need_execute=1
          fuseclient_need_execute=1
        fi
      ;;
    esac
  else
    has_module_param=0
    fdir_need_execute=1
    fstore_need_execute=1
    fauth_need_execute=1
    fvote_need_execute=1
    fuseclient_need_execute=1
  fi
}

# Get node host from command args, if not specify, will handle all nodes.
# It must specify after module name.
has_node_param=0
check_node_param() {
  if [ $# -gt 2 ] && ! [[ $3 =~ ^- ]] && [ $has_module_param = 1 ]; then
    node_host_need_execute=$3
    has_node_param=1
  fi
}

# Load cluster groups from cluster config files.
#   fdir_group=(172.16.168.128)
#   fstore_group_count=1
#   fstore_group_1=(172.16.168.128)
#   fauth_group=(172.16.168.128)
load_cluster_groups() {
  if [ $fdir_need_execute -eq 1 ]; then
    parse_fdir_servers "conf/fdir/cluster.conf"
  fi
  if [ $fstore_need_execute -eq 1 ]; then
    parse_fstore_servers "conf/fstore/cluster.conf"
  fi
  if [ $fauth_need_execute -eq 1 ]; then
    parse_fauth_servers "conf/auth/cluster.conf"
  fi
  if [ $fvote_need_execute -eq 1 ]; then
    parse_fvote_servers "conf/vote/cluster.conf"
  fi
}

replace_cluster_osname_mark() {
  local mark_file=$1
  local remote_osname=$2
  sed_replace "s#^cluster_host_osname=.*#cluster_host_osname=$remote_osname#g" $mark_file
}

save_cluster_osname_mark() {
  check_fcfs_cache_path
  local remote_osname=$1
  local installed_mark_file=$fcfs_cache_path/$fcfs_installed_file
  if [ -f $installed_mark_file ]; then
    local cluster_osname_marks=`grep cluster_host_osname $installed_mark_file`
    if [ -z $cluster_osname_marks ]; then
      echo "cluster_host_osname=$remote_osname" >> $installed_mark_file
    else
      # replace old value
      replace_cluster_osname_mark $installed_mark_file "$remote_osname"
    fi
  else
    echo "cluster_host_osname=$remote_osname" >> $installed_mark_file
  fi
}

# Check remote host os and version.
#   The whole cluster's host must have same os type.
#   The hosts os version must great than or equal xxx_MIN_VERSION
check_remote_os_and_version() {
  pkg_manage_tool=""
  cluster_host_osname=""
  declare -a all_server_ips
  all_server_ips+=(${fdir_group[@]})
  for (( i=1 ; i <= $fstore_group_count ; i++ )); do
    local fstore_group="fstore_group_$i[@]"
    all_server_ips+=(${!fstore_group})
  done
  all_server_ips+=(${fauth_group[@]})
  all_server_ips+=(${fvote_group[@]})
  all_server_ips+=(${fuseclient_ip_array[@]})

  declare -a distinct_server_ips
  for server_ip in ${all_server_ips[@]}; do
    if ! [[ ${distinct_server_ips[*]} =~ (^|[[:space:]])"$server_ip"($|[[:space:]]) ]]; then
      distinct_server_ips+=("$server_ip")
      echo "INFO: Begin check os name and version on server $server_ip"
      local remote_uname=$(get_remote_uname $server_ip)
      if [ $remote_uname = 'Linux' ]; then
        local remote_osname=$(get_remote_osname $server_ip)
        if [ "$cluster_host_osname" != '' ]; then
          if [ "$cluster_host_osname" != "$remote_osname" ]; then
            echo "Error: Cluster's servers must have same OS, $cluster_host_osname and $remote_osname cannot be mixed use"
            exit 1
          fi
        else
          cluster_host_osname=$remote_osname
          if [[ " ${YUM_OS_ARRAY[@]} " =~ " ${remote_osname} " ]]; then
            pkg_manage_tool="yum"
          elif [[ " ${APT_OS_ARRAY[@]} " =~ " ${remote_osname} " ]]; then
            pkg_manage_tool="apt"
          fi
        fi
        local remote_osversion
        if [ $remote_osname = 'CentOS' ]; then
          remote_osversion=$(get_centos_version $server_ip)
        elif [[ " ${YUM_OS_ARRAY[@]} " =~ " ${remote_osname} " ]] || [[ " ${APT_OS_ARRAY[@]} " =~ " ${remote_osname} " ]]; then
          remote_osversion=$(get_os_version $server_ip)
        else
          echo "Error: Unsupport OS, $remote_osname on server $server_ip"
          exit 1
        fi
        declare -i remote_os_major_version=$(echo $remote_osversion | awk -F '.' '{print $1}')
        local min_version_name="MIN_VERSION_OF_$remote_osname"
        local min_version=${!min_version_name}
        if [ $remote_os_major_version -lt $min_version ]; then
          echo "$remote_osname's version must be great than or equal $min_version, but was $remote_os_major_version on server $server_ip"
          exit 1
        fi
      else
        echo "ERROR: Unsupport OS, $remote_uname on server $server_ip"
        exit 1
      fi
    fi
  done
  if [ "$cluster_host_osname" != '' ]; then
    save_cluster_osname_mark "$cluster_host_osname"
  else
    echo "ERROR: Check remote host os failed."
    exit 1
  fi
}

fuseclient_share_fdir=0
fuseclient_share_fstore=0
fuseclient_share_fauth=0
fuseclient_share_fvote=0

check_if_client_share_servers() {
  if ! [ ${#fuseclient_ip_array[@]} -eq 0 ]; then
    for fuseclient_server_ip in ${fuseclient_ip_array[@]}; do
      for fdir_server_ip in ${fdir_group[@]}; do
        if [ $fdir_server_ip = "$fuseclient_server_ip" ]; then
          fuseclient_share_fdir=1
        fi
      done
      for (( i=1 ; i <= $fstore_group_count ; i++ )); do
        local fstore_group="fstore_group_$i[@]"
        for fstore_server_ip in ${!fstore_group}; do
          if [ $fstore_server_ip = "$fuseclient_server_ip" ]; then
            fuseclient_share_fstore=1
          fi
        done
      done
      for fauth_server_ip in ${fauth_group[@]}; do
        if [ $fauth_server_ip = "$fuseclient_server_ip" ]; then
          fuseclient_share_fauth=1
        fi
      done
      for fvote_server_ip in ${fvote_group[@]}; do
        if [ $fvote_server_ip = "$fuseclient_server_ip" ]; then
          fuseclient_share_fvote=1
        fi
      done
    done
  fi
}
#---6. Settings and cluster info section end---#

#---7. Iterate hosts for execute command section begin---#
execute_command_on_fdir_servers() {
  local command_name=$1
  local function_name=$2
  local module_name=$3
  if [ $fdir_need_execute -eq 1 ]; then
    local fdir_node_match_setting=0
    for fdir_server_ip in ${fdir_group[@]}; do
      if [ -z $node_host_need_execute ] || [ $fdir_server_ip = "$node_host_need_execute" ]; then
        if [ $command_name = 'status' ]; then
          echo ""
          echo "INFO: Begin display $module_name status on server $fdir_server_ip."
        else
          echo "INFO: Begin $command_name $module_name on server $fdir_server_ip."
        fi
        $function_name $fdir_server_ip "$module_name" $command_name
      fi
      if [ $fdir_server_ip = "$node_host_need_execute" ]; then
        fdir_node_match_setting=1
      fi
    done
    if ! [ -z $node_host_need_execute ] && [ $fdir_node_match_setting -eq 0 ]; then
      echo "ERROR: The node $node_host_need_execute not match host in fdir cluster.conf."
    fi
  fi
}

execute_command_on_fstore_servers() {
  local command_name=$1
  local function_name=$2
  local module_name=$3
  if [ $fstore_need_execute -eq 1 ]; then
    local fstore_node_match_setting=0
    for (( i=1 ; i <= $fstore_group_count ; i++ )); do
      local fstore_group="fstore_group_$i[@]"
      for fstore_server_ip in ${!fstore_group}; do
        if [ -z $node_host_need_execute ] || [ $fstore_server_ip = "$node_host_need_execute" ]; then
          if [ $command_name = 'status' ]; then
            echo ""
            echo "INFO: Begin display $module_name status on server $fstore_server_ip."
          else
            echo "INFO: Begin $command_name $module_name on server $fstore_server_ip."
          fi
          $function_name $fstore_server_ip "$module_name" $command_name
        fi
        if [ $fstore_server_ip = "$node_host_need_execute" ]; then
          fstore_node_match_setting=1
        fi
      done
    done
    if ! [ -z $node_host_need_execute ] && [ $fstore_node_match_setting -eq 0 ]; then
      echo "ERROR: The node $node_host_need_execute not match host in fstore cluster.conf."
    fi
  fi
}

execute_command_on_fauth_servers() {
  local command_name=$1
  local function_name=$2
  local module_name=$3
  if [ $fauth_need_execute -eq 1 ]; then
    local fauth_node_match_setting=0
    for fauth_server_ip in ${fauth_group[@]}; do
      if [ -z $node_host_need_execute ] || [ $fauth_server_ip = "$node_host_need_execute" ]; then
        if [ $command_name = 'status' ]; then
          echo ""
          echo "INFO: Begin display $module_name status on server $fauth_server_ip."
        else
          echo "INFO: Begin $command_name $module_name on server $fauth_server_ip."
        fi
        $function_name $fauth_server_ip "$module_name" $command_name
      fi
      if [ $fauth_server_ip = "$node_host_need_execute" ]; then
        fauth_node_match_setting=1
      fi
    done
    if ! [ -z $node_host_need_execute ] && [ $fauth_node_match_setting -eq 0 ]; then
      echo "ERROR: The node $node_host_need_execute not match host in fauth cluster.conf."
    fi
  fi
}

execute_command_on_fvote_servers() {
  local command_name=$1
  local function_name=$2
  local module_name=$3
  if [ $fvote_need_execute -eq 1 ]; then
    local fvote_node_match_setting=0
    for fvote_server_ip in ${fvote_group[@]}; do
      if [ -z $node_host_need_execute ] || [ $fvote_server_ip = "$node_host_need_execute" ]; then
        if [ $command_name = 'status' ]; then
          echo ""
          echo "INFO: Begin display $module_name status on server $fvote_server_ip."
        else
          echo "INFO: Begin $command_name $module_name on server $fvote_server_ip."
        fi
        $function_name $fvote_server_ip "$module_name" $command_name
      fi
      if [ $fvote_server_ip = "$node_host_need_execute" ]; then
        fvote_node_match_setting=1
      fi
    done
    if ! [ -z $node_host_need_execute ] && [ $fvote_node_match_setting -eq 0 ]; then
      echo "ERROR: The node $node_host_need_execute not match host in fvote cluster.conf."
    fi
  fi
}

execute_command_on_fuseclient_servers() {
  local command_name=$1
  local function_name=$2
  local module_name=$3
  if [ $fuseclient_need_execute -eq 1 ]; then
    if [ ${#fuseclient_ip_array[@]} -eq 0 ]; then
      echo "ERROR: Param fuseclient_ips has no value in $fcfs_settings_file."
      exit 1
    else
      local fuseclient_node_match_setting=0
      for fuseclient_server_ip in ${fuseclient_ip_array[@]}; do
        if [ -z $node_host_need_execute ] || [ $fuseclient_server_ip = "$node_host_need_execute" ]; then
          if [ $command_name = 'status' ]; then
            echo ""
            echo "INFO: Begin display $module_name status on server $fuseclient_server_ip."
          else
            echo "INFO: begin $command_name $module_name on server $fuseclient_server_ip."
          fi
          $function_name $fuseclient_server_ip "$module_name" $command_name
        fi
        if [ $fuseclient_server_ip = "$node_host_need_execute" ]; then
          fuseclient_node_match_setting=1
        fi
      done
      if ! [ -z $node_host_need_execute ] && [ $fuseclient_node_match_setting -eq 0 ]; then
        echo "ERROR: The node $node_host_need_execute not match param fuseclient_ips in $fcfs_settings_file."
        exit 1
      fi
    fi
  fi
}
#---7. Iterate hosts for execute command section end---#

#---8. Install section begin---#
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
    if [ $osname = 'Fedora' ]; then
      if [ $os_major_version -lt 20 ]; then
        os_major_version=6
      elif [ $os_major_version -lt 28 ]; then
        os_major_version=7
      elif [ $os_major_version -lt 34 ]; then
        os_major_version=8
      else
        os_major_version=9
      fi
    elif [ $osname = 'Alibaba' ]; then
      if [ $os_major_version -lt 3 ]; then
        os_major_version=7
      else
        os_major_version=8
      fi
    elif [ $osname = 'Amazon' ]; then
      if [ $os_major_version -le 2 ]; then
        os_major_version=7
      else
        os_major_version=8
      fi
    fi
  else
    echo "Error: Unsupport OS, $uname" 1>&2
    exit 1
  fi
}

check_yum_install_fastos_repo() {
  repo=$(rpm -q FastOSrepo 2>/dev/null)
  if [ $? -ne 0 ]; then
    if [ $os_major_version -eq 7 ]; then
      sudo rpm -ivh http://www.fastken.com/yumrepo/el7/noarch/FastOSrepo-1.0.1-1.el7.noarch.rpm
    else
      sudo rpm -ivh http://www.fastken.com/yumrepo/el8/noarch/FastOSrepo-1.0.1-1.el8.noarch.rpm
    fi
    if [ $? -ne 0 ]; then
      echo "ERROR: FastOSrepo rpm install failed."
      exit 1
    fi
  fi
}

check_apt_install_fastos_repo() {
  if [ ! -f /etc/apt/sources.list.d/fastos.list ]; then
    sudo apt-get install curl gpg -y
    sudo curl http://www.fastken.com/aptrepo/packages.fastos.pub | sudo gpg --dearmor > /tmp/fastos-archive-keyring.gpg
    sudo install -D -o root -g root -m 644 /tmp/fastos-archive-keyring.gpg /usr/share/keyrings/fastos-archive-keyring.gpg
    sudo sh -c 'echo "deb [signed-by=/usr/share/keyrings/fastos-archive-keyring.gpg] http://www.fastken.com/aptrepo/fastos/ fastos main" > /etc/apt/sources.list.d/fastos.list'
    sudo sh -c 'echo "deb [signed-by=/usr/share/keyrings/fastos-archive-keyring.gpg] http://www.fastken.com/aptrepo/fastos-debug/ fastos-debug main" > /etc/apt/sources.list.d/fastos-debug.list'
    sudo rm -f /tmp/fastos-archive-keyring.gpg
  fi
}

execute_yum() {
  local yum_command=$1
  local program_name=$2
  if [ $os_major_version -ge 7 ]; then
    check_yum_install_fastos_repo
    if [ $yum_command = 'install' ] && [[ $program_name == *"FastCFS-fused"* ]]; then
      sudo rpm -q fuse >/dev/null
      if [ $? -eq 0 ]; then
        echo "INFO: Remove old version fuse."
        sudo yum remove fuse -y
        if [ $? -ne 0 ]; then
          echo "ERROR: Remove old version fuse failed."
          exit 1
        fi
      fi
    fi
    echo "INFO: yum $yum_command $program_name -y."
    sudo yum $yum_command $program_name -y
    if [ $? -ne 0 ]; then
      echo "ERROR: \"yum $yum_command $program_name -y\" execute failed."
      exit 1
    fi
  else
    echo "ERROR: Unsupport OS, $uname" 1>&2
    echo "       Command setup and install can only be used for CentOS 7 or higher."
    exit 1
  fi
}

execute_apt() {
  local apt_command=$1
  local program_name=$2
  if [ $apt_command = 'install' ]; then
    check_apt_install_fastos_repo
    sudo apt-get update
  fi
  echo "INFO: apt $apt_command $program_name -y."
  sudo apt-get $apt_command $program_name -y
  if [ $? -ne 0 ]; then
    echo "ERROR: \"apt $apt_command $program_name -y\" execute failed."
    exit 1
  fi
}

execute_yum_on_remote() {
  local remote_host=$1
  local program_name=$2
  local yum_command=$3
  execute_remote_command_no_return $remote_host "
$(declare -f check_remote_osname);
check_remote_osname;
$(declare -f check_yum_install_fastos_repo);
$(declare -f execute_yum);
execute_yum $yum_command \"$program_name\""
  if [ $? -ne 0 ]; then
    exit 1
  fi
}

execute_apt_on_remote() {
  local remote_host=$1
  local program_name=$2
  local apt_command=$3
  execute_remote_command_no_return $remote_host "
$(declare -f check_apt_install_fastos_repo);
$(declare -f execute_apt);
execute_apt $apt_command \"$program_name\""
  if [ $? -ne 0 ]; then
    exit 1
  fi
}

replace_installed_version_mark() {
  # Replace host with ip of current host.
  local mark_file=$1
  local installed_version=$2
  sed_replace "s#^fastcfs_version_installed=.*#fastcfs_version_installed=$installed_version#g" $mark_file
}

save_installed_version_mark() {
  check_fcfs_cache_path
  local installed_mark_file=$fcfs_cache_path/$fcfs_installed_file
  if [ -f $installed_mark_file ]; then
    local installed_marks=`grep fastcfs_version_installed $installed_mark_file`
    if [ -z $installed_marks ]; then
      echo "fastcfs_version_installed=$fastcfs_version" >> $installed_mark_file
    else
      # replace old value
      replace_installed_version_mark $installed_mark_file "$fastcfs_version"
    fi
  else
    echo "fastcfs_version_installed=$fastcfs_version" >> $installed_mark_file
  fi
}

remove_installed_mark() {
  local installed_mark_file=$fcfs_cache_path/$fcfs_installed_file
  if [ -f $installed_mark_file ]; then
    if ! rm -rf $installed_mark_file; then
      echo "ERROR: Delete installed mark file failed, $installed_mark_file!"
      exit 1
    fi
  fi
}

save_installed_mark() {
  check_fcfs_cache_path
  local installed_mark_file=$fcfs_cache_path/$fcfs_installed_file
  if [ -f $installed_mark_file ]; then
    local installed_marks=`grep fastcfs_installed $installed_mark_file`
    if [ -z $installed_marks ]; then
      echo "fastcfs_installed=1" >> $installed_mark_file
    fi
  else
    echo "fastcfs_installed=1" >> $installed_mark_file
  fi
}

check_installed_version() {
  # First install cannot specify module
  if [ -z $fastcfs_version_installed ] && [ $has_module_param = 1 ]; then
    echo "ERROR: First execute setup or install cannot specify module."
    exit 1
  elif ! [ -z $fastcfs_version_installed ] && [ $fastcfs_version_installed = $fastcfs_version ]; then
    if [ "$shell_command" = "reinstall" ]; then
      # Before reinstall programm, need user to reconfirm
      for ((;;)) do
        echo -n "WARN: Reinstall the installed program confirm[y/N]"
        read var
        if ! [ "$var" = "y" ] && ! [ "$var" = "Y" ] && ! [ "$var" = "yes" ] && ! [ "$var" = "YES" ]; then
          exit 1
        fi
        break;
      done
    else
      echo "ERROR: FastCFS $fastcfs_version have installed."
      exit 1
    fi
  elif ! [ -z $fastcfs_version_installed ] && ! [ $fastcfs_version_installed = $fastcfs_version ]; then
    if version_le $fastcfs_version_installed $fastcfs_version; then
      # Upgrade cannot specify module
      if [ $has_module_param = 1 ]; then
        echo "ERROR: Upgrade cannot specify module."
        exit 1
      fi
      for ((;;)) do
        echo -n "WARN: Upgrade installed program confirm, from $fastcfs_version_installed to $fastcfs_version ? [y/N]"
        read var
        if ! [ "$var" = "y" ] && ! [ "$var" = "Y" ] && ! [ "$var" = "yes" ] && ! [ "$var" = "YES" ]; then
          exit 1
        fi
        break;
      done
    else
      # Downgrade must remove old version first
      echo "ERROR: Downgrade install must remove old version first."
      exit 1
    fi
  fi
}

# Install packages to target by yum manage nodes.
install_packages_by_yum() {
  check_installed_version
  fdir_programs="fastDIR-server-$fdir libfastcommon-$libfastcommon libserverframe-$libserverframe FastCFS-auth-client-$fauth"
  execute_command_on_fdir_servers install execute_yum_on_remote "$fdir_programs"
  fstore_programs="faststore-server-$fstore libfastcommon-$libfastcommon libserverframe-$libserverframe FastCFS-auth-client-$fauth"
  execute_command_on_fstore_servers install execute_yum_on_remote "$fstore_programs"
  fauth_programs="FastCFS-auth-server-$fauth libfastcommon-$libfastcommon libserverframe-$libserverframe FastCFS-auth-client-$fauth fastDIR-client-$fdir"
  execute_command_on_fauth_servers install execute_yum_on_remote "$fauth_programs"
  fvote_programs="FastCFS-vote-server-$fvote libfastcommon-$libfastcommon libserverframe-$libserverframe FastCFS-vote-client-$fvote"
  execute_command_on_fvote_servers install execute_yum_on_remote "$fvote_programs"
  fuseclient_programs="FastCFS-fused-$fuseclient libfastcommon-$libfastcommon libserverframe-$libserverframe FastCFS-auth-client-$fauth FastCFS-api-libs-$fcfsapi faststore-client-$fstore fastDIR-client-$fdir"
  execute_command_on_fuseclient_servers install execute_yum_on_remote "$fuseclient_programs"
  save_installed_mark
  save_installed_version_mark
}

# Remove packages from target by yum manage nodes.
erase_packages_by_yum() {
  # Before remove programm, need user to reconfirm
  for ((;;)) do
    echo -n "WARN: Delete the installed program confirm[y/N]"
    read var
    if ! [ "$var" = "y" ] && ! [ "$var" = "Y" ] && ! [ "$var" = "yes" ] && ! [ "$var" = "YES" ]; then
      exit 1
    fi
    break;
  done
  fdir_programs="fastDIR-server libfastcommon libserverframe FastCFS-auth-client"
  execute_command_on_fdir_servers erase execute_yum_on_remote "$fdir_programs"
  fstore_programs="faststore-server libfastcommon libserverframe FastCFS-auth-client"
  execute_command_on_fstore_servers erase execute_yum_on_remote "$fstore_programs"
  fauth_programs="FastCFS-auth-server libfastcommon libserverframe FastCFS-auth-client fastDIR-client"
  execute_command_on_fauth_servers erase execute_yum_on_remote "$fauth_programs"
  fvote_programs="FastCFS-vote-server libfastcommon libserverframe FastCFS-vote-client"
  execute_command_on_fvote_servers erase execute_yum_on_remote "$fvote_programs"
  fuseclient_programs="FastCFS-fused libfastcommon libserverframe FastCFS-auth-client FastCFS-api-libs faststore-client fastDIR-client"
  execute_command_on_fuseclient_servers erase execute_yum_on_remote "$fuseclient_programs"
  remove_installed_mark
}

# Reinstall packages to target by yum manage nodes.
reinstall_packages_by_yum() {
  check_installed_version
  fdir_programs="fastDIR-server-$fdir libfastcommon-$libfastcommon libserverframe-$libserverframe FastCFS-auth-client-$fauth"
  execute_command_on_fdir_servers reinstall execute_yum_on_remote "$fdir_programs"
  fstore_programs="faststore-server-$fstore libfastcommon-$libfastcommon libserverframe-$libserverframe FastCFS-auth-client-$fauth"
  execute_command_on_fstore_servers reinstall execute_yum_on_remote "$fstore_programs"
  fauth_programs="FastCFS-auth-server-$fauth libfastcommon-$libfastcommon libserverframe-$libserverframe FastCFS-auth-client-$fauth fastDIR-client-$fdir"
  execute_command_on_fauth_servers reinstall execute_yum_on_remote "$fauth_programs"
  fvote_programs="FastCFS-vote-server-$fauth libfastcommon-$libfastcommon libserverframe-$libserverframe FastCFS-vote-client-$fauth"
  execute_command_on_fvote_servers reinstall execute_yum_on_remote "$fvote_programs"
  fuseclient_programs="FastCFS-fused-$fuseclient libfastcommon-$libfastcommon libserverframe-$libserverframe FastCFS-auth-client-$fauth FastCFS-api-libs-$fcfsapi faststore-client-$fstore fastDIR-client-$fdir"
  execute_command_on_fuseclient_servers reinstall execute_yum_on_remote "$fuseclient_programs"
  save_installed_mark
  save_installed_version_mark
}

# Install packages to target by apt manage nodes.
install_packages_by_apt() {
  execute_command_on_fdir_servers install execute_apt_on_remote "fastdir-server"
  execute_command_on_fstore_servers install execute_apt_on_remote "faststore-server"
  execute_command_on_fauth_servers install execute_apt_on_remote "fastcfs-auth-server"
  execute_command_on_fvote_servers install execute_apt_on_remote "fastcfs-vote-server"
  execute_command_on_fuseclient_servers install execute_apt_on_remote "fastcfs-fused"
  save_installed_mark
}

# Remove packages from target by apt manage nodes.
erase_packages_by_apt() {
  # Before remove programm, need user to reconfirm
  for ((;;)) do
    echo -n "WARN: Delete the installed program confirm[y/N]"
    read var
    if ! [ "$var" = "y" ] && ! [ "$var" = "Y" ] && ! [ "$var" = "yes" ] && ! [ "$var" = "YES" ]; then
      exit 1
    fi
    break;
  done
  execute_command_on_fdir_servers remove execute_apt_on_remote "fastdir-server"
  execute_command_on_fstore_servers remove execute_apt_on_remote "faststore-server"
  execute_command_on_fauth_servers remove execute_apt_on_remote "fastcfs-auth-server"
  execute_command_on_fvote_servers remove execute_apt_on_remote "fastcfs-vote-server"
  execute_command_on_fuseclient_servers remove execute_apt_on_remote "fastcfs-fused"
  remove_installed_mark
}
#---8. Install section end---#

#---9. Config section begin---#
check_remote_path() {
  dest_path=$1
  if ! [ -d $dest_path ]; then
    if ! sudo mkdir -p $dest_path; then
      echo "ERROR: Create target conf path failed, $dest_path!"
      exit 1
    fi
  fi
}

check_path_on_remote() {
  remote_host=$1
  remote_target_path=$2
  execute_remote_command_no_return $remote_host "
$(declare -f check_remote_path);
check_remote_path $remote_target_path"
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
      if ! sudo mkdir -p $check_path; then
        echo "ERROR: Create target path in file $check_conf_file failed, $check_path!"
        exit 1
      else
        echo "INFO: Created target path in file $check_conf_file successfully, $check_path."
      fi
    fi
  done
}

check_paths_infile() {
  remote_host=$1
  remote_target_file=$2$3
  execute_remote_command_no_return $remote_host "
$(declare -f check_paths_infile_remote);
check_paths_infile_remote $remote_target_file"
}

copy_config_to_remote() {
  local target_server_ip=$1
  local target_module=$2
  local src_path="conf/$target_module"
  local config_files=""
  local dest_path=""

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
      src_path="conf/auth"
      dest_path=$AUTH_CONF_PATH
    ;;
    'keys')
      config_file_array="${AUTH_KEYS_FILES[*]}"
      src_path="conf/auth/keys"
      dest_path=$AUTH_KEYS_PATH
    ;;
    'fvote')
      config_file_array="${VOTE_CONF_FILES[*]}"
      src_path="conf/vote"
      dest_path=$VOTE_CONF_PATH
    ;;
    'fuseclient')
      config_file_array="${FUSE_CONF_FILES[*]}"
      src_path="conf/fcfs"
      dest_path=$FUSE_CONF_PATH
    ;;
    *)
      echo "ERROR: Target module name is invalid, $target_module."
      exit 1
    ;;
  esac

  check_path_on_remote $target_server_ip $dest_path

  for CONF_FILE in ${config_file_array[@]}; do
    local tmp_src_file=$src_path/$CONF_FILE
    if [ -f $tmp_src_file ]; then
      echo "INFO: Copy file $tmp_src_file to $dest_path of server $target_server_ip."
      scp $tmp_src_file $target_server_ip:/tmp/
      execute_remote_command_no_return $target_server_ip "sudo cp /tmp/$CONF_FILE $dest_path"
      execute_remote_command_no_return $target_server_ip "sudo rm /tmp/$CONF_FILE"
      if [ $? -ne 0 ]; then
        exit 1
      fi
      local file_extension="${CONF_FILE##*.}"
      if [ $file_extension = "conf" ]; then
        check_paths_infile $target_server_ip $dest_path $CONF_FILE
      fi
    else
      echo "WARN: File $tmp_src_file does not exist."
    fi
  done
}

save_configed_mark() {
  check_fcfs_cache_path
  local installed_mark_file=$fcfs_cache_path/$fcfs_installed_file
  if [ -f $installed_mark_file ]; then
    local configed_marks=`grep fastcfs_configed $installed_mark_file`
    if [ -z $configed_marks ]; then
      echo "fastcfs_configed=1" >> $installed_mark_file
    fi
  else
    echo "fastcfs_configed=1" >> $installed_mark_file
  fi
}

deploy_config_files() {
  if [ -z $fastcfs_configed ] && [ $has_module_param = 1 ]; then
    echo "ERROR: First execute config cannot specify module."
    exit 1
  fi
  execute_command_on_fvote_servers config copy_config_to_remote fvote
  execute_command_on_fdir_servers config copy_config_to_remote fdir
  execute_command_on_fstore_servers config copy_config_to_remote fstore
  execute_command_on_fauth_servers config copy_config_to_remote fauth
  execute_command_on_fauth_servers config copy_config_to_remote keys
  execute_command_on_fuseclient_servers config copy_config_to_remote fuseclient

  if [ $fuseclient_share_fvote -eq 0 ]; then
    execute_command_on_fuseclient_servers config copy_config_to_remote fvote
  fi
  if [ $fuseclient_share_fdir -eq 0 ]; then
    execute_command_on_fuseclient_servers config copy_config_to_remote fdir
  fi
  if [ $fuseclient_share_fstore -eq 0 ]; then
    execute_command_on_fuseclient_servers config copy_config_to_remote fstore
  fi
  if [ $fuseclient_share_fauth -eq 0 ]; then
    execute_command_on_fuseclient_servers config copy_config_to_remote fauth
    execute_command_on_fuseclient_servers config copy_config_to_remote keys
  fi
  # Save configed mark into installed.settings
  save_configed_mark
}
#---9. Config section end---#

#---10. Service op section begin---#
service_op_on_remote() {
  service_name=$1
  operate_mode=$2
  conf_file=$3
  sudo $service_name $conf_file $operate_mode
  if [ $? -ne 0 ] && [ $operate_mode != "stop" ] && [ $operate_mode != "status" ]; then
    echo "ERROR: Service $service_name $operate_mode failed."
    exit 1
  fi
}

service_op() {
  target_server_ip=$1
  target_module=$2
  operate_mode=$3
  case "$target_module" in
    'fvote')
      service_name="fcfs_voted"
      conf_file="${VOTE_CONF_PATH}server.conf"
    ;;
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
      conf_file="${FUSE_CONF_PATH}fuse.conf"
    ;;
    *)
      echo "ERROR: Target module name is invalid, $target_module."
      exit 1
    ;;
  esac
  execute_remote_command_no_return $target_server_ip "
$(declare -f service_op_on_remote);
service_op_on_remote $service_name $operate_mode $conf_file"
  if [ $? -ne 0 ] && [ $operate_mode != "stop" ]; then
    exit 1
  fi
}

cluster_service_op() {
  operate_mode=$1
  execute_command_on_fvote_servers $operate_mode service_op fvote
  if [ $operate_mode != 'stop' ] && [ $operate_mode != 'status' ]; then
     sleep 1
  fi
  execute_command_on_fdir_servers $operate_mode service_op fdir
  execute_command_on_fstore_servers $operate_mode service_op fstore
  if [ $operate_mode != 'stop' ] && [ $operate_mode != 'status' ]; then
     sleep 3
  fi
  execute_command_on_fauth_servers $operate_mode service_op fauth
  if [ $operate_mode != 'stop' ] && [ $operate_mode != 'status' ]; then
     sleep 1
  fi
  execute_command_on_fuseclient_servers $operate_mode service_op fuseclient
}
#---10. Service op section end---#

#---11. Tail log section begin---#
tail_remote_log() {
  local tail_server=$1
  local tail_args=$2
  execute_remote_command_no_return $tail_server "tail $tail_args"
  echo "========================================================"
  echo "========Log boundary===================================="
  echo "========================================================"
}

# View last N lines log of the cluster with system command tail;
tail_log() {
  shift
  if [ $has_module_param = 1 ]; then
    shift
    if [ $has_node_param = 1 ]; then
      shift
    fi
  fi
  local tail_args="$*"
  execute_command_on_fdir_servers tail tail_remote_log "$tail_args $FDIR_LOG_FILE"
  execute_command_on_fstore_servers tail tail_remote_log "$tail_args $STORE_LOG_FILE"
  execute_command_on_fauth_servers tail tail_remote_log "$tail_args $AUTH_LOG_FILE"
  execute_command_on_fvote_servers tail tail_remote_log "$tail_args $VOTE_LOG_FILE"
  execute_command_on_fuseclient_servers tail tail_remote_log "$tail_args $FUSE_LOG_FILE"
}
#---11. Tail log section end---#

#---12. Command execute section begin---#
check_module_param $*
check_node_param $*
load_fcfs_settings
load_cluster_groups
load_installed_settings
case "$shell_command" in
  'setup' | 'install' | 'reinstall')
    check_remote_os_and_version
    if [[ " ${YUM_OS_ARRAY[@]} " =~ " ${cluster_host_osname} " ]]; then
      if [ -z $fastcfs_version ]; then
        echo "ERROR: Param fastcfs_version in $fcfs_settings_file cannot be empty."
        exit 1
      fi
      load_dependency_settings $fastcfs_version
    fi
  ;;
  'erase' | 'remove')
    if [[ " ${YUM_OS_ARRAY[@]} " =~ " ${cluster_host_osname} " ]]; then
      pkg_manage_tool="yum"
    elif [[ " ${APT_OS_ARRAY[@]} " =~ " ${cluster_host_osname} " ]]; then
      pkg_manage_tool="apt"
    fi
  ;;
esac

case "$shell_command" in
  'setup' | 'config')
    check_if_client_share_servers
  ;;
esac

if [ -z $fastcfs_installed ]; then
  case "$shell_command" in
    'reinstall' | 'erase' | 'remove' | 'config' | 'start' | 'restart' | 'stop' | 'tail' | 'status')
      echo "ERROR: The FastCFS softwares have not been installed, you must execute setup or install first."
      exit 1
    ;;
  esac
fi
if [ -z $fastcfs_configed ]; then
  case "$shell_command" in
    'start' | 'restart' | 'stop' | 'tail' | 'status')
      echo "ERROR: The FastCFS softwares have not been configed, you must execute config first."
      exit 1
    ;;
  esac
fi

case "$shell_command" in
  'setup')
    ("install_packages_by_$pkg_manage_tool";deploy_config_files;cluster_service_op restart)
  ;;
  'install')
    ("install_packages_by_$pkg_manage_tool")
  ;;
  'reinstall')
    if [ "$pkg_manage_tool" = "yum" ]; then
      ("reinstall_packages_by_$pkg_manage_tool")
    else
      echo "ERROR: The reinstall command can only be used for yum."
      exit 1
    fi
  ;;
  'erase' | 'remove')
    ("erase_packages_by_$pkg_manage_tool")
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
  'status')
    cluster_service_op status
  ;;
  *)
    print_usage
    exit 1
  ;;
esac

exit 0
#---12. Command execute section end---#
