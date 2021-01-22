%define FastCFSFused  FastCFS-fused
%define FastCFSAPI   FastCFS-api-libs
%define FastCFSDevel FastCFS-api-devel
%define FastCFSDebuginfo FastCFS-debuginfo
%define FastCFSConfig FastCFS-fuse-config
%define CommitVersion %(echo $COMMIT_VERSION)

Name: FastCFS
Version: 1.2.0
Release: 1%{?dist}
Summary: a high performance cloud native distributed file system for databases
License: AGPL v3.0
Group: Arch/Tech
URL:  http://github.com/happyfish100/FastCFS/
Source: http://github.com/happyfish100/FastCFS/%{name}-%{version}.tar.gz

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n) 

BuildRequires: fastDIR-devel >= 1.2.0
BuildRequires: faststore-devel >= 1.2.0
BuildRequires: fuse3-devel >= 3.10.1
Requires: %__cp %__mv %__chmod %__grep %__mkdir %__install %__id
Requires: %{FastCFSFused} = %{version}-%{release}
Requires: %{FastCFSAPI} = %{version}-%{release}

%description
a high performance distributed file system which can be used as the back-end storage of databases and cloud platforms.
commit version: %{CommitVersion}

%package -n %{FastCFSFused}
Requires: %{FastCFSAPI} = %{version}-%{release}
Requires: fuse3 >= 3.10.1
Requires: %{FastCFSConfig} >= 1.0.0
Summary: FastCFS fuse

%package -n %{FastCFSAPI}
Requires: fastDIR-client >= 1.2.0
Requires: faststore-client >= 1.2.0
Summary: FastCFS api library

%package -n %{FastCFSDevel}
Requires: %{FastCFSAPI} = %{version}-%{release}
Summary: header files of FastCFS api library

%package -n %{FastCFSConfig}
Requires: faststore-config >= 1.0.0
Summary: FastCFS fuse config files for sample

%description -n %{FastCFSFused}
FastCFS fuse
commit version: %{CommitVersion}

%description -n %{FastCFSAPI}
FastCFS api library
commit version: %{CommitVersion}

%description -n %{FastCFSDevel}
This package provides the header files of libfcfsapi
commit version: %{CommitVersion}

%description -n %{FastCFSConfig}
FastCFS fuse config files for sample
commit version: %{CommitVersion}


%prep
%setup -q

%build
./make.sh clean && ./make.sh

%install
rm -rf %{buildroot}
DESTDIR=$RPM_BUILD_ROOT ./make.sh install
CONFDIR=%{buildroot}/etc/fastcfs/fcfs/
SYSTEMDIR=%{buildroot}/usr/lib/systemd/system/
mkdir -p $CONFDIR
mkdir -p $SYSTEMDIR
cp conf/*.conf $CONFDIR
cp systemd/fastcfs.service $SYSTEMDIR

%post

%preun

%postun

%clean
rm -rf %{buildroot}

%files

%post -n %{FastCFSFused}
mkdir -p /opt/fastcfs/fcfs
mkdir -p /opt/fastcfs/fuse

%files -n %{FastCFSFused}
/usr/bin/fcfs_fused
%config(noreplace) /usr/lib/systemd/system/fastcfs.service

%files -n %{FastCFSAPI}
%defattr(-,root,root,-)
/usr/lib64/libfcfsapi.so*

%files -n %{FastCFSDevel}
%defattr(-,root,root,-)
/usr/include/fastcfs/*

%files -n %{FastCFSConfig}
%defattr(-,root,root,-)
%config(noreplace) /etc/fastcfs/fcfs/*.conf

%changelog
* Fri Jan 1 2021 YuQing <384681@qq.com>
- first RPM release (1.0)
