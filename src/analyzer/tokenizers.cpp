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
#include "tokenizers.hpp"
#include "strus/tokenizerInterface.hpp"
#include "textwolf/charset_utf8.hpp"
#include <string>
#include <cstring>
#include <vector>
#include <stdexcept>

using namespace strus;

static const char* findPattern( const std::string& pattern, const char* si, const char* se)
{
	si = (const char*)std::memchr( si, pattern[0], se-si);
	for (; si; si = (const char*)std::memchr( si, pattern[0], se-si))
	{
		if ((std::size_t)(se-si) >= pattern.size() && 0==std::memcmp( si, pattern.c_str(), pattern.size()))
		{
			return si+pattern.size();
		}
		++si;
	}
	return 0;
}

static bool isAlphaNum( char ch)
{
	return ((ch|32) >= 'a' && (ch|32) <= 'z') || (ch >= '0' && ch <= '9');
}

static bool isSpace( char ch)
{
	return (ch <= 32);
}

static bool isTag( const char* tagname, char const* si, const char* se)
{
	char const* ti = tagname;
	for (; si < se && *ti == *si; ++ti,++si){}
	if (*ti) return false;
	return (*si == '>' || (unsigned char)*si <= 32);
}

static const char* skipIdentifier( char const* si, const char* se)
{
	for (; si != se && isAlphaNum(*si); ++si){}
	return si;
}

static const char* skipSpace( char const* si, const char* se)
{
	for (; si != se && isSpace(*si); ++si){}
	return si;
}

static const char* skipValue( char const* si, const char* se)
{
	if (si != se)
	{
		if (*si == '\'' || *si == '\"')
		{
			char eb = *si;
			const char* ni = (const char*)std::memchr( si, eb, se-si);
			if (ni) si = ni;
			++si;
		}
		else if (isAlphaNum(*si))
		{
			return skipIdentifier( si, se);
		}
	}
	return si;
}

static const char* skipUrl( char const* si, const char* se, char delim)
{
	for (; si != se && isAlphaNum(*si); ++si){}
	if (si[0] == ':' && si[1] == '/')
	{
		return (const char*)std::memchr( si, delim, se-si);
	}
	return 0;
}

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

class CharTable
{
public:
	CharTable( const char* op)
	{
		std::size_t ii;
		for (ii=0; ii<sizeof(m_ar); ++ii) m_ar[ii] = false;
		for (ii=0; op[ii]; ++ii)
		{
			m_ar[(unsigned char)(op[ii])] = true;
		}
	}

	bool operator[]( char ch) const
	{
		return m_ar[ (unsigned char)ch];
	}

private:
	bool m_ar[128];
};

typedef bool (*TokenDelimiter)( char const* si, const char* se);

static bool wordBoundaryDelimiter( char const* si, const char* se)
{
	static const CharTable ascii_delimiter("#|{}[]<>&:.;,!?%/()*+-'\"`=");
	if ((unsigned char)*si <= 32)
	{
		return true;
	}
	else if ((unsigned char)*si >= 128)
	{
		unsigned int chr = utf8decode( si, se);
		if (chr == 133) return true;
		if (chr >= 0x2000 && chr <= 0x206F) return true;
		if (chr == 0x3000) return true;
		if (chr == 0xFEFF) return true;
		return false;
	}
	else if (ascii_delimiter[ *si])
	{
		return true;
	}
	else
	{
		return false;
	}
}

static bool whiteSpaceDelimiter( char const* si, const char* se)
{
	static const CharTable ascii_delimiter("'{}[]<>");
	if ((unsigned char)*si <= 32)
	{
		return true;
	}
	else if ((unsigned char)*si >= 128)
	{
		unsigned int chr = utf8decode( si, se);
		if (chr == 133) return true;
		if (chr >= 0x2000 && chr <= 0x200F) return true;
		if (chr >= 0x2028 && chr <= 0x2029) return true;
		if (chr == 0x202F) return true;
		if (chr >= 0x205F && chr <= 0x2060) return true;
		if (chr == 0x3000) return true;
		if (chr == 0xFEFF) return true;
		return false;
	}
	else if (ascii_delimiter[ *si])
	{
		return true;
	}
	else
	{
		return false;
	}
}

static const char* skipToToken( char const* si, const char* se, TokenDelimiter delim)
{
	while (si && si != se)
	{
		if (*si == '<')
		{
			++si;
			if (0==std::memcmp( si, "!--", 3))
			{
				si += 3;
				const char* pe = findPattern( "-->", si, se);
				if (pe) si = pe;
			}
			else if (isTag( "ref", si, se))
			{
				si += 4;
				const char* pe = findPattern( "</ref>", si, se);
				if (pe) si = pe;
			}
			else if (isTag( "math", si, se))
			{
				si += 5;
				const char* pe = findPattern( "</math>", si, se);
				if (pe) si = pe;
			}
			else if (isTag( "span", si, se))
			{
				si += 5;
				const char* pe = findPattern( "</span>", si, se);
				if (pe) si = pe;
			}
			else if (isTag( "pre", si, se))
			{
				si += 4;
				const char* pe = findPattern( "</pre>", si, se);
				if (pe) si = pe;
			}
			else if (isTag( "div", si, se))
			{
				si += 4;
				const char* pe = findPattern( "</div>", si, se);
				if (pe) si = pe;
			}
			else if (isTag( "gallery", si, se))
			{
				si += 8;
				const char* pe = findPattern( "</gallery>;", si, se);
				if (pe) si = pe;
			}
			else if (isTag( "nowiki", si, se))
			{
				si += 7;
				const char* pe = findPattern( "</nowiki>", si, se);
				if (pe) si = pe;
			}
			else
			{
				const char* pe = (const char*)std::memchr( si, '>', se-si);
				if (pe) si = pe+1;
			}
		}
		else if (*si == '[')
		{
			++si;
			if (si < se && *si == '[')
			{
				const char* pe = findPattern( "]]", si+1, se);
				if (pe) si = pe;
			}
			else
			{
				char const* ue = skipUrl( si, se, ']');
				if (ue) si = ue+1;
			}
		}
		else if (*si == '{')
		{
			++si;
			if (si < se && *si == '{')
			{
				const char* pe = findPattern( "}}", si+1, se);
				if (pe) si = pe;
			}
			else if (si < se && *si == '|')
			{
				const char* xi = (const char*)std::memchr( si, '\n', se-si);
				if (xi) si=xi+1;
			}
			else
			{
				const char* xi = (const char*)std::memchr( si, '}', se-si);
				if (xi) si=xi+1;
			}
		}
		else if (*si == '!' || *si == '|' || *si == '*')
		{
			++si;
			char const* ai = skipIdentifier( skipSpace( si, se), se);
			if (ai < se && *ai == '=')
			{
				si = skipValue( ai+1, se);
			}
			si = skipSpace( si, se);
		}
		else if (*si <= 32)
		{
			si = skipSpace( si, se);
		}
		else if (delim( si, se))
		{
			si = skipChar(si);
		}
		else
		{
			break;
		}
	}
	return (si)?si:se;
}

static void tokenizeWithDelimiter( std::vector<analyzer::Token>& res, int logicalPosOfs, const char* src, char const* si, const char* se, TokenDelimiter delim)
{
	for (si = skipToToken( si, se, delim); si != se; si = skipToToken( si, se, delim))
	{
		const char* start = si;
		for (; si != se && !delim( si, se); si = skipChar(si)){}
		res.push_back( analyzer::Token( start - src + logicalPosOfs, start - src, si - start));
	}
}


class TokenizerWikimediaTextWord
	:public TokenizerInterface
{
public:
	TokenizerWikimediaTextWord(){}

	virtual ~TokenizerWikimediaTextWord(){}

	virtual std::vector<analyzer::Token>
			tokenize( Context* ctx, const char* src, std::size_t srcsize) const
	{
		std::vector<analyzer::Token> rt;
		tokenizeWithDelimiter( rt, 0, src, src, src+srcsize, wordBoundaryDelimiter);
		return rt;
	}
};


class TokenizerWikimediaTextSplit
	:public TokenizerInterface
{
public:
	TokenizerWikimediaTextSplit(){}

	virtual ~TokenizerWikimediaTextSplit(){}

	virtual std::vector<analyzer::Token>
			tokenize( Context* ctx, const char* src, std::size_t srcsize) const
	{
		std::vector<analyzer::Token> rt;
		tokenizeWithDelimiter( rt, 0, src, src, src+srcsize, whiteSpaceDelimiter);
		return rt;
	}
};

enum TokenClass
{
	TokenClassContent,
	TokenClassWhiteSpaceSep,
	TokenClassWordSep
};

static void tokenizeTokenClass( TokenClass tokenClass_, std::vector<analyzer::Token>& res, int logicalPosOfs, const char* src, char const* si, const char* se)
{
	switch (tokenClass_)
	{
		case TokenClassContent:
			res.push_back( analyzer::Token( (si - src) + logicalPosOfs, si - src, se-si));
			break;
		case TokenClassWhiteSpaceSep:
			tokenizeWithDelimiter( res, logicalPosOfs, src, si, se, whiteSpaceDelimiter);
			break;
		case TokenClassWordSep:
			tokenizeWithDelimiter( res, logicalPosOfs, src, si, se, wordBoundaryDelimiter);
			break;
	}
}


class TokenizerWikimediaLink
	:public TokenizerInterface
{
public:
	enum WhatToExtract {Text,Id};

	explicit TokenizerWikimediaLink( TokenClass tokenClass_, WhatToExtract what_)
		:m_tokenClass(tokenClass_),m_what(what_){}
	virtual ~TokenizerWikimediaLink(){}

	class Argument
		:public TokenizerInterface::Argument
	{
	public:
		Argument( const std::string& name_)
			:m_name(name_),m_pattern(getPattern(name_))
		{}
		Argument( const Argument& o)
			:m_name(o.m_name),m_pattern(o.m_pattern){}
		Argument( const std::vector<std::string>& arg)
		{
			if (arg.size() > 0)
			{
				if (arg.size() > 1) throw std::runtime_error( "too many arguments for 'link' tokenizer");
				m_name = arg[0];
				m_pattern = getPattern( m_name);
			}
			else
			{
				m_name = "";
				m_pattern = getPattern( m_name);
			}
		}

		/// \brief Destructor
		virtual ~Argument(){}

		const std::string& name() const		{return m_name;}
		const std::string& pattern() const	{return m_pattern;}

	private:
		static std::string getPattern( const std::string& name_)
		{
			std::string rt("[[");
			if (name_.size())
			{
				rt.append( name_);
				rt.push_back( ':');
			}
			return rt;
		}

	private:
		std::string m_name;
		std::string m_pattern;
	};

	class Context
		:public TokenizerInterface::Context
	{
	public:
		Context( const Argument& arg_)
			:m_arg(arg_){}
		Context()
			:m_arg(""){}

		/// \brief Destructor
		virtual ~Context(){}

		const Argument arg() const	{return m_arg;}

	private:
		Argument m_arg;
	};

	virtual TokenizerInterface::Argument* createArgument( const std::vector<std::string>& arg) const
	{
		return new Argument(arg);
	}

	virtual TokenizerInterface::Context* createContext( const TokenizerInterface::Argument* arg) const
	{
		if (!arg)
		{
			return new Context( Argument( ""));
		}
		else
		{
			return new Context( *reinterpret_cast<const Argument*>(arg));
		}
	}

	virtual std::vector<analyzer::Token>
			tokenize( TokenizerInterface::Context* ctx_, const char* src, std::size_t srcsize) const
	{
		Context* ctx = reinterpret_cast<Context*>(ctx_);
		std::vector<analyzer::Token> rt;
		char const* si = findPattern( ctx->arg().pattern(), src, src + srcsize);
		char const* se = src + srcsize;

		for (; si; si = findPattern( ctx->arg().pattern(), si, se))
		{
			char const* linkStart = si - ctx->arg().pattern().size();
			char const* pe = (const char*)std::memchr( si, ']', se-si);
			if (pe && pe[1] == ']')
			{
				char const* start = si;
				if (ctx->arg().name().empty())
				{
					if (0!=std::memchr( si, ':', pe-si))
					{
						start = 0;
					}
				}
				if (start)
				{
					switch (m_what)
					{
						case Id:
						{
							const char* end = (const char*)std::memchr( si, '|', pe-si);
							if (!end || skipSpace( end, pe) == pe) end = pe;
							tokenizeTokenClass( m_tokenClass, rt, -(si-linkStart), src, si, end);
							break;
						}
						case Text:
						{
							start = (const char*)std::memchr( si, '|', pe-si);
							if (start && skipSpace( start, pe) != pe)
							{
								++start;
							}
							else
							{
								start = si;
							}
							tokenizeTokenClass( m_tokenClass, rt, -(start-linkStart), src, start, pe);
							break;
						}
					}
				}
				si = pe+2;
			}
		}
		return rt;
	}

private:
	TokenClass m_tokenClass;
	WhatToExtract m_what;
};

class TokenizerWikimediaUrl
	:public TokenizerInterface
{
public:
	enum WhatToExtract {Title,Link};

	TokenizerWikimediaUrl( TokenClass tokenClass_, WhatToExtract what_)
		:m_tokenClass(tokenClass_),m_what(what_){}
	virtual ~TokenizerWikimediaUrl(){}

	virtual std::vector<analyzer::Token>
			tokenize( Context* ctx, const char* src, std::size_t srcsize) const
	{
		std::vector<analyzer::Token> rt;
		char const* si = (const char*)std::memchr( src, '[', srcsize);
		char const* se = src + srcsize;

		for (; si; si = (const char*)std::memchr( si, '[', se-si))
		{
			const char* urlStart = si;
			++si;
			char const* pe = skipUrl( si, se, ']');
			if (pe)
			{
				switch (m_what)
				{
					case Title:
					{
						char const* start = (const char*)std::memchr( si, ' ', pe-si);
						if (!start) start = (const char*)std::memchr( si, '\t', pe-si);
						if (start)
						{
							++start;
						}
						else
						{
							start = si;
						}
						tokenizeTokenClass( m_tokenClass, rt, -(start-urlStart), src, start, pe);
						break;
					}
					case Link:
					{
						char const* end = (const char*)std::memchr( si, ' ', pe-si);
						if (!end) end = (const char*)std::memchr( si, '\t', pe-si);
						if (end)
						{
							tokenizeTokenClass( m_tokenClass, rt, -(si-urlStart), src, si, end);
						}
						else
						{
							tokenizeTokenClass( m_tokenClass, rt, -(si-urlStart), src, si, pe);
						}
						break;
					}
				}
				si = pe + 1;
			}
			else if (*si == '[')
			{
				pe = findPattern( "]]", si+1, se);
				if (pe) si = pe+2;
			}
			else
			{
				pe = (const char*)std::memchr( si, ']', se-si);
				if (pe) si = pe+1;
			}
		}
		return rt;
	}

private:
	TokenClass m_tokenClass;
	WhatToExtract m_what;
};


class TokenizerWikimediaCitation
	:public TokenizerInterface
{
public:
	explicit TokenizerWikimediaCitation( TokenClass tokenClass_)
		:m_tokenClass(tokenClass_){}
	virtual ~TokenizerWikimediaCitation(){}

	class Argument
		:public TokenizerInterface::Argument
	{
	public:
		Argument( const std::string& name_, const std::string& attribute_)
			:m_name(name_),m_pattern(getPattern(name_)),m_attribute(getAttribute(attribute_))
		{}
		Argument( const Argument& o)
			:m_name(o.m_name),m_pattern(o.m_pattern),m_attribute(o.m_attribute){}
		Argument( const std::vector<std::string>& arg)
		{
			if (arg.size() > 0)
			{
				if (arg.size() > 2) throw std::runtime_error( "too many arguments for 'link' tokenizer");
				m_name = arg[0];
				m_pattern = getPattern( m_name);
				if (arg.size() == 2)
				{
					m_attribute = getAttribute( arg[1]);
				}
			}
		}

		/// \brief Destructor
		virtual ~Argument(){}

		const std::string& name() const		{return m_name;}
		const std::string& pattern() const	{return m_pattern;}
		const std::string& attribute() const	{return m_attribute;}

	private:
		static std::string getPattern( const std::string& name_)
		{
			std::string rt("{{");
			if (name_.size())
			{
				rt.append( name_);
				rt.push_back( ' ');
			}
			return rt;
		}

		static std::string getAttribute( const std::string& name_)
		{
			if (name_.empty()) return std::string();
			std::string rt( "|");
			rt.append( name_);
			rt.push_back( '=');
			return rt;
		}

	private:
		std::string m_name;
		std::string m_pattern;
		std::string m_attribute;
	};

	class Context
		:public TokenizerInterface::Context
	{
	public:
		Context( const Argument& arg_)
			:m_arg(arg_){}
		Context()
			:m_arg("",""){}

		/// \brief Destructor
		virtual ~Context(){}

		const Argument arg() const	{return m_arg;}

	private:
		Argument m_arg;
	};

	virtual Argument* createArgument( const std::vector<std::string>& arg) const
	{
		return new Argument(arg);
	}

	virtual TokenizerInterface::Context* createContext( const TokenizerInterface::Argument* arg) const
	{
		if (!arg)
		{
			return new Context( Argument( "", ""));
		}
		else
		{
			return new Context( *reinterpret_cast<const Argument*>(arg));
		}
	}

	virtual std::vector<analyzer::Token>
			tokenize( TokenizerInterface::Context* ctx_, const char* src, std::size_t srcsize) const
	{
		Context* ctx = reinterpret_cast<Context*>(ctx_);
		std::vector<analyzer::Token> rt;
		char const* si = findPattern( ctx->arg().pattern(), src, src + srcsize);
		char const* se = src + srcsize;

		for (; si; si = findPattern( ctx->arg().pattern(), si, se))
		{
			char const* citationStart = si - ctx->arg().pattern().size();
			char const* pe = (const char*)std::memchr( si, '}', se-si);
			if (pe && pe[1] == '}')
			{
				if (ctx->arg().attribute().size())
				{
					char const* ai = findPattern( ctx->arg().attribute(), si, pe);
					if (ai)
					{
						char const* ae = (const char*)std::memchr( ai, '|', pe-ai);
						if (!ae) ae = (const char*)std::memchr( ai, '}', pe-ai);
						if (ae)
						{
							tokenizeTokenClass( m_tokenClass, rt, -(ai-citationStart), src, ai, ae);
						}
					}
				}
				else
				{
					tokenizeTokenClass( m_tokenClass, rt, -(si-citationStart), src, si, pe);
				}
				si = pe+2;
			}
		}
		return rt;
	}
private:
	TokenClass m_tokenClass;
};

class TokenizerWikimediaReference
	:public TokenizerInterface
{
public:
	explicit TokenizerWikimediaReference( TokenClass tokenClass_)
		:m_tokenClass(tokenClass_){}
	virtual ~TokenizerWikimediaReference(){}

	class Argument
		:public TokenizerInterface::Argument
	{
	public:
		Argument( const std::string& name_, const std::string& attribute_)
			:m_name(name_),m_pattern(getPattern(name_)),m_attribute(getAttribute(attribute_))
		{}
		Argument( const Argument& o)
			:m_name(o.m_name),m_pattern(o.m_pattern),m_attribute(o.m_attribute){}
		Argument( const std::vector<std::string>& arg)
		{
			if (arg.size() > 0)
			{
				if (arg.size() > 2) throw std::runtime_error( "too many arguments for 'link' tokenizer");
				m_name = arg[0];
				m_pattern = getPattern( m_name);
				if (arg.size() == 2)
				{
					m_attribute = getAttribute( arg[1]);
				}
			}
		}

		/// \brief Destructor
		virtual ~Argument(){}

		const std::string& name() const		{return m_name;}
		const std::string& pattern() const	{return m_pattern;}
		const std::string& attribute() const	{return m_attribute;}

	private:
		static std::string getPattern( const std::string& name_)
		{
			std::string rt("<");
			if (name_.size())
			{
				rt.append( name_);
			}
			return rt;
		}

		static std::string getAttribute( const std::string& name_)
		{
			if (name_.empty()) return std::string();
			std::string rt( " ");
			rt.append( name_);
			rt.push_back( '=');
			return rt;
		}

	private:
		std::string m_name;
		std::string m_pattern;
		std::string m_attribute;
	};

	class Context
		:public TokenizerInterface::Context
	{
	public:
		Context( const Argument& arg_)
			:m_arg(arg_){}
		Context()
			:m_arg("",""){}

		/// \brief Destructor
		virtual ~Context(){}

		const Argument arg() const	{return m_arg;}

	private:
		Argument m_arg;
	};

	virtual TokenizerInterface::Argument* createArgument( const std::vector<std::string>& arg) const
	{
		return new Argument(arg);
	}

	virtual TokenizerInterface::Context* createContext( const TokenizerInterface::Argument* arg) const
	{
		if (!arg)
		{
			return new Context( Argument( "", ""));
		}
		else
		{
			return new Context( *reinterpret_cast<const Argument*>(arg));
		}
	}

	virtual std::vector<analyzer::Token>
			tokenize( TokenizerInterface::Context* ctx_, const char* src, std::size_t srcsize) const
	{
		Context* ctx = reinterpret_cast<Context*>(ctx_);
		std::vector<analyzer::Token> rt;
		char const* si = findPattern( ctx->arg().pattern(), src, src + srcsize);
		char const* se = src + srcsize;

		for (; si && si != se; si = findPattern( ctx->arg().pattern(), si, se))
		{
			const char* refStart = si - ctx->arg().pattern().size();
			if (*si != ' ' && *si != '>') continue;
			const char* pe = (const char*)std::memchr( si+1, '>', se-si-1);
			if (pe)
			{
				if (ctx->arg().attribute().size())
				{
					char const* ai = findPattern( ctx->arg().attribute(), si, pe);
					if (ai)
					{
						if (ai < pe && *ai == '\'')
						{
							++ai;
							const char* ae = (const char*)std::memchr( ai, '\'', pe-ai);
							if (ae)
							{
								tokenizeTokenClass( m_tokenClass, rt, -(ai-refStart), src, ai, ae);
							}
						}
						else
						{
							char const* ae = (const char*)std::memchr( ai, ' ', pe-ai);
							if (!ae) ae = (const char*)std::memchr( ai, '>', pe-ai);
							if (ae)
							{
								tokenizeTokenClass( m_tokenClass, rt, -(ai-refStart), src, ai, ae);
							}
						}
					}
				}
				else
				{
					tokenizeTokenClass( m_tokenClass, rt, -(si-refStart), src, si, pe);
				}
				si = pe+1;
			}
		}
		return rt;
	}
private:
	TokenClass m_tokenClass;
};


const TokenizerInterface* strus::tokenizerWikimediaUrlId()
{
	static const TokenizerWikimediaUrl rt( TokenClassContent, TokenizerWikimediaUrl::Link);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaUrlContent()
{
	static const TokenizerWikimediaUrl rt( TokenClassContent, TokenizerWikimediaUrl::Title);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaUrlWord()
{
	static const TokenizerWikimediaUrl rt( TokenClassWordSep, TokenizerWikimediaUrl::Title);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaUrlSplit()
{
	static const TokenizerWikimediaUrl rt( TokenClassWhiteSpaceSep, TokenizerWikimediaUrl::Title);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaLinkId()
{
	static const TokenizerWikimediaLink rt( TokenClassContent, TokenizerWikimediaLink::Id);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaLinkContent()
{
	static const TokenizerWikimediaLink rt( TokenClassContent, TokenizerWikimediaLink::Text);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaLinkSplit()
{
	static const TokenizerWikimediaLink rt( TokenClassWhiteSpaceSep, TokenizerWikimediaLink::Text);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaLinkWord()
{
	static const TokenizerWikimediaLink rt( TokenClassWordSep, TokenizerWikimediaLink::Text);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaCitationContent()
{
	static const TokenizerWikimediaCitation rt( TokenClassContent);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaCitationSplit()
{
	static const TokenizerWikimediaCitation rt( TokenClassWhiteSpaceSep);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaCitationWord()
{
	static const TokenizerWikimediaCitation rt( TokenClassWordSep);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaReferenceContent()
{
	static const TokenizerWikimediaReference rt( TokenClassContent);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaReferenceSplit()
{
	static const TokenizerWikimediaReference rt( TokenClassWhiteSpaceSep);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaReferenceWord()
{
	static const TokenizerWikimediaReference rt( TokenClassWordSep);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaTextWord()
{
	static const TokenizerWikimediaTextWord rt;
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaTextSplit()
{
	static const TokenizerWikimediaTextSplit rt;
	return &rt;
}


