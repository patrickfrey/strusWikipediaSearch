/*
---------------------------------------------------------------------
    The template library textwolf implements an input iterator on
    a set of XML path expressions without backward references on an
    STL conforming input iterator as source. It does no buffering
    or read ahead and is dedicated for stream processing of XML
    for a small set of XML queries.
    Stream processing in this Object refers to processing the
    document without buffering anything but the current result token
    processed with its tag hierarchy information.

    Copyright (C) 2010,2011,2012,2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3.0 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of textwolf can be found at 'http://github.com/patrickfrey/textwolf'
	For documentation see 'http://patrickfrey.github.com/textwolf'

--------------------------------------------------------------------
*/
/// \file textwolf/istreamiterator.hpp
/// \brief Definition of iterators for textwolf on STL input streams (std::istream)

#ifndef __TEXTWOLF_ISTREAM_ITERATOR_HPP__
#define __TEXTWOLF_ISTREAM_ITERATOR_HPP__
#include "textwolf/exception.hpp"
#include <iostream>
#include <fstream>
#include <iterator>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <stdint.h>

/// \namespace textwolf
/// \brief Toplevel namespace of the library
namespace textwolf {

/// \class IStreamIterator
/// \brief Input iterator on an STL input stream
class IStreamIterator
	:public throws_exception
{
public:
	/// \brief Default constructor
	IStreamIterator(){}
	/// \brief Destructor
	~IStreamIterator()
	{
		std::free(m_buf);
	}

	/// \brief Constructor
	/// \param [in] input input to iterate on
	IStreamIterator( std::istream& input, std::size_t bufsize=8192)
		:m_input(&input),m_buf((char*)std::malloc(bufsize)),m_bufsize(bufsize),m_readsize(0),m_readpos(0),m_abspos(0)
	{
		input.unsetf( std::ios::skipws);
		input.exceptions ( std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit );
		fillbuf();
	}

	/// \brief Copy constructor
	/// \param [in] o iterator to copy
	IStreamIterator( const IStreamIterator& o)
		:m_input(o.m_input),m_buf((char*)std::malloc(o.m_bufsize)),m_bufsize(o.m_bufsize),m_readsize(o.m_readsize),m_readpos(o.m_readpos),m_abspos(o.m_abspos)
	{
		std::memcpy( m_buf, o.m_buf, o.m_readsize);
	}

	/// \brief Element access
	/// \return current character
	inline char operator* ()
	{
		return (m_readpos < m_readsize)?m_buf[m_readpos]:0;
	}

	/// \brief Pre increment
	inline IStreamIterator& operator++()
	{
		if (m_readpos+1 >= m_readsize)
		{
			fillbuf();
		}
		else
		{
			++m_readpos;
		}
		return *this;
	}

	int operator - (const IStreamIterator& o) const
	{
		return (int)m_readpos - o.m_readpos;
	}

	uint64_t position() const
	{
		return m_abspos + m_readpos;
	}

private:
	bool fillbuf()
	{
		try
		{
			m_input->read( m_buf, m_bufsize);
			m_abspos += m_readsize;
			m_readsize = m_input->gcount();
			m_readpos = 0;
			return true;
		}
		catch (const std::istream::failure& err)
		{
			if (m_input->eof())
			{
				m_abspos += m_readsize;
				m_readsize = m_input->gcount();
				m_readpos = 0;
				return (m_readsize > 0);
			}
			throw exception( FileReadError);
		}
		catch (const std::exception& err)
		{
			throw exception( FileReadError);
		}
		catch (...)
		{
			throw exception( FileReadError);
		}
	}

private:
	std::istream* m_input;
	char* m_buf;
	std::size_t m_bufsize;
	std::size_t m_readsize;
	std::size_t m_readpos;
	uint64_t m_abspos;
};

}//namespace
#endif
