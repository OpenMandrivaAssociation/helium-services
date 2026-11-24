#!/bin/bash
# Build hunspell-to-bdic. Honors CXX, CXXFLAGS, CPPFLAGS, and LDFLAGS.
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=${1:?output path}
cxx=${CXX:-c++}

# shellcheck disable=SC2086
exec "$cxx" -std=c++20 ${CXXFLAGS:-} ${CPPFLAGS:-} \
	$(pkg-config --cflags icu-uc libcrypto) \
	-I"$root" -I"$root/stub" \
	-o "$out" \
	"$root/hunspell_to_bdic.cc" \
	"$root/stub.cc" \
	"$root/chrome/tools/convert_dict/aff_reader.cc" \
	"$root/chrome/tools/convert_dict/dic_reader.cc" \
	"$root/chrome/tools/convert_dict/hunspell_reader.cc" \
	"$root/third_party/hunspell/google/bdict.cc" \
	"$root/third_party/hunspell/google/bdict_reader.cc" \
	"$root/third_party/hunspell/google/bdict_writer.cc" \
	${LDFLAGS:-} \
	$(pkg-config --libs icu-uc libcrypto)
