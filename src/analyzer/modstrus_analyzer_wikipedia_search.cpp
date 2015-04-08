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

static bool wordBoundaryDelimiter_european( char const* si, const char* se)
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
	else if ((*si|32) >= 'a' && (*si|32) <= 'z')
	{
		return false;
	}
	else
	{
		return true;
	}
}

class SeparationTokenizerInstance
	:public strus::TokenizerInstanceInterface
{
public:
	SeparationTokenizerInstance( TokenDelimiter delim)
		:m_delim(delim){}

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
			rt.push_back( strus::analyzer::Token( start-src, start-src, si-start));
		}
		return rt;
	}
private:
	TokenDelimiter m_delim;
};

class SeparationTokenizer
	:public strus::TokenizerInterface
{
public:
	SeparationTokenizer( TokenDelimiter delim)
		:m_delim(delim){}

	strus::TokenizerInstanceInterface* createInstance() const
	{
		return new SeparationTokenizerInstance( m_delim);
	}

private:
	TokenDelimiter m_delim;
};

class SeparationTokenizerConstructor
	:public strus::TokenizerConstructorInterface
{
public:
	SeparationTokenizerConstructor( TokenDelimiter delim)
		:m_delim(delim){}

	strus::TokenizerInterface* create( const std::vector<std::string>& args, const strus::TextProcessorInterface*) const
	{
		if (args.size()) throw std::runtime_error( "no arguments expected for word separation tokenizer");
		return new SeparationTokenizer( m_delim);
	}

private:
	TokenDelimiter m_delim;
};


static const SeparationTokenizerConstructor wordSeparationTokenizer_european( wordBoundaryDelimiter_european);

const strus::TokenizerConstructorInterface* getWordSeparationTokenizer_european()
{
	return &wordSeparationTokenizer_european;
}

static const strus::TokenizerConstructor tokenizers[] =
{
	{"content_europe", getWordSeparationTokenizer_european},
	{0,0}
};

static const strus::NormalizerConstructor normalizers[] =
{
	{0,0}
};

extern "C" DLL_PUBLIC strus::AnalyzerModule entryPoint;

strus::AnalyzerModule entryPoint( tokenizers, normalizers);




