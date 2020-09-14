#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lufz-util.h"

using namespace std;
using namespace lufz;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <lexicon-file>\n", argv[0]);
    return 1;
  }
  vector<PhraseAndImportance> lexicon;
  if (!ReadLexicon(argv[1], &lexicon)) {
    return 1;
  }

  map<string, int> mapped_lexicon;
  for (int i = 0; i < lexicon.size(); ++i) {
    string lc_phrase = LowerCase(lexicon[i].phrase);
    const auto &old_entry = mapped_lexicon.find(lc_phrase);
    if (old_entry != mapped_lexicon.end()) {
      if (lexicon[i].phrase[0] != tolower(lexicon[i].phrase[0])) {
        fprintf(stderr, "%s ignored as an upper-cased dupe of lexicon entry %s!\n",
                lexicon[i].phrase.c_str(),
                lexicon[old_entry->second].phrase.c_str());
        // Prefer the other entry
        continue;
      } else {
        fprintf(stderr, "%s preferred as a lower-cased dupe of lexicon entry %s!\n",
                lexicon[i].phrase.c_str(),
                lexicon[old_entry->second].phrase.c_str());
      }
    }
    mapped_lexicon[lc_phrase] = i;
    lexicon[i].importance = 1;
  }

  const int NGRAM_LIMIT = 6;

  int64_t num_lines = 0;
  int64_t num_doc_lines = 0;
  int64_t num_probes = 0;
  int64_t num_hits = 0;
  char buf[MAX_LINE_LENGTH];
  while (fgets(buf, sizeof(buf), stdin)) {
    ++num_lines;
    if (!strncmp(buf, "<doc", 4) || !strncmp(buf, "</doc", 5)) {
      ++num_doc_lines;
      continue;
    }
    string wikiline = LowerCase(Normalize(string(buf)));
    vector<string> words;
    int start = 0;
    for (int i = 0; i < wikiline.length(); ++i) {
      if (wikiline[i] == ' ') {
        words.push_back(wikiline.substr(start, i - start));
        start = i + 1;
      }
    }
    if (start < wikiline.length()) {
      words.push_back(wikiline.substr(start));
    }
    for (int i = 0; i < words.size(); ++i) {
      for (int j = 1; j <= NGRAM_LIMIT; ++j) {
        if (i + j > words.size()) {
          continue;
        }
        string ngram = words[i];
        for (int k = 1; k < j; k++) {
          ngram += " " + words[i + k];
        }
        ++num_probes;
        const auto& found = mapped_lexicon.find(ngram);
        if (found != mapped_lexicon.end()) {
          lexicon[found->second].importance += 1;
          ++num_hits;
        }
      }
    }
    if (num_lines % 100000 == 0) {
      fprintf(stderr, "After reading %lld lines (%lld doc lines)...\n",
              num_lines, num_doc_lines);
      fprintf(stderr, "#probes: %lld #hits: %lld...\n", num_probes, num_hits);
      for (int i = 0; i < 10; ++i) {
        fprintf(stderr, "%llf %s\n", lexicon[1000*i].importance,
                lexicon[1000*i].phrase.c_str());
      }
    }
  }

  sort(lexicon.begin(), lexicon.end(),
       [](const PhraseAndImportance& a, const PhraseAndImportance& b) -> bool {
         return a.importance > b.importance;
       });
  for (int i = 0; i < lexicon.size(); ++i) {
    printf("%.1llf %s\n", lexicon[i].importance, lexicon[i].phrase.c_str());
  }
  return 0;
}

