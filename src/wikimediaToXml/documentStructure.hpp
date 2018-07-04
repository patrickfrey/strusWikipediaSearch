/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/// \brief Intermediate document format for converting "Wikimedia XML" to pure and simplified XML
/// \file documentStructure.hpp
#ifndef _STRUS_WIKIPEDIA_DOCUMENT_STRUCTURE_HPP_INCLUDED
#define _STRUS_WIKIPEDIA_DOCUMENT_STRUCTURE_HPP_INCLUDED
#include "strus/base/string_format.hpp"
#include <string>
#include <map>
#include <vector>
#include <utility>

/// \brief strus toplevel namespace
namespace strus {

class Paragraph
{
public:
	enum Type {
			Title,
			EntityStart,
			EntityEnd,
			QuotationStart,
			QuotationEnd,
			DoubleQuoteStart,
			DoubleQuoteEnd,
			BlockQuoteStart,
			BlockQuoteEnd,
			DivStart,
			DivEnd,
			PoemStart,
			PoemEnd,
			SpanStart,
			SpanEnd,
			FormatStart,
			FormatEnd,
			HeadingStart,
			HeadingEnd,
			ListItemStart,
			ListItemEnd,
			AttributeStart,
			AttributeEnd,
			CitationStart,
			CitationEnd,
			RefStart,
			RefEnd,
			PageLinkStart,
			PageLinkEnd,
			WebLinkStart,
			WebLinkEnd,
			TableStart,
			TableEnd,
			TableTitleStart,
			TableTitleEnd,
			TableHeadStart,
			TableHeadEnd,
			TableRowStart,
			TableRowEnd,
			TableColStart,
			TableColEnd,
			Text,
			Char,
			NoWiki,
			Math,
			CitationLink,
			TableLink
	};
	static const char* typeName( Type tp)
	{
		static const char* ar[] = {
			"Title",
			"EntityStart",
			"EntityEnd",
			"QuotationStart",
			"QuotationEnd",
			"DoubleQuoteStart",
			"DoubleQuoteEnd",
			"BlockQuoteStart",
			"BlockQuoteEnd",
			"DivStart",
			"DivEnd",
			"PoemStart",
			"PoemEnd",
			"SpanStart",
			"SpanEnd",
			"FormatStart",
			"FormatEnd",
			"HeadingStart",
			"HeadingEnd",
			"ListItemStart",
			"ListItemEnd",
			"AttributeStart",
			"AttributeEnd",
			"CitationStart",
			"CitationEnd",
			"RefStart",
			"RefEnd",
			"PageLinkStart",
			"PageLinkEnd",
			"WebLinkStart",
			"WebLinkEnd",
			"TableStart",
			"TableEnd",
			"TableTitleStart",
			"TableTitleEnd",
			"TableHeadStart",
			"TableHeadEnd",
			"TableRowStart",
			"TableRowEnd",
			"TableColStart",
			"TableColEnd",
			"Text",
			"Char",
			"NoWiki",
			"Math",
			"CitationLink",
			"TableLink", 0};
		return ar[tp];
	}
	enum StructType {
			StructNone,
			StructEntity,
			StructQuotation,
			StructDoubleQuote,
			StructBlockQuote,
			StructDiv,
			StructPoem,
			StructSpan,
			StructFormat,
			StructHeading,
			StructList,
			StructAttribute,
			StructCitation,
			StructRef,
			StructPageLink,
			StructWebLink,
			StructTable,
			StructTableTitle,
			StructTableHead,
			StructTableRow,
			StructTableCol
	};
	static const char* structTypeName( StructType st)
	{
		static const char* ar[] = {"None","Entity","Quotation","DoubleQuote","BlockQuote","Div","Poem","Span","Format","Heading","List","Attribute","Citation","Ref","PageLink","WebLink","Table","TableTitle","TableHead","TableRow","TableCol"};
		return ar[ st];
	}
	static StructType structType( Type ti)
	{
		static StructType ar[] = {
			StructNone/*Title*/,
			StructEntity/*EntityStart*/,
			StructNone/*EntityEnd*/,
			StructQuotation/*QuotationStart*/,
			StructNone/*QuotationEnd*/,
			StructDoubleQuote/*DoubleQuoteStart*/,
			StructNone/*DoubleQuoteEnd*/,
			StructBlockQuote/*BlockQuoteStart*/,
			StructNone/*BlockQuoteEnd*/,
			StructDiv/*DivStart*/,
			StructNone/*DivEnd*/,
			StructPoem/*BlockPoem*/,
			StructNone/*BlockPoem*/,
			StructSpan/*SpanStart*/,
			StructNone/*SpanEnd*/,
			StructFormat/*FormatStart*/,
			StructNone/*FormatEnd*/,
			StructHeading/*HeadingStart*/,
			StructNone/*HeadingEnd*/,
			StructList/*ListItemStart*/,
			StructNone/*ListItemEnd*/,
			StructAttribute/*AttributeStart*/,
			StructNone/*AttributeEnd*/,
			StructCitation/*CitationStart*/,
			StructNone/*CitationEnd*/,
			StructRef/*RefStart*/,
			StructNone/*RefEnd*/,
			StructPageLink/*PageLinkStart*/,
			StructNone/*PageLinkEnd*/,
			StructWebLink/*WebLinkStart*/,
			StructNone/*WebLinkEnd*/,
			StructTable/*TableStart*/,
			StructNone/*TableEnd*/,
			StructTableTitle/*TableTitleStart*/,
			StructNone/*TableTitleEnd*/,
			StructTableHead/*TableHeadStart*/,
			StructNone/*TableHeadEnd*/,
			StructTableRow/*TableRowStart*/,
			StructNone/*TableRowEnd*/,
			StructTableCol/*TableColStart*/,
			StructNone/*TableColEnd*/,
			StructNone/*Text*/,
			StructNone/*NoWiki*/,
			StructNone/*Math*/,
			StructNone/*PageLink*/,
			StructNone/*WebLink*/,
			StructNone/*CitationLink*/,
			StructNone/*TableLink*/};
		return ar[ti];
	}
	static Type invType( Type ti)
	{
		static Type ar[] = {
			Title/*Title*/,
			EntityEnd/*EntityStart*/,
			EntityStart/*EntityEnd*/,
			QuotationEnd/*QuotationStart*/,
			QuotationStart/*QuotationEnd*/,
			DoubleQuoteEnd/*DoubleQuoteStart*/,
			DoubleQuoteStart/*DoubleQuoteEnd*/,
			BlockQuoteEnd/*BlockQuoteStart*/,
			BlockQuoteStart/*BlockQuoteEnd*/,
			DivEnd/*DivStart*/,
			DivStart/*DivEnd*/,
			PoemEnd/*PoemStart*/,
			PoemStart/*PoemEnd*/,
			SpanEnd/*SpanStart*/,
			SpanStart/*SpanEnd*/,
			FormatEnd/*FormatStart*/,
			FormatStart/*FormatEnd*/,
			HeadingEnd/*HeadingStart*/,
			HeadingStart/*HeadingEnd*/,
			ListItemEnd/*ListItemStart*/,
			ListItemStart/*ListItemEnd*/,
			AttributeEnd/*AttributeStart*/,
			AttributeStart/*AttributeEnd*/,
			CitationEnd/*CitationStart*/,
			CitationStart/*CitationEnd*/,
			RefEnd/*RefStart*/,
			RefStart/*RefEnd*/,
			PageLinkEnd/*PageLinkStart*/,
			PageLinkStart/*PageLinkEnd*/,
			WebLinkEnd/*WebLinkStart*/,
			WebLinkStart/*WebLinkEnd*/,
			TableEnd/*TableStart*/,
			TableStart/*TableEnd*/,
			TableTitleEnd/*TableTitleStart*/,
			TableTitleStart/*TableTitleEnd*/,
			TableHeadEnd/*TableHeadStart*/,
			TableHeadStart/*TableHeadEnd*/,
			TableRowEnd/*TableRowStart*/,
			TableRowStart/*TableRowEnd*/,
			TableColEnd/*TableColStart*/,
			TableColStart/*TableColEnd*/,
			Text/*Text*/,
			NoWiki/*NoWiki*/,
			Math/*Math*/,
			CitationLink/*CitationLink*/,
			TableLink/*TableLink*/
			};
		return ar[ti];
	}
	StructType structType() const
	{
		return structType( m_type);
	}
	const char* typeName() const
	{
		return typeName( m_type);
	}
	const char* structTypeName() const
	{
		return structTypeName( structType( m_type));
	}

	Paragraph()
		:m_type(Text),m_id(),m_text(){}
	Paragraph( Type type_, const std::string& id_, const std::string& text_)
		:m_type(type_),m_id(id_),m_text(text_){}
	Paragraph( const Paragraph& o)
		:m_type(o.m_type),m_id(o.m_id),m_text(o.m_text){}

	Type type() const					{return m_type;}
	const std::string& id() const				{return m_id;}
	const std::string& text() const				{return m_text;}

	void setId( const std::string& id_)
	{
		m_id = id_;
	}
	void setText( const std::string& text_)
	{
		m_text = text_;
	}
	void addText( const std::string& text_)
	{
		m_text += text_;
	}

private:
	Type m_type;
	std::string m_id;
	std::string m_text;
};


class DocumentStructure
{
public:
	explicit DocumentStructure()
		:m_id(),m_parar(),m_citations(),m_tables(),m_structStack(),m_tableDefs(),m_errors(),m_tableCnt(0),m_citationCnt(0),m_refCnt(0),m_lastHeadingIdx(0){}
	DocumentStructure( const DocumentStructure& o)
		:m_id(o.m_id),m_parar(o.m_parar),m_citations(o.m_citations),m_tables(o.m_tables),m_structStack(o.m_structStack),m_tableDefs(o.m_tableDefs),m_errors(o.m_errors),m_tableCnt(o.m_tableCnt),m_citationCnt(o.m_citationCnt),m_refCnt(o.m_refCnt),m_lastHeadingIdx(o.m_lastHeadingIdx){}

	const std::string& id() const
	{
		return m_id;
	}
	Paragraph::StructType currentStructType() const;
	int currentStructIndex() const;

	void setTitle( const std::string& text);

	void addText( const std::string& text)
	{
		addSingleItem( Paragraph::Text, "", text, true/*joinText*/);
	}
	void addChar( const std::string& text)
	{
		addSingleItem( Paragraph::Char, "", text, false/*joinText*/);
	}
	void clearOpenText();

	void addMath( const std::string& text)
	{
		addSingleItem( Paragraph::Math, "", text, false/*joinText*/);
	}
	void addNoWiki( const std::string& text)
	{
		addSingleItem( Paragraph::NoWiki, "", text, false/*joinText*/);
	}
	void openRef()
	{
		closeOpenQuoteItems();
		closeAutoCloseItem( Paragraph::RefStart);
		openStructure( Paragraph::RefStart, "ref", ++m_refCnt);
	}
	void closeRef()
	{
		closeStructure( Paragraph::RefStart, "");
	}
	void openPageLink( const std::string& id)
	{
		openStructure( Paragraph::PageLinkStart, id.c_str(), 0);
	}
	void closePageLink()
	{
		closeStructure( Paragraph::PageLinkStart, "");
	}
	void openWebLink( const std::string& id)
	{
		openStructure( Paragraph::WebLinkStart, id.c_str(), 0);
	}
	void closeWebLink();

	void openHeading( int idx)
	{
		m_lastHeadingIdx = idx;
		closeOpenStructures();
		openAutoCloseItem( Paragraph::HeadingStart, "h", idx);
	}
	void addHeadingItem()
	{
		closeOpenStructures();
		openAutoCloseItem( Paragraph::HeadingStart, "h", m_lastHeadingIdx+1);
	}
	void closeHeading()
	{
		closeAutoCloseItem( Paragraph::HeadingStart);
	}
	void addEntityMarker()
	{
		addQuoteItem( Paragraph::EntityStart);
	}
	void addQuotationMarker()
	{
		addQuoteItem( Paragraph::QuotationStart);
	}
	void addDoubleQuoteMarker()
	{
		addQuoteItem( Paragraph::DoubleQuoteStart);
	}
	void openDoubleQuote()
	{
		closeOpenQuoteItems();
		openStructure( Paragraph::DoubleQuoteStart, "", 0);
	}
	void closeDoubleQuote()
	{
		closeStructure( Paragraph::DoubleQuoteStart, "");
	}
	void openBlockQuote()
	{
		closeOpenQuoteItems();
		closeAutoCloseItem( Paragraph::BlockQuoteStart);
		openStructure( Paragraph::BlockQuoteStart, "", 0);
	}
	void closeBlockQuote()
	{
		closeStructure( Paragraph::BlockQuoteStart, "");
	}
	void openDiv()
	{
		closeOpenQuoteItems();
		openStructure( Paragraph::DivStart, "", 0);
	}
	void closeDiv()
	{
		closeStructure( Paragraph::DivStart, "");
	}
	void openPoem()
	{
		closeOpenQuoteItems();
		closeAutoCloseItem( Paragraph::PoemStart);
		openStructure( Paragraph::PoemStart, "", 0);
	}
	void closePoem()
	{
		closeStructure( Paragraph::PoemStart, "");
	}
	void openSpan()
	{
		closeOpenQuoteItems();
		closeAutoCloseItem( Paragraph::SpanStart);
		openStructure( Paragraph::SpanStart, "", 0);
	}
	void closeSpan()
	{
		closeStructure( Paragraph::SpanStart, "");
	}
	void openFormat()
	{
		closeOpenQuoteItems();
		openStructure( Paragraph::FormatStart, "", 0);
	}
	void closeFormat()
	{
		closeStructure( Paragraph::FormatStart, "");
	}
	void implicitOpenTableIfUndefined()
	{
		strus::Paragraph::StructType tp = currentStructType();

		if (tp != strus::Paragraph::StructTable
		&&  tp != strus::Paragraph::StructTableTitle
		&&  tp != strus::Paragraph::StructTableHead
		&&  tp != strus::Paragraph::StructTableRow
		&&  tp != strus::Paragraph::StructTableCol)
		{
			openTable();
		}
	}

	void openTable()
	{
		closeOpenQuoteItems();
		closeDanglingStructures( Paragraph::TableStart);
		openStructure( Paragraph::TableStart, "table", ++m_tableCnt);
	}
	void closeTable()
	{
		if (m_tableDefs.empty())
		{
			addError("table close without table defined");
		}
		else
		{
			closeStructure( Paragraph::TableStart, "");
		}
	}
	void addTableTitle()
	{
		closeDanglingStructures( Paragraph::TableStart);
		if (m_tableDefs.empty())
		{
			addError("table add title without table defined");
			openListItem( 1);
		}
		else
		{
			openAutoCloseItem( Paragraph::TableTitleStart, "title", 0);
		}
	}
	void addTableHead()
	{
		closeDanglingStructures( Paragraph::TableStart);
		if (m_tableDefs.empty())
		{
			addError("table open head without table defined");
			openListItem( 1);
		}
		else
		{
			openAutoCloseItem( Paragraph::TableHeadStart, "head", ++m_tableDefs.back().headitr);
		}
	}
	void addTableRow()
	{
		closeDanglingStructures( Paragraph::TableStart);
		if (m_tableDefs.empty())
		{
			addError("table open row without table defined");
			openListItem( 1);
		}
		else
		{
			m_tableDefs.back().coliter = 0;
			openAutoCloseItem( Paragraph::TableRowStart, "row", ++m_tableDefs.back().rowiter);
		}
	}
	void addTableCol()
	{
		closeDanglingStructures( Paragraph::TableRowStart);
		if (m_tableDefs.empty())
		{
			addError("table open col without table defined");
			openListItem( 1);
		}
		else
		{
			openAutoCloseItem( Paragraph::TableColStart, "col", ++m_tableDefs.back().coliter);
		}
	}
	void addAttribute( const std::string& id)
	{
		closeAutoCloseItem( Paragraph::AttributeStart);
		Paragraph::StructType tp = currentStructType();
		if (tp == Paragraph::StructCitation
		||  tp == Paragraph::StructRef
		||  tp == Paragraph::StructAttribute)
		{
			openAutoCloseItem( Paragraph::AttributeStart, id.c_str(), 0);
		}
		else
		{
			addError( strus::string_format( "add attribute without context: '%s'", id.c_str()));
		}
	}
	void openListItem( int lidx)
	{
		closeOpenEolnItem();
		openAutoCloseItem( Paragraph::ListItemStart, "l", lidx);
	}
	void closeOpenEolnItem()
	{
		closeOpenQuoteItems();
		closeAutoCloseItem( Paragraph::ListItemStart);
		closeAutoCloseItem( Paragraph::HeadingStart);
	}
	void closeOpenQuoteItems();

	void openCitation( const std::string& citclass)
	{
		closeOpenQuoteItems();
		openStructure( Paragraph::CitationStart, "cit", ++m_citationCnt);
		if (!citclass.empty())
		{
			m_parar.push_back( Paragraph( Paragraph::AttributeStart, "class", citclass));
			m_parar.push_back( Paragraph( Paragraph::AttributeEnd, "", ""));
		}
	}

	void closeCitation()
	{
		closeAutoCloseItem( Paragraph::AttributeStart);
		closeStructure( Paragraph::CitationStart, "");
	}

	void addError( const std::string& msg)
	{
		m_errors.push_back( msg + " [state " + Paragraph::structTypeName( currentStructType()) + "]");
	}

	const std::vector<std::string>& errors() const
	{
		return m_errors;
	}
	void finish();

	std::string toxml( bool beautified) const;
	std::string tostring() const;
	std::string statestring() const;

	static std::string getInputXML( const std::string& title, const std::string& content);

private:
	void finishStructure( int structStartidx);
	void finishTable( int structStartidx);
	void openStructure( Paragraph::Type startType, const char* prefix, int lidx=0);
	void closeStructure( Paragraph::Type startType, const std::string& alt_text);
	void closeOpenStructures();

	void addSingleItem( Paragraph::Type type, const std::string& id, const std::string& text, bool joinText);
	void addQuoteItem( Paragraph::Type startType);
	void openAutoCloseItem( Paragraph::Type startType, const char* prefix, int lidx);
	void closeAutoCloseItem( Paragraph::Type startType);
	void closeDanglingStructures( const Paragraph::Type& starttype);
	void checkStartEndSectionBalance( const std::vector<Paragraph>::const_iterator& start, const std::vector<Paragraph>::const_iterator& end);

private:
	struct StructRef
	{
		int idx;
		int start;

		StructRef( int idx_, int start_)
			:idx(idx_),start(start_){}
		StructRef( const StructRef& o)
			:idx(o.idx),start(o.start){}
	};
	struct TableDef
	{
		int headitr;
		int rowiter;
		int coliter;
		int start;

		explicit TableDef( int start_)
			:headitr(0),rowiter(0),coliter(0),start(start_){}
		TableDef( const TableDef& o)
			:headitr(o.headitr),rowiter(o.rowiter),coliter(o.coliter),start(o.start){}
	};

private:
	std::string m_id;
	std::vector<Paragraph> m_parar;
	std::vector<Paragraph> m_citations;
	std::vector<Paragraph> m_tables;
	std::vector<StructRef> m_structStack;
	std::vector<TableDef> m_tableDefs;
	std::vector<std::string> m_errors;
	int m_tableCnt;
	int m_citationCnt;
	int m_refCnt;
	int m_lastHeadingIdx;
};

}//namespace
#endif

