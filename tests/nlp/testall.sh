#!/bin/sh

export PYTHONHASHSEED=123
cat simple.input.txt | ../../scripts/strusnlp_spacy.py -K -V > simple.result.txt
diff simple.result.txt simple.expect.txt
