Summary: A filemanager similar to XTree
Name: ytree
Version: 1.98
Release: 1
Source: http://www.han.de/~werner/ytree-1.98.tar.gz
Copyright: GPL
Group: System Environment/Shells
BuildRoot: %{_tmppath}/root-%{name}-%{version}
BuildRequires: ncurses-devel >= 5.4
BuildRequires: readline >= 4.3

%description
A console based file manager in the tradition of Xtree.

%prep
%setup

%build
make RPM_OPT_FLAGS="$RPM_OPT_FLAGS" DESTDIR="%{_prefix}"

%install
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/share/man/man1
install -m644 -o root -g root ytree.1 $RPM_BUILD_ROOT/usr/share/man/man1/ytree.1
install -m755 -o root -g bin -s ytree $RPM_BUILD_ROOT/usr/bin

%clean
rm -rf $RPM_BUILD_ROOT

%post

%files
%doc CHANGES COPYING README THANKS ytree.conf
/usr/share/man/man1/ytree.1.gz
/usr/bin/ytree

%changelog
* Sun Dec 19 2004 Victor Vislobokov <corochoone@perm.ru> 1.82-1rhfc3.vv
- Fedora Core 3 adaptation
* Fri Dec 31 2004 Werner Bregulla <werner@frolix.han.de> 1.83
- update to 1.83
