/*
 * (c)GPL3
 *
 * Copyright: 2022 P.L. Lucas <selairi@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * this program. If not, see <https://www.gnu.org/licenses/>. 
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <iostream>

#define debug if(m_debug) std::cerr << "[" << __PRETTY_FUNCTION__ << "] Line:" << __LINE__ << "\t"

#define debug_get_func std::string("[") + std::string(__PRETTY_FUNCTION__) + std::string("]")

#define debug_error std::cerr << "ERROR: [" << __PRETTY_FUNCTION__ << "] Line:" << __LINE__ << "\t"


extern bool m_debug;

#endif
