/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_WEIGHTING_BM25_DPFC_HPP_INCLUDED
#define _STRUS_WEIGHTING_BM25_DPFC_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class WeightingFunctionContextBM25_dpfc
/// \brief Weighting function based on the BM25 formula with an artificial ff calculated from the real ff and some discrete increments given by some feature occurrence relation checks based on proximity and structures (sentences) 
class WeightingFunctionContextBM25_dpfc
	:public WeightingFunctionContextInterface
{
public:
	WeightingFunctionContextBM25_dpfc(
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
		double title_ff_incr,
		double sequence_ff_incr,
		double sentence_ff_incr,
		double relevant_df_factor,
		ErrorBufferInterface* errorhnd);

	struct Feature
	{
		PostingIteratorInterface* itr;
		double weight;
		double idf;
		bool relevant;

		Feature( PostingIteratorInterface* itr_, double weight_, double idf_, bool relevant_)
			:itr(itr_),weight(weight_),idf(idf_),relevant(relevant_){}
		Feature( const Feature& o)
			:itr(o.itr),weight(o.weight),idf(o.idf),relevant(o.relevant){}
	};

	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			float weight_,
			const TermStatistics& stats_);

	virtual double call( const Index& docno);

private:
	double m_k1;
	double m_b;
	double m_avgDocLength;
	double m_ffpart;
	double m_nofCollectionDocuments;
	std::vector<Feature> m_weight_featar;
	std::vector<Feature> m_struct_featar;
	PostingIteratorInterface* m_title_itr;
	MetaDataReaderInterface* m_metadata;
	int m_metadata_content_doclen;
	int m_metadata_title_doclen;
	unsigned int m_proximityMinDist;
	double m_title_ff_incr;
	double m_sequence_ff_incr;
	double m_sentence_ff_incr;
	double m_relevant_df_factor;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

/// \class WeightingFunctionInstanceBM25_dpfc
/// \brief Weighting function instance based on the BM25 formula with an artificial ff calculated from the real ff and some discrete increments given by some feature occurrence relation checks based on proximity and structures (sentences) 
class WeightingFunctionInstanceBM25_dpfc
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceBM25_dpfc( ErrorBufferInterface* errorhnd_)
		:m_k1(1.5),m_b(0.75),m_avgdoclen(1000),m_ffpart(0.5),m_proximityMinDist(150),m_title_ff_incr(1.5),m_sequence_ff_incr(1.5),m_sentence_ff_incr(0.5),m_relevant_df_factor(0.5),m_errorhnd(errorhnd_)
	{}

	virtual ~WeightingFunctionInstanceBM25_dpfc(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata,
			const GlobalStatistics& stats) const;

	virtual std::string tostring() const;

private:
	double m_k1;
	double m_b;
	double m_avgdoclen;
	double m_ffpart;
	std::string m_attribute_content_doclen;
	std::string m_attribute_title_doclen;
	unsigned int m_proximityMinDist;
	double m_title_ff_incr;
	double m_sequence_ff_incr;
	double m_sentence_ff_incr;
	double m_relevant_df_factor;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class WeightingFunctionBM25_dpfc
/// \brief Weighting function based on the BM25 formula with an artificial ff calculated from the real ff and some discrete increments given by some feature occurrence relation checks based on proximity and structures (sentences) 
class WeightingFunctionBM25_dpfc
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunctionBM25_dpfc( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionBM25_dpfc(){}

	virtual WeightingFunctionInstanceInterface* createInstance() const;

	virtual Description getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

