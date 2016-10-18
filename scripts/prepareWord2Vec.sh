#!/bin/sh

strusInspect -s "path=storage1" content orig | perl -pe 's/^[0-9]+[:]//' > docs.1.txt
strusInspect -s "path=storage2" content orig | perl -pe 's/^[0-9]+[:]//' > docs.2.txt
strusInspect -s "path=storage3" content orig | perl -pe 's/^[0-9]+[:]//' > docs.3.txt

for dd in 1 2 3;
do
cat docs.$dd.txt | iconv -c -f utf-8 -t utf-8 - | perl -pe 's/\&[a-z]*\;/ /g' | perl -pe 's/nbsp[\;]/ /g' | perl -pe 's/[—––_=\/;,()\[\]\{\}\"-\-]/ /g' | perl -pe 's/([\!\?\:\.])/ \1 /g' | perl -pe 's/[ ]+/ /g' | perl -pe 's/[ ][.][ ]/ .\n/g' > docs.stripped.$dd.txt
scripts/strusnlp.py dict docs.stripped.$dd.txt 30 > dict.$dd.txt
done
scripts/strusnlp.py joindict dict.{1,2,3}.txt > dict.txt

for dd in 1 2 3;
do
scripts/strusnlp.py concat docs.stripped.$dd.txt dict.txt > docs.word2vec.$dd.txt
done
cat docs.word2vec.{1,2,3}.txt > docs.word2vec.txt

../word2vec/bin/word2vec -size 300 -window 8 -sample 1e-5 -negative 8 -threads 4 -min-count 4 -alpha 0.025 -classes 0 -debug 1 -binary 1 -save-vocab vocab.txt -cbow 0 -train docs.word2vec.txt -output vectors.bin



