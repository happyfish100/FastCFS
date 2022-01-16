#!/bin/bash
#
# fcfs_conf.sh is a ops tool for quickly generate FastCFS cluster config files.
# It only relying on bash shell access to the local fcfs_conf.settings file.
# It runs fully on your workstation, requiring no servers, databases, or anything like that.
#
# If you want to generate cluster config files with specify server ips quickly,
# this is for you.
#
fcfs_settings_file="fcfs_conf.settings"
# fcfs_tpl_file_server="http://fastcfs.cn/fastcfs/ops/dependency"
#conf.2.3.0.tpl.tar.gz
fcfs_tpl_file_server="http://fastcfs.cn/fastcfs/ops/config"
fcfs_cache_path=".fcfs"

LOCAL_CONF_PATH="conf"
STORE_CONF_FILES=(client.conf server.conf cluster.conf storage.conf)
FDIR_CONF_FILES=(client.conf cluster.conf server.conf storage.conf)
AUTH_CONF_FILES=(auth.conf client.conf cluster.conf server.conf session.conf)
AUTH_KEYS_FILES=(session_validate.key)
FUSE_CONF_FILES=(fuse.conf)
TRUNCATE_MARK_LINE="## Important:server group mark, don't modify this line."

shell_name=$0
shell_command=$1
uname=$(uname)

#---Usage info section begin---#
print_usage() {
  # Print usage to console.
  echo ""
  echo "Usage: $shell_name <command> [module]"
  echo ""
  echo "A self-sufficient config file generator shell for FastCFS cluster"
  echo ""
  echo "Commands:"
  echo "  create     Create config files for FastCFS cluster with specify ips in $fcfs_settings_file"
  echo "  help       Show the detail of commands and examples"
  echo ""
}

print_detail_usage() {
  # Print usage to console.
  print_usage
  echo "Modules:"
  echo "  fdir         fastDIR server"
  echo "  fstore       faststore server"
  echo "  fauth        FastCFS auth server"
  echo "  fuseclient   FastCFS fuse client"
  echo ""
  echo "Node:"
  echo "  You can specify a single module, or command will be executed on modules."
  echo ""
  echo "Examples:"
  echo "  $shell_name create"
  echo "          Create config files for all modules."
  echo ""
  echo "  $shell_name create fdir"
  echo "          Create config files for fastDIR cluster."
  echo ""
  echo ""
}

case "$shell_command" in
  'create')
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

#---Tool functions begin---#
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

create_path_not_exist() {
  local target_path=$1
  if ! [ -d $target_path ]; then
    if ! mkdir -p $target_path; then
      echo "ERROR: Create path failed, $target_path!"
      exit 1
    else
      echo "INFO: Created path successfully, $target_path."
    fi
  fi
}
#---Tool functions end---#

#---Settings and cluster info section begin---#
# Load cluster settings from file fcfs.settings
load_fcfs_settings() {
  if ! [ -f $fcfs_settings_file ]; then
    echo "ERROR: File $fcfs_settings_file does not exist"
    exit 1
  else
    fcfs_settings=`sed -e 's/#.*//' -e '/^$/ d' $fcfs_settings_file`
    eval $fcfs_settings
    split_to_array $auth_ips auth_ip_array
    split_to_array $fdir_ips fdir_ip_array
    split_to_array $fstore_groups fstore_group_array
  fi
}

# Check config file templates in local cache path.
check_conf_template() {
  conf_file_version=$1
  if [ -z $conf_file_version ]; then
    echo "ERROR: Config file version cannot be empty."
    exit 1
  fi
  local conf_tpl_dir="conf.$conf_file_version.tpl"
  config_file_template_path="$fcfs_cache_path/$conf_tpl_dir"
  if ! [ -d $config_file_template_path ]; then
    # Template path not exist in local cache path,
    # will create it and download templates from remote server match the version
    echo "WARN: Template path $config_file_template_path does not exist, getting it from remote server $fcfs_tpl_file_server."
    create_path_not_exist $fcfs_cache_path
    cd $fcfs_cache_path
    local template_tar_file="$conf_tpl_dir.tar.gz"
    remote_template_url="$fcfs_tpl_file_server/$template_tar_file"
    download_res=`curl -f -o $template_tar_file $remote_template_url`

    if ! [ -f $template_tar_file ]; then
      echo "ERROR: Cannot download file $template_tar_file from $remote_template_url."
      echo "       Please make sure that remote template file exists and network is accessible."
      exit 1
    fi
    # 解压模版文件
    tar -xzvf $template_tar_file
    if ! [ -d $conf_tpl_dir ]; then
      echo "ERROR: dir $conf_tpl_dir still not exist after unzip file $template_tar_file."
      exit 1
    fi
    cd -
  fi
}

# Get module name from command args, if not specify, will handle all modules.
fdir_need_execute=0
fstore_need_execute=0
fauth_need_execute=0
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
      'fuseclient')
        fuseclient_need_execute=1
      ;;
      *)
        has_module_param=0
        if ! [[ $module_name =~ ^- ]]; then
          echo "ERROR: Module name invalid, $module_name."
          echo "       Allowed module names: fdir, fstore, fauth, fuseclient."
          exit 1
        else
          fdir_need_execute=1
          fstore_need_execute=1
          fauth_need_execute=1
          fuseclient_need_execute=1
        fi
      ;;
    esac
  else
    has_module_param=0
    fdir_need_execute=1
    fstore_need_execute=1
    fauth_need_execute=1
    fuseclient_need_execute=1
  fi
}
#---Settings and cluster info section end---#
check_module_param $*
load_fcfs_settings
if [ -z $fastcfs_version ]; then
  echo "ERROR: Param fastcfs_version in $fcfs_settings_file cannot be empty."
  exit 1
fi

# Check local template file, if not exist, download it from server.
check_conf_template $fastcfs_version
copy_config_from_template() {
  local config_file_array=$1
  local src_path=$2
  local dest_path=$3

  for CONF_FILE in ${config_file_array[@]}; do
    local tmp_src_file=$src_path/$CONF_FILE
    if [ -f $tmp_src_file ]; then
      echo "INFO: Copy template file $CONF_FILE to $dest_path."
      cp $tmp_src_file $dest_path
    else
      echo "WARN: File $tmp_src_file does not exist."
    fi
  done
}

create_fdir_conf_files() {
  if [ ${#fdir_ip_array[@]} -gt 0 ]; then
    echo "INFO: Begin create fdir config files."
    local fdir_path="$LOCAL_CONF_PATH/fdir"
    local fdir_tpl_path="$config_file_template_path/fdir"
    create_path_not_exist $fdir_path
    copy_config_from_template "${FDIR_CONF_FILES[*]}" $fdir_tpl_path $fdir_path
    local fdir_cluster_file="$fdir_path/cluster.conf"
    if ! [ -f $fdir_cluster_file ]; then
      echo "ERROR: fdir cluster file not exist, $fdir_cluster_file, can't append server to it"
      exit 1
    fi
    
    # Trancate cluster.conf with mark line
    sed_replace "/$TRUNCATE_MARK_LINE/,$ d" $fdir_cluster_file

    # Replace server ip
    let server_id=1
    for fdir_server_ip in ${fdir_ip_array[@]}; do
      echo "[server-$server_id]" >> $fdir_cluster_file
      echo "host = $fdir_server_ip" >> $fdir_cluster_file
      echo "" >> $fdir_cluster_file
      let server_id=$server_id+1
    done
  else
    echo "ERROR: Param fdir_ips in $fcfs_settings_file cannot be empty."
    exit 1
  fi
}

create_fstore_conf_files() {
  if [ -z $data_group_count ]; then
    echo "ERROR: Param data_group_count in $fcfs_settings_file must not empty."
    exit 1
  fi
  if [ -z $fstore_group_count ]; then
    echo "ERROR: Param fstore_group_count in $fcfs_settings_file must not empty."
    exit 1
  fi
  let data_group_count_int=$data_group_count
  let fstore_group_count_int=$fstore_group_count

  if [ $fstore_group_count_int -gt 0 ]; then
    # Check params setting valid.
    if [ $data_group_count_int -lt $fstore_group_count_int ]; then
      echo "ERROR: Param data_group_count in $fcfs_settings_file must be great than fstore_group_count."
      exit 1
    fi
    for (( i=1 ; i <= $fstore_group_count_int ; i++ )); do
      local fstore_group="fstore_group_$i"
      if [ -z ${!fstore_group} ]; then
        echo "ERROR: Param $fstore_group in $fcfs_settings_file cannot be empty."
        exit 1
      fi
    done
    
    echo "INFO: Begin create fstore config files."
    local fstore_path="$LOCAL_CONF_PATH/fstore"
    local fstore_tpl_path="$config_file_template_path/fstore"
    create_path_not_exist $fstore_path
    copy_config_from_template "${STORE_CONF_FILES[*]}" $fstore_tpl_path $fstore_path
    local fstore_cluster_file="$fstore_path/cluster.conf"
    if ! [ -f $fstore_cluster_file ]; then
      echo "ERROR: fstore cluster file not exist, $fstore_cluster_file, can't append server to it"
      exit 1
    fi

    # Trancate cluster.conf with mark line
    sed_replace "/$TRUNCATE_MARK_LINE/,$ d" $fstore_cluster_file

    # Replace server_group_count and data_group_count, and append server ips
    let data_groups_per_fstore_group=$data_group_count_int/$fstore_group_count_int
    let remainder_count=$data_group_count_int-$data_groups_per_fstore_group*$fstore_group_count_int
    let data_group_ids_start=1
    let data_group_ids_end=$data_groups_per_fstore_group

    sed_replace "s#^server_group_count *=.*#server_group_count = $fstore_group_count_int#g" $fstore_cluster_file
    sed_replace "s#^data_group_count *=.*#data_group_count = $data_group_count_int#g" $fstore_cluster_file

    let server_id_start=1
    let store_server_id=1
    # for fstore_group in ${fstore_group_array[@]}; do
    for (( i=1 ; i <= $fstore_group_count_int ; i++ )); do
      local fstore_group="fstore_group_$i"
      split_to_array ${!fstore_group} fstore_ips_array
      let server_ids=${#fstore_ips_array[@]}
      if [ $server_ids -gt 0 ]; then
        # Append server group and server ids
        echo "[server-group-$i]" >> $fstore_cluster_file
        if [ $server_ids -eq 1 ]; then
          echo "server_ids = $server_id_start" >> $fstore_cluster_file
        else
          let server_id_end=$server_id_start+$server_ids-1
          echo "server_ids = [$server_id_start, $server_id_end]" >> $fstore_cluster_file
        fi
        let server_id_start=$server_id_start+$server_ids

        let next_start_add_one=0
        # Append data group ids
        if [ $remainder_count -gt 0 ]; then
          let data_group_ids_end=$data_group_ids_end+1
          let remainder_count=$remainder_count-1
          let next_start_add_one=1
        fi
        echo "data_group_ids = [$data_group_ids_start, $data_group_ids_end]" >> $fstore_cluster_file
        echo "" >> $fstore_cluster_file

        let data_group_ids_start=$data_group_ids_end+1
        let data_group_ids_end=$data_group_ids_end+$data_groups_per_fstore_group

        # Append server hosts
        for fstore_server_ip in ${fstore_ips_array[@]}; do
          echo "[server-$store_server_id]" >> $fstore_cluster_file
          echo "host = $fstore_server_ip" >> $fstore_cluster_file
          echo "" >> $fstore_cluster_file
          let store_server_id=$store_server_id+1
        done
      else
        echo "ERROR: Param $fstore_group in $fcfs_settings_file cannot be empty."
        exit 1 
      fi
    done
  else
    echo "ERROR: Param fstore_group_count in $fcfs_settings_file must be great than 0."
    exit 1
  fi
}

create_fauth_conf_files() {
  if [ ${#auth_ip_array[@]} -gt 0 ]; then
    echo "INFO: Begin create fauth config files."
    local auth_path="$LOCAL_CONF_PATH/auth"
    local auth_tpl_path="$config_file_template_path/auth"
    create_path_not_exist $auth_path
    copy_config_from_template "${AUTH_CONF_FILES[*]}" $auth_tpl_path $auth_path
    local auth_cluster_file="$auth_path/cluster.conf"
    if ! [ -f $auth_cluster_file ]; then
      echo "ERROR: auth cluster file not exist, $auth_cluster_file, can't append server to it"
      exit 1
    fi

    # Trancate cluster.conf with mark line
    sed_replace "/$TRUNCATE_MARK_LINE/,$ d" $auth_cluster_file

    # Replace server ip
    let server_id=1
    for auth_server_ip in ${auth_ip_array[@]}; do
      echo "[server-$server_id]" >> $auth_cluster_file
      echo "host = $auth_server_ip" >> $auth_cluster_file
      echo "" >> $auth_cluster_file
      let server_id=$server_id+1
    done
    # Copy auth keys
    local auth_keys_path="$LOCAL_CONF_PATH/auth/keys"
    local auth_keys_tpl_path="$config_file_template_path/auth/keys"
    create_path_not_exist $auth_keys_path
    copy_config_from_template "${AUTH_KEYS_FILES[*]}" $auth_keys_tpl_path $auth_keys_path
  else
    echo "ERROR: Param auth_ips in $fcfs_settings_file cannot be empty."
    exit 1
  fi
}

create_fuseclient_conf_files() {
  echo "INFO: Begin create fuseclient config files."
  local fuseclient_path="$LOCAL_CONF_PATH/fcfs"
  local fuseclient_tpl_path="$config_file_template_path/fcfs"
  create_path_not_exist $fuseclient_path
  copy_config_from_template "${FUSE_CONF_FILES[*]}" $fuseclient_tpl_path $fuseclient_path
}

create_config_files() {
  create_path_not_exist $LOCAL_CONF_PATH

  if [ $fdir_need_execute -eq 1 ]; then
    create_fdir_conf_files
  fi
  if [ $fstore_need_execute -eq 1 ]; then
    create_fstore_conf_files
  fi
  if [ $fauth_need_execute -eq 1 ]; then
    create_fauth_conf_files
  fi
  if [ $fuseclient_need_execute -eq 1 ]; then
    create_fuseclient_conf_files
  fi
}

case "$shell_command" in
  'create')
    create_config_files
  ;;
  *)
    print_usage
    exit 1
  ;;
esac

exit 0