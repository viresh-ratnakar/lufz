# Lufz

## Version: 0.06

## Code for attaching importance scores to words in a lexicon and for indexing the lexicon.

#### Author: Viresh Ratnakar

Start with a lexicon file that is plain text file listing one word/phrase
on each line. I have used the UKACD18 file (after editing it a bit) for
English, and a few other sources for other languages. You can find several
options for English at the [Qxw site](https://www.quinapalus.com/xwfaq.html).

Say this file is called `words.txt`.

## Build

Just use the command `make` to build all the binaries.

## add-wiki-popularity

- Grab all of English Wikipedia (this part is essentially taken from the steps
outlined in the [Nutrimatic project](https://github.com/egnor/nutrimatic).

- Download the latest Wikipedia database dump (this is an ~18GB file!):
```
wget https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2
```

- Extract the text from the articles using Wikipedia Extractor
 (this generates ~16GB of text, and can take several hours!):
```
wget https://github.com/attardi/wikiextractor/archive/refs/heads/master.zip
unzip master.zip
mv wikiextractor-master/wikiextractor .
python -m wikiextractor.WikiExtractor enwiki-latest-pages-articles.xml.bz2
```
This will write many files named `text/??/wiki_??`.

- Run `add-wiki-popularity`. This might take a couple of hours.
```
find text -type f | xargs cat | ./add-wiki-popularity English words.txt > importance-and-words.tsv
```
The created file importance-and-words.txt is a copy of words.txt with a numeric
occurrence count prefixed to each line, with a tab character as the separator.

## index-word-list

- Run it on the `importance-and-words.tsv` file.
- The output will be a file containing JavaScript code that creates an object
  called `exetLexicon` that has an array called `lexicon` of all the words,
  with an empty string at index 0), an array called `importance` containing all
  the importance scores, and an object called `index` that maps various
  indexing keys to arrays of word indices, and an array called anagrams
  that is a sharded index for searching for anagrams. It also has arrays
  phones and a sharded index phindex, for pronunciations.
- The needed parameter is the name of a file that contains pronunciations.
  in a simple TSV format (word\tpronunciation). The pronunciation can be
  in ARPAbet or IPA format.
- For English, you can derive it from CMUdict
  [get it here](http://svn.code.sf.net/p/cmusphinx/code/trunk/cmudict/cmudict-0.7b),
  (please follow its license instructions).
- If you don't have pronunciations available, just create an empty file.
- The `crossed_words.txt` file can contain a list of words to avoid (such as
  profanities or offesive words). You can pass an empty file if you do not
  have/want such a list.
```
./index-word-list English importance-and-words.txt words_and_phones.tsv crossed_words.txt > lufz-en-lexicon.js
```

### Indexing details

The exetLexicon.index object has keys that look like 'AB???': When you want to
look for a phrase with only some letters known, replace all unknown
letters by '?', get rid of all spaces, uppercase the string and then look
in index. If not found, iteratively replace the last known character
with '?' and look up again. When you get a hit, go through it to keep it
only if it matches the original, unmodified key.

The exetLexicon.anagrams array is of length 2000. Each entry is an array
of lexicon indices. To find anagrams of a string, uppercase it, remove
all unknown characters and spaces, sort it (this is the "key"), take the
JavaHash() of the key modulo 2000 (adding 2000 if negative), to find the
shard index. Go through all entries in the shard (~100) and filter out those
that do not have the exact same key.

The exetLexicon.phindex array is just like the anagrams array, but is an
index of the pronunciations.

I wrote this code for use in the [Exet
project](https://github.com/viresh-ratnakar/exet), which is a web app for
crossword construction.

