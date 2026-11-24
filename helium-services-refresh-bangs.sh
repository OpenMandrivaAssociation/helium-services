#!/bin/bash
# Replace /srv/www/helium-services/bangs/bangs.json from BANG_SOURCE.
# The packaged snapshot stays in place when the download fails.
set -euo pipefail

source_url="${BANG_SOURCE:-https://raw.githubusercontent.com/imputnet/helium-services/refs/heads/main/svc/bangs/bangs.json}"
out_dir="${BANG_DIR:-/srv/www/helium-services/bangs}"

mkdir -p "$out_dir"
tmp="$(mktemp "$out_dir/.bangs.XXXXXXXX")"
cleanup() {
	rm -f "$tmp" "$tmp.gz"
}
trap cleanup EXIT

curl --fail --location --retry 3 --retry-delay 5 --max-time 60 \
	-o "$tmp" "$source_url"
gzip -k -9 "$tmp"
chmod 0644 "$tmp" "$tmp.gz"
mv -f "$tmp" "$out_dir/bangs.json"
mv -f "$tmp.gz" "$out_dir/bangs.json.gz"
trap - EXIT
