/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \file textwolf/cstringiterator.hpp
/// \brief textwolf iterator on strings

#ifndef __TEXTWOLF_CSTRING_ITERATOR_HPP__
#define __TEXTWOLF_CSTRING_ITERATOR_HPP__
#include <string>
#include <cstring>
#include <cstdlib>

/// \namespace textwolf
/// \brief Toplevel namespace of the library
namespace textwolf {

/// \class CStringIterator
/// \brief Input iterator on a constant string returning null characters after EOF as required by textwolf scanners
class CStringIterator
{
public:
	/// \brief Default constructor
	CStringIterator()
		:m_src(0)
		,m_size(0)
		,m_pos(0){}

	/// \brief Constructor
	/// \param [in] src null terminated C string to iterate on
	/// \param [in] size number of bytes in the string to iterate on
	CStringIterator( const char* src, unsigned int size)
		:m_src(src)
		,m_size(size)
		,m_pos(0){}

	/// \brief Constructor
	/// \param [in] src string to iterate on
	CStringIterator( const char* src)
		:m_src(src)
		,m_size(std::strlen(src))
		,m_pos(0){}

	/// \brief Constructor
	/// \param [in] src string to iterate on
	CStringIterator( const std::string& src)
		:m_src(src.c_str())
		,m_size(src.size())
		,m_pos(0){}

	/// \brief Copy constructor
	/// \param [in] o iterator to copy
	CStringIterator( const CStringIterator& o)
		:m_src(o.m_src)
		,m_size(o.m_size)
		,m_pos(o.m_pos){}

	/// \brief Element access
	/// \return current character
	inline char operator* ()
	{
		return (m_pos < m_size)?m_src[m_pos]:0;
	}

	/// \brief Preincrement
	inline CStringIterator& operator++()
	{
		m_pos++;
		return *this;
	}

	/// \brief Return current char position
	inline unsigned int pos() const	{return m_pos;}

	/// \brief Set current char position
	inline void pos( unsigned int i)	{m_pos=(i<m_size)?i:m_size;}

	inline int operator - (const CStringIterator& o) const
	{
		if (m_src != o.m_src) return 0;
		return (int)(m_pos - o.m_pos);
	}

private:
	const char* m_src;
	unsigned int m_size;
	unsigned int m_pos;
};

}//namespace
#endif
