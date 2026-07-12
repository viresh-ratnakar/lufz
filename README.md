# Lufz

## Version: 0.10

## Code for attaching importance scores to words in a lexicon and for indexing the lexicon.

#### Author: Viresh Ratnakar

Start with a lexicon file that is plain text file listing one word/phrase
on each line. I have used the UKACD18 file (after editing it a bit) for
English, and a few other sources for other languages. You can find several
options for English at the [Qxw site](https://www.quinapalus.com/xwfaq.html).

June 2026: I've now also created a "Nediger-List" word list for English,
which uses [Will Nediger's manually curated list](https://codeberg.org/bewilderingly/Nediger-list).
For all "Lufz" lists (my UKACD18-based lexicon, and the Hindi/Portuguese ones),
the version number will be the same as the lufz code version. For other lists
(such as Nediger-List, I'll use a date-based version id, such as
"Nediger-List-v23Jun2026").

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
wget https://raw.githubusercontent.com/apertium/WikiExtractor/master/WikiExtractor.py
export PYTHONPATH="$PWD"
python3 WikiExtractor.py --infn enwiki-latest-pages-articles.xml.bz2
```
This will write a giant file named `wiki.txt`. You may kill the extractor process
once `wc -l wiki.txt` crosses 30,000,000 (as the next `add-wiki-popularity` step
reads at most 30M lines).

- Run `add-wiki-popularity`. This might take a couple of hours.
```
cat wiki.txt | ./add-wiki-popularity English words.txt > importance-and-words.tsv
```
The created file importance-and-words.txt is a copy of words.txt with a numeric
occurrence count prefixed to each line, with a tab character as the separator.

## index-word-list

- Run it on the `importance-and-words.tsv` file. It can also be run on a wordlist
  file that has the "word or phrase;score" line format.
- The output will be a file containing JavaScript code that creates an object
  called `exetLexicon` that has an array called `lexicon` of all the words,
  with an empty string at index 0), an object called `index` that maps various
  indexing keys to arrays of word indices, and an array called anagrams
  that is a sharded index for searching for anagrams. It also has arrays
  phones and a sharded index phindex, for pronunciations.
- The JavaScript code specifies the full object through parsing a JSON string,
  as directly specifying the large object (with large arrays) leads to stack
  overflow on some platform (but the JSON.parse() code is more robust).
- The output file has multiple independent "sections", each looking like this:
  ```
    exetLexicon = {...((typeof exetLexicon == "object" && exetLexicon) ? exetLexicon : {}), ...JSON.parse(`{
    ...
    }`)};
  ```
  This allows the output to be split into multiple independent files if needed
  (I need the file size to be under 25 MB to host on github).
- The needed parameter is the name of a file that contains pronunciations.
  in a simple TSV format (word\tpronunciation). The pronunciation can be
  in ARPAbet or IPA format.
- For English, you can derive it from CMUdict
  [get it here](http://svn.code.sf.net/p/cmusphinx/code/trunk/cmudict/cmudict-0.7b),
  (please follow its license instructions).
- If you don't have pronunciations available, just create an empty file.
- The `crossed_words.txt` file can contain a list of words to avoid (such as
  profanities or offesive words). You can pass an empty file or an empty-string
  ("") as the file name  if you do not have/want such a list.
- You can add the option `-s` to also get a "scores" array containing all the
  importance scores. We do not do this for lufz (we just use the %ile position
  in the sorted order), but for other wordlists, the score values themselves
  are intended to be used in applications.
- You can override the "id" used (I do that for non-"Lufz" word lists, such as
  "Nediger-List") with `-i <id>`. For Lufz word lists, the id is of the form
  "Lufz-en-v0.09" (it uses the lufz code version at its end). For
  "Nediger List", the id is of the form "Nediger-List-v23Jun2026".
```
./index-word-list [-s] [-i <id>] English importance-and-words.txt words_and_phones.tsv crossed_words.txt > lufz-en-lexicon.js
```

## Adding stemming and regional spelling variants info for English

For English, the generated `lufz-en-lexicon.js` file needs to be supplemented
with stemming and regional spelling variant sinfo, generated as described below.
The generated data can be pasted into the same file or can be loaded separately.

Link or copy the following files into the directory where you have generated
the `lufz-en-lexicon.js` file:

- `stemming/get-stems-and-regional-spellings.html`
- [`exet-lexicon.js`](https://raw.githubusercontent.com/viresh-ratnakar/exet/refs/heads/master/exet-lexicon.js)
- [`stemming/wink-porter2-stemmer/wink-porter2-stemmer-master/src/wink-porter2-stemmer.js`](https://github.com/winkjs/wink-porter2-stemmer/blob/master/src/wink-porter2-stemmer.js)

Open the HTML file (`get-stems-and-regional-spellings.html`) in a web browser.
Specify the `?lex=lufz-en-lexicon` parameter to identify the English lexicon
file that's missing stemming/regional-spellings info, without the `.js` suffix
(omitting this URL param will make the default value of `lufz-en-lexicon` get
used). This will save a file named `lufz-en-lexicon-stems.js` (or
`<lex-url-param>-stems.js`) into the browser's Downloads folder. Copy and paste
the contents of this downloaded file at the bottom of `lufz-en-lexicon.js` (or
whatever the lexicon is named). You can also load it separately, listing both
file names within `exetConfig` (see `exet.html`).

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

## lufz-diff

This is a simple tool to generate the diffs between two wordlists. The
wordlists can contain just lines with words/phrases, or the can have the
line format "score[tab]word-or-phrase", or the line format
"word-or-phrase;score".

```
./lufz-diff English list1 list2 > diffs.txt
```

You can filter four distinct component files from the diff file:

```
$ grep '\[Only-in-1\]' diffs.txt | sed 's/.*\]//' > diff-only-in-1.txt
$ grep '\[Only-in-1-in-this-form\]' diffs.txt | sed 's/.*\]//' > diff-only-in-1-in-this-form.txt
$ grep '\[Only-in-2\]' diffs.txt | sed 's/.*\]//' > diff-only-in-2.txt
$ grep '\[Only-in-2-in-this-form\]' diffs.txt | sed 's/.*\]//' > diff-only-in-2-in-this-form.txt
```

The entries in `diff-only-in-1.txt` are only present on `list1`, they are
missing from `list2` even allowing for differences in spacing/capitalization.
The entries in `diff-only-in-1-in-this-form.txt` are only present on `list1` in
those particular forms; they are present in `list2`, but in some other form
(spacing/capitalization).

