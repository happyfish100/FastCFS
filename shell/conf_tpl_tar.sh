#!/bin/bash
#
# conf_tpl_tar.sh is a tool for quickly pack FastCFS cluster config files to template tar.
# this is for FastCFS developer.
#
BUILD_PATH="build"
FDIR_LIB="fastDIR"
STORE_LIB="faststore"

shell_name=$0
conf_file_version=$1
update_sources=$2

#---Usage info section begin---#
print_usage() {
  # Print usage to console.
  echo ""
  echo "Usage: $shell_name <version> [update]"
  echo ""
  echo "A self-sufficient config file templates pack shell for FastCFS cluster"
  echo ""
  echo "version:"
  echo "  Version of the tar file, exp: 2.0.1."
  echo "  Tar file example: conf.2.0.1.template.tar.gz"
  echo ""
  echo "update: Pull source code from remote before execute tar command."
  echo ""
}

if [ -z $conf_file_version ]; then
  print_usage
  exit 1
fi
#---Usage info section end---#

pull_source_code() {
  local module_name=$1
  local source_path=$2
  if ! [ -d $source_path ]; then
    echo "ERROR: source code path {$source_path} for $module_name not exist."
    exit 1
  fi
  cd $source_path
  echo "INFO: =====Begin to pull $module_name, it will take some time...====="
  git checkout master
  git pull
  cd -
}

pull_source_codes() {
  # Update source code from github.com.
  echo "INFO: Begin to pull source codes..."
  # Pull fastCFS self.
  pull_source_code "FastCFS" ../
  # Pull fastDIR
  pull_source_code fastDIR ../../fastDIR
  # Pull faststore
  pull_source_code faststore ../../faststore
}

if [ "$update_sources" = "update" ]; then
  pull_source_codes
fi

config_file_template_path="conf.$conf_file_version.tpl"
config_file_template_tar="$config_file_template_path.tar.gz"
# Create build path if not exists.
if ! [ -d $config_file_template_path ]; then
  mkdir -m 775 $config_file_template_path
  echo "INFO: temp tpl path {$config_file_template_path} not exist, created."
else
  rm -rf "$config_file_template_path/auth"
  rm -rf "$config_file_template_path/fcfs"
  rm -rf "$config_file_template_path/fdir"
  rm -rf "$config_file_template_path/fstore"
fi
# Copy config files from source to temp tpl path
mkdir -p "$config_file_template_path/auth/keys"
mkdir "$config_file_template_path/fcfs"
mkdir "$config_file_template_path/fdir"
mkdir "$config_file_template_path/fstore"
`cp ../src/auth/conf/keys/*.key $config_file_template_path/auth/keys/`
`cp ../src/auth/conf/*.conf $config_file_template_path/auth/`
`cp ../conf/*.conf $config_file_template_path/fcfs/`
`cp ../../fastDIR/conf/*.conf $config_file_template_path/fdir/`
`cp ../../faststore/conf/*.conf $config_file_template_path/fstore/`
`tar -czvf $config_file_template_tar $config_file_template_path`
exit 0