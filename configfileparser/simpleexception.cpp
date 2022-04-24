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

#include "simpleexception.h"

SimpleException::SimpleException(std::string error) : std::logic_error::logic_error(error)
{
  //this->error = error;
}

SimpleException::SimpleException(const char* error) : std::logic_error::logic_error(error)
{
  //this->error = std::string(error);
}


//const char *SimpleException::what() 
//{
//  return error.c_str();
//}


