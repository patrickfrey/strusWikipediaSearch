#!/usr/bin/python

import tornado.ioloop
import tornado.web
import os
import sys
import time
import strusIR

backend = strusIR.Backend( "path=storage; cache=512M")

class MainHandler(tornado.web.RequestHandler):
	def get(self):
		self.write("Hello, world")

class QueryHandler(tornado.web.RequestHandler):
	def get(self):
		try:
			querystr = self.get_argument("q", None)
			firstrank = int( self.get_argument("i", 0))
			nofranks = int( self.get_argument("n", 20))
			scheme = self.get_argument("s", "BM25")
			starttime = time.clock()
			results = backend.evaluateQuery( querystr, firstrank, nofranks)
			endtime = time.clock()
			self.render("search.html.tpl", scheme=scheme, querystr=querystr, firstrank=firstrank, nofranks=nofranks, results=results, exectime=(endtime-starttime))
		except Exception as e:
			self.render("search_error.html.tpl", message=e, scheme=scheme, querystr=querystr, firstrank=firstrank, nofranks=nofranks)

application = tornado.web.Application([
	(r"/", MainHandler),
	(r"/query", QueryHandler),
	(r"/static/(.*)",tornado.web.StaticFileHandler, {"path": os.path.dirname(os.path.realpath(sys.argv[0]))},)
])


if __name__ == "__main__":
	try:
		print( "Starting server ...\n");
		application.listen(8080)
		print( "Listening on port 8080\n");
		tornado.ioloop.IOLoop.current().start()
		print( "Terminated\n");
	except Exception as e:
		print( e);



