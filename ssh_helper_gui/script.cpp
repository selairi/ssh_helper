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

#include "script.h"

std::shared_ptr<ConfigItemMap> Script::getConfig()
{
  std::shared_ptr<ConfigItemMap> map = std::make_shared<ConfigItemMap>();
  map->setKey("name", name.toStdString() );
  map->setKey("command", script.toStdString() );
  std::string str = sudo ? std::string("yes") : std::string();
  map->setKey(std::string("sudo"), str);

  return map;
}


Script::Script() : ScriptGeneric()
{
  type = ScriptType::SCRIPT;
  sudo = false;
}


Monitor::Monitor() : ScriptGeneric()
{
  type = ScriptType::MONITOR;
  nThreads = 10;
}


std::shared_ptr<ConfigItemMap> Monitor::getConfig()
{
  std::shared_ptr<ConfigItemMap> map = std::make_shared<ConfigItemMap>();
  map->setKey("name", name.toStdString());
  map->setKey("threads", std::to_string(nThreads));

  return map;
}


GuiScript::GuiScript() : ScriptGeneric()
{
  type = ScriptType::GUI_SCRIPT;
}

std::shared_ptr<ConfigItemMap> GuiScript::getConfig()
{
  std::shared_ptr<ConfigItemMap> map = std::make_shared<ConfigItemMap>();
  map->setKey("name", name.toStdString());
  QString args_string;
  QString command = args["command"];
  QMap<QString, QString>::const_iterator i = args.constBegin();
  while (i != args.constEnd()) {
    QString value = i.value();
    value = value.replace("\n", "\n\t");
    args_string += i.key() + ": " + value + "\n"; 
    command = command.replace("{{" + i.key() + "}}", i.value());
    ++i;
  }
  map->setKey("args", args_string.toStdString());
  map->setKey("command", command.toStdString());
  std::string str = sudo ? std::string("yes") : std::string();
  map->setKey(std::string("sudo"), str);

  return map;
}


SCPScript::SCPScript() : ScriptGeneric()
{
  type = ScriptType::SCP;
  name = "Copy file to client: ";
}

std::shared_ptr<ConfigItemMap> SCPScript::getConfig()
{
  std::shared_ptr<ConfigItemMap> map = std::make_shared<ConfigItemMap>();
  map->setKey("name", name.toStdString());
  map->setKey("user", user.toStdString());
  map->setKey("orig", orig.toStdString());
  map->setKey("dest", dest.toStdString());
  map->setKey("md5", md5.toStdString());

  return map;
}
