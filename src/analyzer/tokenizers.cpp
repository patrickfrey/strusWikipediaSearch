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

const char* skipUrl( char const* si, const char* se, char delim)
{
	for (; si != se && (*si|32) >= 'a' && (*si|32) <= 'z'; ++si){}
	if (si[0] == ':' && si[1] == '/')
	{
		return (const char*)std::memchr( si, delim, se-si);
	}
	return 0;
}

static const char* skipToToken( char const* si, const char* se)
{
	while (si && si != se)
	{
		if (*si == '&')
		{
			if (std::memcmp( si, "&quot;", 6) == 0)
			{
				si += 6;
				continue;
			}
			if (std::memcmp( si, "&lt;", 6) == 0)
			{
				si += 4;
				si = findPattern( "&gt;", si, se);
				continue;
			}
		}
		else if (*si == '[')
		{
			++si;
			if (si < se && si[1] == '[')
			{
				si = findPattern( "]]", si, se);
				continue;
			}
			char const* ue = skipUrl( si, se, ']');
			if (ue)
			{
				si = ue+1;
				continue;
			}
		}
		else if (*si == '{')
		{
			++si;
			if (si < se && si[1] == '{')
			{
				si = findPattern( "}}", si, se);
				continue;
			}
		}
	}
	return (si)?si:se;
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

	bool operator[]( char ch) const		{return m_ar[ (unsigned char)ch];}
private:
	bool m_ar[256];
};

static void tokenizeWordSeparation( std::vector<analyzer::Token>& res, const char* src, char const* si, const char* se)
{
	static const CharTable delimiter("{}[]<>&:.;,!?%/()+-'\"`=");
	for (si = skipToToken( si, se); si != se; si = skipToToken( si, se))
	{
		while (si != se && ((unsigned char)*si <= 32 || delimiter[ *si])){}
		if (si != se)
		{
			char const* start = si;
			while (si != se && ((unsigned char)*si > 32 && !delimiter[ *si])){}
			res.push_back( analyzer::Token( start - src, si - start));
		}
	}
}

static void tokenizeWhiteSpace( std::vector<analyzer::Token>& res, const char* src, char const* si, const char* se)
{
	static const CharTable delimiter("{[&");
	
	for (si = skipToToken( si, se); si != se; si = skipToToken( si, se))
	{
		while (si != se && ((unsigned char)*si <= 32 || delimiter[ *si])){}
		if (si != se)
		{
			char const* start = si;
			while (si != se && ((unsigned char)*si > 32 && !delimiter[ *si])){}
			res.push_back( analyzer::Token( start - src, si - start));
		}
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
		tokenizeWordSeparation( rt, src, src, src+srcsize);
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
		tokenizeWhiteSpace( rt, src, src, src+srcsize);
		return rt;
	}
};

enum TokenClass
{
	TokenClassContent,
	TokenClassWhiteSpaceSep,
	TokenClassWordSep
};

static void tokenizeTokenClass( TokenClass tokenClass_, std::vector<analyzer::Token>& res, const char* src, char const* si, const char* se)
{
	switch (tokenClass_)
	{
		case TokenClassContent:
			res.push_back( analyzer::Token( si - src, se-si));
			break;
		case TokenClassWhiteSpaceSep:
			tokenizeWhiteSpace( res, src, si, se);
			break;
		case TokenClassWordSep:
			tokenizeWordSeparation( res, src, si, se);
			break;
	}
}


class TokenizerWikimediaLink
	:public TokenizerInterface
{
public:
	explicit TokenizerWikimediaLink( TokenClass tokenClass_)
		:m_tokenClass(tokenClass_){}
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
			char const* pe = (const char*)std::memchr( si, ']', se-si);
			if (pe && pe[1] == ']')
			{
				if (ctx->arg().name().empty())
				{
					char const* start = (const char*)std::memchr( si, ':', pe-si);
					tokenizeTokenClass( m_tokenClass, rt, src, start?start:si, pe);
				}
				else
				{
					tokenizeTokenClass( m_tokenClass, rt, src, si, pe);
				}
				si = pe+2;
			}
		}
		return rt;
	}

private:
	TokenClass m_tokenClass;
};

class TokenizerWikimediaUrl
	:public TokenizerInterface
{
public:
	TokenizerWikimediaUrl(){}
	virtual ~TokenizerWikimediaUrl(){}

	virtual std::vector<analyzer::Token>
			tokenize( Context* ctx, const char* src, std::size_t srcsize) const
	{
		std::vector<analyzer::Token> rt;
		char const* si = (const char*)std::memchr( src, '[', srcsize);
		char const* se = src + srcsize;

		for (; si; si = (const char*)std::memchr( si, '[', se-si))
		{
			++si;
			char const* pe = skipUrl( si, se, ']');
			if (pe)
			{
				rt.push_back( analyzer::Token( si-src, pe-si));
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
							tokenizeTokenClass( m_tokenClass, rt, src, ai, ae);
						}
					}
				}
				else
				{
					tokenizeTokenClass( m_tokenClass, rt, src, si, pe);
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
			std::string rt("&lt;");
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

		for (; si; si = findPattern( ctx->arg().pattern(), si, se))
		{
			if (*si != ' ' && *si != '&') continue;
			char const* pe = findPattern( "&gt;", si, se);
			if (pe)
			{
				if (ctx->arg().attribute().size())
				{
					char const* ai = findPattern( ctx->arg().attribute(), si, pe);
					if (ai)
					{
						if (ai+6 < pe && 0==std::memcmp( ai, "&quot;", 6))
						{
							ai+=6;
							char const* ae = findPattern( "&quot;", ai, pe);
							if (ae)
							{
								tokenizeTokenClass( m_tokenClass, rt, src, ai, ae);
							}
						}
						else
						{
							char const* ae = (const char*)std::memchr( ai, ' ', pe-ai);
							if (!ae) ae = (const char*)std::memchr( ai, '&', pe-ai);
							if (ae)
							{
								tokenizeTokenClass( m_tokenClass, rt, src, ai, ae);
							}
						}
					}
				}
				else
				{
					tokenizeTokenClass( m_tokenClass, rt, src, si, pe);
				}
				si = pe+4;
			}
		}
		return rt;
	}
private:
	TokenClass m_tokenClass;
};


const TokenizerInterface* strus::tokenizerWikimediaUrlContent()
{
	static const TokenizerWikimediaUrl rt;
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaLinkContent()
{
	static const TokenizerWikimediaLink rt( TokenClassContent);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaLinkSplit()
{
	static const TokenizerWikimediaLink rt( TokenClassWhiteSpaceSep);
	return &rt;
}

const TokenizerInterface* strus::tokenizerWikimediaLinkWord()
{
	static const TokenizerWikimediaLink rt( TokenClassWordSep);
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


