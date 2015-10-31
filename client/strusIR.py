import strus
import itertools

class Backend:
	def __init__(self, storageconfig):
		self.context = strus.Context()
		self.context.addResourcePath("./resources")
		self.storage = self.context.createStorageClient( storageconfig )
		self.analyzer = self.context.createQueryAnalyzer()
		self.queryeval = self.context.createQueryEval()
		self.analyzer.definePhraseType(
			"text", "stem", "word", 
			["lc", ["dictmap", "irregular_verbs_en.txt"], ["stem", "en"], ["convdia", "en"], "lc"]
		)
		self.queryeval.addTerm( "sentence", "sent", "")
		self.queryeval.addSelectionFeature( "selfeat")

		self.queryeval.addWeightingFunction(
				1.0, "BM25", {
					"k1": 0.75, "b": 2.1, "avgdoclen": 500,
					".match": "docfeat"
				})
		self.queryeval.addSummarizer(
				"LINK", "matchvariables", {
					".match": "sumfeat",
					"assign": "",
					"type": "linkid"
				})
		self.queryeval.addSummarizer(
				"TITLE", "attribute", {
					"name": "title"
				})
		self.queryeval.addSummarizer(
				"CONTENT", "matchphrase", {
					"type": "orig", "len": 40, "nof": 3, "structseek": 30, "mark": '<b>$</b>',
					".struct": "sentence", ".match": "sumfeat"
				})

	def evaluateQueryText( self, querystr, firstrank, nofranks):
		query = self.queryeval.createQuery( self.storage)
		terms = self.analyzer.analyzePhrase( "text", querystr)

		if len( terms) > 0:
			for term in terms:
				query.pushTerm( term.type(), term.value())
				query.pushDuplicate()
				query.defineFeature( "docfeat")

			query.pushExpression( "contains", len(terms))
			query.defineFeature( "selfeat")

		query.setMaxNofRanks( nofranks)
		query.setMinRank( firstrank)
		return query.evaluate()

	def evaluateQueryExtractLinks( self, querystr, firstrank, nofranks):
		query = self.queryeval.createQuery( self.storage)
		terms = self.analyzer.analyzePhrase( "text", querystr)

		if len( terms) > 0:
			if len( terms) == 0:
				return []

			elif len( terms) > 1:
				# Iterate on all permutation pairs of query features and create
				# combined features for retrieval and summarization:
				# Note: The comments with prefix [STK]: show the expected state of the stack
				#	(V=term value,T=sentence delimiter,E=expression,L=link matcher)
				for pair in itertools.permutations(
						itertools.takewhile(
							lambda x: x<terms.size(), itertools.count()), 2):
					if pair[0] + 1 == pair[1]:
						# ... A sequence of terms is translated into three query expressions:
						#	a) search for sequence inside a sentence in documents, weight 6.0.
						#		The summarizer extracts links within a distance of 10 in the same sentence
						#	b) search for the terms in a distance <= 5 inside a sentence, weight 4.0.
						#		The summarizer extracts links within a distance of 20 in the same sentence
						#	c) search for the terms in a distance <= 100 inside a sentence, weight 2.0.
						#		The summarizer extracts links within a distance of 100 in the same sentence
						query.pushTerm( "sent", "");
						# [STK]: T 
						query.pushTerm( terms[pair[0]].type(), terms[pair[0]].value())
						# [STK]: T V
						query.pushTerm( terms[pair[1]].type(), terms[pair[1]].value())
						# [STK]: T V V
						query.pushDuplicate( 3)
						# [STK]: T V V T V V
						query.pushDuplicate( 3)
						# [STK]: T V V T V V T V V
						query.pushExpression( "sequence_struct", 3, 3)
						# [STK]: T V V T V V T V V E
						query.swapElement( 6)
						# [STK]: E T V V T V V
						query.pushExpression( "within_struct", 3, 5)
						# [STK]: E T V V E
						query.swapElement( 3)
						# [STK]: E E T V V 
						query.pushExpression( "within_struct", 3, 100)
						# [STK]: Es Ew Ew
						weight   = [ 6.0, 4.0, 2.0 ]
						linkdist = [  10,  20, 100 ]
						ii = 0
						while ii < 3:
							query.swapElements( 3 - ii)
							# [STK]: ~ E
							query.pushTerm( "sent", "")
							# [STK]: ~ E T
							query.swapElements( 1)
							# [STK]: ~ T E
							query.pushDuplicate()
							# [STK]: ~ T E E
							query.defineFeature( "docfeat", weight[ ii])
							# [STK]: ~ T E
							query.pushTerm( "linkvar", "");
							# [STK]: ~ T E L
							query.attachVariable( "LINK");
							query.swapElements( 1)
							# [STK]: ~ T L E
							query.pushExpression( "within_struct", 3, linkdist[ ii])
							# [STK]: ~ E
							query.defineFeature( "sumfeat", weight[ ii])
							# [STK]: ~ E
							++ii
					else:
						# ... An arbitrary combination of terms is translated into two query expressions:
						#	b) search for the terms in a distance <= 5 inside a sentence, weight 1.0+2.0/d^2,
						#		where d ist the distance of the terms in the query.
						#		The summarizer extracts links within a distance of 20 in the same sentence
						#	c) search for the terms in a distance <= 100 inside a sentence, weight 0.5+1.0/d^2,
						#		where d ist the distance of the terms in the query.
						#		The summarizer extracts links within a distance of 100 in the same sentence
						query.pushTerm( "sent", "");
						# [STK]: T
						query.pushTerm( terms[pair[0]].type(), terms[pair[0]].value())
						# [STK]: T V
						query.pushTerm( terms[pair[1]].type(), terms[pair[1]].value())
						# [STK]: T V V
						dist = pair[1] - pair[0];
						query.pushDuplicate( 3)
						# [STK]: T V V T V V
				 		query.pushExpression( "within_struct", 3, 5)
						# [STK]: T V V E
						query.swapElement( 3)
						# [STK]: E T V V
						query.pushExpression( "within_struct", 3, 100)
						# [STK]: E E
						# [STK]: E E
						weight   = [ 2.0, 1.0 ]
						linkdist = [  20, 100 ]
						ii = 0
						while ii < 2:
							query.swapElements( 1 - ii)
							# [STK]: ~ E
							query.pushTerm( "sent", "")
							# [STK]: ~ E T
							query.swapElements( 1)
							# [STK]: ~ T E
							query.pushDuplicate()
							# [STK]: ~ T E E
							proxweight = weight[ii]/2 + weight[ii]/(dist*dist)
							query.defineFeature( "docfeat", proxweight)
							# [STK]: ~ T E
							query.pushTerm( "linkvar", "")
							# [STK]: ~ T E L
							query.attachVariable( "LINK") # => link to extract
							query.swapElements( 1)
							# [STK]: ~ T L E
							query.pushExpression( "within_struct", 3, linkdist[ ii])
							# [STK]: ~ E
							query.defineFeature( "sumfeat", proxweight)
							# [STK]: ~
							++ii
			else:
				# Only a single term in the query, weight 1.0
				#	The summarizer extracts links within a distance of 100 in the same sentence
				query.pushTerm( "sent", "")
				# [STK]: T
				query.pushTerm( term[0].type(), term[0].value())
				# [STK]: T V
				query.pushDuplicate()
				# [STK]: T V V
				query.defineFeature( "docfeat", 1.0)
				# [STK]: T V
				query.pushTerm( "linkvar", "")
				query.attachVariable( "LINK")
				# [STK]: T V L
				query.swapElements( 1)
				# [STK]: T L V
				query.pushExpression( "within_struct", 3, 100)
				# [STK]: E
				query.defineFeature( "sumfeat", 1.0)
				# [STK]:

			# Define the selector as all fetures must be in a candidate document:
			for term in terms:
				query.pushTerm( term.type(), term.value())
			query.pushExpression( "contains", len(terms))
			query.defineFeature( "selfeat")

		query.setMaxNofRanks( nofranks)
		query.setMinRank( firstrank)
		return query.evaluate()
	

