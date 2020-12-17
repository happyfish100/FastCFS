#!/bin/bash

get_gcc_version() {
  LANG=en_US.UTF-8 && gcc -v 2>&1 | grep 'gcc version' | \
          awk -F 'version' '{print $2}' | awk '{print $1}' | \
          awk -F '.' '{print $1}'
}

apt_check_install_gcc() {
  version=$1
  pkg_gcc="gcc-$version"
  pkg_cpp="g++-$version"
  apt list $pkg_gcc 2>&1 | grep $pkg_gcc && apt install $pkg_gcc $pkg_cpp -y
}

apt_install_gcc() {
  apt_check_install_gcc 7
  if [ $? -ne 0 ]; then
    apt_check_install_gcc 8
    if [ $? -ne 0 ]; then
      apt_check_install_gcc 9
    fi
  fi
}

apt_install_required_gcc() {
  os_major_version=$1
  apt install gcc g++ -y
  if [ $? -ne 0 ]; then
    apt_install_gcc $os_major_version
  else
    gcc_version=$(get_gcc_version)
    if [ -z "$gcc_version" ] || [ "$gcc_version" -lt 4 ]; then
      apt_install_gcc $os_major_version
    fi
  fi
}

yum_check_install_gcc() {
  prefix=$1
  version=$2
  pkg_prefix="${prefix}toolset-${version}"
  pkg_gcc="${pkg_prefix}-gcc"
  pkg_cpp="${pkg_prefix}-gcc-c++"
  yum list $pkg_gcc 2>&1 | grep "$pkg_gcc" && \
      yum install $pkg_gcc $pkg_cpp -y && \
      source /opt/rh/$pkg_prefix/enable
}

yum_install_gcc() {
  os_major_version=$1
  if [ $os_major_version -lt 8 ]; then
    yum install centos-release-scl scl-utils-build -y
    prefix='dev'
  else
    prefix='gcc-'
  fi

  yum_check_install_gcc "$prefix" 7
  if [ $? -ne 0 ]; then
    yum_check_install_gcc "$prefix" 8
    if [ $? -ne 0 ]; then
      yum_check_install_gcc "$prefix" 9
    fi
  fi
}

yum_install_required_gcc() {
  os_major_version=$1
  yum install gcc gcc-c++ -y
  if [ $? -ne 0 ]; then
    yum_install_gcc $os_major_version
  else
    gcc_version=$(get_gcc_version)
    if [ -z "$gcc_version" ] || [ "$gcc_version" -lt 4 ]; then
      yum_install_gcc $os_major_version
    fi
  fi
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

  git_version=$(git --version 2>&1 | grep 'version' | awk '{print $3}')
  make_version=$(make --version 2>&1 | grep 'Make' | awk '{print $3}')
  gcc_version=$(get_gcc_version)
  python_version=$(python3 --version 2>&1 | grep Python | awk '{print $2}')
  pip3_version=$(pip3 --version 2>&1 | awk '{print $2}')

  if [ $osname = 'Ubuntu' ]; then
    if [ -z "$git_version" ]; then
      apt install git -y
    fi

    if [ -z "$make_version" ]; then
      apt install make -y
    fi

    if [ -z "$python_version" ]; then
      apt install python3 -y
    fi

    if [ -z "$pip3_version" ]; then
      apt install python3-pip -y
    fi

    if [ -z "$gcc_version" ] || [ "$gcc_version" -lt 4 ]; then
      apt_install_required_gcc
    fi
  else
    if [ -z "$git_version" ]; then
      yum install git -y
    fi

    if [ -z "$make_version" ]; then
      yum install make -y
    fi

    if [ -z "$python_version" ]; then
      yum install python3 -y || yum install python38 -y || yum install python36 -y || exit 2
    fi

    if [ -z "$pip3_version" ]; then
      yum install python3-pip -y
    fi

    if [ -z "$gcc_version" ] || [ "$gcc_version" -lt 4 ]; then
      yum_install_required_gcc
    fi

  fi
else
  echo "Unsupport OS: $uname" 1>&2
  exit 1
fi

MESON_PRG=$(which meson 2>&1)
if [ $? -ne 0 ]; then
  pip3 install meson
fi

NINJA_PRG=$(which ninja 2>&1)
if [ $? -ne 0 ]; then
  pip3 install ninja
fi

git clone https://github.com/libfuse/libfuse.git
cd libfuse/
git checkout fuse-3.10.1
rm -rf build/ && mkdir build/ && cd build/
meson ..
meson configure -D prefix=/usr
meson configure -D examples=false
ninja
ninja install
sed -i 's/#user_allow_other/user_allow_other/g' /etc/fuse.conf
