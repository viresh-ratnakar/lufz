#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <math.h>
#include <stdio.h>

#include "lufz-util.h"

using namespace std;

namespace lufz {

void AddKeyCounts(
    const string& normalized,
    int count,
    LufzUtil* util,
    map<string, int>* indexing_key_counts) {
  const string key = util->Key(normalized);
  const vector<string> key_parts = util->PartsOf(key, false);
  int len = key_parts.size();
  if (len > WILDIZE_ALL_BEYOND) {
    len = WILDIZE_ALL_BEYOND;
  }
  for (int pattern = 0; pattern < (1 << len); pattern++) {
    vector<string> key_variant_parts = key_parts;
    for (int i = 0; i < len; i++) {
      if (pattern & (1 << i)) {
        key_variant_parts[i] = "?";
      }
    }
    string key_variant = util->Join(key_variant_parts);
    (*indexing_key_counts)[key_variant] += count;
  }
}

void AddKeys(
    const string& normalized,
    const set<string>& indexing_keys,
    const vector<int>& lex_indices,
    LufzUtil* util,
    map<string, set<int>>* index) {
  const string key = util->Key(normalized);
  const vector<string> key_parts = util->PartsOf(key, false);
  int len = key_parts.size();
  if (len > WILDIZE_ALL_BEYOND) {
    len = WILDIZE_ALL_BEYOND;
  }
  for (int pattern = 0; pattern < (1 << len); pattern++) {
    vector<string> key_variant_parts = key_parts;
    for (int i = 0; i < len; i++) {
      if (pattern & (1 << i)) {
        key_variant_parts[i] = "?";
      }
    }
    string key_variant = util->Join(key_variant_parts);
    if (indexing_keys.count(key_variant) == 0) {
      continue;
    }
    for (int li : lex_indices) {
      if ((*index)[key_variant].count(li) > 0) {
        fprintf(stderr, "Hmm. For normalized=[%s], key=[%s], key_variant=[%s], we already have index %d\n",
                normalized.c_str(), key.c_str(), key_variant.c_str(), li);
      }
      (*index)[key_variant].insert(li);
    }
  }
}

void AddAgmKey(
    const string& normalized,
    const vector<int>& lex_indices,
    LufzUtil* util,
    vector<vector<int>>* agm_shards) {
  string key = util->AgmKey(normalized);
  int shard = util->IndexShard(key, AGM_INDEX_SHARDS);
  for (int li : lex_indices) {
    (*agm_shards)[shard].push_back(li);
  }
}

/**
 * Read a pronunciations file (such as the file derived from
 * http://svn.code.sf.net/p/cmusphinx/code/trunk/cmudict/cmudict-0.7b) and add
 * pronunciations (removing stress markers) to lexicon.
 * Format:
 * <entry>\t<phones>
 * Example:
 * BANANA\tB AH N AE N AH
 * The phones can be in ARPAbet format (as in CMUdict), or in IPA format.
 */
bool AddPronunciations(
    LufzUtil* util,
    LufzUtil* phone_util,
    const char* phones_file,
    Lexicon* lexicon) {
  if (!lexicon) {
    fprintf(stderr, "Null lexicon passed");
    return false;
  }
  FILE* fp = fopen(phones_file, "r");
  if (!fp) {
    fprintf(stderr, "Could not open %s\n", phones_file);
    return false;
  }
  fprintf(stderr, "Adding proninciations from %s\n", phones_file);

  unordered_map<string, int> lexicon_index;
  for (int i = 0; i < lexicon->phrase_infos.size(); i++) {
    const auto& phrase_info = lexicon->phrase_infos[i];
    lexicon_index[phrase_info.normalized] = i;
  }

  char buf[MAX_LINE_LENGTH];
  int num_pronunciations_used = 0;
  int num_pronunciations_total = 0;
  double total_phone_len = 0;
  int max_phone_len = 0;
  while (fgets(buf, sizeof(buf), fp)) {
    ++num_pronunciations_total;
    string line(buf);
    vector<string> line_parts = util->Split(line, "\t");
    if (line_parts.size() != 2) {
      fprintf(stderr, "Expect exactly one tab - ignoring line: %s\n", buf);
      continue;
    }
    string normalized = util->StrLetterizedPrunedPartsOf(line_parts[0]);
    if (normalized.empty()) {
      // Skip comment line or weird-phrase line.
      continue;
    }
    if (lexicon_index.find(normalized) == lexicon_index.end()) {
      continue;
    }
    vector<string> phone_parts = phone_util->LettersOf(line_parts[1]);
    if (phone_parts.empty()) {
      fprintf(stderr, "Empty pronunciation in: %s\n", buf);
      continue;
    }
    string phone = phone_util->Join(phone_parts);
    int index = lexicon_index.at(normalized);
    PhraseInfo& phrase_info = lexicon->phrase_infos[index];
    if (num_pronunciations_used % 100 == 0) {
      fprintf(stderr, "Added pronunciation [%s] for %s\n",
          phone.c_str(),
          normalized.c_str());
    }
    ++num_pronunciations_used;
    total_phone_len += phone_parts.size();
    if (phone_parts.size() > max_phone_len) {
      max_phone_len = phone_parts.size();
    }
    phrase_info.phones.insert(phone_parts);
  }
  fclose(fp);

  fprintf(stderr, "Read pronunciations file, used %d out of %d\n",
          num_pronunciations_used, num_pronunciations_total);
  fprintf(stderr, "Max phone len = %d, avg = %3.2f\n",
          max_phone_len, total_phone_len / num_pronunciations_used);
  return true;
}

}  // namespace lufz

int main(int argc, char* argv[]) {
  using namespace lufz;

  if (argc != 5) {
    fprintf(stderr, "Usage: %s <Language> <lexicon_file> <cmu-pronunciations-file> <crossed-words>\n",
            argv[0]);
    return 2;
  }

  LufzUtil util(argv[1]);
  LufzUtil phone_util("Phonetics");

  Lexicon lexicon;

  if (!util.ReadLexicon(argv[2], &lexicon, argv[4])) {
    return 2;
  }
  fprintf(stderr, "Read lexicon, have %d entries\n", lexicon.phrase_infos.size());

  if (!AddPronunciations(&util, &phone_util, argv[3], &lexicon)) {
    return 2;
  }

  /**
   * First, do a pass to decide which indexing keys to keep. This is
   * much faster than building the full index and then pruning
   * away keys that do not have many entries.
   */
  map<string, int> indexing_key_counts;
  fprintf(stderr, "Computing indexing_key_counts...\n");
  for (int i = 0; i < lexicon.phrase_infos.size(); ++i) {
    const PhraseInfo& phrase_info = lexicon.phrase_infos[i];
    const string& normalized = phrase_info.normalized;
    if (normalized.empty()) continue;
    int count = phrase_info.forms.size();
    AddKeyCounts(normalized, count, &util, &indexing_key_counts);
    if (i > 0 && i % 1000 == 0) {
      fprintf(stderr, "Indexing key counts at %d: %s\n", i, normalized.c_str());
    }
  }
  fprintf(stderr, "Pre-filtering, index has size: %d\n", indexing_key_counts.size());
  std::set<std::string> indexing_keys;
  // Filter
  const int MIN_COUNT = 1024;
  int ki = 0;
  for (const auto& [key, count] : indexing_key_counts) {
    if (ki % 10000 == 0) {
      fprintf(stderr, "Indexing key count #%d for [%s] = %d\n", ki, key.c_str(), count);
    }
    ki++;
    if (count < MIN_COUNT && !util.AllWild(key)) {
      continue;
    }
    indexing_keys.insert(key);
  }
  fprintf(stderr, "Post-filtering, index has size: %d\n", indexing_keys.size());

  fprintf(stderr, "Building index and agm-index...\n");
  map<string, set<int>> index;
  vector<vector<int>> agm_shards(AGM_INDEX_SHARDS);
  for (int i = 0; i < lexicon.phrase_infos.size(); ++i) {
    const PhraseInfo& phrase_info = lexicon.phrase_infos[i];
    const string& normalized = phrase_info.normalized;
    if (normalized.empty()) continue;
    vector<int> lex_indices;
    for (int j = 0; j < phrase_info.forms.size(); ++j) {
      lex_indices.push_back(phrase_info.base_index + j);
    }
    AddKeys(normalized, indexing_keys, lex_indices, &util, &index);
    AddAgmKey(normalized, lex_indices, &util, &agm_shards);
    if (i > 0 && i % 1000 == 0) {
      fprintf(stderr, "Indexed at %d: %s\n", i, normalized.c_str());
    }
  }

  fprintf(stderr, "Building phones-index...\n");
  vector<set<int>> phone_shards(PHONE_INDEX_SHARDS);
  for (int i = 0; i < lexicon.phrase_infos.size(); ++i) {
    const PhraseInfo& phrase_info = lexicon.phrase_infos[i];
    for (const vector<string>& phone : phrase_info.phones) {
      string phone_str = phone_util.Join(phone);
      int shard = phone_util.IndexShard(phone_str, PHONE_INDEX_SHARDS);
      for (int j = 0; j < phrase_info.forms.size(); ++j) {
        phone_shards[shard].insert(phrase_info.base_index + j);
      }
    }
  }

  struct KeyInfoByLen {
    int num_keys;
    int total_phrases;
    int max_phrases_for_a_key;
    int num_distinct_phrases;
    KeyInfoByLen() :
      num_keys(0),
      total_phrases(0),
      max_phrases_for_a_key(0),
      num_distinct_phrases(0) {}
  };

  map<int, KeyInfoByLen> len_counts;
  for (const auto& kv : index) {
    int vsize = kv.second.size();
    const vector<string> parts = util.PartsOf(kv.first, false);
    KeyInfoByLen counts = len_counts[parts.size()];
    counts.num_keys++;
    counts.total_phrases += vsize;
    if (util.AllWild(kv.first)) {
      counts.num_distinct_phrases = vsize;
    } else if (vsize > counts.max_phrases_for_a_key) {
      counts.max_phrases_for_a_key = vsize;
    }
    len_counts[parts.size()] = counts;
  }

  int64_t total_keys = 0, total_vals = 0, total_distinct_phrases = 0;
  for (const auto& lc : len_counts) {
    fprintf(stderr, "len:%d #keys: %d #phrases: %d "
                    "max-phrases-for-akey: %d num-distinct-phrases: %d\n",
            lc.first, lc.second.num_keys, lc.second.total_phrases,
            lc.second.max_phrases_for_a_key, lc.second.num_distinct_phrases);
    total_keys += lc.second.num_keys;
    total_vals += lc.second.total_phrases;
    total_distinct_phrases += lc.second.num_distinct_phrases;
  }
  fprintf(stderr, "Total #keys: %lld #phrases: %lld #distinct-phrases: %lld\n",
          total_keys, total_vals, total_distinct_phrases);

  map<int, int> agm_counts;
  int biggest_key = -1;
  int biggest_count = 0;
  for (int i = 0; i < AGM_INDEX_SHARDS; i++) {
    const auto& shard = agm_shards[i];
    int vsize = shard.size();
    agm_counts[vsize]++;
    if (vsize > biggest_count) {
      biggest_key = i;
      biggest_count = vsize;
    }
  }
  for (const auto& lc : agm_counts) {
    fprintf(stderr, "agmvalslen:%5d #keys: %3d\n", lc.first, lc.second);
  }
  fprintf(stderr, "Total# agm keys: %d\n", agm_shards.size());
  fprintf(stderr, "Bulkiest key: %d [%d]\n", biggest_key, biggest_count);

  // Output the JSON object that looks this:
  // exetLexicon = {
  //   id: 'Lufz-en-v0.06',
  //   language: 'en',
  //   script: 'Latin',
  //   letters: [ 'A', 'B', ... ],
  //   lexicon: [ "a", "the", ....],
  //   index: {
  //     '???': [42, 390, 2234, ...],
  //     'A??': [234, 678, ...],
  //     ...
  //   },
  //   anagrams: [
  //     [43, 1, ...],
  //     [43, 1, ...],
  //     ...
  //   ],
  //   phones: [[], [], ..., [["B","AH","N","AE","N","AH"]], ...],
  //   phindex: {
  //     [42, ...],
  //     [142,i 3232, ...],
  //     ...
  //   }
  // };
  printf("exetLexicon = {");
  printf("\n  id: \"Lufz-%s-%s\",",
         util.Language().c_str(), VERSION.c_str());
  printf("\n  language: \"%s\",", util.Language().c_str());
  printf("\n  script: \"%s\",", util.Script().c_str());
  printf("\n  letters: [");
  for (const string& letter : lexicon.letters) {
    printf("\"%s\", ", letter.c_str());
  }
  printf("],");
  printf("\n  lexicon: [");
  int row = 0;
  for (int i = 0; i < lexicon.phrase_infos.size(); ++i) {
    if (row % 100 == 0) printf("\n    ");
    const PhraseInfo& phrase_info = lexicon.phrase_infos[i];
    for (const string& form : phrase_info.forms) {
      printf("\"%s\", ", form.c_str());
      row++;
    }
  }
  printf("\n  ],");
  printf("\n  index: {\n");
  for (const auto& kv : index) {
    printf("    '%s': [", kv.first.c_str());
    int counter = 0;
    for (int lex_index : kv.second) {
      if (counter % 100 == 0) printf("\n      ");
      counter++;
      printf("%d, ", lex_index);
    }
    printf("\n    ],\n");
  }
  printf("  },");
  printf("\n  anagrams: [\n");
  for (const auto& shard : agm_shards) {
    printf("    [");
    for (int i = 0; i < shard.size(); ++i) {
      if (i % 100 == 0) printf("\n      ");
      printf("%d, ", shard[i]);
    }
    printf("\n    ],\n");
  }
  printf("  ],");
  printf("\n  phones: [");
  row = 0;
  for (int i = 0; i < lexicon.phrase_infos.size(); ++i) {
    if (row % 100 == 0) printf("\n    ");
    const PhraseInfo& phrase_info = lexicon.phrase_infos[i];
    for (const string& form : phrase_info.forms) {
      row++;
      printf("[");
      int j = 0;
      for (const vector<string>& phone : phrase_info.phones) {
        if (j > 0) printf(",");
        j++;
        printf("[");
        for (int k = 0; k < phone.size(); k++) {
          if (k > 0) printf(",");
          printf("\"%s\"", phone[k].c_str());
        }
        printf("]");
      }
      printf("], ");
    }
  }
  printf("\n  ],");
  printf("\n  phindex: [\n");
  for (const auto& shard : phone_shards) {
    printf("    [");
    int i = 0;
    for (int idx : shard) {
      if (i % 100 == 0) printf("\n      ");
      i++;
      printf("%d, ", idx);
    }
    printf("\n    ],\n");
  }
  printf("  ],");
  printf("};\n");

  return 0;
}

