#!/bin/sh

export PYTHONHASHSEED=123
cat simple.input.txt | ../../scripts/strusnlp_spacy.py -K -V > simple.result.txt
diff simple.result.txt simple.expect.txt
cat simple.input.txt | ../../scripts/strusnlp_spacy.py > simple_raw.result.txt
diff simple_raw.result.txt simple_raw.expect.txt
