#!/usr/bin/python3
#
# Do taggig of noun phrases
# Read input from stdin
#
# This script requires resources to be installed:
#    $ sudo mkdir /usr/share/nltk_data
#    $ sudo python -m nltk.downloader -d /usr/share/nltk_data all
#
import nltk
import sys

def tagContent( text):
    tokens = nltk.word_tokenize( text)
    tagged = nltk.pos_tag( tokens)
    prev = ""
    rt = ""
    for tt in tagged:
        type = tt[0]
        val = tt[1]
        if type == prev:
            type = ".."
        else:
            prev = type
        rt += type + "\t" + val + "\n"
    return rt

def process( content):
    print( "--\n[%s]\n" % content)
    print( "%s" % tagContent( content))

content = ""
for line in sys.stdin:
    content += line
    if line == ".\n" or line == ";\n" or line == "\n":
        process( content)
        content = ""

process( content)







