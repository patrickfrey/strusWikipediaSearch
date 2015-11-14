import strus
import itertools
import heapq

class Backend:
	def createQueryEvalBM25(self):
		# Create a simple BM25 query evaluation scheme with fixed 
		# a,b,k1 and avg document lenght and title with abstract 
		# as summarization attributes:
		rt = self.context.createQueryEval()
		rt.addTerm( "sentence", "sent", "")
		rt.addSelectionFeature( "selfeat")

		# Query evaluation scheme:
		rt.addWeightingFunction(
			1.0, "BM25", {
				"k1": 0.75, "b": 2.1, "avgdoclen": 500,
				".match": "docfeat"
			})
		# Summarizer for getting the document title:
		rt.addSummarizer(
			"TITLE", "attribute", {
				"name": "title"
			})
		# Summarizer for abstracting:
		rt.addSummarizer(
			"CONTENT", "matchphrase", {
				"type": "orig", "len": 40, "nof": 3,
				"structseek": 30, "mark": '<b>$</b>',
				".struct": "sentence", ".match": "docfeat"
			})
		return rt

	def createQueryEvalNBLNK(self):
		# Create a simple BM25 query evaluation scheme with fixed
		# a,b,k1 and avg document lenght and the weighted extracted
		# links close to matches as query evaluation result:
		rt = self.context.createQueryEval()
		rt.addTerm( "sentence", "sent", "")
		rt.addSelectionFeature( "selfeat")
		
		# Query evaluation scheme for link extraction candidate selection:
		rt.addWeightingFunction(
			1.0, "BM25", {
				"k1": 0.75, "b": 2.1, "avgdoclen": 500,
				".match": "docfeat"
			})
		# Summarizer to extract the weighted links:
		rt.addSummarizer(
			"LINK", "accuvariable", {
				".match": "sumfeat",
				"var": "LINK",
				"type": "linkid"
			})
		return rt

	def __init__(self, storageconfig):
		# self.context = strus.Context()
		self.context = strus.Context("localhost:7181")
		# self.context.addResourcePath("./resources")
		# self.storage = self.context.createStorageClient( storageconfig )
		self.storage = self.context.createStorageClient()
		self.analyzer = self.context.createQueryAnalyzer()
		# Query phrase type definition according to document analyzer
		# configuration:
		self.analyzer.definePhraseType(
			"text", "stem", "word", 
			["lc", ["dictmap", "irregular_verbs_en.txt"],
				["stem", "en"], ["convdia", "en"], "lc"]
		)
		self.queryeval = {}
		self.queryeval["BM25"] = self.createQueryEvalBM25()
		self.queryeval["NBLNK"] = self.createQueryEvalNBLNK()

	# Query evaluation method for a classical information retrieval query with BM25:
	def evaluateQueryText( self, querystr, firstrank, nofranks):
		queryeval = self.queryeval[ "BM25"]
		query = queryeval.createQuery( self.storage)
		terms = self.analyzer.analyzePhrase( "text", querystr)

		if len( terms) > 0:
			selexpr = ["contains"]
			for term in terms:
				selexpr.append( [term.type(), term.value()] )
				query.defineFeature( "docfeat", [term.type(), term.value()], 1.0)

			query.defineFeature( "selfeat", selexpr, 1.0 )

		query.setMaxNofRanks( nofranks)
		query.setMinRank( firstrank)
		return query.evaluate()

	# Helper method to define the query features created from terms 
	# of the query string, that are neighbours in the query:
	def __defineSubsequentQueryTermFeatures( self, query, term1, term2, order):
		# Pairs of terms appearing subsequently in the query are 
		# translated into 3 query expressions:
		#	1) search for sequence inside a sentence in documents,
		#		weight 5.5 (reverse order) or 7.0 (same order).
		#		The summarizer extracts links within 
		#		a distance of 50 in the same sentence
		#	2) search for the terms in a distance <= 5 inside
		#		a sentence, weight 4.0.
		#		The summarizer extracts links within a distance
		#		of 50 in the same sentence
		#	3) search for the terms in a distance <= 20 inside 
		#		a sentence, weight 2.0.
		#		The summarizer extracts links within
		#		a distance of 50 in the same sentence
		expr = [
				[ "sequence_struct", 3,
					["sent"],
					[term1.type(), term1.value()],
					[term2.type(), term2.value()]
				], 
				[ "within_struct", 5,
					["sent"],
					[term1.type(), term1.value()],
					[term2.type(), term2.value()]
				],
				[ "within_struct", 20,
					["sent"],
					[term1.type(), term1.value()],
					[term2.type(), term2.value()]
				]
		]
		if order == 1:
			weight = [ 3.0, 2.0, 1.5 ]
		else:
			weight = [ 2.7, 1.8, 1.2 ]

		ii = 0
		while ii < 3:
			# The summarization expression attaches a variable 
			# LINK ("=LINK") to links (terms of type 'linkvar'):
			sumexpr = [ "within_struct", 50, ["sent"],
					["=LINK", "linkvar"], expr[ ii] ]
			query.defineFeature( "docfeat", expr[ ii], weight[ ii] )
			query.defineFeature( "sumfeat", sumexpr, weight[ ii] )
			ii += 1

	# Helper method to define the query features created from terms 
	# of the query string, that are not neighbours in the query:
	def __defineNonSubsequentQueryTermFeatures( self, query, term1, term2):
		# Pairs of terms not appearing subsequently in the query are 
		# translated into two query expressions:
		#	1) search for the terms in a distance <= 5 inside
		#		a sentence, weight 2.5,
		#		where d ist the distance of the terms in the query.
		#		The summarizer extracts links within a distance
		#		of 50 in the same sentence
		#	2) search for the terms in a distance <= 100 inside
		#		a sentence, weight 1.0,
		#		where d ist the distance of the terms in the query.
		#		The summarizer extracts links within a distance
		#		of 50 in the same sentence
		expr = [
				[ "within_struct", 5,
					["sent"],
					[term1.type(), term1.value()],
					[term2.type(), term2.value()]
				],
				[ "within_struct", 20,
					["sent"],
					[term1.type(), term1.value()],
					[term2.type(), term2.value()]
				]
		]
		weight = [ 1.6, 1.2 ]
		ii = 0
		while ii < 2:
			# The summarization expression attaches a variable 
			# LINK ("=LINK") to links (terms of type 'linkvar'):
			sumexpr = [ "within_struct", 50, ["sent"],
					["=LINK", "linkvar"], expr[ ii] ]
			query.defineFeature( "docfeat", expr[ ii], weight[ ii] )
			query.defineFeature( "sumfeat", sumexpr, weight[ ii] )
			ii += 1

	# Helper method to define the query features created from a single
	# term query:
	def __defineSingleTermQueryFeatures( self, query, term):
		# Single term query:
		expr = [ term.type(), term.value() ]
		# The summarization expression attaches a variable 
		# LINK ("=LINK") to links (terms of type 'linkvar'):
		sumexpr = [ "within_struct", 50, ["sent"],
				["=LINK", "linkvar"], expr ]
		query.defineFeature( "docfeat", expr, 1.0 )
		query.defineFeature( "sumfeat", sumexpr, 1.0 )

	def evaluateQueryLinks( self, querystr, firstrank, nofranks):
		queryeval = self.queryeval[ "NBLNK"]
		query = queryeval.createQuery( self.storage)
		terms = self.analyzer.analyzePhrase( "text", querystr)

		if len( terms) == 0:
			# Return empty result for empty query:
			return []

		# Build the weighting features. Queries with more than one term are building
		# the query features from pairs of terms:
		if len( terms) > 1:
			# Iterate on all permutation pairs of query features and create
			# combined features for retrieval and summarization:
			for pair in itertools.permutations(
					itertools.takewhile(
						lambda x: x<len(terms), itertools.count()), 2):
				if pair[0] + 1 == pair[1]:
					self.__defineSubsequentQueryTermFeatures(
						query, terms[pair[0]], terms[pair[1]], +1 )
				elif pair[1] + 1 == pair[0]:
					self.__defineSubsequentQueryTermFeatures(
						query, terms[pair[0]], terms[pair[1]], -1 )
				else:
					self.__defineNonSubsequentQueryTermFeatures(
						query, terms[pair[0]], terms[pair[1]] )
		else:
			self.__defineSingleTermQueryFeatures( query, terms[0] )

		# Define the selector as the set of documents that contain all query terms:
		selexpr = ["contains"]
		for term in terms:
			selexpr.append( [term.type(), term.value()] )
		query.defineFeature( "selfeat", selexpr, 1.0 )

		# Evaluate the ranked list for getting the documents to inspect for links close to matches:
		query.setMaxNofRanks( (firstrank + nofranks) * 50 + 50)
		query.setMinRank( 0)
		results = query.evaluate()

		# Build the table of all links with weight of the top ranked documents:
		linktab = {}
		for result in results:
			for attribute in result.attributes():
				if attribute.name() == 'LINK':
					weight = 0.0
					if attribute.value() in linktab:
						weight = linktab[ attribute.value()]
					linktab[ attribute.value()] = weight + attribute.weight()

		# Extract the topmost weighted documents in the linktable as result:
		heap = []
		for key, value in linktab.iteritems():
			heapq.heappush( heap, {'link':key, 'weight':value})
		toplinks = heapq.nlargest( firstrank + nofranks, heap, lambda k: k['weight'])
		rt = []
		idx = 0
		maxrank = firstrank + nofranks
		for elem in toplinks[firstrank:maxrank]:
			rt.append( {'title':elem['link'], 'weight':elem['weight']})
		return rt


