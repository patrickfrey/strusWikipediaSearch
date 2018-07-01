/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/// \brief Intermediate document format for converting "Wikimedia XML" to pure and simplified XML
/// \file documentStructure.cpp
#include "documentStructure.hpp"
#include "outputString.hpp"
#include "textwolf/istreamiterator.hpp"
#include "textwolf/xmlscanner.hpp"
#include "textwolf/xmlprinter.hpp"
#include "textwolf/charset_utf8.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>

typedef textwolf::XMLPrinter<textwolf::charset::UTF8,textwolf::charset::UTF8,std::string> XmlPrinterBase;

/// \brief strus toplevel namespace
using namespace strus;

class XmlPrinter
	:public XmlPrinterBase
{
public:
	XmlPrinter()
		:XmlPrinterBase(){}

	bool isInTagDeclaration()
	{
		return (state() == XmlPrinterBase::TagElement);
	}

	void printHeader( std::string& buf)
	{
		if (!XmlPrinterBase::printHeader( "UTF-8", "yes", buf))
		{
			const char* errstr = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (errstr?errstr:"") + " when printing XML header");
		}
	}

	void printOpenTag( const std::string& tag, std::string& buf)
	{
		if (!XmlPrinterBase::printOpenTag( tag.c_str(), tag.size(), buf))
		{
			const char* errstr = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (errstr?errstr:"") + " when printing open tag: " + outputString(tag.c_str(),tag.c_str()+tag.size()));
		}
	}

	void printOpenTag( const char* name, std::string& buf)
	{
		if (!XmlPrinterBase::printOpenTag( name, std::strlen(name), buf))
		{
			const char* errstr = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (errstr?errstr:"") + " when printing open tag: " + outputString(name,name+std::strlen(name)));
		}
	}

	void printAttribute( const std::string& name, std::string& buf)
	{
		if (!XmlPrinterBase::printAttribute( name.c_str(), name.size(), buf))
		{
			const char* errstr = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (errstr?errstr:"") + " when printing attribute: " + outputString(name.c_str(),name.c_str()+name.size()));
		}
	}

	void printAttribute( const char* name, std::string& buf)
	{
		if (!XmlPrinterBase::printAttribute( name, std::strlen(name), buf))
		{
			const char* errstr = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (errstr?errstr:"") + " when printing attribute: " + outputString(name,name+std::strlen(name)));
		}
	}

	void printValue( const std::string& val, std::string& buf)
	{
		if (!XmlPrinterBase::printValue( val.c_str(), val.size(), buf))
		{
			const char* err = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (err?err:"") + " when printing: " + outputString(val.c_str(),val.c_str()+val.size()));
		}
	}

	void printValue( const char* si, const char* se, std::string& buf)
	{
		if (!XmlPrinterBase::printValue( si, se-si, buf))
		{
			const char* err = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (err?err:"") + " when printing: " + outputString(si,se));
		}
	}

	void switchToContent( std::string& buf)
	{
		if (!XmlPrinterBase::printValue( "", 0, buf))
		{
			const char* err = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (err?err:"") + " when switching to content");
		}
	}

	void printCloseTag( std::string& buf)
	{
		if (!XmlPrinterBase::printCloseTag( buf))
		{
			const char* err = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (err?err:"") + " when printing close tag");
		}
	}
};

static std::string collectEntityText( std::vector<Paragraph>::const_iterator pi, const std::vector<Paragraph>::const_iterator& pe)
{
	std::string rt;
	for (; pi != pe && pi->type() != Paragraph::EntityEnd; ++pi)
	{
		if (pi->type() == Paragraph::Text || pi->type() == Paragraph::NoWiki || pi->type() == Paragraph::Math)
		{
			rt.append( pi->text());
		}
	}
	return rt;
}

void DocumentStructure::checkStartEndSectionBalance( const std::vector<Paragraph>::const_iterator& start, const std::vector<Paragraph>::const_iterator& end)
{
	std::vector<Paragraph::Type> stk;
	std::vector<Paragraph>::const_iterator ii = start;
	for (; ii != end; ++ii)
	{
		switch (ii->type())
		{
			case Paragraph::EntityStart:
			case Paragraph::QuotationStart:
			case Paragraph::DoubleQuoteStart:
			case Paragraph::BlockQuoteStart:
			case Paragraph::SpanStart:
			case Paragraph::SmallStart:
			case Paragraph::HeadingStart:
			case Paragraph::ListItemStart:
			case Paragraph::AttributeStart:
			case Paragraph::CitationStart:
			case Paragraph::RefStart:
			case Paragraph::PageLinkStart:
			case Paragraph::WebLinkStart:
			case Paragraph::TableStart:
			case Paragraph::TableTitleStart:
			case Paragraph::TableHeadStart:
			case Paragraph::TableRowStart:
			case Paragraph::TableColStart:
				stk.push_back( ii->type());
				break;
			case Paragraph::EntityEnd:
			case Paragraph::QuotationEnd:
			case Paragraph::DoubleQuoteEnd:
			case Paragraph::BlockQuoteEnd:
			case Paragraph::SpanEnd:
			case Paragraph::SmallEnd:
			case Paragraph::HeadingEnd:
			case Paragraph::ListItemEnd:
			case Paragraph::AttributeEnd:
			case Paragraph::CitationEnd:
			case Paragraph::RefEnd:
			case Paragraph::PageLinkEnd:
			case Paragraph::WebLinkEnd:
			case Paragraph::TableEnd:
			case Paragraph::TableTitleEnd:
			case Paragraph::TableHeadEnd:
			case Paragraph::TableRowEnd:
			case Paragraph::TableColEnd:
				if (stk.empty()) throw std::runtime_error( strus::string_format( "structure open/close not balanced: ...%s", ii->typeName()));
				if (stk.back() != Paragraph::invType( ii->type())) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", Paragraph::typeName( stk.back()), ii->typeName()));
				stk.pop_back();
				break;
			case Paragraph::Title:
			case Paragraph::Text:
			case Paragraph::NoWiki:
			case Paragraph::Math:
			case Paragraph::CitationLink:
				break;
		}
	}
	if (!stk.empty()) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", Paragraph::typeName( stk.back()), ""));
}

void DocumentStructure::closeDanglingStructures( const Paragraph::Type& starttype)
{
	while (!m_structStack.empty())
	{
		int startidx = m_structStack.back().start;
		Paragraph para = m_parar[ startidx];
		if (para.type() == starttype)
		{
			return;
		}
		else if (para.type() == Paragraph::EntityStart)
		{
			m_parar[ m_structStack.back().start] = Paragraph( Paragraph::Text, "", "\"");
			m_structStack.pop_back();
		}
		else if (para.type() == Paragraph::QuotationStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::DoubleQuoteStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::BlockQuoteStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::SpanStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::SmallStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::HeadingStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::ListItemStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::AttributeStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::CitationStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::RefStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::PageLinkStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::WebLinkStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableStart)
		{
			if (starttype == Paragraph::TableRowStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableTitleStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableHeadStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableRowStart)
		{
			if (starttype == Paragraph::TableColStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableColStart)
		{
			finishStructure( startidx);
		}
		else
		{
			throw std::runtime_error("internal: unknown structure");
		}
	}
}

void DocumentStructure::addQuoteItem( Paragraph::Type startType)
{
	Paragraph::Type endType = Paragraph::invType( startType);
	if (!m_structStack.empty() && m_parar[ m_structStack.back().start].type() == startType)
	{
		m_parar.push_back( Paragraph( endType, "", ""));
		if (startType == Paragraph::EntityStart)
		{
			m_parar[ m_structStack.back().start].setId( collectEntityText( m_parar.begin() + m_structStack.back().start, m_parar.end()));
		}
		m_structStack.pop_back();
	}
	else
	{
		m_structStack.push_back( StructRef( 0, m_parar.size()));
		m_parar.push_back( Paragraph( startType, "", ""));
	}
}

void DocumentStructure::openAutoCloseItem( Paragraph::Type startType, const char* prefix, int lidx)
{
	Paragraph::Type endType = Paragraph::invType( startType);
	while (!m_structStack.empty() && m_parar[ m_structStack.back().start].type() == startType && m_structStack.back().idx <= lidx)
	{
		Paragraph para = m_parar[ m_structStack.back().start];
		m_parar.push_back( Paragraph( endType, para.id(), para.text()));
		m_structStack.pop_back();
	}
	m_structStack.push_back( StructRef( lidx, m_parar.size()));
	if (lidx > 0)
	{
		m_parar.push_back( Paragraph( startType, strus::string_format("%s%d", prefix, lidx), ""));
	}
	else
	{
		m_parar.push_back( Paragraph( startType, prefix, ""));
	}
}

void DocumentStructure::closeAutoCloseItem( Paragraph::Type startType)
{
	Paragraph::Type endType = Paragraph::invType( startType);
	if (m_structStack.empty()) return;
	Paragraph para = m_parar[ m_structStack.back().start];
	if (para.type() == startType)
	{
		m_parar.push_back( Paragraph( endType, para.id(), para.text()));
		m_structStack.pop_back();
	}
}

void DocumentStructure::openStructure( Paragraph::Type startType, const char* prefix, int lidx)
{
	if (startType == Paragraph::TableStart)
	{
		m_tableDefs.push_back( TableDef( m_parar.size()));
	}
	m_structStack.push_back( StructRef( 0, m_parar.size()));
	if (lidx > 0)
	{
		m_parar.push_back( Paragraph( startType, strus::string_format("%s%d", prefix, lidx), ""));
	}
	else
	{
		m_parar.push_back( Paragraph( startType, prefix, ""));
	}
}

void DocumentStructure::finishStructure( int startidx)
{
	Paragraph para = m_parar[ startidx];
	if (para.type() == Paragraph::PageLinkStart)
	{
		if (startidx == (int)m_parar.size()-1 && para.text().empty())
		{
			m_parar[ startidx].setText( para.id());
		}
	}
	Paragraph::Type endType = Paragraph::invType( para.type());
	m_parar.push_back( Paragraph( endType, "", ""));
	if (endType == Paragraph::CitationEnd)
	{
		m_citations.insert( m_citations.end(), m_parar.begin() + startidx, m_parar.end());
		m_parar.resize( startidx);
		m_parar.push_back( Paragraph( Paragraph::CitationLink, para.id(), ""));
	}
	else if (endType == Paragraph::TableEnd)
	{
		if (!m_tableDefs.empty())
		{
			m_tableDefs.pop_back();
		}
		else
		{
			m_errors.push_back( strus::string_format( "close of table called without table definition"));
		}
	}
	else if (endType == Paragraph::TableRowEnd)
	{
		if (!m_tableDefs.empty())
		{
			m_tableDefs.back().coliter = 0;
		}
		else
		{
			m_errors.push_back( strus::string_format( "close of table row called without table definition"));
		}
	}
	m_structStack.pop_back();
}

void DocumentStructure::closeStructure( Paragraph::Type startType)
{
	closeDanglingStructures( startType);
	if (m_structStack.empty())
	{
		m_errors.push_back( strus::string_format( "close of %s structure called without open ()", Paragraph::structTypeName( Paragraph::structType( startType))));
		return;
	}
	int startidx = m_structStack.back().start;
	const Paragraph& para = m_parar[ startidx];
	if (para.type() == startType)
	{
		finishStructure( startidx);
	}
	else
	{
		const char* stnam = Paragraph::structTypeName( Paragraph::structType( para.type()));
		m_errors.push_back( strus::string_format( "close of %s structure called without open (%s)", Paragraph::structTypeName( Paragraph::structType( startType)), stnam));
		return;
	}
}

void DocumentStructure::closeOpenStructures()
{
	closeDanglingStructures( Paragraph::Title/*no match => remove all*/);
	m_parar.insert( m_parar.end(), m_citations.begin(), m_citations.end());
	m_citations.clear();
}

static std::string getDocidFromTitle( const std::string& txt)
{
	std::string rt;
	char const* si = txt.c_str();
	while (*si && (unsigned char)*si <= 32) ++si;
	while (*si)
	{
		char ch = *si | 32;
		if ((unsigned char)*si <= 32)
		{
			while (*si && (unsigned char)*si <= 32) ++si;
			rt.push_back( '_');
		}
		else if ((ch >= 'a' && ch <= 'z') || (*si >= '0' && *si <= '9') || *si == '-' || *si == '_')
		{
			rt.push_back( *si++);
		}
		else if ((unsigned char)*si >= 128)
		{
			rt.push_back( '#');
			while ((unsigned char)*si >= 128)
			{
				char buf[ 16];
				std::snprintf( buf, sizeof(buf), "%02x", (unsigned int)(unsigned char)*si);
				rt.append( buf);
				++si;
			}
		}
		else
		{
			rt.push_back( '#');
			char buf[ 16];
			std::snprintf( buf, sizeof(buf), "%02x", (unsigned int)(unsigned char)*si);
			rt.append( buf);
			++si;
		}
	}
	if (!rt.empty() && rt[ rt.size()-1] == '_') rt.resize( rt.size()-1);
	return rt;
}

static std::string encodeXmlContentString( const std::string& txt, bool encodeEoln)
{
	std::string rt;
	char const* si = txt.c_str();
	for (; *si; ++si)
	{
		if ((unsigned char)*si < 32 && encodeEoln)
		{
			rt.push_back( ' ');
		}
		if (*si == '\"')
		{
			rt.append( "&quot;");
		}
		else if (*si == '&')
		{
			rt.append( "&amp;");
		}
		else if (*si == '<')
		{
			rt.append( "&lt;");
		}
		else if (*si == '>')
		{
			rt.append( "&gt;");
		}
		else
		{
			rt.push_back( *si);
		}
	}
	return rt;
}

void DocumentStructure::setTitle( const std::string& text)
{
	Paragraph para( Paragraph::Title, m_id = getDocidFromTitle( text), text);
	if (!m_parar.empty() && m_parar[0].type() == Paragraph::Title)
	{
		m_parar[0] = para;
	}
	else
	{
		m_parar.insert( m_parar.begin(), para);
	}
}

Paragraph::StructType DocumentStructure::currentStructType() const
{
	if (m_structStack.empty()) return Paragraph::StructNone;
	return m_parar[ m_structStack.back().start].structType();
}

static bool isSpaceOnlyText( const std::string& text)
{
	char const* si = text.c_str();
	for (; *si && (unsigned char)*si <= 32; ++si){}
	return !*si;
}

void DocumentStructure::clearOpenText()
{
	if (!m_parar.empty() && m_parar.back().type() == Paragraph::Text)
	{
		m_parar.pop_back();
	}
}

void DocumentStructure::addSingleItem( Paragraph::Type type, const std::string& id, const std::string& text, bool joinText)
{
	if (joinText)
	{
		if (!m_parar.empty() && m_parar.back().type() == type)
		{
			m_parar.back().addText( text);
		}
		else if (isSpaceOnlyText( text))
		{
			//... ignore it
		}
		else
		{
			m_parar.push_back( Paragraph( type, id, text));
		}
	}
	else
	{
		m_parar.push_back( Paragraph( type, id, text));
	}
}

void DocumentStructure::finish()
{
	closeOpenStructures();
	checkStartEndSectionBalance( m_parar.begin(), m_parar.end());
}

std::string DocumentStructure::getInputXML( const std::string& title, const std::string& content)
{
	std::string rt;
	std::vector<Paragraph::StructType> stk;
	XmlPrinter output;
	output.printHeader( rt);
	output.printOpenTag( "mediawiki", rt);
	output.printOpenTag( "page", rt);
	output.printOpenTag( "title", rt);
	output.printValue( title, rt);
	output.printCloseTag(rt);//title
	output.printOpenTag( "revision", rt);
	output.printOpenTag( "text", rt);
	output.printValue( content, rt);
	output.printCloseTag(rt);//text
	output.printCloseTag(rt);//revision
	output.printCloseTag(rt);//page
	output.printCloseTag(rt);//mediawiki
	return rt;
}

static void printTagOpen( XmlPrinter& output, std::string& rt, const char* tagnam, const std::string& id, const std::string& text)
{
	output.printOpenTag( tagnam, rt);
	if (!id.empty())
	{
		output.printAttribute( "id", rt);
		output.printValue( id, rt);
	}
	if (!text.empty())
	{
		output.switchToContent( rt);
		output.printValue( text, rt);
	}
}

static void printTagContent( XmlPrinter& output, std::string& rt, const char* tagnam, const std::string& id, const std::string& text)
{
	printTagOpen( output, rt, tagnam, id, text);
	output.printCloseTag( rt);
}

std::string DocumentStructure::toxml( bool beautified) const
{
	std::string rt;
	std::vector<Paragraph::StructType> stk;
	XmlPrinter output;
	output.printHeader( rt);
	output.printOpenTag( "doc", rt);
	std::vector<Paragraph>::const_iterator pi = m_parar.begin(), pe = m_parar.end();
	for(; pi != pe; ++pi)
	{
		if (beautified) output.printValue( std::string("\n") + std::string( 2*stk.size(), ' '), rt);
		switch (pi->type())
		{
			case Paragraph::Title:
				printTagContent( output, rt, "docid", "", pi->id());
				if (beautified) output.printValue( std::string("\n") + std::string( 2*stk.size(), ' '), rt);
				printTagContent( output, rt, "title", "", pi->text());
				break;
			case Paragraph::EntityStart:
				stk.push_back( Paragraph::StructEntity);
				printTagOpen( output, rt, "entity", pi->id(), pi->text());
				break;
			case Paragraph::EntityEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::QuotationStart:
				stk.push_back( Paragraph::StructQuotation);
				printTagOpen( output, rt, "quot", pi->id(), pi->text());
				break;
			case Paragraph::QuotationEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::DoubleQuoteStart:
				stk.push_back( Paragraph::StructDoubleQuote);
				printTagOpen( output, rt, "dquot", pi->id(), pi->text());
				break;
			case Paragraph::DoubleQuoteEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::BlockQuoteStart:
				stk.push_back( Paragraph::StructBlockQuote);
				printTagOpen( output, rt, "bquot", pi->id(), pi->text());
				break;
			case Paragraph::BlockQuoteEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::SpanStart:
				stk.push_back( Paragraph::StructSpan);
				printTagOpen( output, rt, "span", pi->id(), pi->text());
				break;
			case Paragraph::SpanEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::SmallStart:
				stk.push_back( Paragraph::StructSmall);
				printTagOpen( output, rt, "small", pi->id(), pi->text());
				break;
			case Paragraph::SmallEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::HeadingStart:
				stk.push_back( Paragraph::StructHeading);
				printTagOpen( output, rt, "heading", pi->id(), pi->text());
				break;
			case Paragraph::HeadingEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::ListItemStart:
				stk.push_back( Paragraph::StructList);
				printTagOpen( output, rt, "list", pi->id(), pi->text());
				break;
			case Paragraph::ListItemEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::AttributeStart:
				stk.push_back( Paragraph::StructAttribute);
				if (output.isInTagDeclaration())
				{
					if (pi->id().empty())
					{
						printTagOpen( output, rt, "attr", pi->id(), pi->text());
					}
					else if (!pi->text().empty())
					{
						output.printAttribute( pi->id(), rt);
						output.printValue( pi->text(), rt);
					}
				}
				else
				{
					printTagOpen( output, rt, "attr", pi->id(), pi->text());
				}
				break;
			case Paragraph::AttributeEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::CitationStart:
				stk.push_back( Paragraph::StructCitation);
				printTagOpen( output, rt, "citation", pi->id(), pi->text());
				break;
			case Paragraph::CitationEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::RefStart:
				stk.push_back( Paragraph::StructRef);
				printTagOpen( output, rt, "ref", pi->id(), pi->text());
				break;
			case Paragraph::RefEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::PageLinkStart:
				stk.push_back( Paragraph::StructPageLink);
				printTagOpen( output, rt, "pagelink", pi->id(), pi->text());
				break;
			case Paragraph::PageLinkEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::WebLinkStart:
				stk.push_back( Paragraph::StructWebLink);
				printTagOpen( output, rt, "weblink", pi->id(), pi->text());
				break;
			case Paragraph::WebLinkEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableStart:
				stk.push_back( Paragraph::StructTable);
				printTagOpen( output, rt, "table", pi->id(), pi->text());
				break;
			case Paragraph::TableEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableTitleStart:
				stk.push_back( Paragraph::StructTableTitle);
				printTagOpen( output, rt, "heading", pi->id(), pi->text());
				break;
			case Paragraph::TableTitleEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableHeadStart:
				stk.push_back( Paragraph::StructTableHead);
				printTagOpen( output, rt, "head", pi->id(), pi->text());
				break;
			case Paragraph::TableHeadEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableRowStart:
				stk.push_back( Paragraph::StructTableRow);
				printTagOpen( output, rt, "row", pi->id(), pi->text());
				break;
			case Paragraph::TableRowEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableColStart:
				stk.push_back( Paragraph::StructTableCol);
				printTagOpen( output, rt, "column", pi->id(), pi->text());
				break;
			case Paragraph::TableColEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::Text:
				if (stk.empty())
				{
					printTagContent( output, rt, "text", pi->id(), pi->text());
				}
				else
				{
					output.printValue( pi->text(), rt);
				}
				break;
			case Paragraph::NoWiki:
				if (stk.empty())
				{
					printTagContent( output, rt, "nowiki", pi->id(), pi->text());
				}
				else
				{
					output.printValue( pi->text(), rt);
				}
				break;
			case Paragraph::Math:
				if (stk.empty())
				{
					printTagContent( output, rt, "math", pi->id(), pi->text());
				}
				else
				{
					output.printValue( pi->text(), rt);
				}
				break;
			case Paragraph::CitationLink:
				printTagContent( output, rt, "citlink", pi->id(), pi->text());
				break;
		}
	}
	output.printCloseTag( rt);
	return rt;
}

std::string DocumentStructure::tostring() const
{
	std::ostringstream out;
	std::vector<Paragraph>::const_iterator pi = m_parar.begin(), pe = m_parar.end();
	for(; pi != pe; ++pi)
	{
		out << pi->typeName() << " " << encodeXmlContentString( pi->id(), true) << " \"" << encodeXmlContentString( pi->text(), true) << "\"\n";
	}
	return out.str();
}

std::string DocumentStructure::statestring() const
{
	std::string rt;
	std::vector<StructRef>::const_iterator ri = m_structStack.begin(), re = m_structStack.end();
	for (int ridx=0; re != ri; --re,++ridx)
	{
		if (ridx) rt.append( ", ");
		rt.append( Paragraph::structTypeName( Paragraph::structType( m_parar[ (*(re-1)).start].type())));
	}
	if (!m_tableDefs.empty())
	{
		rt.append( strus::string_format( " [tables %d]", (int)m_tableDefs.size()));
	}
	return rt;
}
