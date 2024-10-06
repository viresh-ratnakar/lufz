#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <set>
#include <algorithm>
#include <string>
#include <unordered_map>

#include "lufz-utf8.h"
#include "lufz-util.h"

namespace lufz {

LufzUtil::LufzUtil(const std::string& config_name) {
  if (lufz_configs.count(config_name) == 0) {
    fprintf(stderr, "No config named %s\n", config_name.c_str());
    exit(0);
  }
  LufzUTF8Init();
  config_ = &lufz_configs.at(config_name);
  language_ = config_->language;
  script_ = lufz_scripts.at(config_->script);
  for (const std::string& v : config_->vowels) {
    letter_indices_[v] = letters_.size();
    letters_.push_back(v);
  }
  for (const std::string& c : config_->consonants) {
    letter_indices_[c] = letters_.size();
    letters_.push_back(c);
  }
}

std::string LufzUtil::Join(
    const std::vector<std::string>& v,
    const std::string& delimiter) {
  std::string result;
  bool first = true;
  for (const std::string& s : v) {
    if (!first) {
      result += delimiter;
    } else {
      first = false;
    }
    result += s;
  }
  return result;
}

std::vector<std::string> LufzUtil::Split(
    const std::string& str,
    const std::string& delimiter) {
  std::vector<std::string> tokens;
  int start = 0;
  int end = str.find(delimiter, start);
  while (end != std::string::npos) {
    tokens.push_back(str.substr(start, end - start));
    start = end + delimiter.length();
    end = str.find(delimiter, start);
  }
  tokens.push_back(str.substr(start));
  return tokens;
}

bool LufzUtil::EndsWith(const std::string& str, const std::string& suffix) {
  if (suffix.length() > str.length()) {
    return false;
  }
  int suffixIndex = str.length() - suffix.length();
  for (int i = 0; i < suffix.length(); i++) {
    if (str[suffixIndex + i] != suffix[i]) {
      return false;
    }
  }
  return true;
}

#define MAX_CHARS_IN_LETTER 8

std::vector<std::string> LufzUtil::PartsOf(const std::string& s, bool map_spaces) {
  /** Apply conversions */
  std::string s_converted;
  int i = 0;
  while (i < s.length()) {
    std::string from, to;
    for (const auto& a_to_b : config_->conversions) {
      const std::string& a = a_to_b.first;
      const std::string& b = a_to_b.second;
      if (s.substr(i, a.length()) == a) {
        from = a;
        to = b;
        break;
      }
    }
    if (from.length() > 0) {
      s_converted += to;
      i += from.length();
    } else {
      s_converted += s.substr(i, 1);
      i++;
    }
  }

  std::string s_converted_spaced;
  if (map_spaces) {
    /** Replace spaces */
    i = 0;
    while (i < s_converted.length()) {
      std::string from, to;
      for (const auto& sp : config_->spaces) {
        if (s_converted.substr(i, sp.length()) == sp) {
          from = sp;
          to = " ";
          break;
        }
      }
      if (from.length() > 0) {
        i += from.length();
      } else {
        to = s_converted.substr(i, 1);
        i++;
      }
      s_converted_spaced += to;
    }
  } else {
    s_converted_spaced = s_converted;
  }

  std::vector<std::string> chars = LufzUTF8Chars(s_converted_spaced);
  if (config_->script == LATIN) {
    /**
     * Try removing diacritics from chars that are not in the letter
     * set.
     */
    for (int j = 0; j < chars.size(); j++) {
      const std::string& chr = chars[j];
      if (IsLetter(chr)) {
        continue;
      }
      if (lufz_utf8chars.count(chr) > 0) {
        std::string mapped_char = lufz_utf8chars.at(chr).latin_char;
        if (IsLetter(mapped_char)) {
          chars[j] = mapped_char;
        }
      }
    }
  }

  std::vector<std::string> parts;
  i = 0;
  while (i < chars.size()) {
    std::string part;
    int num_chars = 0;
    for (int l = MAX_CHARS_IN_LETTER; l >= 1; l--) {
      if (i + l > chars.size()) continue;
      std::vector<std::string> part_chars;
      for (int j = i; j < i + l; j++) {
        part_chars.push_back(chars[j]);
      }
      if (IsLetter(part_chars)) {
        num_chars = l;
        part = Join(part_chars);
        break;
      }
    }
    if (num_chars == 0) {
      num_chars = 1;
      part = chars[i];
    }
    i += num_chars;
    /** Avoid leading and consecutive spaces */
    if (part == " " &&
        (parts.size() == 0 || parts[parts.size() - 1] == " ")) {
      continue;
    }
    parts.push_back(part);
  }
  /** Remove trailing spaces */
  while (parts.size() > 0 && parts[parts.size() - 1] == " ") {
    parts.pop_back();
  }
  return parts;
}

std::vector<std::string> LufzUtil::PrunedPartsOf(
    const std::string& s,
    std::vector<std::string>* parts_of) {
  std::vector<std::string> result;
  std::vector<std::string> parts;
  if (parts_of == nullptr) {
    parts_of = &parts;
  }
  *parts_of = PartsOf(s);
  for (const std::string& part : *parts_of) {
    if (IsLetter(part) || IsPunctuation(part) ||
        ((part == " ") &&
         result.size() > 0 && result[result.size() - 1] != " ")) {
      /** Note that we trim away spaces from the front and repeated spaces. */
      result.push_back(part);
    }
  }
  /** Remove trailing spaces */
  while (result.size() > 0 && result[result.size() - 1] == " ") {
    result.pop_back();
  }
  return result;
}

std::vector<std::string> LufzUtil::LetterizedPrunedPartsOf(
    const std::string& s,
    std::vector<std::string>* parts_of,
    std::vector<std::string>* pruned_parts_of) {
  std::vector<std::string> result;
  std::vector<std::string> parts;
  if (parts_of == nullptr) {
    parts_of = &parts;
  }
  std::vector<std::string> pruned_parts;
  if (pruned_parts_of == nullptr) {
    pruned_parts_of = &pruned_parts;
  }
  *pruned_parts_of = PrunedPartsOf(s, parts_of);
  for (const std::string& part : *pruned_parts_of) {
    std::string letter;
    if (IsLetter(part, &letter)) {
      result.push_back(letter);
    } else if (result.size() > 0 && result[result.size() - 1] != " ") {
      result.push_back(" ");
    }
  }
  /** Remove trailing spaces */
  while (result.size() > 0 && result[result.size() - 1] == " ") {
    result.pop_back();
  }
  return result;
}

std::vector<std::string> LufzUtil::LettersOf(
    const std::string& s,
    std::vector<std::string>* parts_of,
    std::vector<std::string>* pruned_parts_of,
    std::vector<std::string>* letterized_pruned_parts_of) {
  std::vector<std::string> result;
  std::vector<std::string> parts;
  if (parts_of == nullptr) {
    parts_of = &parts;
  }
  std::vector<std::string> pruned_parts;
  if (pruned_parts_of == nullptr) {
    pruned_parts_of = &pruned_parts;
  }
  std::vector<std::string> letterized_pruned_parts;
  if (letterized_pruned_parts_of == nullptr) {
    letterized_pruned_parts_of = &letterized_pruned_parts;
  }
  *letterized_pruned_parts_of =
      LetterizedPrunedPartsOf(s, parts_of, pruned_parts_of);
  for (const std::string& part : *letterized_pruned_parts_of) {
    if (IsLetter(part)) {
      result.push_back(part);
    }
  } 
  return result;
}

bool LufzUtil::IsLetter(const std::vector<std::string>& chars) {
  int num_combiners = 0;
  for (int i = chars.size() - 1; i >= 0; i--) {
    const std::string& chr = chars[i];
    if (config_->combiners.count(chr) == 0) {
      break;
    }
    num_combiners++;
  }
  std::string maybe_letter;
  for (int i = 0; i < chars.size() - num_combiners; i++) {
    maybe_letter += chars[i];
  }
  return (letter_indices_.count(maybe_letter) > 0);
}

bool LufzUtil::IsLetter(const std::string& s, std::string* letter) {
  std::string ignored_letter;
  if (!letter) {
    letter = &ignored_letter;
  }
  if (letter_indices_.count(s) > 0) {
    *letter = s;
    return true;
  }
  std::vector<std::string> chars = LufzUTF8Chars(s);
  for (int i = 0; i < chars.size(); i++) {
    const std::string& chr = chars[i];
    if (lufz_utf8chars.count(chr) > 0) {
      chars[i] = lufz_utf8chars.at(chr).upper;
    }
  }
  if (IsLetter(chars)) {
    *letter = Join(chars);
    return true;
  }
  letter->clear();
  return false;
}

bool LufzUtil::IsPunctuation(const std::string& s) {
  return config_->punctuations.count(s) > 0;
}

bool LufzUtil::AllWild(const std::string& s) {
  std::vector<std::string> parts = PartsOf(s, false);
  for (const auto& part : parts) {
    if (part != "?") return false;
  }
  return true;
}

std::string LufzUtil::Key(const std::string& s) {
  std::string key;
  std::vector<std::string> letters = LettersOf(s);
  for (int i = 0; i < letters.size(); i++) {
    key += (i < WILDIZE_ALL_BEYOND) ? letters[i] : "?";
  }
  return key;
}

// JavaScript for Java's hashCode:
/*
String.prototype.hashCode = function() {
    var hash = 0;
    if (this.length == 0) {
        return hash;
    }
    for (var i = 0; i < this.length; i++) {
        var char = this.charCodeAt(i);
        hash = ((hash<<5)-hash)+char;
        hash = hash & hash; // Convert to 32bit integer
    }
    return hash;
}
*/

int LufzUtil::JavaHash(const std::string& key) {
  int hash = 0;
  for (char c : key) {
    hash = ((hash << 5) - hash) + c;
  }
  return hash;
}

int LufzUtil::IndexShard(const std::string& key, int num_shards) {
  int shard = JavaHash(key) % num_shards;
  if (shard < 0) {
    shard += num_shards;
  }
  return shard;
}

std::string LufzUtil::AgmKey(const std::string& s) {
  std::vector<std::string> letters = LettersOf(s);
  sort(letters.begin(), letters.end());
  return(Join(letters));
}

bool LufzUtil::ReadLexicon(const char* lexicon_file, Lexicon* lexicon, const char* crossed_words_file) {
  lexicon->phrase_infos.clear();
  FILE* fp = !strcmp(lexicon_file, "-") ? stdin : fopen(lexicon_file, "r");
  if (!fp) {
    fprintf(stderr, "Could not open %s\n", lexicon_file);
    return false;
  }
  char buf[MAX_LINE_LENGTH];
  int num_importances_found = 0;
  int num_lines = 0;

  std::set<std::string> crossed_words;
  if (crossed_words_file && strlen(crossed_words_file) > 0) {
    FILE* cfp = fopen(crossed_words_file, "r");
    if (!cfp) {
      fprintf(stderr, "Could not open %s\n", crossed_words_file);
      return false;
    }
    while (fgets(buf, sizeof(buf), cfp)) {
      std::string crossed_word(buf);
      std::string normalized_crossed_word =
          StrLetterizedPrunedPartsOf(crossed_word);
      if (!normalized_crossed_word.empty()) {
        crossed_words.insert(normalized_crossed_word);
      }
    }
    fclose(cfp);
    fprintf(stderr, "Read %d crossed words from %s\n", crossed_words.size(), crossed_words_file);
  }

  PhraseInfo empty_string_entry;
  empty_string_entry.base_index = 0;
  empty_string_entry.forms = {""};
  lexicon->phrase_infos.push_back(empty_string_entry);

  std::unordered_map<std::string, int> lexicon_index;
  int most_forms = 0;
  int most_forms_index = 0;
  while (fgets(buf, sizeof(buf), fp)) {
    ++num_lines;
    buf[strcspn(buf, "\r\n")] = 0;  // Remove trailing newline
    std::string line(buf);
    std::vector<std::string> line_parts = Split(line, "\t");

    long double importance = 0;
    std::string phrase;

    if (line_parts.size() == 1) {
      phrase = line_parts[0];
    } else if (line_parts.size() == 2) {
      ++num_importances_found;
      phrase = line_parts[1];
      char *buf_beyond_number = NULL;
      importance = strtold(line_parts[0].c_str(), &buf_beyond_number);
      if (isnan(importance) || isinf(importance)) {
        fprintf(stderr, "Skipping [%s] as it has a weird importance score\n", buf);
        continue;
      }
    } else {
      fprintf(stderr, "Skipping [%s] as it has %d parts (need 1 or 2)\n", buf, line_parts.size());
      continue;
    }
    std::vector<std::string> parts;
    std::vector<std::string> pruned_parts;
    std::vector<std::string> letterized_pruned_parts;
    std::vector<std::string> letter_parts = LettersOf(
        phrase, &parts, &pruned_parts, &letterized_pruned_parts);
    if (parts.empty() || parts.size() != pruned_parts.size()) {
      fprintf(stderr, "Skipping [%s] as it has unrecognized parts\n",
              phrase.c_str());
      continue;
    }
    if (letter_parts.size() > MAX_ENTRY_LENGTH) {
      fprintf(stderr, "Skipping [%s] as it is longer than %d\n",
              phrase.c_str(), MAX_ENTRY_LENGTH);
      continue;
    }

    std::string normalized = Join(letterized_pruned_parts);
    if (crossed_words.count(normalized) > 0) {
      fprintf(stderr, "Skipping [%s] as it's listed in crossed_words\n",
              phrase.c_str());
      continue;
    }

    if (lexicon_index.count(normalized) == 0) {
      lexicon_index[normalized] = lexicon->phrase_infos.size();
      PhraseInfo new_phrase_info;
      new_phrase_info.normalized = normalized;
      new_phrase_info.importance = 0;
      lexicon->phrase_infos.push_back(new_phrase_info);
    }
    int index = lexicon_index.at(normalized);
    PhraseInfo* phrase_info = &lexicon->phrase_infos[index];
    phrase_info->forms.insert(Join(pruned_parts));
    if (phrase_info->forms.size() > most_forms) {
      most_forms = phrase_info->forms.size();
      most_forms_index = index;
    }
    phrase_info->importance = std::max(phrase_info->importance, importance);

    for (const auto& letter : letter_parts) {
      lexicon->letters.insert(letter);
    }
  }
  fclose(fp);

  fprintf(stderr, "Read lexicon of size  %d: found %d importances\n",
          lexicon->phrase_infos.size(), num_importances_found);
  fprintf(stderr, "Entry with most forms: [%s] at %d\n",
          lexicon->phrase_infos[most_forms_index].normalized.c_str(),
          most_forms_index);
  for (const std::string& form : lexicon->phrase_infos[most_forms_index].forms) {
    fprintf(stderr, "    %s\n", form.c_str());
  }
  if (num_importances_found > 0) {
    if (num_importances_found != num_lines) {
      fprintf(stderr,
              "Only %d lines had an importance value, "
              "out of %d. Need all or none\n",
              num_importances_found, num_lines);
      return false;
    }
    sort(lexicon->phrase_infos.begin() + 1, lexicon->phrase_infos.end(),
         [](const PhraseInfo& a, const PhraseInfo& b) -> bool {
           if (a.importance == b.importance) {
             return a.normalized.length() < b.normalized.length();
           } 
           return a.importance > b.importance;
         });
  }
  if (lexicon->phrase_infos.size() > 1) {
    lexicon->phrase_infos[0].importance = std::max(
        lexicon->phrase_infos[0].importance,
        lexicon->phrase_infos[1].importance + 1);
  }
  int base_index = 0;
  for (int i = 0; i < lexicon->phrase_infos.size(); i++) {
    lexicon->phrase_infos[i].base_index = base_index;
    base_index += lexicon->phrase_infos[i].forms.size();
  }

  lexicon->vowels = config_->vowels;
  lexicon->consonants = config_->consonants;
  lexicon->combiners = config_->combiners;
  lexicon->punctuations = config_->punctuations;
  lexicon->spaces = config_->spaces;
  lexicon->conversions = config_->conversions;

  return true;
}

}  // namespace lufz
