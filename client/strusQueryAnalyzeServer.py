#!/usr/bin/python
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

analyzer = strusctx.createQueryAnalyzer()
analyzer.addSearchIndexElement(
        "stem", "text", "word", 
        ["lc", ["dictmap", "irregular_verbs_en.txt"], ["stem", "en"], ["convdia", "en"], "lc"]
    )
analyzer.addPatternLexem( "lexem", "text", "word", ["lc"] )
analyzer.addPatternLexem( "punct", "text", ["regex", "[,]"], ["lc"] )
analyzer.definePatternMatcherPostProcFromFile( "vecsfeat", "std", "pattern_searchfeat_qry.bin" )
analyzer.addSearchIndexElementFromPatternMatch( "vecsfeat", "vecsfeat", [] )


# Pack a message with its length (processCommand protocol)
def packMessage( msg):
    return struct.pack( ">H%ds" % len(msg), len(msg), msg)

def unpackMessage( msg, msgofs):
    (strsize,) = struct.unpack_from( ">H", msg, msgofs)
    msgofs += struct.calcsize( ">H")
    (str,) = struct.unpack_from( "%ds" % (strsize), msg, msgofs)
    msgofs += strsize
    return [str,msgofs]

RelatedTerm  = collections.namedtuple('RelatedTerm', ['value', 'index', 'weight'])

# Server callback function that intepretes the client message sent, executes the command and packs the result for the client
@tornado.gen.coroutine
def processCommand( message):
    rt = bytearray(b"Y")
    try:
        messagesize = len(message)
        messageofs = 1
        if message[0] == 'Q':
            # QUERY:
            terms = []
            # Build query to evaluate from the request:
            messagesize = len(message)
            while (messageofs < messagesize):
                if (message[ messageofs] == 'N'):
                    (nofranks,) = struct.unpack_from( ">H", message, messageofs+1)
                    messageofs += struct.calcsize( ">H") + 1
                if (message[ messageofs] == 'X'):
                    (querystr,messageofs) = unpackMessage( message, messageofs+1)
                else:
                    raise tornado.gen.Return( b"Eunknown parameter")

            # Analyze query:
            relatedlist = []
            terms = analyzer.analyzeField( "text", querystr)
            f_indices = []
            for term in terms:
                if term.type == "vecsfeat" and term.value[0] == 'F':
                    f_indices.append( int( term.value[1:]))
            # Calculate nearest neighbours:
            if len( f_indices) > 0:
                if len( f_indices) > 1:
                    vec = vecstorage.featureVector( f_indices[0])
                    for nextvec in f_indices[1:]:
                        vec = map( double.__add__, vec, nextvec)
                    neighbour_set = vecsearcher.findSimilar( vec, nofranks)
                    for neighbour in neighbour_set:
                        fname = vecstorage.featureName( neighbour)
                        relatedlist.append( RelatedTerm( fname, neighbour, 0.0 ))
                else:
                    neighbour_set = set()
                    for concept in vecstorage.featureConcepts( "", f_indices[0]):
                        for neighbour in vecstorage.conceptFeatures( "", concept):
                            neighbour_set.append( neighbour)
                    for neighbour in neighbour_set:
                        fname = vecstorage.featureName( neighbour)
                        relatedlist.append( RelatedTerm( fname, neighbour, 0.0 ))

            # Build the result and pack it into the reply message for the client:
            for term in terms:
                rt.append( 'T')
                rt.append( 'T')
                rt += packMessage( term.type())
                rt.append( 'V')
                rt += packMessage( term.value())
                rt.append( 'P')
                rt += struct.pack( ">I", term.pos())
                rt.append( 'W')
                rt += struct.pack( ">f", term.weight())
                rt.append( '_')
            for related in relatedlist:
                rt.append( 'R')
                rt.append( 'V')
                rt += packMessage( related.value)
                rt.append( 'I')
                rt += struct.pack( ">I", related.index)
                rt.append( 'W')
                rt += struct.pack( ">f", related.weight)
                rt.append( '_')
        else:
            raise Exception( "unknown protocol command '%c'" % (message[0]))
    except Exception as e:
        raise tornado.gen.Return( bytearray( b"E" + str(e)) )
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

        vecstorage = strusctx.createStorageClient( config )
        vecsearcher = vecstorage.createSearcher( 0, vecstorage.nofFeatures() )

        # Start server:
        print( "Starting server ...")
        server = strusMessage.RequestServer( processCommand, processShutdown)
        server.start( myport)
        print( "Terminated")
    except Exception as e:
        print( e)


