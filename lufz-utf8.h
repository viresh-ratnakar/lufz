#ifndef LUFZ_UTF8_H_
#define LUFZ_UTF8_H_

/**
 * This file defines Language/Script/UTF8-related constants. The star item is
 * the vector lufz_utf8chars that lists all UTF8 chars that are letters or
 * components of letters.
 *
 * Before using this vector, the function LufzUTF8Init() should be called. It's
 * OK to call this initialization function multiple times.
 */

#include <map>
#include <string>
#include <vector>

namespace lufz {

/**
 * An enumeration of supported scripts.
 */
#define ALL 0
#define LATIN 1
#define CYRILLIC 2
#define GREEK 3
#define DEVANAGARI 4
#define BENGALI 5
#define GUJARATI 6
#define GURMUKHI 7
#define KANNADA 8
#define MALAYALAM 9
#define SINHALA 10
#define TAMIL 11
#define TELUGU 12
#define ORIYA 13

/**
 * Names of supported scripts.
 */
const std::map<int, std::string> lufz_scripts = {
  {ALL, "All"},
  {LATIN, "Latin"},
  {CYRILLIC, "Cyrillic"},
  {GREEK, "Greek"},
  {DEVANAGARI, "Devanagari"},
  {BENGALI, "Bengali"},
  {GUJARATI, "Gujarati"},
  {GURMUKHI, "Gurmukhi"},
  {KANNADA, "Kannada"},
  {MALAYALAM, "Malayalam"},
  {SINHALA, "Sinhala"},
  {TAMIL, "Tamil"},
  {TELUGU, "Telugu"},
  {ORIYA, "Oriya"},
};

/**
 * Letter/non-letter (used in UTF8Char.letterness.
 */
#define NOT_LETTER 0
#define LETTER 1

/**
 * Letter type case (used in UTF8Char.typecase.
 */
#define NO_CASE 0
#define SMALL 1
#define CAPITAL 2

typedef struct UTF8CharStruct {
  std::string utf8char;
  int script;
  std::string root; /** common name in both upper/lower-case variants. */
  int letterness;
  int typecase;
  std::string latin_char;  /** If empty, set to utf8char in LufzUTF8Init() */
  std::string lower;  /** Set in LufzUTF8Init() */
  std::string upper;  /** Set in LufzUTF8Init() */
} UTF8Char;

/**
 * Initialization. Sets some computed fields in lufz_utf8chars[].
 */
void LufzUTF8Init();

/**
 * Given a string, split it into its constituents from lufz_utf8chars[].
 */
std::vector<std::string> LufzUTF8Chars(const std::string& s);

/**
 * The max length (in bytes) of any enry in lufz_utf8chars[].
 * Computed in LufzUTF8Init().
 */
extern int lufz_utf8chars_maxlen;

/**
 * A lookup table for a utf8char to its UTF8Char struct.
 * Computed in LufzUTF8Init().
 */
extern std::map<std::string, UTF8Char> lufz_utf8chars;

}  // namespace lufz

#endif  // LUFZ_UTF8_H_
