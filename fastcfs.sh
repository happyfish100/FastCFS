#!/bin/bash
#
# fastcfs install, config and start a local fastcfs cluster on Linux.
# fastcfs's primary goals are to be the best shell for local FastCFS
# application development and to support all fastcfs features that fit.
#
# fastcfs shell support commands
#    setup -- combin install, config and restart, quickly setup single node FastCFS cluster
#    install -- pull code, make, install
#    config -- copy config files to default conf path(/etc/fastcfs/)
#    start -- start fdir_serverd, fs_serverd, fcfs_fused service
#    restart -- restart fdir_serverd, fs_serverd, fcfs_fused service
#    stop -- stop fdir_serverd, fs_serverd, fcfs_fused service
#
# When setup, This shell will make and install these six libs in order:
#     libfastcommon
#     libserverframe
#     libdiskallocator
#     auth_client
#     fastDIR
#     faststore
#     FastCFS
#
# If no source code in build path, it will git clone from:
#     https://gitee.com/fastdfs100/libfastcommon.git
#     https://gitee.com/fastdfs100/libserverframe.git
#     https://gitee.com/fastdfs100/libdiskallocator.git
#     https://gitee.com/fastdfs100/fastDIR.git
#     https://gitee.com/fastdfs100/faststore.git
#
# FastCFS modules and repositores
COMMON_LIB="libfastcommon"
COMMON_LIB_URL="https://gitee.com/fastdfs100/libfastcommon.git"
FRAME_LIB="libserverframe"
FRAME_LIB_URL="https://gitee.com/fastdfs100/libserverframe.git"
DISK_ALLOCATOR_LIB="libdiskallocator"
DISK_ALLOCATOR_LIB_URL="https://gitee.com/fastdfs100/libdiskallocator.git"
FDIR_LIB="fastDIR"
FDIR_LIB_URL="https://gitee.com/fastdfs100/fastDIR.git"
STORE_LIB="faststore"
STORE_LIB_URL="https://gitee.com/fastdfs100/faststore.git"
FASTCFS_LIB=".."
AUTHCLIENT_LIB=".."

STORE_CONF_FILES=(client.conf server.conf cluster.conf storage.conf)
STORE_CONF_PATH="/etc/fastcfs/fstore/"

FDIR_CONF_FILES=(client.conf cluster.conf server.conf storage.conf)
FDIR_CONF_PATH="/etc/fastcfs/fdir/"

AUTH_CONF_FILES=(auth.conf client.conf cluster.conf server.conf session.conf)
AUTH_CONF_PATH="/etc/fastcfs/auth/"
AUTH_KEYS_FILES=(session_validate.key)
AUTH_KEYS_PATH="/etc/fastcfs/auth/keys/"

FUSE_CONF_FILES=(fuse.conf)
FUSE_CONF_PATH="/etc/fastcfs/fcfs/"

BUILD_PATH="build"
this_shell_name=$0
mode=$1    # setup|install|config|start|restart|stop
make_shell=make.sh
force_reconfig=0
uname=$(uname)

pull_source_code() {
  if [ $# != 2 ]; then
    echo "ERROR: $0 - pull_source_code() function need repository name and url!"
    exit 1
  fi

  module_name=$1
  module_url=$2
  if ! [ -d $module_name ]; then
    echo "INFO: The $module_name local repository does not exist!"
    echo "INFO: =====Begin to clone $module_name , it will take some time...====="
    git clone $module_url
    git checkout master
  else
    echo "INFO: The $module_name local repository have existed."
    cd $module_name
    echo "INFO: =====Begin to pull $module_name, it will take some time...====="
    git checkout master
    git pull
    cd ..
  fi
}

make_op() {
  if [ $# -lt 3 ]; then
    echo "ERROR: $0 - make_op() function need module repository name and mode!"
    exit 1
  fi

  module_name=$1
  module_src_path=$2
  extend_options=$4
  make_mode=$3

  module_lib_path=$BUILD_PATH/$module_src_path
  if ! [ -d $module_lib_path ]; then
    echo "ERROR: $0 - module repository {$module_name} does not exist!"
    echo "ERROR: You must checkout from remote repository first!"
    exit 1
  else  
    cd $module_lib_path
    echo "INFO: =====Begin to $make_mode module [$module_name]====="
    command="./$make_shell"
    if [ ${#extend_options} -gt 0 ]; then
      command="$command $extend_options"
    fi
    if [ $make_mode != "make" ]; then
      command="$command $make_mode"
    fi
    echo "INFO: Execute command ($command) in path ($PWD)"
    result=`$command`
    echo "INFO: Execute result=("
    echo "$result"
    echo ")"
    cd -
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
  if ! [ -z $2 ]; then
    IFS=',' read -ra $2 <<< "$1"
  fi
}

placeholder_replace() {
  filename=$1
  placeholder=$2
  value=$3
  sed_replace "s#\${$placeholder}#$value#g" $filename
}

parse_config_arguments() {
  for arg do
    case "$arg" in
      --force)
        force_reconfig=1
      ;;
    esac
  done
}

check_paths_infile() {
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

replace_host() {
  # Replace host with ip of current host.
  replace_conf_file=$1
  target_ip=$2
  sed_replace "s#^host *=.*#host = $target_ip#g" $replace_conf_file
}

copy_config_to() {
  config_file_array=$1
  src_path=$2
  dest_path=$3
  if ! [ -d $dest_path ]; then
    if ! mkdir -p $dest_path; then
      echo "ERROR: Create target conf path failed:$dest_path!"
      exit 1
    fi
  fi
  for CONF_FILE in ${config_file_array[@]}; do
    tmp_src_file=$src_path/$CONF_FILE
    tmp_dest_file=${dest_path}$CONF_FILE
    if [ -f $tmp_src_file ]; then
      if [ -f $tmp_dest_file ]; then
        if ! [ $force_reconfig -eq 1 ]; then
          echo "ERROR: file $tmp_dest_file exist, "
          echo 'you must specify --force to force reconfig, it will overwrite the exists files!'
          echo "Usage: $this_shell_name config --force"
          exit 1
        else
          # Backup exists file
          cp -f $tmp_dest_file $tmp_dest_file.bak
        fi
      fi
      echo "INFO: Copy file $CONF_FILE to $dest_path"
      cp -f $tmp_src_file $tmp_dest_file
    elif ! [ -f $tmp_dest_file ]; then
      echo "ERROR: file $tmp_dest_file not exist"
      exit 1
    fi
    file_extension="${tmp_dest_file##*.}"
    if [ $file_extension = "conf" ]; then
      check_paths_infile $tmp_dest_file
      replace_host $tmp_dest_file "$IP"
    fi
  done
}

config_faststore() {
  # Copy faststore's config file to target config path
  echo "INFO: Config faststore:"
  store_src_conf_path=$BUILD_PATH/$STORE_LIB/conf
  copy_config_to "${STORE_CONF_FILES[*]}" $store_src_conf_path $STORE_CONF_PATH
}

config_fastdir() {
  # Copy fastDIR's config file to target config path
  echo "INFO: Config fastDIR:"
  fdir_src_conf_path=$BUILD_PATH/$FDIR_LIB/conf
  copy_config_to "${FDIR_CONF_FILES[*]}" $fdir_src_conf_path $FDIR_CONF_PATH
}

config_auth() {
  # Copy auth's config file to target config path
  echo "INFO: Config auth:"
  auth_src_conf_path=src/auth/conf
  copy_config_to "${AUTH_CONF_FILES[*]}" $auth_src_conf_path $AUTH_CONF_PATH
  auth_src_keys_path=src/auth/conf/keys
  copy_config_to "${AUTH_KEYS_FILES[*]}" $auth_src_keys_path $AUTH_KEYS_PATH
}

config_fuse() {
  # Copy fuse's config file to target config path
  echo "INFO: Config fuse:"
  fuse_src_conf_path=conf
  copy_config_to "${FUSE_CONF_FILES[*]}" $fuse_src_conf_path $FUSE_CONF_PATH
}

get_first_local_ip() {
  ip_cmd=`which ip`
  if [ -z "$ip_cmd" ]; then
    ipconfig_cmd=`which ifconfig`
    if [ -z "$ipconfig_cmd" ]; then
      echo "ERROR: Command ip or ifconfig not found, please install one first." 1>&2
      exit
    else
      CMD="$ipconfig_cmd -a | grep -w inet | grep -v 127.0.0.1 | awk '{print \$2}' | tr -d 'addr:' | head -n 1"
    fi
  else
    CMD="$ip_cmd addr | grep -w inet | grep -oP '(?<=inet\s)\d+(\.\d+){3}' | grep -v 127.0.0.1 | head -n 1"
  fi
  sh -c "$CMD"
}

pull_source_codes() {
  # Clone or pull source code from github.com if not exists.
  echo "INFO: Begin to pull source codes..."
  # Pull fastCFS self.
  git pull
  # Create build path if not exists.
  if ! [ -d $BUILD_PATH ]; then
    mkdir -m 775 $BUILD_PATH
    echo "INFO: Build path {$BUILD_PATH} not exist, created."
  fi
  cd $BUILD_PATH
  
  # Pull libfastcommon
  pull_source_code $COMMON_LIB $COMMON_LIB_URL 

  # Pull libserverframe
  pull_source_code $FRAME_LIB $FRAME_LIB_URL

  # Pull libdiskallocator
  pull_source_code $DISK_ALLOCATOR_LIB $DISK_ALLOCATOR_LIB_URL

  # Pull fastDIR
  pull_source_code $FDIR_LIB $FDIR_LIB_URL

  # Pull faststore
  pull_source_code $STORE_LIB $STORE_LIB_URL
  cd ..
}

make_install() {
  lib_name=$1
  lib_src_path=$2
  op_extend_param=$3
  make_op $lib_name $lib_src_path clean
  make_op $lib_name $lib_src_path make $op_extend_param
  make_op $lib_name $lib_src_path install $op_extend_param
}

make_installs() {
  # Make and install all lib repositories sequentially.
  make_install $COMMON_LIB $COMMON_LIB
  make_install $FRAME_LIB $FRAME_LIB
  make_install $DISK_ALLOCATOR_LIB $DISK_ALLOCATOR_LIB
  make_install auth_client $AUTHCLIENT_LIB --module=auth_client
  make_install $FDIR_LIB $FDIR_LIB
  make_install $STORE_LIB $STORE_LIB
  make_install FastCFS $FASTCFS_LIB --exclude=auth_client
  echo "INFO: Congratulation, setup complete successfully!"
  echo "INFO: Then, you can execute command [fastcfs.sh config] next step."
}

service_op() {
  operate_mode=$1
  fs_serverd ${STORE_CONF_PATH}server.conf $operate_mode
  fdir_serverd ${FDIR_CONF_PATH}server.conf $operate_mode
  if [ $operate_mode != 'stop' ]; then
     sleep 3
  fi
  fcfs_authd ${AUTH_CONF_PATH}server.conf $operate_mode
  if [ $operate_mode != 'stop' ]; then
     sleep 1
  fi
  fcfs_fused ${FUSE_CONF_PATH}fuse.conf $operate_mode
}

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

install_all_softwares() {
  if [ $osname = 'CentOS' ] && [ $os_major_version -eq 7 -o $os_major_version -eq 8 ]; then
    check_install_fastos_repo
    rpm -q fuse >/dev/null && yum remove fuse -y
    yum install FastCFS-auth-server fastDIR-server faststore-server FastCFS-fused -y
  else
    ./libfuse_setup.sh
    pull_source_codes
    make_installs
  fi
}

config_all_modules() {
  IP=$(get_first_local_ip)
  parse_config_arguments $*
  config_faststore
  config_fastdir
  config_auth
  config_fuse
}

case "$mode" in
  'setup')
    install_all_softwares
    config_all_modules $*
    service_op restart
  ;;
  'install')
    install_all_softwares
  ;;
  'config')
    config_all_modules $*
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
  *)
    # usage
    echo "Usage: $0 {setup|install|config|start|restart|stop}"
    exit 1
  ;;
esac

exit 0
