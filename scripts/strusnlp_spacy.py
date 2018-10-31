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
from recordtype import recordtype
import spacy
import en_core_web_sm

spacy_nlp = en_core_web_sm.load()
NlpToken = recordtype('NlpToken', ['strustag','strusrole','nlptag','nlprole', 'value', 'ref'])
Subject = recordtype('Subject', ['value', 'prp', 'occurrence', 'ref'])

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

def tagSentenceStrusTags( stk):
    rt = []
    prev = ""
    mapprev = ""
    hasVerb = False
    hasEntitiesOnly = True
    for elem in stk:
        if elem.nlptag:
            if elem.nlptag[0] == 'V':
                hasVerb = True
            elif elem.nlptag[:3] != 'NNP':
                hasEntitiesOnly = False

    if hasEntitiesOnly:
        for elem in stk:
            elem.strustag = "N"
            rt.append( elem)
        return rt
    elif hasVerb:
        for eidx,elem in enumerate(stk):
            type = elem.nlptag
            utype = unifyType( type)
            maptype = mapTag( type)
            if utype == prev:
                type = "_"
                maptype = "_";
            elif maptype == mapprev and maptype[-1:] == '!':
                maptype = '_'
            else:
                prev = utype
                mapprev = maptype
            elem.strustag = maptype
            elem.nlptag = type
            rt.append( elem)
        return rt
    else:
        return stk

def tagSentenceSubjects( stk):
    rt = []
    doShift = False
    doShiftNext = False
    hasVerb = False
    for elem in stk:
        if elem.nlptag and elem.nlptag[0] == 'V':
            hasVerb = True
    if not hasVerb:
        return stk
    subjindices = []
    startidx = -1
    for eidx,elem in enumerate( stk):
        if elem.nlptag[:2] == 'NN' or elem.nlptag == 'PRP':
            startidx = eidx
        if elem.nlprole == "nsubj" or (elem.nlprole == "conj" and doShiftNext):
            doShift = True
        else:
            if elem.nlprole == "cc" and elem.nlptag == "CC":
                doShiftNext = True
            else:
                doShiftNext = False
            if doShift:
                if startidx >= 0:
                    subjindices.append( startidx)
                    startidx = -1
                doShift = False
            if not (elem.nlptag[:2] == 'NN' or elem.nlptag == '_'):
                startidx = -1
    follow = False
    for eidx,elem in enumerate( stk):
        if eidx in subjindices:
            elem.strusrole = "S"
            follow = True
        if follow:
            if elem.strustag == '_':
                elem.strusrole = "_"
            else:
                follow = False
        rt.append( elem)
    return rt

def enrichSentenceTokens( stk):
    return tagSentenceSubjects( tagSentenceStrusTags( stk))

def printSentence( stk):
    rt = ""
    for elem in stk:
        sr = (elem.strusrole or "")
        nr = (elem.nlprole or "")
        st = (elem.strustag or "")
        nt = (elem.nlptag or "")
        vv = (elem.value or "")
        rt += ("%s (%s)\t%s\t%s\t%s\n" % (sr,nr,st,nt,vv))
    return rt

def printSubject( subject):
    vv = (subject.value or "")
    pr = None
    if subject.prp:
        pr = ','.join( subject.prp)
    oc = (subject.occurrence or "")
    rf = (subject.ref or "")
    return ("%s\t%s\t%s\t%s" % (vv,pr,oc,rf))

# return Sentence[]
def getDocumentSentences( text):
    tokens = []
    sentences = []
    last_subjects = []
    tg = spacy_nlp( text)
    for node in tg:
        value = str(node)
        if (value.strip()):
            tokens.append( NlpToken( None, None, node.tag_, node.dep_, value, None))
        if node.dep_ == "punct" and value in [';','.']:
            sentences.append( enrichSentenceTokens( tokens))
            tokens = []
    if tokens:
        sentences.append( enrichSentenceTokens( tokens))
    return sentences


def matchName( obj, candidate):
    cd = candidate
    for nam in obj:
        if nam[ -1:] == '.':
            prefix = nam[ :-1]
            found = False
            for eidx,elem in enumerate(cd):
                if len(elem) > len(prefix) and elem[ :len(prefix)] == prefix:
                    del cd[ eidx]
                    found = True
                    break
            if found:
                continue
        if nam in cd:
            del cd[ cd.index( nam)]
        else:
            return False
    else:
        return False
    return True

# param sentences NlpToken[][]
# return Subject[]
def getDocumentSubjects( title, sentences):
    rt = {}
    endtitle = title.find('(')
    if endtitle >= 0:
        titlesubject = title[ :endtitle ].split()
    else:
        titlesubject = title
    print( "TITLE SUBJECT %s\n" % titlesubject)
    sidx = 0
    lastSentSubjects = []
    for sent in sentences:
        lastSubject = None
        sentSubjects = []
        print( "SENT:\n%s" % printSentence( sent))
        for tok in sent:
            print( "TOK %s ROLE %s NLP %s" % (tok.value, tok.strusrole, tok.nlprole))
            if tok.strusrole == 'S':
                if lastSubject:
                    sentSubjects.append( lastSubject)
                lastSubject = [tok.value]
            elif tok.strusrole == '_' and lastSubject:
                lastSubject.append( tok.value)
            elif lastSubject:
                sentSubjects.append( lastSubject)
                lastSubject = None
            if tok.nlptag[0:3] == "PRP":
                print( "PRP %s" % tok.value)
                key = ""
                for lss in lastSentSubjects:
                    if key:
                        key += ","
                    key += '_'.join( lss)
                if key in rt:
                    rt[ key].prp.append( tok.value)
                else:
                    rt[ key] = Subject( lastSentSubjects, [], 1, None)
        if lastSubject:
            sentSubjects.append( lastSubject)
        lastSentSubjects = sentSubjects
    for rtelem in rt:
        print( "%s -> %s" % (rtelem, printSubject( rt[rtelem])))
    return rt

def tagDocument( title, text):
    rt = ""
    sentences = getDocumentSentences( text)
    subjects = getDocumentSubjects( title, sentences)
    for sent in sentences:
        rt += printSentence( sent)
    for subj in subjects:
        rt += ("SUBJ %s -> %s\n" % (subj, printSubject( subjects[ subj])))
    return rt

def substFilename( filename):
    rt = ""
    fidx = 0
    while fidx < len(filename):
        if filename[ fidx] == '%':
            rt += chr( int( filename[fidx+1:fidx+3], 16))
            fidx += 3
        else:
            rt += filename[ fidx]
            fidx += 1
    return rt

def getTitleFromFileName( filename):
    base = os.path.basename( filename)
    fnam = os.path.splitext( base)[0]
    return substFilename( fnam)

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
                title = getTitleFromFileName( filename)
                sys.stderr.write( "title %s\n", title)
                result += tagDocument( title, content)
                printOutput( filename, result)
                result = ""
            filename = line[6:]
        else:
            content += line
    if content:
        title = getTitleFromFileName( filename)
        result += tagDocument( title, content)
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








