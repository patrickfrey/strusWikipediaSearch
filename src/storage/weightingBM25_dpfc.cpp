/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "weightingBM25_dpfc.hpp"
#include "fixedSizeStructAllocator.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <ctime>
#include <set>
#include <limits>
#include <cstdarg>

using namespace strus;

// Internationalization (dummy)
#define _TXT(A) A

#define CATCH_ERROR_ARG1_MAP_RETURN( MSG, ARG, HND, VALUE)\
	catch( const std::bad_alloc&)\
	{\
		(HND).report( "out of memory in wikipedia tokenizer");\
		return VALUE;\
	}\
	catch( const std::runtime_error& err)\
	{\
		(HND).report( MSG, ARG, err.what());\
		return VALUE;\
	}\

#define CATCH_ERROR_ARG1_MAP( MSG, ARG, HND)\
	catch( const std::bad_alloc&)\
	{\
		(HND).report( "out of memory in wikipedia tokenizer");\
	}\
	catch( const std::runtime_error& err)\
	{\
		(HND).report( MSG, ARG, err.what());\
	}\

namespace strus
{
std::runtime_error runtime_error( const char* format, ...)
{
	char buffer[ 1024];
	va_list args;
	va_start( args, format);
	int buffersize = vsnprintf( buffer, sizeof(buffer), format, args);
	buffer[ sizeof(buffer)-1] = 0;
	std::runtime_error rt( std::string( buffer, buffersize));
	va_end (args);
	return rt;
}
}

WeightingFunctionContextBM25_dpfc::WeightingFunctionContextBM25_dpfc(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		double k1_,
		double b_,
		double avgDocLength_,
		double ffpart_,
		double nofCollectionDocuments_,
		const std::string& attribute_content_doclen_,
		const std::string& attribute_title_doclen_,
		unsigned int proximityMinDist_,
		double title_ff_incr_,
		double sequence_ff_incr_,
		double sentence_ff_incr_,
		double relevant_df_factor_,
		ErrorBufferInterface* errorhnd_)
	:m_k1(k1_),m_b(b_),m_avgDocLength(avgDocLength_),m_ffpart(ffpart_)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_weight_featar()
	,m_struct_featar()
	,m_title_itr(0),m_metadata(metadata_)
	,m_metadata_content_doclen(metadata_->elementHandle( attribute_content_doclen_.empty()?std::string("doclen"):attribute_content_doclen_))
	,m_metadata_title_doclen(attribute_title_doclen_.size()?metadata_->elementHandle( attribute_title_doclen_):-1)
	,m_proximityMinDist(proximityMinDist_)
	,m_title_ff_incr(title_ff_incr_)
	,m_sequence_ff_incr(sequence_ff_incr_)
	,m_sentence_ff_incr(sentence_ff_incr_)
	,m_relevant_df_factor(relevant_df_factor_)
	,m_errorhnd(errorhnd_)
{
	if (m_metadata_content_doclen < 0)
	{
		throw strus::runtime_error( _TXT("no meta data element for the document lenght defined"));
	}
	if (m_metadata_title_doclen < 0 && !attribute_title_doclen_.empty())
	{
		throw strus::runtime_error( _TXT("unknown meta data element '%s' for the title lenght defined"), attribute_title_doclen_.c_str());
	}
	
}

void WeightingFunctionContextBM25_dpfc::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr_,
		double weight_,
		const TermStatistics& stats_)
{
	try
	{
		if (boost::algorithm::iequals( name_, "match"))
		{
			double nofMatches = stats_.documentFrequency()>=0?stats_.documentFrequency():(GlobalCounter)itr_->documentFrequency();
			double idf = 0.0;
			bool relevant = (m_nofCollectionDocuments * m_relevant_df_factor > nofMatches);
	
			if (m_nofCollectionDocuments > nofMatches * 2)
			{
				idf = logl(
						(m_nofCollectionDocuments - nofMatches + 0.5)
						/ (nofMatches + 0.5));
			}
			if (idf < 0.00001)
			{
				idf = 0.00001;
			}
			m_weight_featar.push_back( Feature( itr_, weight_, idf, relevant));
		}
		else if (boost::algorithm::iequals( name_, "struct"))
		{
			m_struct_featar.push_back( Feature( itr_, weight_, 0.0, false));
		}
		else if (boost::algorithm::iequals( name_, "title"))
		{
			if (m_title_itr) throw strus::runtime_error( _TXT( "duplicate '%s' weighting function feature parameter '%s'"), "BM25_dpfc", name_.c_str());
			m_title_itr = itr_;
		}
		else
		{
			throw strus::runtime_error( _TXT( "unknown '%s' weighting function feature parameter '%s'"), "BM25_dpfc", name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding weighting feature to '%s' weighting: %s"), "BM25_dpfc", *m_errorhnd);
}


struct FeatStruct
{
	Index itridx;
	Index pos;

	FeatStruct()
		:itridx(0),pos(0){}
	FeatStruct( const Index& itridx_, const Index& pos_)
		:itridx(itridx_),pos(pos_){}
	FeatStruct( const FeatStruct& o)
		:itridx(o.itridx),pos(o.pos){}

	bool operator<( const FeatStruct& o) const
	{
		if (pos == o.pos) return itridx < o.itridx;
		return (pos < o.pos);
	}
};

enum {MaxNofFeatures=256};
typedef std::set<
		FeatStruct,
		std::less<FeatStruct>,
		FixedSizeStructAllocator<FeatStruct,MaxNofFeatures> > FeatStructSet;


static void handleSequence( const FeatStructSet& wset, double* accu, double weight, const Index& docno)
{
	FeatStructSet::iterator first = wset.begin();
	FeatStructSet::iterator next = first; ++next;

	for (;next != wset.end() && next->pos == first->pos+1; ++next)
	{
		if (next->itridx == first->itridx+1)
		{
			accu[ first->itridx] += weight;
			accu[ next->itridx] += weight;
		}
		else if (next->itridx+1 == first->itridx)
		{
			accu[ first->itridx] += weight/2;
			accu[ next->itridx] += weight/2;
		}
	}
}

static Index handleSameSentence( std::vector<WeightingFunctionContextBM25_dpfc::Feature>& struct_featar, const FeatStructSet& wset, double* accu, const Index& startPos, double weight, const Index& docno)
{
	typedef WeightingFunctionContextBM25_dpfc::Feature Feature;
	std::vector<Feature>::iterator si = struct_featar.begin(), se = struct_featar.end();
	Index pos;
	Index endOfSentence = std::numeric_limits<Index>::max();
	for (; si != se; ++si)
	{
		pos = si->itr->skipPos( startPos);
		if (pos != 0 && pos < endOfSentence)
		{
			endOfSentence = pos;
		}
	}
	FeatStructSet::iterator first = wset.begin();
	FeatStructSet::iterator next = first; ++next;

	for (;next != wset.end() && next->pos < endOfSentence; ++next)
	{
		if (next->pos >= startPos)
		{
			accu[ first->itridx] += weight;
			accu[ next->itridx] += weight;
		}
	}
	return endOfSentence;
}


double WeightingFunctionContextBM25_dpfc::call( const Index& docno)
{
	double rt = 0.0;
	FeatStructSet wset;
	double accu[ MaxNofFeatures];

	if (m_weight_featar.size() > MaxNofFeatures || m_struct_featar.size() > MaxNofFeatures)
	{
		m_errorhnd->report( _TXT("to many features passed to weighting function '%s'"), "BM25_dpfc");
		return 0.0;
	}

	// Initialize accumulator for ff increments
	Index title_start = 1;
	Index title_end = 0;
	if (m_title_itr && docno == m_title_itr->skipDoc( docno))
	{
		title_start = m_title_itr->skipPos(0);
	}
	if (m_metadata_title_doclen >= 0)
	{
		title_end = title_start + (unsigned int)m_metadata->getValue( m_metadata_title_doclen);
	}
	std::vector<Feature>::const_iterator fi = m_weight_featar.begin(), fe = m_weight_featar.end();
	for (Index fidx=0; fi != fe; ++fi,++fidx)
	{
		accu[ fidx] = 0.0;
		if (docno == fi->itr->skipDoc( docno))
		{
			Index firstpos = fi->itr->skipPos(0);
			if (firstpos)
			{
				wset.insert( FeatStruct( fidx, firstpos));
			}
			if (firstpos >= title_start && firstpos <= title_end)
			{
				accu[ fidx] += m_title_ff_incr/m_metadata_title_doclen;
			}
		}
	}

	// Initialize structure elements
	std::vector<Feature>::const_iterator si = m_struct_featar.begin(), se = m_struct_featar.end();
	for (; si != se; ++si)
	{
		si->itr->skipDoc( docno);
	}

	// Calculate ff increments based on proximity criteria:
	while (wset.size() > 1)
	{
		// [0] Small optimization for not checkin elements with a too big distance (m_proximityMinDist) to others
		FeatStructSet::iterator first = wset.begin();
		FeatStructSet::iterator next = first; ++next;
		for (; next != wset.end(); ++next)
		{
			if (m_weight_featar[ next->itridx].relevant) break;
		}
		if (next == wset.end()) break;

		while (first != next && first->pos + (Index)m_proximityMinDist < next->pos)
		{
			FeatStruct elem = *wset.begin();
			wset.erase( wset.begin());
			elem.pos = m_weight_featar[ elem.itridx].itr->skipPos( next->pos-m_proximityMinDist);
			if (elem.pos)
			{
				wset.insert( elem);
			}
			first = wset.begin();
			continue;
		}
		// [1] Check sequence of subsequent elements in query:
		handleSequence( wset, accu, m_sequence_ff_incr, docno);

		// [2] Check elements in the same sentence:
		if (m_struct_featar.size() && m_sentence_ff_incr > std::numeric_limits<float>::epsilon())
		{
			Index nextSentence = handleSameSentence( m_struct_featar, wset, accu, first->pos, m_sentence_ff_incr, docno);
			if (nextSentence != std::numeric_limits<Index>::max())
			{
				nextSentence = handleSameSentence( m_struct_featar, wset, accu, nextSentence+1, m_sentence_ff_incr/2, docno);
				if (nextSentence != std::numeric_limits<Index>::max())
				{
					nextSentence = handleSameSentence( m_struct_featar, wset, accu, nextSentence+1, m_sentence_ff_incr/3, docno);
				}
			}
		}
		FeatStruct elem = *wset.begin();
		wset.erase( wset.begin());
		elem.pos = m_weight_featar[ elem.itridx].itr->skipPos( elem.pos+1);
		if (elem.pos)
		{
			wset.insert( elem);
		}
	}

	// Calculate BM25 taking proximity based ff increments into account
	fi = m_weight_featar.begin(), fe = m_weight_featar.end();
	for (Index fidx=0; fi != fe; ++fi,++fidx)
	{
		if (docno==fi->itr->skipDoc( docno))
		{
			m_metadata->skipDoc( docno);

			double ff = (fi->itr->frequency() * m_ffpart) + accu[ fidx];
			if (ff == 0.0)
			{
			}
			else if (m_b)
			{
				double doclen = m_metadata->getValue( m_metadata_content_doclen);
				double rel_doclen = (doclen+1) / m_avgDocLength;
				rt += fi->weight * fi->idf
					* (ff * (m_k1 + 1.0))
					/ (ff + m_k1 * (1.0 - m_b + m_b * rel_doclen));
			}
			else
			{
				rt += fi->weight * fi->idf
					* (ff * (m_k1 + 1.0))
					/ (ff + m_k1 * 1.0);
			}
		}
	}
	return rt;
}


static NumericVariant parameterValue( const std::string& name, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name.c_str(), value.c_str());
	return rt;
}

void WeightingFunctionInstanceBM25_dpfc::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (boost::algorithm::iequals( name, "match") || boost::algorithm::iequals( name, "struct"))
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name.c_str(), "BM25_dpfc");
		}
		else if (boost::algorithm::iequals( name, "doclen"))
		{
			m_attribute_content_doclen = value;
			if (value.empty())
			{
				m_errorhnd->report( _TXT("empty value passed as '%s' weighting function parameter '%s'"), "BM25_dpfc", name.c_str());
				return;
			}
		}
		else if (boost::algorithm::iequals( name, "doclen_title"))
		{
			m_attribute_title_doclen = value;
			if (value.empty())
			{
				m_errorhnd->report( _TXT("empty value passed as '%s' weighting function parameter '%s'"), "BM25_dpfc", name.c_str());
				return;
			}
		}
		else if (boost::algorithm::iequals( name, "k1")
		||  boost::algorithm::iequals( name, "b")
		||  boost::algorithm::iequals( name, "avgdoclen")
		||  boost::algorithm::iequals( name, "ffpart")
		||  boost::algorithm::iequals( name, "proxmindist")
		||  boost::algorithm::iequals( name, "titleinc")
		||  boost::algorithm::iequals( name, "seqinc")
		||  boost::algorithm::iequals( name, "strinc")
		||  boost::algorithm::iequals( name, "relevant"))
		{
			addNumericParameter( name, parameterValue( name, value));
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' textual weighting function parameter '%s'"), "BM25_dpfc", name.c_str());
			return;
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' weighting: %s"), "BM25_dpfc", *m_errorhnd);
}

void WeightingFunctionInstanceBM25_dpfc::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	if (boost::algorithm::iequals( name, "match") || boost::algorithm::iequals( name, "struct"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name.c_str(), "BM25_dpfc");
	}
	else if (boost::algorithm::iequals( name, "k1"))
	{
		m_k1 = (double)value;
	}
	else if (boost::algorithm::iequals( name, "b"))
	{
		m_b = (double)value;
	}
	else if (boost::algorithm::iequals( name, "avgdoclen"))
	{
		m_avgdoclen = (double)value;
	}
	else if (boost::algorithm::iequals( name, "ffpart"))
	{
		m_ffpart = (double)value;
	}
	else if (boost::algorithm::iequals( name, "proxmindist"))
	{
		m_proximityMinDist = (unsigned int)value;
	}
	else if (boost::algorithm::iequals( name, "titleinc"))
	{
		m_title_ff_incr = (double)value;
	}
	else if (boost::algorithm::iequals( name, "seqinc"))
	{
		m_sequence_ff_incr = (double)value;
	}
	else if (boost::algorithm::iequals( name, "strinc"))
	{
		m_sentence_ff_incr = (double)value;
	}
	else if (boost::algorithm::iequals( name, "relevant"))
	{
		m_relevant_df_factor = (double)value;
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' numeric weighting function parameter '%s'"), "BM25_dpfc", name.c_str());
	}
}

WeightingFunctionContextInterface* WeightingFunctionInstanceBM25_dpfc::createFunctionContext(
		const StorageClientInterface* storage_,
		MetaDataReaderInterface* metadata,
		const GlobalStatistics& stats) const
{
	try
	{
		GlobalCounter nofdocs = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage_->nofDocumentsInserted();
		return new WeightingFunctionContextBM25_dpfc( storage_, metadata, m_k1, m_b, m_avgdoclen, m_ffpart, nofdocs, m_attribute_content_doclen, m_attribute_title_doclen, m_proximityMinDist, m_title_ff_incr, m_sequence_ff_incr, m_sentence_ff_incr, m_relevant_df_factor, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' function context: %s"), "BM25_dpfc", *m_errorhnd, 0);
}

std::string WeightingFunctionInstanceBM25_dpfc::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << std::setw(2) << std::setprecision(5)
			<< "b=" << m_b << ", k1=" << m_k1 << ", avgdoclen=" << m_avgdoclen << ", doclen=" << m_attribute_content_doclen << ", doclen_title=" << m_attribute_title_doclen << ", proxmindist=" << m_proximityMinDist << ", titleinc=" << m_title_ff_incr << ", seqinc=" << m_sequence_ff_incr << ", strinc=" << m_sentence_ff_incr << ", relevant=" << m_relevant_df_factor << std::endl;
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' function to string: %s"), "BM25_dpfc", *m_errorhnd, std::string());
}


WeightingFunctionInstanceInterface* WeightingFunctionBM25_dpfc::createInstance( const QueryProcessorInterface*) const
{
	try
	{
		return new WeightingFunctionInstanceBM25_dpfc( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' function: %s"), "BM25_dpfc", *m_errorhnd, 0);
}

FunctionDescription WeightingFunctionBM25_dpfc::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt(_TXT("Calculate the document weight with with a modified weighting scheme based on \"BM25\" taking proximity of features into account. It uses a modified value of the ff in the formula. The ff is incremented with some discrete values specified in the configuration. This 'bonifications' are given' for appearance in the title, for appearance in the same structure (e.g. sentence) as another query term and for near proximity to another query term"));
		rt( P::Feature, "match", _TXT( "defines the query features to weight"), "");
		rt( P::Feature, "struct", _TXT( "defines the structure delimiting features"), "");
		rt( P::Feature, "title", _TXT( "defines the title features"), "");
		rt( P::Numeric, "k1", _TXT("parameter of the BM25 weighting scheme"), "0:1000");
		rt( P::Numeric, "b", _TXT("parameter of the BM25 weighting scheme"), "0.0001:1000");
		rt( P::Numeric, "avgdoclen", _TXT("the average document lenght"), "0:");
		rt( P::Numeric, "ffpart", _TXT("fraction of the real ff that is used in the formula. Should be smaller than 1.0, because ff's growing high do not contribute anymore to the weight"), "0.0:1.0");
		rt( P::Metadata, "doclen", _TXT("the meta data element name referencing the document lenght for each document weighted"), "");
		rt( P::String, "proxmindist", _TXT("a minimum distance two features must have so that proximity is taken into account"), "0:");
		rt( P::String, "titleinc", _TXT("the ff increment for features appearing in the title"), "0:");
		rt( P::String, "seqinc", _TXT("the ff increment for features followed imeadiately by another query term"), "0:");
		rt( P::String, "strinc", _TXT("the ff increment for features appearing in the same structure as another feature"), "0:");
		rt( P::String, "relevant", _TXT("the maximum df relative to the number of documents in a collection a feature must have in order to be considered as relevant for ff increments"), "0:");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), "BM25", *m_errorhnd, FunctionDescription());
}

