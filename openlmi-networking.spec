%global logfile %{_localstatedir}/log/openlmi-install.log

Name:           openlmi-networking
Version:        0.3.0
Release:        3%{?dist}
Summary:        CIM providers for network management

License:        LGPLv2+
URL:            http://fedorahosted.org/openlmi/
Source0:        http://fedorahosted.org/released/%{name}/%{name}-%{version}.tar.gz

# Upstream name has been changed from cura-networking to openlmi-networking
Provides:       cura-networking%{?_isa} = %{version}-%{release}
Obsoletes:      cura-networking < 0.0.5-1

BuildRequires:  cmake
BuildRequires:  openlmi-providers-devel >= 0.5.0
BuildRequires:  konkretcmpi-devel >= 0.9.1
BuildRequires:  sblim-cmpi-devel
BuildRequires:  cim-schema
BuildRequires:  glib2-devel
BuildRequires:  check-devel
BuildRequires:  NetworkManager-glib-devel
BuildRequires:  libuuid-devel
BuildRequires:  python-sphinx
BuildRequires:  python-sphinx-theme-openlmi


# For openlmi-register-pegasus script
Requires:       python2
# sblim-sfcb or tog-pegasus
Requires:       cim-server
# Require openlmi-providers because of registration scripts
Requires:       openlmi-providers >= 0.5.0

# Upstream patches:
# Fix bond deactivation
Patch0:         openlmi-networking-0.3.0-fix-bond-deactivation.patch
# Fix creation of network bridges
Patch1:         openlmi-networking-0.3.0-fix-creation-of-network-bridges.patch

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
%patch0 -p1
%patch1 -p1


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
* Wed Nov 19 2014 Radek Novacek <rnovacek@redhat.com> 0.3.0-3
- Fix creation of network bridges
- Resolves: rhbz#1163793

* Mon Oct 06 2014 Radek Novacek <rnovacek@redhat.com> 0.3.0-2
- Fix bonding deactivation
- Resolves: rhbz#1148849

* Thu Sep 04 2014 Radek Novacek <rnovacek@redhat.com> 0.3.0-1
- Rebase to openlmi-networking 0.3.0
- Resolves: rhbz#1122430

* Tue Mar 11 2014 Stephen Gallagher <sgallagh@redhat.com> - 0.2.2-7
- Related: rhbz#1065298 - Revert unrelated patches

* Mon Mar 10 2014 Stephen Gallagher <sgallagh@redhat.com> 0.2.2-6
- Resolves: rhbz#1065298 - lmishell freezes while creating bond

* Thu Feb 27 2014 Radek Novacek <rnovacek@redhat.com> 0.2.2-5
- Fix "React to IP configuration change" patch
- Resolves: rhbz#1069169

* Tue Feb 25 2014 Radek Novacek <rnovacek@redhat.com> 0.2.2-4
- Fix IPv6Type property of LMI_IPAssignmentSettingData class for DHCPv6
- React to IP configuration change
- Resolves: #1066853, #1069169

* Wed Feb 12 2014 Radek Novacek <rnovacek@redhat.com> 0.2.2-3
- Fix LMI_IPElementSettingData refering bridging master setting instead of slave setting
- Support connections that has interface-name option specified
- Fix that bond slave names are not going above 2
- allow virtual device name change for bonds
- Resolves: #1064215, #1064324, #1064280, #1064334

* Fri Jan 24 2014 Daniel Mach <dmach@redhat.com> - 0.2.2-2
- Mass rebuild 2014-01-24

* Wed Jan 08 2014 Radek Novacek <rnovacek@redhat.com> 0.2.2-1
- Version 0.2.2
- Resolves: rhbz#1049451

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com> - 0.2.1-5
- Mass rebuild 2013-12-27

* Tue Dec 10 2013 Radek Novacek <rnovacek@redhat.com> 0.2.1-4
- Fix that association LMI_IPElementSettingData refers wrong classes
- Resolves: rhbz#1038498

* Mon Dec 02 2013 Radek Novacek <rnovacek@redhat.com> 0.2.1-3
- Use proper classname of slave SettingData in CreateSlaveSetting
- bridging: Don't forget master setting when modifying slave setting
- fix the tests
- Fix potential crash when setting master connection that not exists
- Resolves: rhbz#1035674, rhbz#1035676

* Wed Nov 27 2013 Radek Novacek <rnovacek@redhat.com> 0.2.1-2
- Allow to use SlaveSettingData for activation of slave settings
- Resolves: rhbz#1028475

* Mon Nov 04 2013 Radek Novacek <rnovacek@redhat.com> 0.2.1-1
- Version 0.2.1
- Require openlmi-providers-0.4.1
- Resolves: rhbz#1025337, rhbz#1015556

* Mon Oct 14 2013 Radek Novacek <rnovacek@redhat.com> 0.2.0-1
- Version 0.2.0
- Add -doc subpackage
- Remove dependency on sblim-cmpi-base
- Resolves: rhbz#1012549, rhbz#1011903

* Wed Sep 18 2013 Radek Novacek <rnovacek@redhat.com> 0.1.1-3
- Fix crash when unknown connection found
- Resolves: rhbz#1008937

* Tue Sep 10 2013 Radek Novacek <rnovacek@redhat.com> 0.1.1-2
- Fix CQL in indication filters
- Fix for CIM schema > 2.33
- Resolves: rhbz#1006213

* Thu Aug 08 2013 Radek Novacek <rnovacek@redhat.com> 0.1.1-1
- Version 0.1.1
- Log registration to the log file

* Wed Jul 31 2013 Radek Novacek <rnovacek@redhat.com> 0.1.0-1
- Version 0.1.0

* Tue Jul 23 2013 Radek Novacek <rnovacek@redhat.com> 0.0.9-3
- Rebuild for KonkretCMPI 0.9.1

* Mon Jun 17 2013 Radek Novacek <rnovacek@redhat.com> 0.0.9-2
- Fix for NetworkManager 0.9.9

* Mon Jun 17 2013 Radek Novacek <rnovacek@redhat.com> 0.0.9-1
- Version 0.0.9

* Fri May 10 2013 Radek Novacek <rnovacek@redhat.com> 0.0.8-1
- Version 0.0.8

* Mon Apr 29 2013 Radek Novacek <rnovacek@redhat.com> 0.0.7-1
- Version 0.0.7

* Mon Mar 11 2013 Radek Novacek <rnovacek@redhat.com> 0.0.6-4
- Rebuild for konkretcmpi-0.9.0

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.0.6-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Fri Nov 23 2012 Radek Novacek <rnovacek@redhat.com> 0.0.6-2
- Add requires to sblim-cmpi-base, needed for Linux_ComputerSystem

* Tue Nov 13 2012 Radek Novacek <rnovacek@redhat.com> 0.0.6-1
- Version 0.0.6
- Upstream changed license from GPLv2+ to LGPLv2+

* Wed Oct 31 2012 Radek Novacek <rnovacek@redhat.com> 0.0.5-3
- Rebuild for fixed version of konkretcmpi

* Thu Oct 25 2012 Radek Novacek <rnovacek@redhat.com> 0.0.5-2
- Remove %isa from obsoletes

* Tue Oct 23 2012 Radek Novacek <rnovacek@redhat.com> 0.0.5-1
- Package rename to openlmi-networking (instead of cura-networking)
- Update to 0.0.5

* Fri Oct 12 2012 Radek Novacek <rnovacek@redhat.com> 0.0.4-1
- Update to 0.0.4

* Thu Sep 06 2012 Radek Novacek <rnovacek@redhat.com> 0.0.3-4
- Fix crash when getting length of empty list

* Wed Sep 05 2012 Radek Novacek <rnovacek@redhat.com> 0.0.3-3
- Require cura-providers

* Wed Aug 29 2012 Radek Novacek <rnovacek@redhat.com> 0.0.3-2
- Patch that add linking against pthread

* Wed Aug 29 2012 Radek Novacek <rnovacek@redhat.com> 0.0.3-1
- Update to 0.0.3
- BR: cura-providers
- Don't install files that are common with cura-providers
- Prefix changed to LMI_

* Mon Aug 06 2012 Radek Novacek <rnovacek@redhat.com> 0.0.2-1
- Update to upstream version 0.0.2

* Fri Aug 03 2012 Radek Novacek <rnovacek@redhat.com> 0.0.1-1
- Initial package

