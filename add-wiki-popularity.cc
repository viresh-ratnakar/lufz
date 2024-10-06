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
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <Language> <lexicon-file>\n", argv[0]);
    return 1;
  }
  LufzUtil lufz_util(argv[1]);

  Lexicon lexicon;
  if (!lufz_util.ReadLexicon(argv[2], &lexicon)) {
    return 1;
  }

  map<string, int> lexicon_index;
  for (int i = 0; i < lexicon.phrase_infos.size(); ++i) {
    PhraseInfo& phrase_info = lexicon.phrase_infos[i];
    lexicon_index[phrase_info.normalized] = i;
    phrase_info.importance = 1;
  }

  const int NGRAM_LIMIT = 6;
  const int BREAK_AFTER_LINES = 30000000;

  int64_t num_lines = 0;
  int64_t num_doc_lines = 0;
  int64_t num_probes = 0;
  int64_t num_hits = 0;
  char buf[MAX_LINE_LENGTH];
  while (fgets(buf, sizeof(buf), stdin)) {
    ++num_lines;
    if (num_lines > BREAK_AFTER_LINES) {
      fprintf(stderr, "Enough lines read, quitting after reading %lld lines (%lld doc lines)...\n",
              num_lines, num_doc_lines);
      break;
    }
    if (!strncmp(buf, "<doc", 4) || !strncmp(buf, "</doc", 5)) {
      ++num_doc_lines;
      continue;
    }
    string wikiline = lufz_util.StrLetterizedPrunedPartsOf(string(buf));
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
        const auto& found = lexicon_index.find(ngram);
        if (found != lexicon_index.end()) {
          lexicon.phrase_infos[found->second].importance += 1;
          ++num_hits;
        }
      }
    }
    if (num_lines % 100000 == 0) {
      fprintf(stderr, "After reading %lld lines (%lld doc lines)...\n",
              num_lines, num_doc_lines);
      fprintf(stderr, "#probes: %lld #hits: %lld...\n", num_probes, num_hits);
      int samples = 20;
      int step = (lexicon.phrase_infos.size() / samples) - 1;
      for (int i = 0; i < 20; ++i) {
        int idx = (step * i + 42) % lexicon.phrase_infos.size();
        fprintf(stderr, "%llf %s\n", lexicon.phrase_infos[idx].importance,
                lexicon.phrase_infos[idx].normalized.c_str());
      }
    }
  }

  sort(lexicon.phrase_infos.begin() + 1, lexicon.phrase_infos.end(),
       [](const PhraseInfo& a, const PhraseInfo& b) -> bool {
         return a.importance > b.importance;
       });
  if (lexicon.phrase_infos.size() > 1) {
    lexicon.phrase_infos[0].importance = std::max(
        lexicon.phrase_infos[0].importance,
        lexicon.phrase_infos[1].importance + 1);
  }
  int base_index = 0;
  for (int i = 0; i < lexicon.phrase_infos.size(); i++) {
    lexicon.phrase_infos[i].base_index = base_index;
    base_index += lexicon.phrase_infos[i].forms.size();
  }

  for (const auto& phrase_info : lexicon.phrase_infos) {
    for (const auto& form : phrase_info.forms) {
      printf("%.1llf\t%s\n", phrase_info.importance, form.c_str());
    }
  }
  return 0;
}

