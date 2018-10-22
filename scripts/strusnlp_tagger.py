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

def printOutput( filename, content, result):
    print( "#FILE#%s\n" % filename)
    print( "%s" % tagContent( content))

content = ""
result = ""
filename = ""
for line in sys.stdin:
    if len(line) > 6 and line[0:6] == '#FILE#':
        if content != "":
            printOutput( filename, content, result)
        filename = line[6:]
    else:
        content += line
        if line == ".\n" or line == ";\n" or line == "\n":
            result += tagContent( content) + "\n"

printOutput( filename, content, result)








