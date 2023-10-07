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

#include "lufz-utf8.h"
#include "lufz-util.h"

using namespace std;
using namespace lufz;

namespace {
void Print(const Lexicon& lexicon, int index) {
  if (index < 0 || index >= lexicon.phrase_infos.size()) {
    return;
  }
  const auto& phrase_info = lexicon.phrase_infos[index];
  printf("  Normalized at base_index %d: %s\n", phrase_info.base_index, phrase_info.normalized.c_str());
  printf("  Forms: ");
  for (const auto& s : phrase_info.forms) {
    printf("%s, ", s.c_str());
  }
  printf("\n");
}
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <Language> <lexicon_file>\n", argv[0]);
    return 2;
  }

  LufzUtil util(argv[1]);

  Lexicon lexicon;
  if (!util.ReadLexicon(argv[2], &lexicon)) {
    fprintf(stderr, "Could not read %s lexicon file %s\n", argv[1], argv[2]);
    return 2;
  }
  fprintf(stderr, "Read lexicon, have %d entries\n", lexicon.phrase_infos.size());

  char buf[MAX_LINE_LENGTH];
  for (;;) {
    printf("> ");
    fgets(buf, sizeof(buf), stdin);
    std::string line(buf);
    line = line.substr(0, line.length() - 1);  // Remove trailing \n
    int index = atoi(line.c_str());
    Print(lexicon, index);
  }
  return 0;
}

