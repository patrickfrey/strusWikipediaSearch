#!/usr/bin/python3
import tornado.ioloop
import tornado.web
import tornado.gen
import tornado.iostream
import os
import sys
import struct
import collections
import optparse
import strusMessage
import binascii
import time
import strus
import numbers

# server:
global serverno
serverno = 1
global debugtrace
debugtrace = False

# strus context:
global strusctx
strusctx = strus.Context()
# vector retrieval engine:
global vecstorage
vecstorage = None
global vecsearcher
vecsearcher = None

# Strus client connection factory:
msgclient = strusMessage.RequestClient()

# Query analyzer structures:
strusctx.addResourcePath( "./resources")
strusctx.loadModule( "analyzer_pattern");
strusctx.loadModule( "storage_vector_std");
AnalyzerTerm = collections.namedtuple('AnalyzerTerm', ['type','value','length'])

analyzer = strusctx.createQueryAnalyzer()
analyzer.addElement(
        "stem", "text", "word", 
        ["lc", ["dictmap", "irregular_verbs_en.txt"], ["stem", "en"], ["convdia", "en"], "lc"]
    )
analyzer.addElement(
        "selstem", "seltext", "word", 
        ["lc", ["dictmap", "irregular_verbs_en.txt"], ["stem", "en"], ["convdia", "en"], "lc"]
    )

analyzer.addPatternLexem( "lexem", "text", "word", ["lc"] )
analyzer.addPatternLexem( "lexem", "text", ["regex", "[,]"], ["orig"] )
analyzer.definePatternMatcherPostProcFromFile( "vecsfeat", "std", "pattern_searchfeat_qry.bin" )
analyzer.addElementFromPatternMatch( "vecsfeat", "vecsfeat", [] )
analyzer.declareElementPriority( "vecsfeat", 1)

RelatedTerm  = collections.namedtuple('RelatedTerm', ['value', 'index', 'weight'])

# Server callback function that intepretes the client message sent, executes the command and packs the result for the client
@tornado.gen.coroutine
def processCommand( message):
    rt = bytearray(b"Y")
    try:
        messagesize = len(message)
        if messagesize < 1:
            raise tornado.gen.Return( b"Eempty request string")
        messageofs = 1
        if message[0] == ord('Q'):
            # Build query to evaluate from the request:
            while (messageofs < messagesize):
                if (message[ messageofs] == ord('N')):
                    (nofranks,) = struct.unpack_from( ">H", message, messageofs+1)
                    messageofs += struct.calcsize( ">H") + 1
                elif (message[ messageofs] == ord('X')):
                    (querystr,messageofs) = strusMessage.unpackString( message, messageofs+1)
                else:
                    raise tornado.gen.Return( b"Eunknown parameter")

            # Analyze query:
            relatedlist = []
            terms = analyzer.analyzeTermExpression( [["text", querystr], ["seltext", querystr]])

            # Extract vectors referenced:
            f_indices = []
            for term in terms:
                if term.value[0] == 'F':
                    f_indices.append( int( term.value[1:]))

            # Build real list of features for retrieval in the searchindex:
            pos2term = {}
            pos = 0
            for term in terms:
                if term.type != "selstem":
                    if term.length and term.length > 1:
                        pos2term[ pos] = AnalyzerTerm( term.type, term.value, term.length)
                        pos += term.length
                    elif term.type == "stem":
                        pos2term[ pos] = AnalyzerTerm( term.type, term.value, 1)
                        pos += 1
            pos = 0
            for term in terms:
                if term.type == "selstem":
                    if not pos in pos2term:
                        pos2term[ pos] = AnalyzerTerm( "stem", term.value, 1)
                    pos += 1
            finalterms = []
            for pos, term in pos2term.items():
                finalterms.append( term)
            terms = finalterms

            # Calculate nearest neighbours of vectors exctracted:
            if f_indices:
                vec = vecstorage.featureVector( f_indices[0])
                if len( f_indices) > 1:
                    for nextidx in f_indices[1:]:
                        vec = [v + i for v, i in zip(vec, vecstorage.featureVector( nextidx))]
                    neighbour_ranklist = vecsearcher.findSimilar( vec, nofranks)
                else:
                    neighbour_list = []
                    neighbour_set = set()
                    for concept in vecstorage.featureConcepts( "", f_indices[0]):
                        for neighbour in vecstorage.conceptFeatures( "", concept):
                            neighbour_set.add( neighbour)
                    for neighbour in neighbour_set:
                        neighbour_list.append( neighbour)
                    neighbour_ranklist = vecsearcher.findSimilarFromSelection( neighbour_list, vec, nofranks)

                for neighbour in neighbour_ranklist:
                    fname = vecstorage.featureName( neighbour.index)
                    relatedlist.append( RelatedTerm( fname, neighbour.index, neighbour.weight))

            # Build the result and pack it into the reply message for the client:
            for termidx,term in enumerate(terms):
                rt.extend( b'T')
                rt.extend( b'T')
                rt.extend( strusMessage.packString( term.type))
                rt.extend( b'V')
                rt.extend( strusMessage.packString( term.value))
                if (term.length):
                    rt.extend( b'L')
                    rt.extend( struct.pack( ">I", term.length))
                rt.extend( b'_')
            for related in relatedlist:
                rt.extend( b'R')
                rt.extend( b'V')
                rt.extend( strusMessage.packString( related.value))
                rt.extend( b'I')
                rt.extend( struct.pack( ">I", related.index))
                rt.extend( b'W')
                rt.extend( struct.pack( ">d", related.weight))
                rt.extend( b'_')
        else:
            raise Exception( "unknown protocol command '%c'" % (message[0]))
    except Exception as e:
        raise tornado.gen.Return( bytearray( "E%s" % e, 'utf-8'))
    raise tornado.gen.Return( rt)

# Shutdown function:
def processShutdown():
    pass

# Server main:
if __name__ == "__main__":
    try:
        # Parse arguments:
        defaultconfig = "path=storage; cache=512M"
        parser = optparse.OptionParser()
        parser.add_option("-p", "--port", dest="port", default=7182,
                          help="Specify the port of this server as PORT (default %u)" % 7182,
                          metavar="PORT")
        parser.add_option("-c", "--config", dest="config", default=defaultconfig,
                          help="Specify the storage config as CONF (default '%s')" % defaultconfig,
                          metavar="CONF")
        parser.add_option("-i", "--serverno", dest="serverno", default=serverno,
                          help="Specify the number of the storage node as NO (default %s)" % serverno,
                          metavar="NO")
        parser.add_option("-G", "--debug", action="store_true", dest="do_debugtrace", default=False,
                          help="Tell the node to print some messages for tracing what it does")
        (options, args) = parser.parse_args()
        if len(args) > 0:
            parser.error("no arguments expected")
            parser.print_help()

        myport = int(options.port)
        debugtrace = options.do_debugtrace
        serverno = int( options.serverno)
        config = options.config

        vecstorage = strusctx.createVectorStorageClient( config )
        vecsearcher = vecstorage.createSearcher( 0, vecstorage.nofFeatures() )

        # Start server:
        print( "Starting server ...")
        server = strusMessage.RequestServer( processCommand, processShutdown)
        server.start( myport)
        print( "Terminated")
    except Exception as e:
        print( e)


