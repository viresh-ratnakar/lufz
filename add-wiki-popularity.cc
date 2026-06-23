#include <algorithm>
#include <map>
#include <string>
#include <utility>
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

/**
 * If the lexicon had no non-zero importances, then the output shows
 * the measured occurrence counts as the importance values.
 *
 * If there already were some non-zero importance values in the lexicon,
 * then they are treated as the primary key for sorting. The secondary
 * sorting key is the importance count. The adjusted importance value
 * is computed by adding to it a delta, that is obtained by splitting
 * the range from an original importance value to the 1 more than it
 * (or the next bigger importance value, is it is closer than 1) into
 * as many parts as lie in that range. Say, at original importance = 55,
 * there are 10,000 values. They will be sorted by occurrence count,
 * and the kth highest (k=1...10,000) occurring one will get an importance
 * score of:
 *   55 + 1.0*((10,000-k)/10,000)
 */
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
  if (lexicon.phrase_infos.size() <= 1) {
    fprintf(stderr, "Empty lexicon, quitting.");
    return 1;
  }

  map<string, int> lexicon_index;
  bool have_existing_importances = false;
  for (int i = 0; i < lexicon.phrase_infos.size(); ++i) {
    PhraseInfo& phrase_info = lexicon.phrase_infos[i];
    lexicon_index[phrase_info.normalized] = i;
    phrase_info.occurrence_count = 1;
    if (phrase_info.importance > 0) {
      have_existing_importances = true;
    }
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
          lexicon.phrase_infos[found->second].occurrence_count += 1;
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
        fprintf(stderr, "%llf %s\n", lexicon.phrase_infos[idx].occurrence_count,
                lexicon.phrase_infos[idx].normalized.c_str());
      }
    }
  }

  sort(lexicon.phrase_infos.begin() + 1, lexicon.phrase_infos.end(),
       [](const PhraseInfo& a, const PhraseInfo& b) -> bool {
         if (a.importance == b.importance) {
           return a.occurrence_count > b.occurrence_count;
         }
         return a.importance > b.importance;
       });
  lexicon.phrase_infos[0].importance = max(
      lexicon.phrase_infos[0].importance,
      lexicon.phrase_infos[1].importance + 1);

  if (have_existing_importances) {
    vector<pair<int, int>> equi_important_ranges;
    equi_important_ranges.push_back(make_pair(0, 1));
    int start = 1;
    for (int i = 2; i < lexicon.phrase_infos.size(); i++) {
      const PhraseInfo& pi = lexicon.phrase_infos[i];
      if (pi.importance != lexicon.phrase_infos[start].importance) {
        equi_important_ranges.push_back(make_pair(start, i));
        start = i;
      }
    }
    equi_important_ranges.push_back(
        make_pair(start, lexicon.phrase_infos.size()));

    for (int r = 1; r < equi_important_ranges.size(); r++) {
      const auto& range = equi_important_ranges[r];
      const int r_start = range.first;
      const int r_end = range.second;
      const int r_num = r_end - r_start;
      long double imp = lexicon.phrase_infos[r_start].importance;
      const auto& last_range = equi_important_ranges[r-1];
      const int last_r_start = last_range.first;
      long double last_imp = lexicon.phrase_infos[last_r_start].importance;
      long double delta = min(1.0L, last_imp - imp);
      long double incr = delta / r_num;
      for (int i = 0; i < r_num; i++) {
        lexicon.phrase_infos[r_start + i].importance += (incr * (r_num - i - 1));
      }
    }
  }

  int base_index = 0;
  for (int i = 0; i < lexicon.phrase_infos.size(); i++) {
    lexicon.phrase_infos[i].base_index = base_index;
    base_index += lexicon.phrase_infos[i].forms.size();
  }

  for (const auto& phrase_info : lexicon.phrase_infos) {
    for (const auto& form : phrase_info.forms) {
      printf("%llf\t%s\n", phrase_info.importance, form.c_str());
    }
  }
  return 0;
}

