%define FastCFSFUSE FastCFS-fuse
%define FastCFSAPI FastCFS-api
%define FastCFSDevel FastCFS-devel
%define FastCFSDebuginfo FastCFS-debuginfo
%define CommitVersion %(echo $COMMIT_VERSION)

Name: FastCFS
Version: 1.1.1
Release: 1%{?dist}
Summary: a high performance cloud native distributed file system for databases
License: AGPL v3.0
Group: Arch/Tech
URL:  http://github.com/happyfish100/FastCFS/
Source: http://github.com/happyfish100/FastCFS/%{name}-%{version}.tar.gz

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n) 

BuildRequires: fastDIR-devel >= 1.1.1
BuildRequires: faststore-devel >= 1.1.1
BuildRequires: fuse3-devel >= 3.10.1
Requires: %__cp %__mv %__chmod %__grep %__mkdir %__install %__id
Requires: %{FastCFSFUSE} = %{version}-%{release}
Requires: %{FastCFSAPI} = %{version}-%{release}

%description
a high performance distributed file system which can be used as the back-end storage of databases and cloud platforms.
commit version: %{CommitVersion}

%package -n %{FastCFSFUSE}
Requires: %{FastCFSAPI} = %{version}-%{release}
Requires: fuse3 >= 3.10.1
Summary: FastCFS fuse

%package -n %{FastCFSAPI}
Requires: fastDIR-client >= 1.1.1
Requires: faststore-client >= 1.1.1
Summary: FastCFS api library

%package -n %{FastCFSDevel}
Requires: %{FastCFSAPI} = %{version}-%{release}
Summary: header files of FastCFS api library

%description -n %{FastCFSFUSE}
FastCFS fuse
commit version: %{CommitVersion}

%description -n %{FastCFSAPI}
FastCFS api library
commit version: %{CommitVersion}

%description -n %{FastCFSDevel}
This package provides the header files of libfcfsapi
commit version: %{CommitVersion}


%prep
%setup -q

%build
./make.sh clean && ./make.sh

%install
rm -rf %{buildroot}
DESTDIR=$RPM_BUILD_ROOT ./make.sh install

%post

%preun

%postun

%clean
rm -rf %{buildroot}

%files

%files -n %{FastCFSFUSE}
/usr/bin/fcfs_fused

%files -n %{FastCFSAPI}
%defattr(-,root,root,-)
/usr/lib64/libfcfsapi.so*

%files -n %{FastCFSDevel}
%defattr(-,root,root,-)
/usr/include/fastcfs/*

%changelog
* Fri Jan 1 2021 YuQing <384681@qq.com>
- first RPM release (1.0)
