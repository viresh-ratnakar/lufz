#include "lufz-util.h"

#include <algorithm>
#include <string>

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

bool ReadLexicon(const char* lexicon_file,
                 vector<PhraseAndImportance>* lexicon) {
  lexicon->clear();
  FILE* fp = !strcmp(lexicon_file, "-") ? stdin : fopen(lexicon_file, "r");
  if (!fp) {
    fprintf(stderr, "Could not open %s\n", lexicon_file);
    return false;
  }
  char buf[MAX_LINE_LENGTH];
  int num_importances_found = 0;
  while (fgets(buf, sizeof(buf), fp)) {
    PhraseAndImportance phrase_and_importance;
    char *buf_beyond_number = NULL;
    phrase_and_importance.importance = strtold(buf, &buf_beyond_number);
    if (buf_beyond_number != buf) {
      if (isnan(phrase_and_importance.importance) ||
          isinf(phrase_and_importance.importance)) {
        phrase_and_importance.importance = 0;
        buf_beyond_number = buf;
      } else {
        ++num_importances_found;
      }
    }
    // If there is no preceding number, importance gets set to 0.
    phrase_and_importance.phrase = Normalize(string(buf_beyond_number));
    lexicon->push_back(phrase_and_importance);
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
         [](const PhraseAndImportance& a, const PhraseAndImportance& b) -> bool {
           return a.importance > b.importance;
         });
  }
  return true;
}

}  // namespace lufz
