#!/usr/bin/python3
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
import collections
import spacy
import en_core_web_sm

def mapTagValue( tagname):
    if tagname == "." or tagname == ";":
        return "T!" # [delimiter] sentence delimiter
    if tagname == "-LRB-" or tagname == "-RRB-" or tagname == "," or tagname == ":" or tagname == "#":
        return "P!" # [part delimiter] delimiter
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
        return "T" # [delimiter] list item marker
    if tagname == "MD":
        return "m" # modal
    if tagname == "TO":
        return "" # to
    if tagname == "NN" or tagname == "NNS":
        return "N" # noun singular or plural
    if tagname == "NNP" or tagname == "NNPS":
        return "E" # [entity] proper noun, singular or plural
    if tagname == "ADD":
        return "U" # [URI]
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
    if tagname == "HYPH":
        return "" # empty
    if tagname == "AFX":
        return "A" # empty
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
    if tagname == "''":
        return "" # empty
    if tagname == "``":
        return "" # empty
    if tagname == "NFP":
        return "" # empty
    if tagname == "":
        return "" # empty
    if tagname == "XX":
        return "" # empty
    if tagname == "_SP":
        return "" # empty
    sys.stderr.write( "unknown NLP tag %s\n" % tagname)
    return "?"

tgmaplist = [
 "CC", "CD", "DT", "EX", "FW", "IN", "JJ", "JJR", "JJS", "LS", "MD", "TO",
 "NNS", "NN", "NNP", "NNPS", "PDT", "POS", "PRP", "PRP$", "RB", "RBR", "RBS", "RP", "HYPH", "NFP", "AFX",
 "S", "SBAR", "SBARQ", "SINV", "SQ", "SYM", "VBD", "VBG", "VBN", "VBP", "VBZ", "VB", "ADD",
 "WDT", "WP", "WP$", "WRB", ".", ";", ":", ",", "-LRB-", "-RRB-", "$", "#" , "UH", "", "''", "``", "XX", "_SP"
]

tgmap = {key: mapTagValue(key) for key in tgmaplist}

def mapTag( tagname):
    global tgmap
    if tagname in tgmap:
        return tgmap[ tagname]
    else:
        sys.stderr.write( "unknown NLP tag %s\n" % tagname)
        return "?"

def unifyType( type):
    if type == "NNPS":
        return "NNP"
    if type == "NNS":
        return "NN"
    return type

def printStackElements( stk):
    rt = ""
    prev = ""
    mapprev = ""
    doPrintMapType = 0
    hasVerb = False
    hasEntitiesOnly = True
    for elem in stk:
        if elem.nlptag:
            if elem.nlptag[0] == 'V':
                hasVerb = True
            elif elem.nlptag[:3] != 'NNP':
                hasEntitiesOnly = False
    if hasVerb or hasEntitiesOnly:
        for elem in stk:
            utype = unifyType( elem.nlptag)
            maptype = mapTag( type)
            if utype == prev:
                type = "_"
                maptype = "_";
            elif maptype == mapprev and maptype[-1:] == '!':
                maptype = '_'
            else:
                prev = utype
                mapprev = maptype
            rt += maptype + "\t" + type + "\t" + elem.role + "\t" + elem.value + "\n"
    else:
        for elem in stk:
            type = unifyType( elem.nlptag)
            rt += "\t" + type + "\t\t" + elem.value + "\n"
    return rt

spacy_nlp = en_core_web_sm.load()
NlpToken = collections.namedtuple('NlpToken', ['strustag','nlptag','role', 'value'])

def tagContent( text):
    rt = ""
    stk = []
    tg = spacy_nlp( text)
    for node in tg:
        value = str(node)
        stk.append( NlpToken( "", node.tag_, node.dep_, value))
        if node.dep_ == "punct" and value in [';','.']:
            rt += printStackElements( stk)
            stk = []
    rt += printStackElements( stk)
    return rt

doccnt = 0
statusLine = False
startTime = time.time()

def printStatusLine( nofdocs):
    global doccnt
    global startTime
    doccnt += nofdocs
    elapsedTime = (time.time() - startTime)
    timeString = "%.3f seconds" % elapsedTime
    sys.stderr.write( "\rprocessed %d documents  (%s)          " % (doccnt,timeString))

def printOutput( filename, result):
    global statusLine
    if statusLine:
        printStatusLine( 1)
    print( "#FILE#%s" % filename)
    print( "%s" % result)

def printUsage():
    print( "%s [-h|--help] [-V|--verbose] [-S|--status] [-C <chunksize> ]" % __file__)

def parseProgramArguments( argv):
    rt = {}
    try:
        opts, args = getopt.getopt( argv,"hVSC:",["chunksize=","status","verbose"])
        for opt, arg in opts:
            if opt in ("-h", "--help"):
                printUsage()
                sys.exit()
            elif opt in ("-C", "--chunksize"):
                ai = int(arg)
                if ai <= 0:
                    raise RuntimeError( "positive integer expected")
                rt['C'] = ai
            elif opt in ("-V", "--verbose"):
                rt['V'] = True
            elif opt in ("-S", "--status"):
                rt['S'] = True
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
            if content:
                result += tagContent( content + "\n")
                printOutput( filename, result)
                result = ""
            filename = line[6:]
        else:
            content += line
            if line == ".\n" or line == ";\n" or line == "\n":
                result += tagContent( content)
                content = ""
    if content:
        result += tagContent( content + "\n")
    if filename and result:
        printOutput( filename, result)

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
                return content, ii-1
        content += line
    return content, ii

def printContent( content):
    for line in content.split("\n", 20):
        if len(line) > 30:
            sys.stderr.write( "++ %s\n" % line[:30])
        else:
            sys.stderr.write( "++ %s\n" % line)

if __name__ == "__main__":
    argmap = parseProgramArguments( sys.argv[ 1:])
    chunkSize = 0
    verbose = False

    if 'V' in argmap:
        verbose = True
    if 'C' in argmap:
        chunkSize = argmap[ 'C']
    if 'S' in argmap:
        statusLine = True
    if chunkSize > 0:
        content, nofdocs = readChunkStdin( chunkSize)
        if verbose:
            printContent( content)
        while content:
            result = subprocess.check_output([ os.path.abspath(__file__)], input=bytearray(content,"UTF-8")).decode("utf-8")
            if statusLine:
                printStatusLine( nofdocs)
            print( result)
            content, nofdocs = readChunkStdin( chunkSize)
            if verbose:
                printContent( content)
    else:
        processStdin()








