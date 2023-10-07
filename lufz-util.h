#ifndef LUFZ_H_
#define LUFZ_H_

#include <stdlib.h>

#include <set>
#include <string>
#include <vector>

#include "lufz-configs.h"
#include "lufz-utf8.h"

namespace lufz {

const size_t MAX_LINE_LENGTH = 65536;
const int WILDIZE_ALL_BEYOND = 10;
const int MAX_ENTRY_LENGTH = 30;
const std::string VERSION = "v0.07";

const int AGM_INDEX_SHARDS = 2000;
const int INDEX_SHARDS = 2000;
const int PHONE_INDEX_SHARDS = 2000;


struct PhraseInfo {
  std::string normalized;  // Normalized version (letters and spaces only).
  /**
   * Ordered index for forms[0]. 0 is always the dummy empty string.
   */
  int base_index;
  /**
   * Case/punctuation variants.
   * Each entry only uses letters from the lexicon (can be in
   * lower/upper case though—the letters listed in Lexicon.letters[]
   * are only uppercase), spaces, and allowed punctuation
   * marks (typically just ['-]).
   */
  std::set<std::string> forms;
  long double importance;
  /**
   * phones[k] is the kth possible pronunciation for a form.
   * It is a vector of phonemes (IPA or ARPAbet).
   */
  std::set<std::vector<std::string>> phones;
  PhraseInfo() : base_index(-1), importance(0) {}
};

struct Lexicon {
  std::vector<PhraseInfo> phrase_infos;

  /**
   * Only the letters used in the lexicon. All uppoercase.
   */
  std::set<std::string> letters;

  /**
   * The rest of the fields are copied from the config, and
   * may include things that do not occur in the lexicon at all.
   */
  std::vector<std::string> vowels;
  std::vector<std::string> consonants;
  std::set<std::string> combiners;
  std::set<std::string> punctuations;
  std::set<std::string> spaces;
  std::map<std::string, std::string> conversions;
};

class LufzUtil {
 public:
  /**
   * config_name should be a key for one of the predefined configs in
   * lufz-configs.h. Currently supported values (case-sensitive):
   *   "Brazilian"
   *   "English"
   *   "French"
   *   "German"
   *   "Hindi"
   *   "Italian"
   *   "Marathi"
   *   "Phonetics"
   *   "Portuguese"
   *   "Spanish"
   */
  explicit LufzUtil(const std::string& config_name);

  const std::string& Language() {
    return language_;
  }
  const std::string& Script() {
    return script_;
  }
  const std::string& ConfigName() {
    return config_->name;
  }

  std::string Join(const std::vector<std::string>& v, const std::string& delimiter="");
  std::vector<std::string> Split(const std::string& str, const std::string& delimiter="");
  bool EndsWith(const std::string& str, const std::string& suffix);

  /**
   * Returns a vector of "parts" of the string. A part is something that
   * is a letter (or a case-changed variant of the letter), or punctuation,
   * or a space, or a UTF8Char. it does the following preprocessing:
   * - Applies Lexicon.conversions.
   * - Maps spaces (i.e., converts each kind of space listed in Lexicon.spaces
   *   into a regular space unless map_spaces is false;
   * - Removes leading/trailing spaces, combines consective spaces.
   * - If Lexicon.script is LATIN, and if a string is not a letter but it
   *   can be converted to a letter by removing diacritics, then it does so.
   * Eg, for English:
   *   "[Alpha-x 4\t\tD'e " ->
   *   {"[", "A", "l", "p", "h", "a", "-", "x", " ", "4", " ", "D", "'", "e"}
   */
  std::vector<std::string> PartsOf(const std::string& s, bool maps_spaces = true);

  /**
   * Takes the result of PartsOf() and removes entries that are not letters
   * (ignoring case), not spaces, and not in Lexicon.punctuations.
   * Also returns the results of PartsOf() in *parts_of, if not null.
   * Eg, for English:
   *   "[Alpha-x 4\t\tD'e " ->
   *   {"A", "l", "p", "h", "a", "-", "x", " ", "D", "'", "e"}
   */
  std::vector<std::string> PrunedPartsOf(
      const std::string& s,
      std::vector<std::string>* parts_of = nullptr);

  /**
   * Takes the result of PrunedPartsOf() and converts all letter-convertible
   * entries to letters (changing case if needed), and replaces punctuations
   * with spaces, thus returning letters and spaces only. Trims consecutive
   * spaces and leading/trailing spaces.
   * Also returns the results of PartsOf() in *parts_of, and PrunedPartsOf()
   * in *pruned_parts_of, if not null.
   * Eg, for English:
   *   "[Alpha-x 4\t\tD'e " ->
   *   {"A", "L", "P", "H", "A", " ", "X", " ", "D", " ", "E"}
   */
  std::vector<std::string> LetterizedPrunedPartsOf(
      const std::string& s,
      std::vector<std::string>* parts_of = nullptr,
      std::vector<std::string>* pruned_parts_of = nullptr);

  /**
   * Takes the results of LetterizedPrunedPartsOf and retains
   * only the letters.
   * Also returns the results of PartsOf() in *parts_of, and PrunedPartsOf()
   * in *pruned_parts_of, and LetterizedPrunedPartsOf in
   * *letterized_pruned_parts_of, if not null.
   * Eg, for English:
   *   "[Alpha-x 4\t\tD'e " ->
   *   {"A", "L", "P", "H", "A", "X", "D", "E"}
   */
  std::vector<std::string> LettersOf(
      const std::string& s,
      std::vector<std::string>* parts_of = nullptr,
      std::vector<std::string>* pruned_parts_of = nullptr,
      std::vector<std::string>* letterized_pruned_parts_of = nullptr);

  /**
   * Returns the joined output of PartsOf().
   */
  std::string StrPartsOf(const std::string& s) {
    return Join(PartsOf(s));
  }

  /**
   * Returns the joined output of PrunedPartsOf().
   */
  std::string StrPrunedPartsOf(const std::string& s) {
    return Join(PrunedPartsOf(s));
  }

  /**
   * Returns the joined output of LetterizedPrunedPartsOf().
   */
  std::string StrLetterizedPrunedPartsOf(const std::string& s) {
    return Join(LetterizedPrunedPartsOf(s));
  }

  /**
   * Returns the joined output of LettersOf().
   */
  std::string StrLettersOf(const std::string& s) {
    return Join(LettersOf(s));
  }

  /**
   * Returns true if s is in Lexicon.letters, ignoring case. If "letter"
   * is not null, then sets *letter to the actual matched letter when
   * returning true.
   */
  bool IsLetter(const std::string& s, std::string* letter = nullptr);

  /**
   * Return true only for punctuation allowed in Lexicon.punctuations.
   */
  bool IsPunctuation(const std::string& s);

  /**
   * Returns true if PartsOf() yields all "?"s only.
   */
  bool AllWild(const std::string& s);

  /**
   * Return indexing key for s.
   */
  std::string Key(const std::string& s);

  /**
   * Return indexing anagram key for s.
   */
  std::string AgmKey(const std::string& s);

  /**
   * A hash value of the string. We use Java's
   * hashing algo here (and also in downstream JavaScript code).
   */
  static int JavaHash(const std::string& key);

  /**
   * The shard that key belongs to.
   */
  static int IndexShard(const std::string& key, int num_shards);

  /**
   * Pass "-" as the file name for stdin.
   * formats accepted:
   *   <entry>
   * OR (cannot mix the two formats).
   *   <importance>\t<entry>
   * The 0th entry created is always the empty phrase.
   * All entries that map to the same StrLetterizedPrunedPartsof() are
   * combined (but retained as different "forms" of each other. When
   * combining like this, we take the max of importance scores (if any).
   */
  bool ReadLexicon(const char* lexicon_file, Lexicon* lexicon, const char* crossed_words_file = nullptr);

 private:
  /**
   * IsLetter() variant that works on a potential letter that has
   * already been split into its UTF8Chars and uppercased.
   */
  bool IsLetter(const std::vector<std::string>& chars);

  const LufzConfig* config_;
  std::string language_;
  std::string script_;
  std::vector<std::string> letters_;
  std::map<std::string, int> letter_indices_;
};

}  // namespace lufz

#endif  // LUFZ_H_
