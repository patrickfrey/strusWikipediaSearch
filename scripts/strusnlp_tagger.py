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
from pprint import pprint
import sys

def mapTagValue( tagname):
    if tagname == "." or tagname == ";":
        return "." # [delimiter] sentence delimiter
    if tagname == "CC":
        return "" # coordinating conjunction
    if tagname == "CD":
        return "C" # cardinal number
    if tagname == "DT":
        return "" # [determiner] determiner
    if tagname == "EX":
        return "X" # existential there
    if tagname == "FW":
        return "N" # foreign word
    if tagname == "IN":
        return "" # [determiner] preposition or subordinating conjunction
    if tagname == "JJ" or tagname == "JJR" or tagname == "JJS":
        return "A" # [adjective/adverb] adjective
    if tagname == "LS":
        return "." # [delimiter] list item marker
    if tagname == "MD":
        return "m" # modal
    if tagname == "TO":
        return "" # to
    if tagname == "NN" or tagname == "NNS":
        return "N" # noun singular or plural
    if tagname == "NNP" or tagname == "NNPS":
        return "E" # [entity] proper noun, singular or plural
    if tagname == "PDT":
        return "m" # [modal] pre determiner
    if tagname == "POS":
        return "" # [possesive] possessive ending
    if tagname == "PRP":
        return "E" # [entity] personal pronoun
    if tagname == "PRP$":
        return "E" # [entity] possesive pronoun
    if tagname == "RB" or tagname == "RBR" or tagname == "RBS":
        return "A" # [adjective/adverb] adverb or comparative or superlative
    if tagname == "RP":
        return ".." # [particle] particle
    if tagname == "S" or tagname == "SBAR" or tagname == "SBARQ" or tagname == "SINV" or tagname == "SQ":
        return "" # [] declarative clause, question, etc.
    if tagname == "SYM":
        return "N" # symbol
    if tagname == "VBD" or tagname == "VBG" or tagname == "VBN" or tagname == "VBP" or tagname == "VBZ" or tagname == "VB":
        return "V" # Verb, past tense or gerund or present participle or past participle or singular present
    if tagname == "WDT" or tagname == "WP" or tagname == "WP$" or tagname == "WRB":
        return "W" # Verb, past tense or gerund or present participle or past participle or singular presen
    sys.stderr.write( "unknown NLP tag %s\n" % tagname)
    return "?"

tgmaplist = [
 "CC", "CD", "DT", "EX", "FW", "IN", "JJ", "JJR", "JJS", "LS", "MD", "TO",
 "NNS", "NN", "NNP", "NNPS", "PDT", "POS", "PRP", "PRP$", "RB", "RBR", "RBS", "RP",
 "S", "SBAR", "SBARQ", "SINV", "SQ", "SYM", "VBD", "VBG", "VBN", "VBP", "VBZ", "VB",
 "WDT", "WP", "WP$", "WRB", "."
]

tgmap = {key: mapTagValue(key) for key in tgmaplist}

def mapTag( tagname):
    if tagname in tgmap:
        return tgmap[ tagname]
    else:
        sys.stderr.write( "unknown NLP tag %s\n" % tagname)
        return "?"


def tagContent( text):
    tokens = nltk.word_tokenize( text)
    tagged = nltk.pos_tag( tokens)
    prev = ""
    rt = ""
    for tt in tagged:
        type = tt[1]
        val = tt[0]
        maptype = mapTag( type)
        if type == prev:
            type = ".."
        else:
            prev = type
        rt += maptype + "\t" + type + "\t" + val + "\n"
    return rt

doccnt = 0

def printOutput( filename, content, result):
    doccnt += 1
    if doccnt % 10 == 0:
        sys.stderr.write( "\rprocessed %d\n" % doccnt)
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
            result += tagContent( content + " .\n") + "\n"

printOutput( filename, content, result)








