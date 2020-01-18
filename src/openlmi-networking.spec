%global logfile %{_localstatedir}/log/openlmi-install.log

Name:           openlmi-networking
Version:        0.2.1
Release:        1%{?dist}
Summary:        CIM providers for network management

License:        LGPLv2+
URL:            http://fedorahosted.org/openlmi/
Source0:        http://fedorahosted.org/released/%{name}/%{name}-%{version}.tar.gz

# Upstream name has been changed from cura-networking to openlmi-networking
Provides:       cura-networking%{?_isa} = %{version}-%{release}
Obsoletes:      cura-networking < 0.0.5-1

BuildRequires:  cmake
BuildRequires:  openlmi-providers-devel >= 0.7.0
BuildRequires:  konkretcmpi-devel >= 0.9.1
BuildRequires:  sblim-cmpi-devel
BuildRequires:  cim-schema
BuildRequires:  glib2-devel
BuildRequires:  check-devel
BuildRequires:  NetworkManager-glib-devel
BuildRequires:  libuuid-devel
BuildRequires:  python-sphinx-theme-openlmi

# For openlmi-register-pegasus script
Requires:       python2
# sblim-sfcb or tog-pegasus
Requires:       cim-server
# Require openlmi-providers because of registration scripts
Requires:       openlmi-providers >= 0.7.0

%description
%{name} is set of CMPI providers for network management using
Common Information Model (CIM).

%package doc
Summary:        Documentation for %{name}
BuildArch:      noarch
# We explicitly don't require openlmi-networking installed, someone might want
# just to read the documentation on different machine.

%description doc
%{summary}.


%prep
%setup -q


%build
mkdir -p %{_target_platform}
pushd %{_target_platform}
%{cmake} ..
popd

make %{?_smp_mflags} -C %{_target_platform}
make doc %{?_smp_mflags} -C %{_target_platform}


%install
make install/fast DESTDIR=$RPM_BUILD_ROOT -C %{_target_platform}
# install the docs
install -m 755 -d $RPM_BUILD_ROOT/%{_docdir}/%{name}/admin_guide/pic/
cp -r %{_target_platform}/doc/admin/html/* $RPM_BUILD_ROOT/%{_docdir}/%{name}/admin_guide/
# We might not have pictures (test builds on buildbot), ignore the failure
cp -r doc/admin/pic/*.svg $RPM_BUILD_ROOT/%{_docdir}/%{name}/admin_guide/pic/ || :

%files
%doc README COPYING
%{_libdir}/cmpi/libcmpiLMI_Networking.so
%{_datadir}/%{name}/60_LMI_Networking.mof
%{_datadir}/%{name}/70_LMI_NetworkingIndicationFilters.mof
%{_datadir}/%{name}/90_LMI_Networking_Profile.mof
%{_datadir}/%{name}/60_LMI_Networking.reg
%attr(755, root, root) %{_libexecdir}/pegasus/cmpiLMI_Networking-cimprovagt

%files doc
%doc README COPYING
%{_docdir}/%{name}/admin_guide


%pre
# If upgrading, deregister old version
if [ "$1" -gt 1 ]; then
    # delete indication filters
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop unregister \
        %{_datadir}/%{name}/70_LMI_NetworkingIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Networking_Profile.mof || :;
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Networking.mof \
        %{_datadir}/%{name}/60_LMI_Networking.reg || :;
fi >> %logfile 2>&1

%post
/sbin/ldconfig
# Register Schema and Provider
if [ "$1" -ge 1 ]; then
    %{_bindir}/openlmi-mof-register -v %{version} register \
        %{_datadir}/%{name}/60_LMI_Networking.mof \
        %{_datadir}/%{name}/60_LMI_Networking.reg || :;
    # install indication filters
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop register \
        %{_datadir}/%{name}/70_LMI_NetworkingIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop  -c tog-pegasus register \
        %{_datadir}/%{name}/90_LMI_Networking_Profile.mof || :;
fi >> %logfile 2>&1

%preun
# Deregister only if not upgrading
if [ "$1" -eq 0 ]; then
    # delete indication filters
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop unregister \
        %{_datadir}/%{name}/70_LMI_NetworkingIndicationFilters.mof || :;
    %{_bindir}/openlmi-mof-register --just-mofs -n root/interop -c tog-pegasus unregister \
        %{_datadir}/%{name}/90_LMI_Networking_Profile.mof || :;
    %{_bindir}/openlmi-mof-register -v %{version} unregister \
        %{_datadir}/%{name}/60_LMI_Networking.mof \
        %{_datadir}/%{name}/60_LMI_Networking.reg || :;
fi >> %logfile 2>&1

%postun -p /sbin/ldconfig


%changelog
* Mon Nov 04 2013 Radek Novacek <rnovacek@redhat.com> 0.2.1-1
- Version 0.2.1

* Mon Oct 14 2013 Radek Novacek <rnovacek@redhat.com> 0.2.0-1
- Version 0.2.0

* Fri Aug 02 2013 Radek Novacek <rnovacek@redhat.com> 0.1.1-1
- Version 0.1.1
- Require openlmi-providers-0.1.0

* Wed Jul 24 2013 Radek Novacek <rnovacek@redhat.com> 0.1.0-1
- Version 0.1.0

* Mon Jun 17 2013 Radek Novacek <rnovacek@redhat.com> 0.0.9-1
- Version 0.0.9

* Mon May 13 2013 Radek Novacek <rnovacek@redhat.com> 0.0.8-1
- Create the spec file.

