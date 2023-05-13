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
std::vector<std::string> Split(
    const std::string& str,
    const std::string& delimiter) {
  std::vector<std::string> tokens;
  int dlen = delimiter.length();
  size_t start_pos = 0;
  size_t pos = -1;
  while ((start_pos < str.length()) &&
         ((pos = str.find(delimiter, start_pos)) >= 0)) {
    std::string token = str.substr(start_pos, pos - start_pos);
    if (!token.empty()) {
      tokens.push_back(token);
    }
    start_pos += (token.length() + dlen);
  }
  if (start_pos < str.length()) {
    std::string last_token = str.substr(start_pos);
    tokens.push_back(last_token);
  }
  return tokens;
}
std::string Join(
    const std::vector<std::string>& parts,
    const std::string& delimiter) {
  std::string joined_string;
  bool first = true;
  for (const auto& s : parts) {
    if (first) {
      first = false;
    } else {
      joined_string += delimiter;
    }
    joined_string += s;
  }
  return joined_string;
}
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
}
void Print(const std::vector<std::string>& parts) {
  printf("[");
  for (const auto& s : parts) {
    printf("%s", s.c_str());
  }
  printf("] = ");
  for (const auto& s : parts) {
    printf("[%s", s.c_str());
    PrintChars(s);
    printf("]", s.c_str());
  }
  printf("\n");
}
void PrintCharInfo(const std::string& chr) {
  if (lufz_utf8chars.count(chr) == 0) {
    printf("No UTF8Char entry for %s\n", chr.c_str());
    return;
  }
  const UTF8Char& info = lufz_utf8chars.at(chr);
  printf("UTF8Char[%s] = {\n", info.utf8char.c_str());
  printf("  script: %d,\n", info.script);
  printf("  root: %s,\n", info.root.c_str());
  printf("  letterness: %d,\n", info.letterness);
  printf("  typecase: %d,\n", info.typecase);
  printf("  latin_char: %s,\n", info.latin_char.c_str());
  printf("  lower: %s,\n", info.lower.c_str());
  printf("  upper: %s,\n", info.upper.c_str());
  printf("}\n");
}
}

int main(int argc, char* argv[]) {

  std::map<std::string, LufzUtil*> utils;
  for (const auto& pair : lufz_configs) {
    const std::string& config_name = pair.first;
    printf("Initializing LufzUtil(\"%s\")...\n", config_name.c_str());
    utils[config_name] = new LufzUtil(config_name);
  }

  char buf[MAX_LINE_LENGTH];
  for (;;) {
    printf("> ");
    fgets(buf, sizeof(buf), stdin);
    std::string line(buf);
    line = line.substr(0, line.length() - 1);  // Remove trailing \n
    std::vector<std::string> tokens = Split(line, " ");
    if (tokens.empty()) continue;
    if (tokens[0] == "quit") break;
    if (tokens[0] == "info") {
      if (tokens.size() < 2) continue;
      PrintCharInfo(tokens[1]);
      continue;
    }
    if (utils.count(tokens[0]) == 0) {
      printf("No config named %s\n", tokens[0].c_str());
      continue;
    }
    LufzUtil* util = utils[tokens[0]];
    tokens.erase(tokens.begin());
    std::string argument = Join(tokens, " ");

    if (argument.substr(0, 7) == "lexkey ") {
      printf("%s\n", util->Key(argument.substr(7)).c_str());
      continue;
    }
    if (argument.substr(0, 7) == "agmkey ") {
      std::string akey = util->AgmKey(argument.substr(7));
      printf("%s [shard:%d]\n", akey.c_str(), util->IndexShard(akey, AGM_INDEX_SHARDS));
      continue;
    }

    printf(">> UTF8 Chars:\n");
    PrintChars(argument);
    printf("\n");

    std::vector<std::string> raw_parts = util->PartsOf(argument, false);
    printf(">> Raw Parts:\n");
    Print(raw_parts);

    std::vector<std::string> letterized_pruned, pruned, parts;
    std::vector<std::string> letters = util->LettersOf(argument, &parts, &pruned, &letterized_pruned);
    printf(">> Parts:\n");
    Print(parts);
    printf(">> Pruned Parts:\n");
    Print(pruned);
    printf(">> Letterized Pruned Parts:\n");
    Print(letterized_pruned);
    printf(">> Letters:\n");
    Print(letters);
  }
  return 0;
}

