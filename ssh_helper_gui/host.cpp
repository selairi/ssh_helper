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

#include "host.h"

std::shared_ptr<ConfigItemMap> Host::getConfig()
{
  std::shared_ptr<ConfigItemMap> map = std::make_shared<ConfigItemMap>();
  std::shared_ptr<ConfigItemString> item = std::make_shared<ConfigItemString>();
  std::string str = user.toStdString();
  item->setValue(str);
  map->getValue()[std::string("user")] = item;
  item = std::make_shared<ConfigItemString>();
  str = host.toStdString();
  item->setValue(str);
  map->getValue()[std::string("host")] = item;
  item = std::make_shared<ConfigItemString>();
  str = password.toStdString();
  item->setValue(str);
  map->getValue()[std::string("password")] = item;

  return map;
}
