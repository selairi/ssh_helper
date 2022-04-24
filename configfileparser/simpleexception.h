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

#ifndef _SIMPLEEXCEPTION_H_
#define _SIMPLEEXCEPTION_H_

//#include <exception>
#include <stdexcept>
#include <string>

//class SimpleException: public std::exception
class SimpleException : public std::logic_error::logic_error
{
  public:
    SimpleException(std::string error);
    SimpleException(const char* error);
    //virtual const char *what();
  //private:
    //std::string error;
};


#endif
