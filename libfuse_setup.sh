#!/bin/sh

apt_check_install_gcc() {
  version=$1
  name=gcc-$version
  apt list $name 2>&1 | grep $name && apt install $name -y
}

apt_install_gcc() {
  apt_check_install_gcc 9
  if [ $? -ne 0 ]; then
    apt_check_install_gcc 8
    if [ $? -ne 0 ]; then
      apt_check_install_gcc 7
      if [ $? -ne 0 ]; then
        apt list gcc 2>&1 | grep gcc && apt install gcc -y
      fi
    fi
  fi
}

yum_check_install_gcc() {
  version=$1
  name=devtoolset-$version-gcc
  yum list $name 2>&1 | grep "$name" && \
  yum install devtoolset-$version-gcc devtoolset-$version-gcc-c++ -y && \
  source /opt/rh/devtoolset-$version/enable
}

yum_install_gcc() {
  major_version=$1
  if [ $major_version -lt 8 ]; then
    yum install centos-release-scl scl-utils-build -y
    yum_check_install_gcc 9
    if [ $? -ne 0 ]; then
      yum_check_install_gcc 8
      if [ $? -ne 0 ]; then
        yum_check_install_gcc 7
      fi
    fi
  else
    version=9 && name=devtoolset-$version-gcc && \
            yum install gcc-toolset-$version-gcc gcc-toolset-$version-gcc-c++ -y && \
            source /opt/rh/gcc-toolset-$version/enable
    if [ $? -ne 0 ]; then
       yum install gcc gcc-c++ -y
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

  gcc_version=$(LANG=en_US.UTF-8 && gcc -v 2>&1 | grep 'gcc version' | \
          awk -F 'version' '{print $2}' | awk '{print $1}' | awk -F '.' '{print $1}')
  python_version=$(python3 --version 2>&1 | grep Python | awk '{print $2}')
  pip3_version=$(pip3 --version 2>&1 | awk '{print $2}')
  if [ $osname = 'Ubuntu' ]; then
    if [ -z "$python_version" ]; then
      apt install python3 python3-pip -y
    fi

    if [ -z "$pip3_version" ]; then
      apt install python3-pip -y
    fi

    if [ -z "$gcc_version" ] || [ "$gcc_version" -lt 7 ]; then
      apt_install_gcc $os_major_version
    fi
  else
    if [ -z "$python_version" ]; then
      yum install python3 -y || yum install python38 -y || yum install python36 -y || exit 2
    fi

    if [ -z "$pip3_version" ]; then
      yum install python3-pip -y
    fi

    if [ -z "$gcc_version" ] || [ "$gcc_version" -lt 7 ]; then
      yum_install_gcc $os_major_version
    fi
  fi
else
  echo "Unsupport OS: $uname"
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
