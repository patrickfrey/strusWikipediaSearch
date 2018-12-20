/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/analyzerModule.hpp"
#include "strus/base/dll_tags.hpp"
#include "strus/normalizerFunctionInterface.hpp"
#include "strus/normalizerFunctionInstanceInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "errorUtils.hpp"
#include <string>
#include <cstring>

using namespace strus;

#define NORMALIZER_NAME "entityid"
#define _TXT(txt) txt  // No internationalization of the error messages for this module

static const char g_delimiters[] = "\"’`'?!/;:.,-— )([]{}<>_";
static const char g_quotes[] = "\"'’`;.:";

class EntityIdNormalizerInstance
	:public NormalizerFunctionInstanceInterface
{
public:
	explicit EntityIdNormalizerInstance( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual std::string normalize(
			const char* src,
			std::size_t srcsize) const
	{
		try
		{
			std::string rt;
			char const* si = src;
			char const* se = src + srcsize;
			for (;si < se && (0!=std::strchr( g_quotes, *si) || (unsigned char)*si <= 32); ++si){}
			for (;si < se && (0!=std::strchr( g_quotes, *(se-1)) || (unsigned char)*(se-1) <= 32); --se){}
			const char* start = si;
			for (; si < se; ++si)
			{
				if (0!=std::strchr( g_delimiters, *si))
				{
					rt.append( start, si-start);
					bool hasOnlySpace = (unsigned char)*si <= 32 || *si == '_';
					for (++si; si < se && (0!=std::strchr( g_delimiters, *si) || (unsigned char)*si <= 32); ++si)
					{
						hasOnlySpace &= (unsigned char)*si <= 32 || *si == '_';
					}
					rt.push_back( hasOnlySpace ? '_':'-');
					start = si;
				}
			}
			rt.append( start, se-start);
			return rt;
		}
		CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s normalize: %s"), NORMALIZER_NAME, *m_errorhnd, std::string());
	}

	virtual analyzer::FunctionView view() const
	{
		try
		{
			return analyzer::FunctionView( NORMALIZER_NAME)
			;
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error in introspection: %s"), *m_errorhnd, analyzer::FunctionView());
	}

private:
	ErrorBufferInterface* m_errorhnd;
};

class EntityIdNormalizerFunction
	:public NormalizerFunctionInterface
{
public:
	explicit EntityIdNormalizerFunction( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual NormalizerFunctionInstanceInterface* createInstance( const std::vector<std::string>& args, const TextProcessorInterface*) const
	{
		try
		{
			if (!args.empty())
			{
				m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("no arguments expected for normalizer '%s'"), NORMALIZER_NAME);
				return 0;
			}
			return new EntityIdNormalizerInstance( m_errorhnd);
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error creating instance of normalizer: %s"), *m_errorhnd, 0);
	}

	virtual const char* getDescription() const
	{
		return _TXT("Normalizer for entities in the Wikipedia search project");
	}

private:
	ErrorBufferInterface* m_errorhnd;
};

/// \brief Constructor function
static NormalizerFunctionInterface* createEntityIdNormalizerFunction( ErrorBufferInterface* errorhnd)
{
	try
	{
		return new EntityIdNormalizerFunction( errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in constructor function of normalizer: %s"), *errorhnd, 0);
}

static strus::NormalizerConstructor g_normalizers[] = { {"entityid",&createEntityIdNormalizerFunction}, {0,0} };

extern "C" DLL_PUBLIC strus::AnalyzerModule entryPoint;

strus::AnalyzerModule entryPoint( 0, g_normalizers, 0);
