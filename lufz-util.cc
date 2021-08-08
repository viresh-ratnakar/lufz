#include "lufz-util.h"

#include <algorithm>
#include <string>
#include <unordered_map>

#include <math.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

namespace lufz {

bool AllWild(const string& s) {
  for (char c : s) {
    if (c != '?') return false;
  }
  return true;
}

string Normalize(const string& s) {
  string out;
  for (char c : s) {
    char lc = tolower(c);
    if (lc >= 'a' && lc <= 'z') {
      out += c;
    } else if ((c == ' ' || c == '\t') &&
               (out.size() > 0 && out.back() != ' ')) {
      out += ' ';
    } else if (c == '-' || c == '\'') {
      out += c;
    } else if ((c == ',' || c == '!' || c == '?' || c == '.') &&
               (out.size() > 0 && out.back() != ' ')) {
      out += ' ';
    }
  }
  while (out.size() > 0 && out.back() == ' ') {
    out.pop_back();
  }
  return out;
}

string LowerCase(const string& s) {
  string out;
  for (char c : s) {
    out += tolower(c);
  }
  return out;
}

string Key(const string& s) {
  string key;
  int i = 0;
  for (char c : s) {
    c = tolower(c);
    if (c >= 'a' && c <= 'z') {
      key += (i < WILDIZE_ALL_BEYOND) ? c : '?';
      i++;
    }
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

int JavaHash(const string& key) {
  int hash = 0;
  for (char c : key) {
    hash = ((hash << 5) - hash) + c;
  }
  return hash;
}

int IndexShard(const string& key, int num_shards) {
  int shard = JavaHash(key) % num_shards;
  if (shard < 0) {
    shard += num_shards;
  }
  return shard;
}

string AgmKey(const string& s) {
  vector<char> chars;
  for (char c : s) {
    c = tolower(c);
    if (c >= 'a' && c <= 'z') {
      chars.push_back(c);
    }
  }
  sort(chars.begin(), chars.end());
  string out;
  for (char c : chars) {
    out += c;
  }
  return out;
}

bool ReadLexicon(const char* lexicon_file, vector<PhraseInfo>* lexicon) {
  lexicon->clear();
  FILE* fp = !strcmp(lexicon_file, "-") ? stdin : fopen(lexicon_file, "r");
  if (!fp) {
    fprintf(stderr, "Could not open %s\n", lexicon_file);
    return false;
  }
  char buf[MAX_LINE_LENGTH];
  int num_importances_found = 0;
  while (fgets(buf, sizeof(buf), fp)) {
    PhraseInfo phrase_info;
    char *buf_beyond_number = NULL;
    phrase_info.importance = strtold(buf, &buf_beyond_number);
    if (buf_beyond_number != buf) {
      if (isnan(phrase_info.importance) ||
          isinf(phrase_info.importance)) {
        phrase_info.importance = 0;
        buf_beyond_number = buf;
      } else {
        ++num_importances_found;
      }
    }
    // If there is no preceding number, importance gets set to 0.
    phrase_info.phrase = Normalize(string(buf_beyond_number));
    lexicon->push_back(phrase_info);
  }
  fclose(fp);

  fprintf(stderr, "Read lexicon of size  %d: found %d importances\n",
          lexicon->size(), num_importances_found);
  if (num_importances_found > 0) {
    if (num_importances_found != lexicon->size()) {
      fprintf(stderr,
              "Only %d entries had an importance value, "
              "out of %d. Need all or none\n",
              num_importances_found, lexicon->size());
      return false;
    }
    sort(lexicon->begin(), lexicon->end(),
         [](const PhraseInfo& a, const PhraseInfo& b) -> bool {
           return a.importance > b.importance;
         });
  }
  return true;
}

bool AddPronunciations(const char* phones_file, vector<PhraseInfo>* lexicon) {
  if (!lexicon) {
    fprintf(stderr, "Null lexicon passed");
    return false;
  }
  FILE* fp = fopen(phones_file, "r");
  if (!fp) {
    fprintf(stderr, "Could not open %s\n", phones_file);
    return false;
  }

  unordered_map<string, vector<int>> lexicon_index;
  for (int i = 0; i < lexicon->size(); i++) {
    string key = LowerCase(lexicon->at(i).phrase);
    lexicon_index[key].push_back(i);
  }

  char buf[MAX_LINE_LENGTH];
  int num_pronunciations_used = 0;
  int num_pronunciations_total = 0;
  double total_phone_len = 0;
  int max_phone_len = 0;
  while (fgets(buf, sizeof(buf), fp)) {
    ++num_pronunciations_total;
    string line(buf);
    size_t sep = line.find("  ");
    if (sep == string::npos) {
      fprintf(stderr, "Missing 2-space-separator in line: %s\n", buf);
      continue;
    }
    string phrase_key = LowerCase(Normalize(line.substr(0, sep)));
    if (phrase_key.empty() || phrase_key[0] < 'a' || phrase_key[0] > 'z') {
      // Skip comment line or weird-phrase line.
      continue;
    }
    // Multiple entries for a phrase have something like "(1)" at the end.
    size_t paren = phrase_key.find('(');
    if (paren != string::npos) {
      phrase_key = phrase_key.substr(0, paren);
    }
    if (lexicon_index.find(phrase_key) == lexicon_index.end()) {
      continue;
    }
    string phone = Normalize(line.substr(sep + 2));
    string unstressed_phone;
    for (char c : phone) {
      if (c >= '0' && c <= '9') continue;
      unstressed_phone += c;
    }
    if (unstressed_phone.empty()) {
      fprintf(stderr, "Empty pronunciation: %s\n", buf);
      continue;
    }
    if (num_pronunciations_used % 100 == 0) {
      fprintf(stderr, "Added pronunciation [%s] for %s\n",
          unstressed_phone.c_str(),
          (*lexicon)[lexicon_index[phrase_key][0]].phrase.c_str());
    }
    ++num_pronunciations_used;
    total_phone_len += unstressed_phone.length();
    if (unstressed_phone.length() > max_phone_len) {
      max_phone_len = unstressed_phone.length();
    }
    const vector<int>& lexicon_slots = lexicon_index[phrase_key];
    for (int i : lexicon_slots) {
      (*lexicon)[i].phones.push_back(unstressed_phone);
    }
  }
  fclose(fp);

  fprintf(stderr, "Read pronunciations file, used %d out of %d\n",
          num_pronunciations_used, num_pronunciations_total);
  fprintf(stderr, "Max phone len = %d, avg = %3.2f\n",
          max_phone_len, total_phone_len / num_pronunciations_used);
  return true;
}
}  // namespace lufz
