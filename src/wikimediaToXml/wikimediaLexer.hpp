/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/// \brief Lexer for Wikimedia format
/// \file documentStructure.hpp
#ifndef _STRUS_WIKIPEDIA_WIKIMEDIA_LEXER_HPP_INCLUDED
#define _STRUS_WIKIPEDIA_WIKIMEDIA_LEXER_HPP_INCLUDED
#include "strus/base/numstring.hpp"
#include "strus/base/string_conv.hpp"
#include <string>
#include <vector>
#include <map>
#include <utility>

/// \brief strus toplevel namespace
namespace strus {

struct WikimediaLexem
{
	enum Id
	{
		EoF,
		Error,
		Text,
		String,
		Char,
		Math,
		NoWiki,
		NoData,
		Code,
		Timestamp,
		Url,
		BibRef,
		Redirect,
		Markup,
		OpenHeading,
		CloseHeading,
		OpenRef,
		CloseRef,
		HeadingItem,
		ListItem,
		EndOfLine,
		QuotationMarker,
		MultiQuoteMarker,
		OpenSpan,
		CloseSpan,
		OpenFormat,
		CloseFormat,
		OpenBlockQuote,
		CloseBlockQuote,
		OpenDiv,
		CloseDiv,
		OpenPoem,
		ClosePoem,
		OpenCitation,
		CloseCitation,
		OpenWWWLink,
		CloseWWWLink,
		OpenPageLink,
		ClosePageLink,
		OpenTable,
		CloseTable,
		TableTitle,
		TableRowDelim,
		TableColDelim,
		TableHeadDelim,
		ColDelim,
		DoubleColDelim,
		DoubleColDelimNewLine,
		TextBreak,
		Break
	};
	static const char* idName( Id lexemId)
	{
		static const char* ar[] =
		{
			"EOF","Error","Text","String","Char","Math","NoWiki","NoData","Code","Timestamp","URL","BibRef",
			"Redirect","Markup","OpenHeading","CloseHeading",
			"OpenRef","CloseRef","HeadingItem","ListItem",
			"EndOfLine","QuotationMarker","MultiQuoteMarker",
			"OpenSpan","CloseSpan","OpenFormat","CloseFormat","OpenBlockQuote","CloseBlockQuote",
			"OpenDiv","CloseDiv",
			"OpenPoem","ClosePoem","OpenCitation","CloseCitation",
			"OpenWWWLink","CloseWWWLink","OpenPageLink","ClosePageLink",
			"OpenTable", "CloseTable","TableTitle","TableRowDelim","TableColDelim",
			"TableHeadDelim","ColDelim","DoubleColDelim","DoubleColDelimNewLine","TextBreak","Break",0
		};
		return ar[lexemId];
	}
	typedef std::map<std::string,std::string> AttributeMap;

	WikimediaLexem( Id id_, int idx_, const std::string& value_, const AttributeMap& attributes_=AttributeMap())
		:id(id_),idx(idx_),value(value_),attributes(attributes_){}
	WikimediaLexem( Id id_)
		:id(id_),idx(0),value(),attributes(){}
	WikimediaLexem( const WikimediaLexem& o)
		:id(o.id),idx(o.idx),value(o.value),attributes(o.attributes){}

	int attributeToInt( const std::string& name_) const
	{
		std::string name( string_conv::tolower( name_));
		AttributeMap::const_iterator ai = attributes.find( name);
		if (ai == attributes.end()) return 1;
		strus::NumParseError err = NumParseOk;
		int rt = strus::uintFromString( ai->second, 1<<15, err);
		return (err == NumParseOk) ? rt:-1;
	}

	int colspan() const
	{
		return attributeToInt( "colspan");
	}
	int rowspan() const
	{
		return attributeToInt( "rowspan");
	}

	Id id;
	int idx;
	std::string value;
	AttributeMap attributes;
};

class WikimediaLexer
{
public:

	WikimediaLexer( const char* src, std::size_t size)
		:m_prev_si(src),m_si(src),m_se(src+size),m_curHeading(0){}

	WikimediaLexem next();
	std::string rest() const;
	std::string currentSourceExtract( int maxlen) const;
	void unget()					{m_si = m_prev_si;}

private:
	std::string tryParseIdentifier( char assignop);
	std::string tryParseURLPath();
	std::string tryParseURL();
	std::string tryParseFilePath();
	std::string tryParseLinkId();
	std::string tryParseBibRef();
	std::string tryParseBookRef();
	std::string tryParseIsbnRef();
	std::string tryParseBigHexNum();
	std::string tryParseTimestamp();
	std::string tryParseRepPattern( int minlen);
	std::string tryParseCode();
	bool eatFollowChar( char expectChr);

private:
	char const* m_prev_si;
	char const* m_si;
	const char* m_se;
	int m_curHeading;
};

}//namespace
#endif

