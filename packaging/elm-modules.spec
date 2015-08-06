Name:       efl-modules
Summary:    The EFL Modules
Version:    0.1.1
Release:    1
Group:      System/Libraries
License:    Flora-1.1
URL:        http://www.tizen.org
Source0:    %{name}-%{version}.tar.gz
BuildRequires: elementary-devel, eina-devel
BuildRequires: pkgconfig(icu-i18n)

%description
The EFL Modules

%prep
%setup -q

%build
export CFLAGS+=" -fvisibility=hidden -fPIC -Wall"
export LDFLAGS+=" -fvisibility=hidden -Wl,--hash-style=both -Wl,--as-needed -Wl,-z,defs"

%define DEF_SUBDIRS datetime_input_circle
for FILE in %{DEF_SUBDIRS}
do
    cd $FILE
    if [ -f autogen.sh ]
    then
        ./autogen.sh && %configure && make
    else
        make
    fi
    cd ..
done

%install
mkdir -p %{buildroot}/%{_datadir}/license
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/%{_datadir}/license/%{name}

for FILE in %{DEF_SUBDIRS}
do
        (cd $FILE && PREFIX=%{buildroot}/usr make install DESTDIR=%{buildroot})
done

%files
%defattr(-,root,root,-)
%{_libdir}/elementary/modules/*/*/*.so
%{_datadir}/license/%{name}
%manifest %{name}.manifest
## Below is not needed except Machintosh
%exclude %{_libdir}/elementary/modules/*/*/*.la
