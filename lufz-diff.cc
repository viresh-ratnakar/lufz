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

void ExtractMap(Lexicon* lexicon, map<string, int>* lmap) {
  for (int i = 1; i < lexicon->phrase_infos.size(); ++i) {
    const PhraseInfo& phrase_info = lexicon->phrase_infos[i];
    (*lmap)[phrase_info.normalized] = i;
  }
}

int PrintDiffForms(const set<string>& forms1, const set<string>& forms2, const string& prefix) {
  int count = 0;
  for (const string& s : forms1) {
    if (forms2.count(s) == 0) {
      printf("%s%s\n", prefix.c_str(), s.c_str());
      ++count;
    }
  }
  return count;
}

void PrintDiff(Lexicon* la, Lexicon* lb, map<string, int>* mapb, const string& only_a_prefix, const string& only_a_form_prefix) {
  int common = 0;
  int only_in_a = 0;
  for (int i = 1; i < la->phrase_infos.size(); ++i) {
    const PhraseInfo& phrase_info = la->phrase_infos[i];
    if (mapb->count(phrase_info.normalized) > 0) {
      const PhraseInfo& pb = lb->phrase_infos[(*mapb)[phrase_info.normalized]];
      int n = PrintDiffForms(phrase_info.forms, pb.forms, only_a_form_prefix);
      only_in_a += n;
      common += (phrase_info.forms.size() - n);
    } else {
      for (const auto& form : phrase_info.forms) {
        printf("%s%s\n", only_a_prefix.c_str(), form.c_str());
      }
      only_in_a += phrase_info.forms.size();
    }
  }
  fprintf(stderr, "== Common: %d, Only in the first one: %d\n", common, only_in_a);
}

int main(int argc, char* argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <Language> <lexicon-file-1> <lexicon-file-2>\n", argv[0]);
    return 1;
  }
  LufzUtil lufz_util(argv[1], 100 /* max_entry_length */);

  Lexicon lexicon1, lexicon2;
  if (!lufz_util.ReadLexicon(argv[2], &lexicon1, nullptr, true /* normalize_without_spaces */)) {
    return 1;
  }
  if (lexicon1.phrase_infos.size() == 0 || lexicon1.phrase_infos[0].normalized != "") {
    fprintf(stderr, "Lexicon1 did not create an empty first entry!");
    return 1;
  }
  if (!lufz_util.ReadLexicon(argv[3], &lexicon2, nullptr, true /* normalize_without_spaces */)) {
    return 1;
  }
  if (lexicon2.phrase_infos.size() == 0 || lexicon2.phrase_infos[0].normalized != "") {
    fprintf(stderr, "Lexicon2 did not create an empty first entry!");
    return 1;
  }
  map<string, int> map1, map2;
  ExtractMap(&lexicon1, &map1);
  ExtractMap(&lexicon2, &map2);

  PrintDiff(&lexicon1, &lexicon2, &map2, "[Only-in-1]", "[Only-in-1-in-this-form]");
  PrintDiff(&lexicon2, &lexicon1, &map1, "[Only-in-2]", "[Only-in-2-in-this-form]");

  return 0;
}

