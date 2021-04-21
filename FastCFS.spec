%define FastCFSFused        FastCFS-fused
%define FastCFSAPI          FastCFS-api-libs
%define FastCFSAuthServer   FastCFS-auth-server
%define FastCFSAuthClient   FastCFS-auth-client
%define FastCFSAPIDevel     FastCFS-api-devel
%define FastCFSAuthDevel    FastCFS-auth-devel
%define FastCFSDebuginfo    FastCFS-debuginfo
%define FastCFSFuseConfig   FastCFS-fuse-config
%define FastCFSAuthConfig   FastCFS-auth-config
%define CommitVersion %(echo $COMMIT_VERSION)

Name: FastCFS
Version: 2.0.0
Release: 1%{?dist}
Summary: a high performance cloud native distributed file system for databases, KVM and K8s
License: AGPL v3.0
Group: Arch/Tech
URL:  http://github.com/happyfish100/FastCFS/
Source: http://github.com/happyfish100/FastCFS/%{name}-%{version}.tar.gz

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n) 

BuildRequires: fastDIR-devel >= 2.0.0
BuildRequires: faststore-devel >= 2.0.0
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
Requires: %{FastCFSFuseConfig} >= 1.0.0
Summary: FastCFS fuse

%package -n %{FastCFSAPI}
Requires: fastDIR-client >= 2.0.0
Requires: faststore-client >= 2.0.0
Summary: FastCFS api library

%package -n %{FastCFSAPIDevel}
Requires: %{FastCFSAPI} = %{version}-%{release}
Summary: header files of FastCFS api library

%package -n %{FastCFSAuthDevel}
Requires: %{FastCFSAuthClient} = %{version}-%{release}
Summary: header files of FastCFS auth client

%package -n %{FastCFSAuthServer}
Requires: fastDIR-client >= 2.0.0
Summary: FastCFS auth server

%package -n %{FastCFSAuthClient}
Requires: libfastcommon >= 1.0.49
Requires: libserverframe >= 1.1.6
Summary: FastCFS auth client

%package -n %{FastCFSFuseConfig}
Requires: faststore-config >= 1.0.0
Summary: FastCFS fuse config files for sample

%package -n %{FastCFSAuthConfig}
Summary: FastCFS auth config files for sample

%description -n %{FastCFSFused}
FastCFS fuse
commit version: %{CommitVersion}

%description -n %{FastCFSAPI}
FastCFS api library
commit version: %{CommitVersion}

%description -n %{FastCFSAPIDevel}
This package provides the header files of libfcfsapi
commit version: %{CommitVersion}

%description -n %{FastCFSAuthDevel}
This package provides the header files of libfcfsauthclient
commit version: %{CommitVersion}

%description -n %{FastCFSAuthServer}
FastCFS auth server
commit version: %{CommitVersion}

%description -n %{FastCFSAuthClient}
FastCFS auth client
commit version: %{CommitVersion}

%description -n %{FastCFSFuseConfig}
FastCFS fuse config files for sample
commit version: %{CommitVersion}

%description -n %{FastCFSAuthConfig}
FastCFS auth config files for sample
commit version: %{CommitVersion}


%prep
%setup -q

%build
./make.sh clean && ./make.sh

%install
rm -rf %{buildroot}
DESTDIR=$RPM_BUILD_ROOT ./make.sh install
FUSE_CONFDIR=%{buildroot}/etc/fastcfs/fcfs/
AUTH_CONFDIR=%{buildroot}/etc/fastcfs/auth/
SYSTEMDIR=%{buildroot}/usr/lib/systemd/system/
mkdir -p $FUSE_CONFDIR
mkdir -p $AUTH_CONFDIR
mkdir -p $SYSTEMDIR
cp conf/*.conf $FUSE_CONFDIR
cp -R src/auth/conf/* $AUTH_CONFDIR
cp systemd/fastcfs.service $SYSTEMDIR
cp systemd/fcfs_authd.service $SYSTEMDIR

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

%files -n %{FastCFSAPIDevel}
%defattr(-,root,root,-)
/usr/include/fastcfs/api/*

%post -n %{FastCFSAuthServer}
mkdir -p /opt/fastcfs/auth

%files -n %{FastCFSAuthServer}
/usr/bin/fcfs_authd
%config(noreplace) /usr/lib/systemd/system/fcfs_authd.service

%files -n %{FastCFSAuthClient}
/usr/lib64/libfcfsauthclient.so*
/usr/bin/fcfs_user
/usr/bin/fcfs_pool

%files -n %{FastCFSAuthDevel}
%defattr(-,root,root,-)
/usr/include/fastcfs/auth/*

%files -n %{FastCFSFuseConfig}
%defattr(-,root,root,-)
%config(noreplace) /etc/fastcfs/fcfs/*.conf

%files -n %{FastCFSAuthConfig}
%defattr(-,root,root,-)
%config(noreplace) /etc/fastcfs/auth/*.conf
%config(noreplace) /etc/fastcfs/auth/keys/*

%changelog
* Fri Jan 1 2021 YuQing <384681@qq.com>
- first RPM release (1.0)
