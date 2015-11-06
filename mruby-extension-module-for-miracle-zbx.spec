%define distversion %(grep -i "release ." -o /etc/redhat-release |cut -c 9 2> /dev/null)
%define is_ML3 %(if [ "%distversion" = "3" ]; then echo 1; else echo 0; fi)
%define is_ML4 %(if [ "%distversion" = "4" ]; then echo 1; else echo 0; fi)
%define is_ML5 %(if [ "%distversion" = "5" ]; then echo 1; else echo 0; fi)
%define is_ML6 %(if [ "%distversion" = "6" ]; then echo 1; else echo 0; fi)
%define is_ML7 %(if [ "%distversion" = "7" ]; then echo 1; else echo 0; fi)
%define is_amz %(grep -i "Amazon Linux AMI" /etc/system-release > /dev/null 2>&1 && echo 1 || echo 0)

%define dist ML%{?distversion}
%if %is_amz
%define dist %(grep -o '[0-9]*' /etc/system-release-cpe | tr -d '\\n' | awk '{print substr ($0, 3)}' | sed s/^/amz/)
%endif
%{?amzn1:%define is_ML6 1}

%define _default_patch_fuzz 2
%define src_release 1 
Name:           mruby-extension-module-for-miracle-zbx
Version:        1.0.0
Release:        %{src_release}.%{?dist}
Summary:        mruby extension module for MIRACLE ZBX
Group:          Applications/Internet
License:        GPL
URL:            http://www.miraclelinux.com/

Source0:        mruby-extension-module-for-miracle-zbx-1.0.0.tar.gz

Buildroot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
mruby extension module for MIRACLE ZBX.

%prep
%setup -q

%build

%install
rm -rf $RPM_BUILD_ROOT
# set up some required directories
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/zabbix/zabbix_agentd.d
mkdir -p $RPM_BUILD_ROOT%{_libdir}/zabbix/modules
mkdir -p $RPM_BUILD_ROOT%{_libdir}/zabbix/modules/mruby_module

install -m 0640 -p mruby_extension_module.conf $RPM_BUILD_ROOT%{_sysconfdir}/zabbix/zabbix_agentd.d/
install -m 0640 -p mruby_extension_module.so $RPM_BUILD_ROOT%{_libdir}/zabbix/modules/mruby_extension_module.so
install -m 0640 -p mruby_module/sample.rb $RPM_BUILD_ROOT%{_libdir}/zabbix/modules/mruby_module/sample.rb.sample
install -m 0640 -p mruby_module/tw-mrb-test.rb $RPM_BUILD_ROOT%{_libdir}/zabbix/modules/mruby_module/tw-mrb-test.rb.sample

%clean
rm -rf $RPM_BUILD_ROOT

%pre
%post
%preun
%postun

%files
%defattr(-,root,zabbix,-)
%{_libdir}/zabbix/modules/*
%{_libdir}/zabbix/modules/mruby_module/*
%config(noreplace) %attr(640,root,zabbix) %{_sysconfdir}/zabbix/zabbix_agentd.d/mruby_extension_module.conf


%changelog
* Wed Nov 04 2015 Takanori Suzuki <takanori.suzuki@miraclelinux.com> - 1.0.0-1.%{?dist}
- First packaging

