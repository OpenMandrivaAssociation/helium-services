#!/bin/bash
# Publish Helium's spellcheck files from the hunspell dictionaries already
# installed for LibreOffice and other myspell users. Helium requests
# Chromium .bdic names (en-US-10-2.bdic and the rest of the table below).
# Those files are a cache. The word lists stay in /usr/share/hunspell.
# The previous tree is left in place when conversion fails outright.
set -euo pipefail

hunspell_dir=${HUNSPELL_DIR:-/usr/share/hunspell}
root=${DICT_ROOT:-/srv/www/helium-services}
converter=${DICT_CONVERTER:-/usr/libexec/helium-services/hunspell-to-bdic}
manifest_file=${DICT_MANIFEST:-/var/lib/helium-services/dict.manifest}

# Region, version suffix, and myspell basenames in preference order.
# Versions are GetVersionedFileName from Chromium 158.0.8127.36, which
# is what Helium 0.22.3 requests. Basenames follow libreoffice-dictionaries.
dict_table() {
	cat <<'EOF'
af-ZA	-3-0	af_ZA
bg-BG	-3-0	bg_BG
ca-ES	-3-0	ca ca_ES
cs-CZ	-3-0	cs_CZ
cy-GB	-1-0	cy_GB
da-DK	-3-0	da_DK
de-DE	-3-1	de_DE_frami de_DE
el-GR	-3-0	el_GR
en-AU	-10-2	en_AU
en-CA	-10-2	en_CA
en-GB	-10-2	en_GB
en-GB-oxendict	-10-2	en_GB-oxendict en-GB-oxendict en_GB_oxendict
en-US	-10-2	en_US
es-ES	-3-0	es_ES es es_MX
et-EE	-3-0	et_EE
fa-IR	-9-0	fa-IR fa_IR
fo-FO	-3-0	fo_FO
fr-FR	-3-1	fr fr_FR
gl	-1-1	gl_ES gl
he-IL	-3-0	he_IL
hi-IN	-3-0	hi_IN
hr-HR	-3-0	hr_HR
hu-HU	-3-1	hu_HU
hy	-1-1	hy_AM hy
id-ID	-3-0	id_ID
it-IT	-3-0	it_IT
ko	-3-0	ko_KR ko
lt-LT	-3-0	lt lt_LT
lv-LV	-3-0	lv_LV
nb-NO	-3-1	nb_NO
nl-NL	-3-1	nl_NL
pl-PL	-3-0	pl_PL
pt-BR	-3-1	pt_BR
pt-PT	-3-0	pt_PT
ro-RO	-3-1	ro_RO
ru-RU	-3-0	ru_RU
sh	-4-0	sr-Latn sh sr_Latn
sk-SK	-3-0	sk_SK
sl-SI	-3-0	sl_SI
sq	-3-0	sq_AL sq
sr	-4-0	sr sr_RS
sv-SE	-3-0	sv_SE sv
ta-IN	-3-1	ta_IN
tg-TG	-5-0	tg_TG tg
tr-TR	-4-0	tr_TR
uk-UA	-5-0	uk_UA
vi-VN	-3-0	vi_VN vi
EOF
}

if [[ ! -x "$converter" ]]; then
	echo "helium-services: missing $converter" >&2
	exit 1
fi

pick_dict() {
	local name
	for name in "$@"; do
		if [[ -f "$hunspell_dir/$name.aff" && -f "$hunspell_dir/$name.dic" ]]; then
			printf '%s\n' "$name"
			return 0
		fi
	done
	return 1
}

rm -rf "$root/tmp" "$root/tmp2" "$root/tmp.manifest" "$root/tmp.published"
trap 'rm -rf "$root/tmp" "$root/tmp2" "$root/tmp.manifest" "$root/tmp.published"' EXIT
mkdir -p "$root"
: > "$root/tmp.manifest"
# The converter binary is part of the cache key, so a package update
# that changes it republishes the dictionaries.
stat -c 'converter %n %Y %s' "$converter" >> "$root/tmp.manifest"

while IFS=$'\t' read -r region version candidates; do
	[[ -z "${region:-}" ]] && continue
	read -r -a names <<< "$candidates"
	base=$(pick_dict "${names[@]}") || continue
	stat -c '%n %Y %s' "$hunspell_dir/$base.aff" "$hunspell_dir/$base.dic" \
		>> "$root/tmp.manifest"
	printf '%s%s.bdic\n' "$region" "$version" >> "$root/tmp.manifest"
done < <(dict_table)

if [[ -d "$root/dict" && -f "$manifest_file" ]] && cmp -s "$root/tmp.manifest" "$manifest_file"; then
	exit 0
fi

mkdir -p "$root/tmp"
: > "$root/tmp.published"
stat -c 'converter %n %Y %s' "$converter" >> "$root/tmp.published"
ok=0
bad=0
while IFS=$'\t' read -r region version candidates; do
	[[ -z "${region:-}" ]] && continue
	read -r -a names <<< "$candidates"
	base=$(pick_dict "${names[@]}") || continue
	outname="${region}${version}.bdic"
	if ! "$converter" "$hunspell_dir/$base" "$root/tmp/$outname"; then
		echo "helium-services: could not convert $base to $outname" >&2
		rm -f "$root/tmp/$outname"
		bad=$((bad + 1))
		continue
	fi
	gzip -n -9 -c "$root/tmp/$outname" > "$root/tmp/$outname.gz"
	stat -c '%n %Y %s' "$hunspell_dir/$base.aff" "$hunspell_dir/$base.dic" \
		>> "$root/tmp.published"
	printf '%s\n' "$outname" >> "$root/tmp.published"
	ok=$((ok + 1))
done < <(dict_table)

if ((bad > 0 && ok == 0)); then
	echo "helium-services: no spellcheck dictionary could be converted" >&2
	exit 1
fi

find "$root/tmp" -type d -exec chmod 0755 {} +
find "$root/tmp" -type f -exec chmod 0644 {} +
if [[ -d "$root/dict" ]]; then
	mv -f "$root/dict" "$root/tmp2"
fi
mv -f "$root/tmp" "$root/dict"
rm -rf "$root/tmp2"
install -m 0644 "$root/tmp.published" "$manifest_file"
echo "helium-services: published $ok spellcheck dictionaries ($bad failed)" >&2
