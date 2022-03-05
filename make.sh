ENABLE_STATIC_LIB=0
ENABLE_SHARED_LIB=1
TARGET_PREFIX=$DESTDIR/usr
TARGET_CONF_PATH=$DESTDIR/etc/fdir

module=''
exclude=''
for arg do
  case "$arg" in
    --module=*)
      module=${arg#--module=}
    ;;
    --exclude=*)
      exclude=${arg#--exclude=}
    ;;
  esac
done

if [ -z $module ] && [ -z $exclude ]; then
  param1=$1
  param2=$2
else
  if [ "x$module" = 'xall' ]; then
     module=''
  fi
  param1=$2
  param2=$3
fi

DEBUG_FLAG=0
PRELOAD_WITH_CAPI=1

export CC=gcc
CFLAGS='-Wall'
GCC_VERSION=$(gcc -dM -E -  < /dev/null | grep -w __GNUC__ | awk '{print $NF;}')
if [ -n "$GCC_VERSION" ] && [ $GCC_VERSION -ge 7 ]; then
  CFLAGS="$CFLAGS -Wformat-truncation=0 -Wformat-overflow=0"
fi

CFLAGS="$CFLAGS -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE"
if [ "$PRELOAD_WITH_CAPI" = "1" ]; then
  CFLAGS="$CFLAGS -DFCFS_PRELOAD_WITH_CAPI"
fi
if [ "$DEBUG_FLAG" = "1" ]; then
  CFLAGS="$CFLAGS -g -DDEBUG_FLAG"
else
  CFLAGS="$CFLAGS -g -O3"
fi

if [ -f /usr/include/fastcommon/_os_define.h ]; then
  OS_BITS=$(fgrep OS_BITS /usr/include/fastcommon/_os_define.h | awk '{print $NF;}')
elif [ -f /usr/local/include/fastcommon/_os_define.h ]; then
  OS_BITS=$(fgrep OS_BITS /usr/local/include/fastcommon/_os_define.h | awk '{print $NF;}')
else
  OS_BITS=64
fi

uname=$(uname)

if [ "$OS_BITS" -eq 64 ]; then
  if [ "$uname" = "Darwin" ]; then
    LIB_VERSION=lib
  else
    LIB_VERSION=lib64
  fi
else
  LIB_VERSION=lib
fi

LIBS=''
if [ "$uname" = "Linux" ]; then
  if [ "$OS_BITS" -eq 64 ]; then
    LIBS="$LIBS -L/usr/lib64"
  else
    LIBS="$LIBS -L/usr/lib"
  fi
  CFLAGS="$CFLAGS"
elif [ "$uname" = "FreeBSD" ] || [ "$uname" = "Darwin" ]; then
  LIBS="$LIBS -L/usr/lib"
  CFLAGS="$CFLAGS"
  if [ "$uname" = "Darwin" ]; then
    CFLAGS="$CFLAGS -DDARWIN"
    TARGET_PREFIX=$TARGET_PREFIX/local
  fi
elif [ "$uname" = "SunOS" ]; then
  LIBS="$LIBS -L/usr/lib"
  CFLAGS="$CFLAGS -D_THREAD_SAFE"
  LIBS="$LIBS -lsocket -lnsl -lresolv"
elif [ "$uname" = "AIX" ]; then
  LIBS="$LIBS -L/usr/lib"
  CFLAGS="$CFLAGS -D_THREAD_SAFE"
elif [ "$uname" = "HP-UX" ]; then
  LIBS="$LIBS -L/usr/lib"
  CFLAGS="$CFLAGS"
fi

have_pthread=0
if [ -f /usr/lib/libpthread.so ] || [ -f /usr/local/lib/libpthread.so ] || [ -f /lib64/libpthread.so ] || [ -f /usr/lib64/libpthread.so ] || [ -f /usr/lib/libpthread.a ] || [ -f /usr/local/lib/libpthread.a ] || [ -f /lib64/libpthread.a ] || [ -f /usr/lib64/libpthread.a ]; then
  LIBS="$LIBS -lpthread"
  have_pthread=1
elif [ "$uname" = "HP-UX" ]; then
  lib_path="/usr/lib/hpux$OS_BITS"
  if [ -f $lib_path/libpthread.so ]; then
    LIBS="-L$lib_path -lpthread"
    have_pthread=1
  fi
elif [ "$uname" = "FreeBSD" ]; then
  if [ -f /usr/lib/libc_r.so ]; then
    line=$(nm -D /usr/lib/libc_r.so | grep pthread_create | grep -w T)
    if [ $? -eq 0 ]; then
      LIBS="$LIBS -lc_r"
      have_pthread=1
    fi
  elif [ -f /lib64/libc_r.so ]; then
    line=$(nm -D /lib64/libc_r.so | grep pthread_create | grep -w T)
    if [ $? -eq 0 ]; then
      LIBS="$LIBS -lc_r"
      have_pthread=1
    fi
  elif [ -f /usr/lib64/libc_r.so ]; then
    line=$(nm -D /usr/lib64/libc_r.so | grep pthread_create | grep -w T)
    if [ $? -eq 0 ]; then
      LIBS="$LIBS -lc_r"
      have_pthread=1
    fi
  fi
fi

if [ $have_pthread -eq 0 ] && [ "$uname" != "Darwin" ]; then
   /sbin/ldconfig -p | fgrep libpthread.so > /dev/null
   if [ $? -eq 0 ]; then
      LIBS="$LIBS -lpthread"
   else
      echo -E 'Require pthread lib, please check!'
      exit 2
   fi
fi

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

replace_makefile()
{
    cp Makefile.in Makefile
    sed_replace "s#\\\$(CFLAGS)#$CFLAGS#g" Makefile
    sed_replace "s#\\\$(LIBS)#$LIBS#g" Makefile
    sed_replace "s#\\\$(TARGET_PREFIX)#$TARGET_PREFIX#g" Makefile
    sed_replace "s#\\\$(LIB_VERSION)#$LIB_VERSION#g" Makefile
    sed_replace "s#\\\$(TARGET_CONF_PATH)#$TARGET_CONF_PATH#g" Makefile
    sed_replace "s#\\\$(ENABLE_STATIC_LIB)#$ENABLE_STATIC_LIB#g" Makefile
    sed_replace "s#\\\$(ENABLE_SHARED_LIB)#$ENABLE_SHARED_LIB#g" Makefile
}

base_path=$(pwd)

if [ -z $module ] || [ "$module" = 'auth_server' ]; then
  if [ "x$exclude" != 'xauth_server' ]; then
    cd $base_path/src/auth/server
    replace_makefile
    make $param1 $param2
  fi
fi

if [ -z $module ] || [ "$module" = 'auth_client' ]; then
  if [ "x$exclude" != 'xauth_client' ]; then
    cd $base_path/src/auth/client
    replace_makefile
    make $param1 $param2

    cd $base_path/src/auth/client/tools
    replace_makefile
    make $param1 $param2
  fi
fi

if [ -z $module ] || [ "$module" = 'api' ]; then
  if [ "x$exclude" != 'xapi' ]; then
    cd $base_path/src/api
    replace_makefile
    make $param1 $param2

    if [ -z $DESTDIR ]; then
      cd tests || exit
      replace_makefile
      make $param1 $param2
    fi
  fi
fi

if [ "$uname" = "Linux" ]; then
if [ -z $module ] || [ "$module" = 'preload' ]; then
  if [ "x$exclude" != 'xpreload' ]; then
    cd $base_path/src/preload
    replace_makefile
    make $param1 $param2
  fi
fi
fi

if [ -z $module ] || [ "$module" = 'tools' ]; then
  if [ "x$exclude" != 'xtools' ]; then
    cd $base_path/src/tools
    replace_makefile
    make $param1 $param2
  fi
fi

if [ "$uname" = "Linux" ]; then
if [ -z $module ] || [ "$module" = 'fuse' ]; then
  if [ "x$exclude" != 'xfuse' ]; then
    cd $base_path/src/fuse
    replace_makefile
    make $param1 $param2
  fi
fi
fi
