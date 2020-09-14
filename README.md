# Lufz

## Code for attaching importance scores to words in a lexicon and for indexing the lexicon.

#### Author: Viresh Ratnakar

Start with a lexicon file that is plain text file listing one word/phrase
on each line. I have used the UKACD18 file (after editing it a bit). You
can find several options at the
[Qxw site](https://www.quinapalus.com/xwfaq.html).

Say this file is called `words.txt`.

## add-wiki-popularity

- Build:
```
g++ -O -o add-wiki-popularity add-wiki-popularity.cc lufz-util.cc
```

- Grab all of English Wikipedia (this part is essentially taken from the steps
outlined in the [Nutrimatic project](https://github.com/egnor/nutrimatic).

- Download the latest Wikipedia database dump (this is an ~18GB file!):
```
wget https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2
```

- Extract the text from the articles using Wikipedia Extractor
 (this generates ~16GB of text, and can take several hours!):
```
wget https://svn.code.sf.net/p/apertium/svn/trunk/apertium-tools/WikiExtractor.py
python WikiExtractor.py enwiki-latest-pages-articles.xml.bz2
```
This will write many files named `text/??/wiki_??`.

- Run `add-wiki-popularity`. This might take a couple of hours.
```
find text -type f | xargs cat | ./add-wiki-popularity words.txt > importance-and-words.txt
```
The created file importance-and-words.txt is a copy of words.txt with a numeric
occurrence count prefixed to each line.


## index-word-list

- Build:
```
g++ -O -o index-word-list index-word-list.cc lufz-util.cc
```

- Run it on a word list. The output will be a file containing JavaScript code
  that creates an object called `exetLexicon` that has an array called
  `lexicon` of all the words, an array called `importance` containing all
  the importance scores, and an object called `index` that maps various
  indexing keys to arrays of word indices.
```
cat importance-and-words.txt | ./index-word-list > lufz-en-lexicon.js
```

I wrote this code for use in the [Exet
project](https://github.com/viresh-ratnakar/exet), which is a web app for
crossword construction.

