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
void PrintChars(const std::string& s) {
  printf("{%s}", s.c_str());
  const std::vector<std::string> chars = LufzUTF8Chars(s);
  for (const auto& c : chars) {
    printf("(");
    for (unsigned char b : c) {
      printf("%02x", b);
    }
    printf(")");
  }
  printf("\n");
}
}  // namespace

/**
 * Reads stdin. Prints each char that it does not
 * recognize as a Phonetics symbol (IPA/ARPAbet).
 */
int main(int argc, char* argv[]) {
  LufzUtil util("Phonetics");

  char buf[MAX_LINE_LENGTH];
  while (fgets(buf, sizeof(buf), stdin)) {
    std::string line(buf);
    line = line.substr(0, line.length() - 1);  // Remove trailing \n
    std::vector<std::string> parts = util.PartsOf(line);
    int i = 0;
    while (i < parts.size()) {
      std::string part = parts[i];
      if (util.IsLetter(part)) {
        i++;
        continue;
      }
      int j = i + 1;
      while (j < parts.size() && !util.IsLetter(parts[j])) {
        part += parts[j];
        j++;
      }
      PrintChars(part);
      i = j;
    }
  }
  return 0;
}

