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
#include "strus/tokenizerInterface.hpp"
#include "strus/tokenizerInstanceInterface.hpp"
#include "strus/tokenizerConstructorInterface.hpp"
#include "strus/private/dll_tags.hpp"
#include "strus/analyzerModule.hpp"
#include "textwolf/charset_utf8.hpp"
#include <vector>
#include <cstring>

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
		unsigned int chr = utf8decode( si, se);
		if (chr == 133) return true;
		if (chr >= 0x180) return true;
		return false;
	}
	else if (*si >= '0' && *si <= '9')
	{
		return false;
	}
	else if (*si == '&' || *si == '=')
	{
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
		if (std::strchr( punct, *si) == 0) return false;
		return true;
	}
	else
	{
		return false;
	}
}

static bool wordFilter_inv( char const* si, const char* se)
{
	bool onlyDigits = true;
	bool hasDigits = false;
	bool startsWithAlpha = false;
	for (; si != se; ++si)
	{
		if ((*si|32) >= 'a' && (*si|32) >= 'z')
		{
			startsWithAlpha = true;
		}
		break;
	}
	const char* start = si;
	for (; si != se; ++si)
	{
		if (*si == '&' || *si == '=') return false;
		if (*si >= '0' && *si <= '9')
		{
			hasDigits = true;
		}
		else
		{
			if (*si == 's' && si+1 == se)
			{}
			else if (0==std::memcmp(start,"3rd",3) && si+2 == se)
			{}
			else if (0==std::memcmp(si,"th",3) && si+2 == se)
			{}
			else
			{
				onlyDigits = false;
			}
		}
	}
	if (hasDigits)
	{
		if (!onlyDigits) return false;
		if (startsWithAlpha)
		{
			return (si - start <= 2);
		}
		return (*start != '0' && (si - start <= 3 || *start == '1' || *start == '2'));
	}
	return true;
}

static bool wordFilter_fwd( char const*, const char*)
{
	return true;
}

class SeparationTokenizerInstance
	:public strus::TokenizerInstanceInterface
{
public:
	SeparationTokenizerInstance( TokenDelimiter delim_, TokenFilter filter_)
		:m_delim(delim_),m_filter(filter_){}

	const char* skipToToken( char const* si, const char* se) const
	{
		for (; si < se && m_delim( si, se); si = skipChar( si)){}
		return si;
	}

	virtual std::vector<strus::analyzer::Token> tokenize( const char* src, std::size_t srcsize)
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
private:
	TokenDelimiter m_delim;
	TokenFilter m_filter;
};

class SeparationTokenizer
	:public strus::TokenizerInterface
{
public:
	SeparationTokenizer( TokenDelimiter delim_, TokenFilter filter_)
		:m_delim(delim_),m_filter(filter_){}

	strus::TokenizerInstanceInterface* createInstance() const
	{
		return new SeparationTokenizerInstance( m_delim, m_filter);
	}

private:
	TokenDelimiter m_delim;
	TokenFilter m_filter;
};

class SeparationTokenizerConstructor
	:public strus::TokenizerConstructorInterface
{
public:
	SeparationTokenizerConstructor( TokenDelimiter delim_, TokenFilter filter_)
		:m_delim(delim_),m_filter(filter_){}

	strus::TokenizerInterface* create( const std::vector<std::string>& args, const strus::TextProcessorInterface*) const
	{
		if (args.size()) throw std::runtime_error( "no arguments expected for word separation tokenizer");
		return new SeparationTokenizer( m_delim, m_filter);
	}

private:
	TokenDelimiter m_delim;
	TokenFilter m_filter;
};


static const SeparationTokenizerConstructor wordSeparationTokenizer_european_inv( wordBoundaryDelimiter_european_inv, wordFilter_inv);
static const SeparationTokenizerConstructor wordSeparationTokenizer_european_fwd( wordBoundaryDelimiter_european_fwd, wordFilter_fwd);

const strus::TokenizerConstructorInterface* getWordSeparationTokenizer_european_inv()
{
	return &wordSeparationTokenizer_european_inv;
}
const strus::TokenizerConstructorInterface* getWordSeparationTokenizer_european_fwd()
{
	return &wordSeparationTokenizer_european_fwd;
}

static const strus::TokenizerConstructor tokenizers[] =
{
	{"content_europe_inv", getWordSeparationTokenizer_european_inv},
	{"content_europe_fwd", getWordSeparationTokenizer_european_fwd},
	{0,0}
};

static const strus::NormalizerConstructor normalizers[] =
{
	{0,0}
};

extern "C" DLL_PUBLIC strus::AnalyzerModule entryPoint;

strus::AnalyzerModule entryPoint( tokenizers, normalizers);




