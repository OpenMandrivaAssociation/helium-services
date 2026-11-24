// Converts one hunspell .aff/.dic pair into the .bdic file Helium downloads.
// The writer and reader are Chromium 158's convert_dict. The word list is
// checked against the trie before the file is written.

#include <stdio.h>
#include <string.h>

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/strings/stringprintf.h"

#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "chrome/tools/convert_dict/aff_reader.h"
#include "chrome/tools/convert_dict/dic_reader.h"
#include "third_party/hunspell/google/bdict.h"
#include "third_party/hunspell/google/bdict_reader.h"
#include "third_party/hunspell/google/bdict_writer.h"

namespace {

std::vector<std::string> ReadLines(hunspell::LineIterator iter) {
  std::vector<std::string> lines;
  while (std::optional<std::string_view> line = iter.Advance()) {
    lines.emplace_back(*line);
  }
  return lines;
}

bool SameList(const std::vector<std::string>& got,
              const std::vector<std::string>& expected,
              const char* what) {
  if (got.size() != expected.size()) {
    fprintf(stderr, "%s count %zu, expected %zu\n", what, got.size(),
            expected.size());
    return false;
  }
  for (size_t i = 0; i < got.size(); ++i) {
    const std::string& want = expected[i].empty() ? std::string(" ")
                                                   : expected[i];
    if (got[i] != want) {
      fprintf(stderr, "%s entry %zu does not match\n", what, i);
      return false;
    }
  }
  return true;
}

bool VerifyAff(const convert_dict::AffReader& aff,
               const hunspell::BDictReader& reader) {
  const std::vector<std::string> groups = aff.GetAffixGroups();
  std::vector<std::string> expected_groups;
  expected_groups.push_back(
      base::StringPrintf("AF %d", static_cast<int>(groups.size())));
  expected_groups.insert(expected_groups.end(), groups.begin(), groups.end());
  if (!SameList(ReadLines(reader.GetAfLineIterator()), expected_groups,
                "affix groups")) {
    return false;
  }
  if (!SameList(ReadLines(reader.GetAffixLineIterator()), aff.affix_rules(),
                "affix rules")) {
    return false;
  }
  if (!SameList(ReadLines(reader.GetOtherLineIterator()), aff.other_commands(),
                "other commands")) {
    return false;
  }

  hunspell::ReplacementIterator replacements = reader.GetReplacementIterator();
  std::string_view first;
  std::string_view second;
  size_t index = 0;
  while (replacements.GetNext(&first, &second)) {
    if (index >= aff.replacements().size() ||
        aff.replacements()[index].first != first ||
        aff.replacements()[index].second != second) {
      fprintf(stderr, "replacement %zu does not match\n", index);
      return false;
    }
    ++index;
  }
  if (index != aff.replacements().size()) {
    fprintf(stderr, "replacement count %zu, expected %zu\n", index,
            aff.replacements().size());
    return false;
  }
  return true;
}

bool VerifyWords(const convert_dict::AffReader& aff,
                 const convert_dict::DicReader::WordList& words,
                 const std::string& serialized) {
  if (!hunspell::BDict::Verify(base::as_byte_span(serialized))) {
    fprintf(stderr, "bdic header or checksum is invalid\n");
    return false;
  }

  hunspell::BDictReader reader;
  if (!reader.Init(base::as_byte_span(serialized))) {
    fprintf(stderr, "bdic reader rejected the dictionary\n");
    return false;
  }
  if (!VerifyAff(aff, reader)) {
    return false;
  }

  hunspell::WordIterator iter = reader.GetAllWordIterator();
  std::array<int, hunspell::BDict::MAX_AFFIXES_PER_WORD> affix_ids;
  char buf[4096];

  for (size_t i = 0; i < words.size(); ++i) {
    const int affix_matches =
        iter.Advance(base::span<char>(buf, sizeof(buf)), affix_ids.data());
    if (affix_matches == 0) {
      fprintf(stderr, "dictionary ended early at word %zu\n", i);
      return false;
    }
    if (words[i].first != buf) {
      fprintf(stderr, "word %zu does not match\n", i);
      return false;
    }
    if (affix_matches != static_cast<int>(words[i].second.size())) {
      fprintf(stderr, "affix count does not match for word %zu\n", i);
      return false;
    }
    for (size_t affix = 0; affix < words[i].second.size(); ++affix) {
      if (affix_ids[affix] != words[i].second[affix]) {
        fprintf(stderr, "affix id does not match for word %zu\n", i);
        return false;
      }
    }
  }

  std::array<int, hunspell::BDict::MAX_AFFIXES_PER_WORD> extra_ids;
  char extra[4096];
  if (iter.Advance(base::span<char>(extra, sizeof(extra)), extra_ids.data()) !=
      0) {
    fprintf(stderr, "dictionary has more words than the source\n");
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: hunspell-to-bdic DICT_BASE OUTPUT.bdic\n");
    return 2;
  }

  const base::FilePath file_base(argv[1]);
  convert_dict::AffReader aff_reader(file_base.ReplaceExtension(".aff"));
  if (!aff_reader.Read()) {
    fprintf(stderr, "unable to read %s.aff\n", argv[1]);
    return 1;
  }

  convert_dict::DicReader dic_reader(file_base.ReplaceExtension(".dic"));
  if (!dic_reader.Read(&aff_reader)) {
    fprintf(stderr, "unable to read %s.dic\n", argv[1]);
    return 1;
  }

  hunspell::BDictWriter writer;
  writer.SetComment(aff_reader.comments());
  writer.SetAffixRules(aff_reader.affix_rules());
  writer.SetAffixGroups(aff_reader.GetAffixGroups());
  writer.SetReplacements(aff_reader.replacements());
  writer.SetOtherCommands(aff_reader.other_commands());
  writer.SetWords(dic_reader.words());

  const std::string serialized = writer.GetBDict();
  if (!VerifyWords(aff_reader, dic_reader.words(), serialized)) {
    fprintf(stderr, "conversion of %s did not round-trip\n", argv[1]);
    return 1;
  }

  FILE* out = fopen(argv[2], "wb");
  if (!out) {
    fprintf(stderr, "unable to write %s\n", argv[2]);
    return 1;
  }
  const size_t written = fwrite(serialized.data(), 1, serialized.size(), out);
  const int close_failed = fclose(out) != 0;
  if (written != serialized.size() || close_failed) {
    fprintf(stderr, "short write of %s\n", argv[2]);
    return 1;
  }
  return 0;
}
