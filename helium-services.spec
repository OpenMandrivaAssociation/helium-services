# Snapshot of imputnet/helium-services commit
# 080ec153c03bcd142ceafa5d91d1eaa54eec8c73 (2026-11-23). There are no
# upstream release tags.
#
# Packaged stack: extension-proxy, the uBlock asset proxy, bangs.json,
# and spellcheck dictionaries converted from the system hunspell files.
# components, minipush, minidumpster,
# sparkler, and release-collector are separate upstream programs and
# are not part of this package.
#
# Recreate the crate vendor archive when the extension-proxy lock changes:
#   tar xf helium-services-VERSION.tar.gz
#   cd helium-services-COMMIT/svc/extension-proxy
#   cargo vendor --locked vendor
#   tar -C . -cf - vendor | zstd -T0 -10 -o ../helium-services-VERSION-vendor.tar.zst
#   abb store helium-services-VERSION-vendor.tar.zst
#
# Do not use BuildSystem: cargo. Its prep drops Cargo.lock and expects
# the system crate registry. This package vendors crates and keeps the lock.
#
# cargo test is not run. The proxy tests talk to the network.
#
# Spellcheck does not download Chromium's hunspell snapshot. The refresh
# reads myspell dictionaries from /usr/share/hunspell and writes the .bdic
# names Helium 0.22.3 requests. The converter sources are Chromium
# 158.0.8127.36 convert_dict plus a small compatibility layer. Recreate
# the archive from convert-dict/ after editing that directory:
#   tar -C convert-dict -cf - . | zstd -T0 -19 -o helium-services-bdic.tar.zst
#   abb store helium-services-bdic.tar.zst

%global commit 080ec153c03bcd142ceafa5d91d1eaa54eec8c73

Name:		helium-services
Version:	0.20261123
Release:	2
Summary:	Self-hosted services for the Helium browser
License:	AGPL-3.0-only AND BSD-3-Clause
Group:		System/Servers
URL:		https://github.com/imputnet/helium-services
Source0:	https://github.com/imputnet/helium-services/archive/%{commit}.tar.gz#/helium-services-%{version}.tar.gz
Source1:	helium-services-%{version}-vendor.tar.zst
Source2:	README.install.omv
Source3:	helium-services-ubo-serve.ts
Source4:	helium-services.env
Source5:	helium-services.sysusers
Source6:	helium-services.tmpfiles
Source7:	helium-services.nginx.conf
Source8:	helium-services.apache.conf
Source9:	helium-services-refresh-bangs.sh
Source10:	helium-services-refresh-dicts.sh
Source11:	helium-services-extension-proxy.service
Source12:	helium-services-ubo.service
Source13:	helium-services-bangs.service
Source14:	helium-services-bangs.timer
Source15:	helium-services-dicts.service
Source16:	helium-services-dicts.timer
Source17:	helium-services-index.html
Source18:	helium-services-bdic.tar.zst
Source19:	helium-services-dicts.path

Patch0:		helium-services-ubo-bun.patch

BuildRequires:	cargo
BuildRequires:	clang
BuildRequires:	cmake
BuildRequires:	gzip
BuildRequires:	make
BuildRequires:	pkgconfig(icu-uc)
BuildRequires:	pkgconfig(libcrypto)
BuildRequires:	rust
BuildRequires:	rust-packaging

Requires:	bun
Requires:	curl
Requires:	gzip
Requires:	helium-services-webserver-integration = %{EVRD}
Recommends:	helium-services-nginx = %{EVRD}
Recommends:	myspell-en

%description
Helium services serves the browser's extension downloads, component
updates, uBlock Origin filter lists, bangs list, and spellcheck
dictionaries from a machine you run.

extension-proxy and the uBlock proxy listen on localhost. A nginx or
Apache integration package publishes them. The vhost is not enabled for
you: set the hostname, then turn the site on. See README.install.omv.

Installing this package recommends the nginx integration. Install
helium-services-apache as well to use Apache instead of, or next to, nginx.

The uBlock proxy runs on bun. This snapshot still requests uBlock Origin
1.76.4 assets; the environment file can point it at another assets.json.
Crate licenses for the extension proxy are in LICENSES.dependencies.

Spellcheck uses the myspell dictionaries in /usr/share/hunspell, the same
files LibreOffice uses. The package converts them to the .bdic names
Helium downloads. It does not ship or download a second word list.
Installing myspell-en is recommended. Other languages appear when their
myspell package is installed.

%package nginx
Summary:	Nginx site for Helium services
Group:		System/Servers
Requires:	%{name} = %{EVRD}
Requires:	nginx
Provides:	helium-services-webserver-integration = %{EVRD}

%description nginx
Nginx site for the Helium services. The file is installed under
sites-available and is not linked into sites-enabled.

%package apache
Summary:	Apache site for Helium services
Group:		System/Servers
Requires:	%{name} = %{EVRD}
Requires:	apache
# apache-base is the package that owns the default docroot. The extra
# test installs every subpackage in one transaction, then splits the
# known nginx/apache-base pair. It only notices this subpackage is on
# the Apache side when apache-base is a direct dependency.
Requires:	apache-base
Requires:	apache-mod_alias
Requires:	apache-mod_autoindex
Requires:	apache-mod_headers
Requires:	apache-mod_proxy
Requires:	apache-mod_proxy_http
Requires:	apache-mod_rewrite
Provides:	helium-services-webserver-integration = %{EVRD}

%description apache
Apache virtual host for the Helium services. The file is not included
from webapps.d until an administrator links it.

%prep
%autosetup -p1 -n helium-services-%{commit}
cd svc/extension-proxy
/usr/bin/rpmuncompress -x %{SOURCE1}
%cargo_prep -v vendor
cd ../..
mkdir convert-dict
cd convert-dict
/usr/bin/rpmuncompress -x %{SOURCE18}

%build
cd svc/extension-proxy
export CARGO_NET_OFFLINE=true
%{cargo_license} > LICENSES.dependencies
%cargo_build -- --locked --offline
cd ../..
%set_build_flags
bash convert-dict/compile.sh "$PWD/hunspell-to-bdic"

%install
install -D -m 0755 svc/extension-proxy/target/rpm/extension-proxy \
	%{buildroot}%{_libexecdir}/helium-services/extension-proxy
install -D -m 0755 hunspell-to-bdic \
	%{buildroot}%{_libexecdir}/helium-services/hunspell-to-bdic
install -D -m 0755 %{SOURCE9} \
	%{buildroot}%{_libexecdir}/helium-services/refresh-bangs.sh
install -D -m 0755 %{SOURCE10} \
	%{buildroot}%{_libexecdir}/helium-services/refresh-dicts.sh

mkdir -p %{buildroot}%{_prefix}/lib/helium-services/ubo
cp -a svc/ubo/main.ts svc/ubo/lib %{buildroot}%{_prefix}/lib/helium-services/ubo/
install -m 0644 %{SOURCE3} %{buildroot}%{_prefix}/lib/helium-services/ubo/serve.ts
find %{buildroot}%{_prefix}/lib/helium-services/ubo -type d -exec chmod 0755 {} +
find %{buildroot}%{_prefix}/lib/helium-services/ubo -type f -exec chmod 0644 {} +

install -d %{buildroot}/srv/www/helium-services/bangs
install -d %{buildroot}/srv/www/helium-services/dict
install -d %{buildroot}/srv/www/helium-services/htdocs
install -m 0644 svc/bangs/bangs.json \
	%{buildroot}/srv/www/helium-services/bangs/bangs.json
gzip -9 -c svc/bangs/bangs.json \
	> %{buildroot}/srv/www/helium-services/bangs/bangs.json.gz
chmod 0644 %{buildroot}/srv/www/helium-services/bangs/bangs.json.gz
install -m 0644 %{SOURCE17} %{buildroot}/srv/www/helium-services/htdocs/index.html

install -D -m 0640 %{SOURCE4} \
	%{buildroot}%{_sysconfdir}/helium-services/helium-services.env
install -D -m 0644 %{SOURCE5} %{buildroot}%{_sysusersdir}/helium-services.conf
install -D -m 0644 %{SOURCE6} %{buildroot}%{_tmpfilesdir}/helium-services.conf
install -D -m 0644 %{SOURCE11} %{buildroot}%{_unitdir}/helium-services-extension-proxy.service
install -D -m 0644 %{SOURCE12} %{buildroot}%{_unitdir}/helium-services-ubo.service
install -D -m 0644 %{SOURCE13} %{buildroot}%{_unitdir}/helium-services-bangs.service
install -D -m 0644 %{SOURCE14} %{buildroot}%{_unitdir}/helium-services-bangs.timer
install -D -m 0644 %{SOURCE15} %{buildroot}%{_unitdir}/helium-services-dicts.service
install -D -m 0644 %{SOURCE16} %{buildroot}%{_unitdir}/helium-services-dicts.timer
install -D -m 0644 %{SOURCE19} %{buildroot}%{_unitdir}/helium-services-dicts.path

install -D -m 0644 %{SOURCE7} \
	%{buildroot}%{_sysconfdir}/nginx/sites-available/helium-services.conf
install -D -m 0644 %{SOURCE8} \
	%{buildroot}%{_sysconfdir}/httpd/conf/webapps.available/helium-services.conf

cp %{SOURCE2} README.install.omv

%post
envfile=%{_sysconfdir}/helium-services/helium-services.env
if grep -q '^HMAC_SECRET=$' "$envfile"; then
	secret=$(od -An -N32 -tx1 /dev/urandom | tr -d ' \n')
	sed -i "s/^HMAC_SECRET=$/HMAC_SECRET=${secret}/" "$envfile"
fi
systemctl enable --now helium-services-extension-proxy.service
systemctl enable --now helium-services-ubo.service
systemctl enable --now helium-services-bangs.timer
systemctl enable --now helium-services-dicts.timer
systemctl enable --now helium-services-dicts.path
systemctl start --no-block helium-services-dicts.service
if [ "$1" -gt 1 ]; then
	systemctl try-restart helium-services-extension-proxy.service
	systemctl try-restart helium-services-ubo.service
fi

%preun
if [ "$1" = 0 ]; then
	systemctl disable --now helium-services-extension-proxy.service
	systemctl disable --now helium-services-ubo.service
	systemctl disable --now helium-services-bangs.timer
	systemctl disable --now helium-services-dicts.timer
	systemctl disable --now helium-services-dicts.path
fi

%files
%license LICENSE svc/extension-proxy/LICENSES.dependencies convert-dict/LICENSE.bsd
%doc README.md README.install.omv
%{_libexecdir}/helium-services
%{_prefix}/lib/helium-services
%{_unitdir}/helium-services-extension-proxy.service
%{_unitdir}/helium-services-ubo.service
%{_unitdir}/helium-services-bangs.service
%{_unitdir}/helium-services-bangs.timer
%{_unitdir}/helium-services-dicts.service
%{_unitdir}/helium-services-dicts.timer
%{_unitdir}/helium-services-dicts.path
%{_sysusersdir}/helium-services.conf
%{_tmpfilesdir}/helium-services.conf
%dir %attr(0750,root,helium-services) %{_sysconfdir}/helium-services
%dir %attr(0750,helium-services,helium-services) /var/lib/helium-services
%config(noreplace) %attr(0640,root,helium-services) %verify(not md5 size mtime) %{_sysconfdir}/helium-services/helium-services.env
%dir %attr(0755,helium-services,helium-services) /srv/www/helium-services
%dir %attr(0755,helium-services,helium-services) /srv/www/helium-services/bangs
%dir %attr(0755,helium-services,helium-services) /srv/www/helium-services/dict
%dir %attr(0755,root,root) /srv/www/helium-services/htdocs
%attr(0644,helium-services,helium-services) /srv/www/helium-services/bangs/bangs.json
%attr(0644,helium-services,helium-services) /srv/www/helium-services/bangs/bangs.json.gz
%attr(0644,root,root) /srv/www/helium-services/htdocs/index.html

%files nginx
%attr(0644,root,root) %config(noreplace) %{_sysconfdir}/nginx/sites-available/helium-services.conf

%files apache
%dir %{_sysconfdir}/httpd/conf/webapps.available
%attr(0644,root,root) %config(noreplace) %{_sysconfdir}/httpd/conf/webapps.available/helium-services.conf
