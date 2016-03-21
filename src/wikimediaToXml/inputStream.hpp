/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_UTILITIES_INPUT_STREAM_HPP_INCLUDED
#define _STRUS_UTILITIES_INPUT_STREAM_HPP_INCLUDED
#include <string>
#include <fstream>
#include <iostream>

namespace strus {

class InputStream
{
public:
	InputStream( const std::string& docpath);
	~InputStream();
	std::istream& stream();

private:
	std::istream* m_stream;
};

}
#endif


