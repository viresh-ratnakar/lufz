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

struct PhraseInfo {
  std::string phrase;
  long double importance;
  std::vector<std::string> phones;
  PhraseInfo() : importance(0) {}
};

// Pass "-" as the file name for stdin.
bool ReadLexicon(const char* lexicon_file, std::vector<PhraseInfo>* lexicon);

// Read a pronunciations file (such as the file at
// http://svn.code.sf.net/p/cmusphinx/code/trunk/cmudict/cmudict-0.7b) and add
// pronunciations (removing stress markers) to lexicon.
// Format: two spaces separate the phrase from its pronunciation.
// Example:
// BANANA  B AH0 N AE1 N AH0
bool AddPronunciations(const char* phones_file,
                       std::vector<PhraseInfo>* lexicon);

}  // namespace lufz
