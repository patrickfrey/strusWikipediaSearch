#!/usr/bin/python
import tornado.ioloop
import tornado.gen
import tornado.tcpclient
import tornado.tcpserver
import signal
import os
import sys
import struct
import binascii

class TcpConnection( object):
    def __init__(self, stream, command_callback):
        self.stream = stream
        self.command_callback = command_callback

    @tornado.gen.coroutine
    def on_connect(self):
        try:
            while (True):
                msgsizemsg = yield self.stream.read_bytes( struct.calcsize(">I"))
                (msgsize,) = struct.unpack( ">I", msgsizemsg)
                msg = yield self.stream.read_bytes( msgsize)
                reply = yield self.command_callback( msg)
                yield self.stream.write( struct.pack( ">I", len(reply)) + bytes(reply));
        except tornado.iostream.StreamClosedError:
            pass

class RequestServer( tornado.tcpserver.TCPServer):
    def __init__(self, command_callback, shutdown_callback):
        tornado.tcpserver.TCPServer.__init__(self)
        self.command_callback = command_callback
        self.shutdown_callback = shutdown_callback
        self.io_loop = tornado.ioloop.IOLoop.current()

    def do_shutdown( self, signum, frame):
        print('Shutting down')
        self.shutdown_callback()
        self.io_loop.stop()

    @tornado.gen.coroutine
    def handle_stream( self, stream, address):
        connection = TcpConnection( stream, self.command_callback)
        yield connection.on_connect()

    def start( self, port):
        host = "0.0.0.0"
        self.listen( port, host)
        print("Listening on %s:%d..." % (host, port))
        signal.signal( signal.SIGINT, self.do_shutdown)
        self.io_loop.start()


class RequestClient( tornado.tcpclient.TCPClient):
    @tornado.gen.coroutine
    def issueRequest( self, stream, msg):
        blob = struct.pack( ">I", len(msg)) + bytes(msg)
        stream.write( blob);
        replysizemsg = yield stream.read_bytes( struct.calcsize(">I"))
        (replysize,) = struct.unpack( ">I", replysizemsg)
        reply = yield stream.read_bytes( replysize)
        raise tornado.gen.Return( reply)



