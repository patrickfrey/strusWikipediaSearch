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
import re
import time
import getopt
import os
import stat
import subprocess

def mapTagValue( tagname):
    if tagname == "." or tagname == ";":
        return "." # [delimiter] sentence delimiter
    if tagname == "(" or tagname == ")" or tagname == "," or tagname == ":" or tagname == "#":
        return "" # [part delimiter] delimiter
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
        return "_" # [particle] particle
    if tagname == "$" or tagname == "S" or tagname == "SBAR" or tagname == "SBARQ" or tagname == "SINV" or tagname == "SQ":
        return "" # [] declarative clause, question, etc.
    if tagname == "SYM":
        return "N" # symbol
    if tagname == "VBD" or tagname == "VBG" or tagname == "VBN" or tagname == "VBP" or tagname == "VBZ" or tagname == "VB":
        return "V" # Verb, past tense or gerund or present participle or past participle or singular present
    if tagname == "WDT" or tagname == "WP" or tagname == "WP$" or tagname == "WRB":
        return "W" # determiner
    if tagname == "UH":
        return "" # exclamation
    if tagname == "":
        return "" # empty
    sys.stderr.write( "unknown NLP tag %s\n" % tagname)
    return "?"

tgmaplist = [
 "CC", "CD", "DT", "EX", "FW", "IN", "JJ", "JJR", "JJS", "LS", "MD", "TO",
 "NNS", "NN", "NNP", "NNPS", "PDT", "POS", "PRP", "PRP$", "RB", "RBR", "RBS", "RP",
 "S", "SBAR", "SBARQ", "SINV", "SQ", "SYM", "VBD", "VBG", "VBN", "VBP", "VBZ", "VB",
 "WDT", "WP", "WP$", "WRB", ".", ";", ":", ",", "(", ")", "$", "#" , "UH", ""
]

tgmap = {key: mapTagValue(key) for key in tgmaplist}

def mapTag( tagname):
    global tgmap
    if tagname in tgmap:
        return tgmap[ tagname]
    else:
        sys.stderr.write( "unknown NLP tag %s\n" % tagname)
        return "?"


def printStackElements( stk):
    rt = ""
    prev = ""
    doPrintMapType = 0
    if len(stk) > 1:
        doPrintMapType = 1
        if len(stk) <= 3:
            for elem in stk:
                if elem[1][0:3] != "NNP":
                    doPrintMapType = 0
    if doPrintMapType:
        for elem in stk:
            type = elem[1]
            maptype = elem[0]
            if type == "NNPS":
                type = "NNP"
            if type == "NNS":
                type = "NN"
            if type == prev:
                type = "_"
                maptype = "_";
            else:
                prev = type
            rt += maptype + "\t" + type + "\t" + elem[2] + "\n"
    else:
        for elem in stk:
            rt += "\t" + elem[1] + "\t" + elem[2] + "\n"
    return rt

def isDelimiter( tagname):
    if tagname == "." or tagname == ";" or tagname == ":":
        return True
    return False

def tagContent( text):
    text = re.sub( r"""[\s\`\'\"]+""", " ", text)
    tokens = nltk.word_tokenize( text)
    tagged = nltk.pos_tag( tokens)
    stk = []
    rt = ""
    for tt in tagged:
        type = tt[1]
        val = tt[0]
        maptype = mapTag( type)
        if isDelimiter( type):
            rt += printStackElements( stk)
            stk = []
            stk.append( [maptype, type, val] )
            rt += printStackElements( stk)
            stk = []
        else:
            stk.append( [maptype, type, val] )
    rt += printStackElements( stk)
    return rt

doccnt = 0
startTime = time.time()

def printOutput( filename, content, result):
    global doccnt
    global startTime
    doccnt += 1
    elapsedTime = (time.time() - startTime) / doccnt
    timeString = "%.3f" % (elapsedTime*1000.0)

    sys.stderr.write( "\rprocessed %d documents  (%s)          " % (doccnt,timeString))
    print( "#FILE#%s\n" % filename)
    print( "%s" % tagContent( content))

def printUsage():
    print( "%s [-h] [-C <chunksize> ]\n" % argv[0])

def parseProgramArguments( argv):
    rt = {}
    try:
        opts, args = getopt.getopt( argv,"hC:",["chunksize="])
        for opt, arg in opts:
            if opt == '-h':
                printUsage()
                sys.exit()
            elif opt in ("-C", "--chunksize"):
                ai = int(arg)
                if ai <= 0:
                    raise RuntimeError( "positive integer expected")
                rt['C'] = ai
        return rt
    except getopt.GetoptError:
        sys.stderr.write( "bad arguments\n")
        printUsage()
        sys.exit(2)

def processStdin():
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
                result += tagContent( content + " .\n")
    printOutput( filename, content, result)

linebuf = None
def readChunkStdin( nofFiles):
    ii = 0
    content = ""
    global linebuf
    if linebuf:
        if len(linebuf) > 6 and linebuf[0:6] == '#FILE#':
            ii += 1
        else:
            raise RuntimeError("unexpected line in buffer")
        content += linebuf
        linebuf = None
    for line in sys.stdin:
        if len(line) > 6 and line[0:6] == '#FILE#':
            ii += 1
            if ii > nofFiles:
                linebuf = line
                return content
        content += line
    return content

def printContent( content):
    for line in content.splitlines():
        if len(line) > 30:
            sys.stderr.write( "++ %s\n" % line[:30])
        else:
            sys.stderr.write( "++ %s\n" % line)

if __name__ == "__main__":
    argmap = parseProgramArguments( sys.argv[ 1:])
    chunkSize = 0
    if 'C' in argmap:
        chunkSize = argmap[ 'C']
    if chunkSize > 0:
        content = readChunkStdin( chunkSize)
        printContent( content)
        while content:
            result = subprocess.check_output([ os.path.abspath(__file__)], input=bytearray(content,"UTF-8")).decode("utf-8")
            print( result)
            content = readChunkStdin( chunkSize)
            printContent( content)
    else:
        processStdin()








