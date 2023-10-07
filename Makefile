all : lufz-util-test read-lexicon-test lufz-check-phonetics add-wiki-popularity index-word-list

lufz-utf8.o : lufz-utf8.cc lufz-utf8.h lufz-configs.h
	g++ -O -c lufz-utf8.cc

lufz-util.o : lufz-util.cc lufz-util.h lufz-utf8.h lufz-configs.h
	g++ -O -c lufz-util.cc

lufz-util-test : lufz-util-test.cc lufz-utf8.o lufz-util.o
	g++ -O -o lufz-util-test lufz-util-test.cc lufz-utf8.o lufz-util.o

read-lexicon-test : read-lexicon-test.cc lufz-utf8.o lufz-util.o
	g++ -O -o read-lexicon-test read-lexicon-test.cc lufz-utf8.o lufz-util.o

lufz-check-phonetics : lufz-check-phonetics.cc lufz-utf8.o lufz-util.o
	g++ -O -o lufz-check-phonetics lufz-check-phonetics.cc lufz-utf8.o lufz-util.o

add-wiki-popularity : add-wiki-popularity.cc lufz-utf8.o lufz-util.o
	g++ -O -o add-wiki-popularity add-wiki-popularity.cc lufz-utf8.o lufz-util.o

index-word-list : index-word-list.cc lufz-utf8.o lufz-util.o
	g++ -O -o index-word-list index-word-list.cc lufz-utf8.o lufz-util.o

clean :
	rm lufz-util-test read-lexicon-test lufz-check-phonetics add-wiki-popularity index-word-list lufz-utf8.o lufz-util.o
