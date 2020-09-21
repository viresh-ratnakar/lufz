#include <map>
#include <string>
#include <vector>

#include <math.h>
#include <stdio.h>

#include "lufz-util.h"

using namespace std;

namespace lufz {

#define AGM_INDEX_SHARDS 2000
#define INDEX_SHARDS 2000

void AddKeys(const string& phrase, int i, map<string, vector<int>>* index) {
  string basic_key = Key(phrase);
  int len = basic_key.size();
  if (len > WILDIZE_ALL_BEYOND) {
    len = WILDIZE_ALL_BEYOND;
  }
  for (int pattern = 0; pattern < (1 << len); pattern++) {
    string key_variant = basic_key;
    for (int i = 0; i < len; i++) {
      if (pattern & (1 << i)) {
        key_variant[i] = '?';
      }
    }
    (*index)[key_variant].push_back(i);
  }
}

void AddAgmKey(const string& phrase, int i,
               vector<vector<int>>* agm_shards) {
  string key = AgmKey(phrase);
  int shard = IndexShard(key, AGM_INDEX_SHARDS);
  (*agm_shards)[shard].push_back(i);
}


}  // namespace lufz

int main(int argc, char* argv[]) {
  using namespace lufz;

  vector<PhraseAndImportance> lexicon;

  if (!ReadLexicon("-", &lexicon)) {
    return 2;
  }

  map<string, vector<int>> index;
  vector<vector<int>> agm_shards(AGM_INDEX_SHARDS);
  for (int i = 0; i < lexicon.size(); ++i) {
    const string& phrase = lexicon[i].phrase;
    AddKeys(phrase, i, &index);
    AddAgmKey(phrase, i, &agm_shards);
    if (i > 0 && i % 1000 == 0) {
      fprintf(stderr, "Indexed line %d: %s\n", i, phrase.c_str());
    }
  }
  fprintf(stderr, "Pre-filtering, index has size: %d\n", index.size());
  // Filter
  const int MIN_COUNT = 1024;
  auto it = index.begin();
  while (it != index.end()) {
    if (it->second.size() < MIN_COUNT && !AllWild(it->first)) {
      it = index.erase(it);
    } else {
      ++it;
    }
  }
  fprintf(stderr, "Post-filtering, index has size: %d\n", index.size());

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
    KeyInfoByLen counts = len_counts[kv.first.size()];
    counts.num_keys++;
    counts.total_phrases += vsize;
    if (AllWild(kv.first)) {
      counts.num_distinct_phrases = vsize;
    } else if (vsize > counts.max_phrases_for_a_key) {
      counts.max_phrases_for_a_key = vsize;
    }
    len_counts[kv.first.size()] = counts;
  }

  int64_t total_keys = 0, total_vals = 0, total_distinct_phrases = 0;
  for (const auto& lc : len_counts) {
    fprintf(stderr, "len:%d #keys: %d #phrases: %d max-phrases-for-akey: %d num-distinct-phrases: %d\n",
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
  //   id: 'en-ukacd18-lufz-v0.02',
  //   language: 'en',
  //   script: 'Latin',
  //   letters: [ 'A', 'B', ... ],
  //   lexicon: [ "a", "the", ....],
  //   importance: [100, 98, ...],
  //   index: {
  //     '???': [42, 390, 2234, ...],
  //     'a??': [234, 678, ...],
  //     ...
  //   },
  //   anagrams: [
  //     [43, 1, ...],
  //     [43, 1, ...],
  //   ],
  // };
  printf("exetLexicon = {");
  printf("\n  id: \"ukacd18-lufz-v0.02\",");
  printf("\n  language: \"en\",");
  printf("\n  script: \"Latin\",");
  printf("\n  letters: [");
  for (int i = 0; i < 26; i++) {
    printf("\"%c\", ", 'A' + i);
  }
  printf("],");
  printf("\n  lexicon: [");
  for (int i = 0; i < lexicon.size(); ++i) {
    if (i % 100 == 0) printf("\n    ");
    printf("\"%s\", ", lexicon[i].phrase.c_str());
  }
  printf("\n  ],");
  printf("\n  importance: [");
  for (int i = 0; i < lexicon.size(); ++i) {
    if (i % 100 == 0) printf("\n    ");
    if (lexicon[i].importance <= 1.0) {
      printf("0, ");
    } else {
      printf("%.1llf, ", log10l(lexicon[i].importance));
    }
  }
  printf("\n  ],");
  printf("\n  index: {\n");
  for (const auto& kv : index) {
    printf("    '%s': [", kv.first.c_str());
    for (int i = 0; i < kv.second.size(); ++i) {
      if (i % 100 == 0) printf("\n      ");
      printf("%d, ", kv.second[i]);
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
  printf("  ],\n");
  printf("};\n");

  return 0;
}

