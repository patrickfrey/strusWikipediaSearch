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
#include "strus/tokenizerFunctionInterface.hpp"
#include "strus/tokenizerFunctionInstanceInterface.hpp"
#include "strus/tokenizerFunctionContextInterface.hpp"
#include "strus/private/dll_tags.hpp"
#include "strus/analyzerModule.hpp"
#include "strus/analyzerErrorBufferInterface.hpp"
#include "textwolf/charset_utf8.hpp"
#include <vector>
#include <cstring>
#include <stdexcept>

#define CATCH_ERROR_MAP_RETURN( MSG, HND, VALUE)\
	catch( const std::bad_alloc&)\
	{\
		(HND).report( "out of memory in wikipedia tokenizer");\
		return VALUE;\
	}\
	catch( const std::runtime_error& err)\
	{\
		(HND).report( "error in wikipedia tokenizer: %s", err.what());\
		return VALUE;\
	}\

static textwolf::charset::UTF8::CharLengthTab g_charLengthTab;

static inline const char* skipChar( const char* si)
{
	return si+g_charLengthTab[*si];
}

static inline unsigned int utf8decode( char const* si, const char* se)
{
	enum {
		B00111111=0x3F,
		B00011111=0x1F
	};
	unsigned int res = (unsigned char)*si;
	unsigned char charsize = g_charLengthTab[ *si];
	if (res > 127)
	{
		res = ((unsigned char)*si)&(B00011111>>(charsize-2));
		for (++si,--charsize; si != se && charsize; ++si,--charsize)
		{
			res <<= 6;
			res |= (unsigned char)(*si & B00111111);
		}
	}
	return res;
}

typedef bool (*TokenDelimiter)( char const* si, const char* se);
typedef bool (*TokenFilter)( char const* si, const char* se);

static bool wordBoundaryDelimiter_european_inv( char const* si, const char* se)
{
	if ((unsigned char)*si <= 32)
	{
		return true;
	}
	else if ((unsigned char)*si >= 128)
	{
		// See https://vazor.com/unicode/index.html
		unsigned int chr = utf8decode( si, se);
		if (chr == 133) return true;
		if (chr >= 0x1E00 && chr <= 0x1EFF) return false;
		if (chr >= 0x250) return true;
		return false;
	}
	else if (*si >= '0' && *si <= '9')
	{
		return false;
	}
	else if (*si == '&' || *si == '=')
	{
		if (si+5 < se && 0==std::memcmp( si, "&nbsp;", 6))
		{
			return true;
		}
		return false;
	}
	else if ((*si|32) >= 'a' && (*si|32) <= 'z')
	{
		return false;
	}
	else
	{
		return true;
	}
}

static bool wordBoundaryDelimiter_european_fwd( char const* si, const char* se)
{
	static const char punct[] = ";.:?!)(";
	if (wordBoundaryDelimiter_european_inv( si, se))
	{
		if (0!=std::strchr( punct, *si)) return false;
		return true;
	}
	else
	{
		return false;
	}
}

static bool wordFilter_inv( const char* src, const char* end)
{
	unsigned int headDigits = 0;
	unsigned int midAlpha = 0;
	unsigned int tailDigits = 0;
	char const* si = src;
	char const* se = end;

	const char* digitsStart = si;
	for (;si != se; ++si)
	{
		if (*si >= '0' && *si <= '9')
		{
			++headDigits;
		}
		else
		{
			break;
		}
	}
	while (si != se)
	{
		if ((unsigned char)*si >= 128)
		{
			unsigned int chr = utf8decode( si, se);
			if (chr >= 0x1E00 && chr <= 0x1EFF)
			{
			}
			else if (chr >= 0x22A)
			{
				return false;
			}
			++midAlpha;
			++si;
		}
		if (((*si|32) >= 'a' && (*si|32) <= 'z') || *si == '_')
		{
			++midAlpha;
			++si;
		}
		else
		{
			break;
		}
	}
	for (; si != se; ++si)
	{
		if (*si >= '0' && *si <= '9')
		{
			++tailDigits;
			
		}
		else
		{
			break;
		}
	}
	if (si != se)
	{
		if (headDigits == 0 && tailDigits != 0)
		{
			if (*si == '.' && si+1 == se) return true;
		}
		return false;
	}
	if (headDigits != 0 && midAlpha != 0 && tailDigits == 0)
	{
		if (se-si==1 && 0==std::memcmp(si,"s",1))
		{
			if (headDigits > 4) return false;
			return (headDigits <= 3 || *digitsStart == '1' || *digitsStart == '2');
		}
		return (headDigits <= 2 && midAlpha <= 3);
	}
	if (headDigits != 0 && midAlpha == 0)
	{
		if (headDigits > 4) return false;
		return (headDigits <= 3 || *digitsStart == '1' || *digitsStart == '2');
	}
	if (headDigits && midAlpha && tailDigits)
	{
		return false;
	}
	return true;
}

static bool wordFilter_fwd( char const*, const char*)
{
	return true;
}

class SeparationTokenizerFunctionContext
	:public strus::TokenizerFunctionContextInterface
{
public:
	SeparationTokenizerFunctionContext( TokenDelimiter delim_, TokenFilter filter_, strus::AnalyzerErrorBufferInterface* errorhnd_)
		:m_delim(delim_),m_filter(filter_),m_errorhnd(errorhnd_){}

	const char* skipToToken( char const* si, const char* se) const
	{
		for (; si < se && m_delim( si, se); si = skipChar( si)){}
		return si;
	}

	virtual std::vector<strus::analyzer::Token> tokenize( const char* src, std::size_t srcsize)
	{
		try
		{
			std::vector<strus::analyzer::Token> rt;
			char const* si = skipToToken( src, src+srcsize);
			const char* se = src+srcsize;
	
			for (;si < se; si = skipToToken(si,se))
			{
				const char* start = si;
				while (si < se && !m_delim( si, se))
				{
					si = skipChar( si);
				}
				if (m_filter( start, si))
				{
					rt.push_back( strus::analyzer::Token( start-src, start-src, si-start));
				}
			}
			return rt;
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error in word separation tokenizer: %s"), *m_errorhnd, std::vector<strus::analyzer::Token>());
	}
private:
	TokenDelimiter m_delim;
	TokenFilter m_filter;
	strus::AnalyzerErrorBufferInterface* m_errorhnd;
};

class SeparationTokenizerFunctionInstance
	:public strus::TokenizerFunctionInstanceInterface
{
public:
	SeparationTokenizerFunctionInstance( TokenDelimiter delim, TokenFilter filter, strus::AnalyzerErrorBufferInterface* errorhnd_)
		:m_delim(delim),m_filter(filter),m_errorhnd(errorhnd_){}

	strus::TokenizerFunctionContextInterface* createFunctionContext() const
	{
		try
		{
			return new SeparationTokenizerFunctionContext( m_delim, m_filter, m_errorhnd);
		}
		CATCH_ERROR_MAP_RETURN( _TXT("failed to create context of word separation tokenizer: %s"), *m_errorhnd, 0);
	}

private:
	TokenDelimiter m_delim;
	TokenFilter m_filter;
	strus::AnalyzerErrorBufferInterface* m_errorhnd;
};

class SeparationTokenizerFunction
	:public strus::TokenizerFunctionInterface
{
public:
	SeparationTokenizerFunction( TokenDelimiter delim_, TokenFilter filter_, strus::AnalyzerErrorBufferInterface* errorhnd_)
		:m_delim(delim_),m_filter(filter_),m_errorhnd(errorhnd_){}

	virtual strus::TokenizerFunctionInstanceInterface* createInstance( const std::vector<std::string>& args, const strus::TextProcessorInterface*) const
	{
		try
		{
			if (args.size()) throw std::runtime_error( "no arguments expected for word separation tokenizer");
			return new SeparationTokenizerFunctionInstance( m_delim, m_filter, m_errorhnd);
		}
		CATCH_ERROR_MAP_RETURN( _TXT("failed to create context of word separation tokenizer: %s"), *m_errorhnd, 0);
	}

private:
	TokenDelimiter m_delim;
	TokenFilter m_filter;
	strus::AnalyzerErrorBufferInterface* m_errorhnd;
};


strus::TokenizerFunctionInterface* createWordSeparationTokenizer_european_inv( strus::AnalyzerErrorBufferInterface* errorhnd)
{
	return new SeparationTokenizerFunction( wordBoundaryDelimiter_european_inv, wordFilter_inv, errorhnd);
}

strus::TokenizerFunctionInterface* createWordSeparationTokenizer_european_fwd( strus::AnalyzerErrorBufferInterface* errorhnd)
{
	return new SeparationTokenizerFunction( wordBoundaryDelimiter_european_fwd, wordFilter_fwd, errorhnd);
}

static const strus::TokenizerConstructor tokenizers[] =
{
	{"content_europe_inv", createWordSeparationTokenizer_european_inv},
	{"content_europe_fwd", createWordSeparationTokenizer_european_fwd},
	{0,0}
};

static const strus::NormalizerConstructor normalizers[] =
{
	{0,0}
};

extern "C" DLL_PUBLIC strus::AnalyzerModule entryPoint;

strus::AnalyzerModule entryPoint( tokenizers, normalizers, 0);




