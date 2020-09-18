#include <string>
#include <vector>
#include <stdlib.h>

namespace lufz {

static const size_t MAX_LINE_LENGTH = 65536;
static const int WILDIZE_ALL_BEYOND = 10;

bool AllWild(const std::string& s);

// Retain only [a-zA-Z], spaces (trimmed), and the two characters [-']
// from s. Insert spaces instead of [,!?.] (trimming consecutive spaces
// that might get created). Delete all other characters // from s.
// Return the resulting string.
std::string Normalize(const std::string& s);

// Return lowercased s.
std::string LowerCase(const std::string& s);

// Return indexing key for s.
std::string Key(const std::string& s);

// Return indexing anagram key for s.
std::string AgmKey(const std::string& s);

int JavaHash(const std::string& key);
int IndexShard(const std::string& key, int num_shards);

struct PhraseAndImportance {
  std::string phrase;
  long double importance;
  PhraseAndImportance() : importance(0) {}
};

// Pass "-" as the file name for stdin.
bool ReadLexicon(const char* lexicon_file,
                 std::vector<PhraseAndImportance>* lexicon);

}  // namespace lufz
