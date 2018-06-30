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
			SpanStart,
			SpanEnd,
			SmallStart,
			SmallEnd,
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
			NoWiki,
			Math,
			CitationLink
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
			"SpanStart",
			"SpanEnd",
			"SmallStart",
			"SmallEnd",
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
			"NoWiki",
			"Math",
			"CitationLink",0};
		return ar[tp];
	}
	enum StructType {
			StructNone,
			StructEntity,
			StructQuotation,
			StructDoubleQuote,
			StructBlockQuote,
			StructSpan,
			StructSmall,
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
		static const char* ar[] = {"None","Entity","Quotation","DoubleQuote","BlockQuote","Span","Small","Heading","List","Attribute","Citation","Ref","PageLink","WebLink","Table","TableTitle","TableHead","TableRow","TableCol"};
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
			StructSpan/*SpanStart*/,
			StructNone/*SpanEnd*/,
			StructSmall/*SmallStart*/,
			StructNone/*SmallEnd*/,
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
			StructNone/*CitationLink*/};
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
			SpanEnd/*SpanStart*/,
			SpanStart/*SpanEnd*/,
			SmallEnd/*SmallStart*/,
			SmallStart/*SmallEnd*/,
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
			CitationLink/*CitationLink*/};
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
		:m_id(),m_parar(),m_citations(),m_structStack(),m_tableDefs(),m_errors(),m_citationCnt(0),m_refCnt(0){}
	DocumentStructure( const DocumentStructure& o)
		:m_id(o.m_id),m_parar(o.m_parar),m_citations(o.m_citations),m_structStack(o.m_structStack),m_tableDefs(o.m_tableDefs),m_errors(o.m_errors),m_citationCnt(o.m_citationCnt),m_refCnt(o.m_refCnt){}

	const std::string& id() const
	{
		return m_id;
	}
	Paragraph::StructType currentStructType() const;

	void setTitle( const std::string& text);

	void addText( const std::string& text)
	{
		addSingleItem( Paragraph::Text, "", text, true/*joinText*/);
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
		closeAutoCloseItem( Paragraph::RefStart);
		openStructure( Paragraph::RefStart, "ref", ++m_refCnt);
	}
	void closeRef()
	{
		closeStructure( Paragraph::RefStart);
	}
	void openPageLink( const std::string& id)
	{
		openStructure( Paragraph::PageLinkStart, id.c_str(), 0);
	}
	void closePageLink()
	{
		closeStructure( Paragraph::PageLinkStart);
	}
	void openWebLink( const std::string& id)
	{
		openStructure( Paragraph::WebLinkStart, id.c_str(), 0);
	}
	void closeWebLink()
	{
		closeStructure( Paragraph::WebLinkStart);
	}
	void openHeading( int idx)
	{
		closeOpenStructures();
		openAutoCloseItem( Paragraph::HeadingStart, "h", idx);
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
	void openBlockQuote()
	{
		closeAutoCloseItem( Paragraph::BlockQuoteStart);
		openStructure( Paragraph::BlockQuoteStart, "", 0);
	}
	void closeBlockQuote()
	{
		closeStructure( Paragraph::BlockQuoteStart);
	}
	void openSpan()
	{
		closeAutoCloseItem( Paragraph::SpanStart);
		openStructure( Paragraph::SpanStart, "", 0);
	}
	void closeSpan()
	{
		closeStructure( Paragraph::SpanStart);
	}
	void openSmall()
	{
		closeAutoCloseItem( Paragraph::SmallStart);
		openStructure( Paragraph::SmallStart, "", 0);
	}
	void closeSmall()
	{
		closeStructure( Paragraph::SmallStart);
	}
	void openTable()
	{
		closeDanglingStructures( Paragraph::TableStart);
		openStructure( Paragraph::TableStart, "table", (int)m_tableDefs.size()+1);
	}
	void closeTable()
	{
		closeStructure( Paragraph::TableStart);
	}
	void addTableTitle()
	{
		closeDanglingStructures( Paragraph::TableStart);
		openAutoCloseItem( Paragraph::TableTitleStart, "table", (int)m_tableDefs.size());
	}
	void addTableHead()
	{
		closeDanglingStructures( Paragraph::TableStart);
		if (m_tableDefs.empty()) {addError("table open head without table defined"); return;}
		openAutoCloseItem( Paragraph::TableHeadStart, "head", ++m_tableDefs.back().headitr);
	}
	void addTableRow()
	{
		closeDanglingStructures( Paragraph::TableStart);
		if (m_tableDefs.empty()) {addError("table open row without table defined"); return;}
		m_tableDefs.back().coliter = 0;
		openAutoCloseItem( Paragraph::TableTitleStart, "row", ++m_tableDefs.back().rowiter);
	}
	void addTableCol()
	{
		closeDanglingStructures( Paragraph::TableRowStart);
		if (m_tableDefs.empty()) {
			addError("table open col without table defined"); return;
		}
		openAutoCloseItem( Paragraph::TableTitleStart, "col", ++m_tableDefs.back().coliter);
	}
	void addAttribute( const std::string& id)
	{
		closeAutoCloseItem( Paragraph::AttributeStart);
		if (currentStructType() != Paragraph::StructCitation)
		{
			addError("add attribute without context"); return;
		}
		openAutoCloseItem( Paragraph::AttributeStart, id.c_str(), 0);
	}
	void openListItem( int lidx)
	{
		openAutoCloseItem( Paragraph::ListItemStart, "l", lidx);
	}
	void closeOpenEolnItem()
	{
		closeAutoCloseItem( Paragraph::ListItemStart);
		closeAutoCloseItem( Paragraph::HeadingStart);
	}
	void openCitation()
	{
		openStructure( Paragraph::CitationStart, "cit", ++m_citationCnt);
	}

	void closeCitation()
	{
		closeAutoCloseItem( Paragraph::AttributeStart);
		closeStructure( Paragraph::CitationStart);
	}

	void addError( const std::string& msg)
	{
		m_errors.push_back( msg);
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
	void openStructure( Paragraph::Type startType, const char* prefix, int lidx=0);
	void closeStructure( Paragraph::Type startType);
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
	std::vector<StructRef> m_structStack;
	std::vector<TableDef> m_tableDefs;
	std::vector<std::string> m_errors;
	int m_citationCnt;
	int m_refCnt;
};

}//namespace
#endif

