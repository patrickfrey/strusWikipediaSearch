/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/// \brief Intermediate document format for converting "Wikimedia XML" to pure and simplified XML
/// \file documentStructure.hpp
#ifndef _STRUS_WIKIPEDIA_OUTPUT_STRING_HPP_INCLUDED
#define _STRUS_WIKIPEDIA_OUTPUT_STRING_HPP_INCLUDED
#include <string>

/// \brief strus toplevel namespace
namespace strus {

std::string outputLineString( const char* si, const char* se, int maxlen=60);
std::string outputString( const char* si, const char* se, int maxlen=60);

}//namespace
#endif


