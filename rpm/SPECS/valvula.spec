%define release_date %(date +"%a %b %d %Y")
%define valvula_version %(cat VERSION)

Name:           valvula
Version:        %{valvula_version}
Release:        5%{?dist}
Summary:        Open source High performance policy daemon
Group:          System Environment/Libraries
License:        GPLv2+ 
URL:            http://www.aspl.es/valvula
Source:         %{name}-%{version}.tar.gz


%define debug_package %{nil}

%description
Valvula is a OpenSource high performance mail policy daemon 
for Postfix, written in ansi C, that provides out of the box 
support for sender login mismatch, mail quotas, per user and 
per domain blacklists and whitelists and much more.

%prep
%setup -q

%build
PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/local/lib/pkgconfig %configure --prefix=/usr --sysconfdir=/etc
make clean
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot} INSTALL='install -p'
find %{buildroot} -name '*.la' -exec rm -f {} ';'
find %{buildroot} -name "mod-mw.*" -delete
find %{buildroot} -name "mod-test.*" -delete
mkdir -p %{buildroot}/etc/init.d
install -p %{_builddir}/%{name}-%{version}/doc/valvulad-rpm-init.d %{buildroot}/etc/init.d/valvulad

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

# %files -f %{name}.lang
%doc AUTHORS COPYING NEWS README THANKS
# %{_libdir}/libaxl.so.*

# %files devel
# %doc COPYING
# %{_includedir}/axl*
# %{_libdir}/libaxl.so
# %{_libdir}/pkgconfig/axl.pc

# libvalvula package
%package -n libvalvula
Summary: Base Valvula library (process support)
Group: System Environment/Libraries
Requires: libaxl1
%description  -n libvalvula
libvalvula is the core library used by the project. It provides
basic functions to parse incoming requests, thread handling and
other support functions.
%files -n libvalvula
   /usr/lib64/libvalvula.a
   /usr/lib64/libvalvula.so
   /usr/lib64/libvalvula.so.0
   /usr/lib64/libvalvula.so.0.0.0

# libvalvula-dev package
%package -n libvalvula-dev
Summary: Base Valvula library (development headers)
Group: System Environment/Libraries
Requires: libvalvula
%description  -n libvalvula-dev
libvalvula core library developement headers
%files -n libvalvula-dev
   /usr/include/valvula/valvula.h
   /usr/include/valvula/valvula_connection.h
   /usr/include/valvula/valvula_ctx.h
   /usr/include/valvula/valvula_handlers.h
   /usr/include/valvula/valvula_hash.h
   /usr/include/valvula/valvula_io.h
   /usr/include/valvula/valvula_listener.h
   /usr/include/valvula/valvula_private.h
   /usr/include/valvula/valvula_reader.h
   /usr/include/valvula/valvula_support.h
   /usr/include/valvula/valvula_thread.h
   /usr/include/valvula/valvula_thread_pool.h
   /usr/include/valvula/valvula_types.h
   /usr/lib64/pkgconfig/valvula.pc

# libvalvula-server package
%package -n libvalvula-server
Summary: Base library used by the server valvulad
Group: System Environment/Libraries
Requires: libvalvula
%description  -n libvalvula-server
libvalvula-server is the base library providing all features that
are needed by valvulad-server and using support from libvalvula.
%files -n libvalvula-server
   /usr/lib64/libvalvulad.a
   /usr/lib64/libvalvulad.so
   /usr/lib64/libvalvulad.so.0
   /usr/lib64/libvalvulad.so.0.0.0


# libvalvula-server-dev package
%package -n libvalvula-server-dev
Summary: Base library used by the server valvulad (dev headers)
Group: System Environment/Libraries
Requires: libvalvula-server
%description  -n libvalvula-server-dev
libvalvula-server development headers
%files -n libvalvula-server-dev
   /usr/include/valvula/valvulad.h
   /usr/include/valvula/valvulad_config.h
   /usr/include/valvula/valvulad_db.h
   /usr/include/valvula/valvulad_log.h
   /usr/include/valvula/valvulad_moddef.h
   /usr/include/valvula/valvulad_module.h
   /usr/include/valvula/valvulad_run.h
   /usr/include/valvula/exarg.h
   /usr/lib64/pkgconfig/valvulad.pc

# valvulad-server package
%package -n valvulad-server
Summary: high performance policy server (for postfix)
Group: System Environment/Libraries
Requires: libvalvula-server
%description  -n valvulad-server
valvulad-server is a high performance, thread based policy server
with support for postfix.
%files -n valvulad-server
   /usr/bin/valvulad
   /etc/valvula/valvula.example.conf
   /usr/bin/check-valvulad.py
   /usr/bin/valvulad-mgr.py
   /etc/cron.d/check-valvulad
   /etc/init.d/valvulad

%post -n valvulad-server
chkconfig valvulad on
if [ ! -d /etc/valvula/mods-enabled ]; then
   mkdir -p /etc/valvula/mods-enabled
fi
if [ ! -f /etc/valvula/valvula.conf ]; then
        cp /etc/valvula/valvula.example.conf /etc/valvula/valvula.conf
fi
service valvulad restart

# valvulad-mod-ticket package
%package -n valvulad-mod-ticket
Summary: mod-ticket support for valvulad-server
Group: System Environment/Libraries
Requires: valvulad-server
%description  -n valvulad-mod-ticket
mod-ticket is a module use to control mail sendings using
preconfigured plans, limitting sendings by day, month, total amount
or period.
%files -n valvulad-mod-ticket
   /usr/lib/valvulad/modules/mod-ticket.a
   /usr/lib/valvulad/modules/mod-ticket.so
   /usr/lib/valvulad/modules/mod-ticket.so.0
   /usr/lib/valvulad/modules/mod-ticket.so.0.0.0
   /etc/valvula/mods-available/mod-ticket.xml


# valvulad-mod-slm package
%package -n valvulad-mod-slm
Summary: mod-slm support for valvulad-server
Group: System Environment/Libraries
Requires: valvulad-server
%description  -n valvulad-mod-slm
mod-slm is a module that implements flexible sender login mismatch
detection and blocking. It implements several automatic policies and 
provides support for exceptions.
%files -n valvulad-mod-slm
   /usr/lib/valvulad/modules/mod-slm.a
   /usr/lib/valvulad/modules/mod-slm.so
   /usr/lib/valvulad/modules/mod-slm.so.0
   /usr/lib/valvulad/modules/mod-slm.so.0.0.0
   /etc/valvula/mods-available/mod-slm.xml

# valvulad-mod-bwl package
%package -n valvulad-mod-bwl
Summary: mod-bwl support for valvulad-server
Group: System Environment/Libraries
Requires: valvulad-server
%description  -n valvulad-mod-bwl
mod-bwl is a module that provides extended blacklist and whilte lists
support to valvula, help the server to define different level of rules
that applies globally, at domain level and at account level.
%files -n valvulad-mod-bwl
   /usr/lib/valvulad/modules/mod-bwl.a
   /usr/lib/valvulad/modules/mod-bwl.so
   /usr/lib/valvulad/modules/mod-bwl.so.0
   /usr/lib/valvulad/modules/mod-bwl.so.0.0.0
   /etc/valvula/mods-available/mod-bwl.xml

# valvulad-mod-mquota package
%package -n valvulad-mod-mquota
Summary: mod-mquota support for valvulad-server
Group: System Environment/Libraries
Requires: valvulad-server
%description  -n valvulad-mod-mquota
mod-mquota is a module that provides sending quotas to valvula. It allows 
to control sending rate by minute, hour, and globally within administrator 
defined periods.
%files -n valvulad-mod-mquota
   /usr/lib/valvulad/modules/mod-mquota.a
   /usr/lib/valvulad/modules/mod-mquota.so
   /usr/lib/valvulad/modules/mod-mquota.so.0
   /usr/lib/valvulad/modules/mod-mquota.so.0.0.0
   /etc/valvula/mods-available/mod-mquota.xml

# valvulad-mod-object-resolver package
%package -n valvulad-mod-object-resolver
Summary: mod-object-resolver support for valvulad-server
Group: System Environment/Libraries
Requires: valvulad-server
%description  -n valvulad-mod-object-resolver
mod-object-resolver tries to aid valvulad engine to detect local domain
and sasl user accounts that are not available using normal configurations
with MySQL.
%files -n valvulad-mod-object-resolver
   /usr/lib/valvulad/modules/mod-object-resolver.a
   /usr/lib/valvulad/modules/mod-object-resolver.so
   /usr/lib/valvulad/modules/mod-object-resolver.so.0
   /usr/lib/valvulad/modules/mod-object-resolver.so.0.0.0
   /etc/valvula/mods-available/mod-object-resolver.xml
   
# valvulad-mod-lmm package
%package -n valvulad-mod-lmm
Summary: mod-lmm support for valvulad-server
Group: System Environment/Libraries
Requires: valvulad-server
%description  -n valvulad-mod-lmm
Valvulad plugin to limit mail from use/forge from unauthorized sources
%files -n valvulad-mod-lmm
   /usr/lib/valvulad/modules/mod-lmm.a
   /usr/lib/valvulad/modules/mod-lmm.so
   /usr/lib/valvulad/modules/mod-lmm.so.0
   /usr/lib/valvulad/modules/mod-lmm.so.0.0.0
   /etc/valvula/mods-available/mod-lmm.xml


%changelog
%include rpm/SPECS/changelog.inc

