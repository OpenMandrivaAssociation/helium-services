#!/bin/bash
# Download the pinned Chromium hunspell dictionary snapshot and publish
# it under /srv/www/helium-services/dict/. Each file is kept plain and
# as a .gz sibling: nginx gzip_static serves the compressed one, Apache
# serves the plain one. The previous tree is left in place when the
# download fails.
set -euo pipefail

commit="${DICT_COMMIT:-eb106bd524eea61487644fffeec0d229fa41791a}"
url="https://chromium.googlesource.com/chromium/deps/hunspell_dictionaries/+archive/${commit}.tar.gz"
root="${DICT_ROOT:-/srv/www/helium-services}"

rm -rf "$root/tmp" "$root/tmp2"
mkdir -p "$root/tmp"
curl --fail --location --retry 3 --retry-delay 5 --max-time 300 \
	-o "$root/tmp/dicts.tar.gz" "$url"
tar -C "$root/tmp" -xzf "$root/tmp/dicts.tar.gz"
rm -f "$root/tmp/dicts.tar.gz"
find "$root/tmp" -type f ! -name '*.gz' -exec gzip -k -9 {} +
if [ -d "$root/dict" ]; then
	mv -f "$root/dict" "$root/tmp2"
fi
mv -f "$root/tmp" "$root/dict"
rm -rf "$root/tmp2"
find "$root/dict" -type d -exec chmod 0755 {} +
find "$root/dict" -type f -exec chmod 0644 {} +
