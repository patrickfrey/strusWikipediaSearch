/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "inputStream.hpp"
#include <stdexcept>

using namespace strus;

InputStream::InputStream( const std::string& docpath)
{
	if (docpath == "-")
	{
		m_stream = &std::cin;
	}
	else
	{
		std::ifstream* in = new std::ifstream();
		in->open( docpath.c_str(), std::fstream::in | std::ios::binary);
		if (!*in)
		{
			delete in;
			throw std::runtime_error( std::string( "failed to read file to insert '") + docpath + "'");
		}
		else
		{
			m_stream = in;
		}
	}
}

InputStream::~InputStream()
{
	if (m_stream != &std::cin)
	{
		delete m_stream;
	}
}

std::istream& InputStream::stream()
{
	return *m_stream;
}


