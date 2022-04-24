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

#include "configfileparser.h"
#include "string_utils.h"
#include "simpleexception.h"
#include "debug.h"
#include <fstream>
#include <regex>

ConfigItem::ConfigItem()
{
  mType = ConfigItemType::NONE;
}

ConfigItem::ConfigItem(ConfigItemType type)
{
  mType = type;
}

void ConfigItemMap::setKey(std::string key, std::string value)
{
  std::shared_ptr<ConfigItemString> item = std::make_shared<ConfigItemString>();
  item->setValue(value);
  getValue()[key] = item;
}

void ConfigItemMap::setKey(std::string key, std::shared_ptr<ConfigItem> value)
{
  getValue()[key] = value;
}


bool ConfigItemMap::contains(std::string key)
{
  return getValue().contains(key);
}

std::string ConfigItemMap::getKeyValue(std::string key)
{
  std::string str;
  if(contains(key)) {
    std::shared_ptr<ConfigItem> value = getValue()[key];
    if(value->getType() == ConfigItemType::STRING) {
      std::shared_ptr<ConfigItemString> s = std::static_pointer_cast<ConfigItemString>(value);
      str = s->getValue();
    }
  }
  return str;
}

ConfigItemType ConfigItem::getType()
{
  return mType;
}


std::string get_tag(const std::string &line, const std::string::size_type &pos)
{
  std::string tag = line.substr(0, pos);
  return strip(tag);
}

std::string get_value(const std::string &line, const std::string::size_type &pos)
{
  std::string value = line.substr(pos + 1, line.size() - pos);
  return strip(value);
}

static std::tuple<std::shared_ptr<ConfigItemString>, std::string /*last_line*/>
string_parser(std::istream &in, int level, std::string value_from_tag, int &n_line)
{
  std::shared_ptr<ConfigItemString> item(new ConfigItemString());
  std::string line, str = value_from_tag;
  int actual_level;
  while(std::getline(in, line)) {
    n_line++;
    // Count level
    actual_level = 0;
    for(std::string::size_type i = 0; i < line.size(); i++)
      if(line[i] == '\t')
        actual_level++;
      else
        break;
    if(level <= actual_level || (level > 0 && line.empty())) {
      // Remove tabs from line starting
      if(!line.empty())
        line = line.substr(level, line.size());
      if(!str.empty())
        str += "\n" + line;
      else
        str = line;
    } else {
      // Tabs level has changed. Stop reading lines.
      item->setValue(str);
      return std::make_tuple(item, line);
    }
  }
  item->setValue(str);
  return std::make_tuple(item, std::string());
}

enum ContainerType {
  VECTOR_TYPE, MAP_TYPE
};

static std::tuple<std::shared_ptr<ConfigItem>, std::string /*last_line*/> 
vector_map_parser(std::istream &in, int level, std::string last_line, int &n_line, enum ContainerType type, const std::set<std::string> &allowed_tags)
{
  std::shared_ptr<ConfigItemVector> vec(new ConfigItemVector());
  std::shared_ptr<ConfigItemMap> map(new ConfigItemMap());
  std::string line = last_line;
  int actual_level;
  std::string tag;
  std::shared_ptr<ConfigItem> item;
  bool ok = true;
  if(last_line.empty()) {
    ok = std::getline(in, line) ? true : false;
    n_line++;
  } else {
    line = last_line;
    last_line.clear();
  }
  while(ok) { 
    // Count level
    actual_level = 0;
    for(std::string::size_type i = 0; i < line.size(); i++)
      if(line[i] == '\t')
        actual_level++;
      else
        break;
    if(level == actual_level) {
      std::string::size_type string_pos = line.find(":");
      std::string::size_type    map_pos = line.find("-");
      std::string::size_type vector_pos = line.find("+");
      bool container_type_found = false;
      if(std::string::npos == string_pos)
        string_pos = line.size() + 1;
      else
        container_type_found = true;
      if(std::string::npos == map_pos)
        map_pos = line.size() + 1;
      else
        container_type_found = true;
      if(std::string::npos == vector_pos)
        vector_pos = line.size() + 1;
      else
        container_type_found = true;
      if(!container_type_found)
        throw(SimpleException( "Line " + std::to_string(n_line) + ": Error reading config file: No +,-,: symbol found. " + line));
      if(string_pos < map_pos && string_pos < vector_pos) {
        std::shared_ptr<ConfigItemString> v;
        std::tie(v, last_line) = string_parser(in, level + 1, get_value(line, string_pos), n_line);
        item = v;
        tag = get_tag(line, string_pos);
        if(allowed_tags.size() > 0 && !allowed_tags.contains(tag))
          throw(SimpleException("Line " + std::to_string(n_line) + ": Unknown tag: " + tag));
        if(type == ContainerType::VECTOR_TYPE)
          vec->getValue().push_back(std::make_tuple(tag, item));
        else //if(type == ContainerType::MAP_TYPE) {
          map->getValue()[tag] = item;
      } else if(vector_pos < string_pos && vector_pos < map_pos) {
        std::tie(item, last_line) = vector_map_parser(in, level + 1, std::string(), n_line, ContainerType::VECTOR_TYPE, allowed_tags);
        tag = get_tag(line, vector_pos);
        if(allowed_tags.size() > 0 && !allowed_tags.contains(tag))
          throw(SimpleException("Line " + std::to_string(n_line) + ": Unknown tag: " + tag));
        if(type == ContainerType::VECTOR_TYPE)
          vec->getValue().push_back(std::make_tuple(tag, item));
        else //if(type == ContainerType::MAP_TYPE)
          map->getValue()[tag] = item;
      } else if(map_pos < string_pos && map_pos < vector_pos) {
        std::tie(item, last_line) = vector_map_parser(in, level + 1, std::string(), n_line, ContainerType::MAP_TYPE, allowed_tags);
        tag = get_tag(line, map_pos);
        if(allowed_tags.size() > 0 && !allowed_tags.contains(tag))
          throw(SimpleException("Line " + std::to_string(n_line) + ": Unknown tag: " + tag));

        if(type == ContainerType::VECTOR_TYPE)
          vec->getValue().push_back(std::make_tuple(tag, item));
        else //if(type == ContainerType::MAP_TYPE)
          map->getValue()[tag] = item;
      }
    } else if(level > actual_level) {
      if(type == ContainerType::VECTOR_TYPE)
        return std::make_tuple(vec, line);
      else //if(type == ContainerType::MAP_TYPE)
        return std::make_tuple(map, line);
    } else {
      throw(SimpleException("Line " + std::to_string(n_line) + ": Error parsing config file: More tabs than required:\n" + line));
    }
    if(last_line.empty()) {
      ok = std::getline(in, line) ? true : false;
      n_line++;
    } else {
      line = last_line;
      last_line.clear();
    }
  }

  line.clear();
  if(type == ContainerType::VECTOR_TYPE)
    return std::make_tuple(vec, line);
  return std::make_tuple(map, line);
}


std::shared_ptr<ConfigItemVector> ConfigFileParser::parser(std::string filename, const std::set<std::string> &allowed_tags)
{
  std::shared_ptr<ConfigItem> vec;
  std::ifstream in;
  std::string last_line;
  int n_line = 0;
  in.open(filename.c_str());
  if(!in.is_open())
    throw(SimpleException(std::string("[ConfigFileParser::parser] File ") + filename + " cannot be opened."));
  try {
    std::tie(vec, last_line) = vector_map_parser(in, 0, std::string(), n_line, ContainerType::VECTOR_TYPE, allowed_tags);
  } catch (SimpleException &e) {
    throw(SimpleException(filename + std::string(": ") + e.what()));
  }
  in.close();

  return std::static_pointer_cast<ConfigItemVector>(vec);
}


std::shared_ptr<ConfigItemVector> ConfigFileParser::parser(std::istream &in, const std::set<std::string> &allowed_tags)
{
  std::shared_ptr<ConfigItem> vec;
  std::string last_line;
  int n_line = 0;
  std::tie(vec, last_line) = vector_map_parser(in, 0, std::string(), n_line, ContainerType::VECTOR_TYPE, allowed_tags);

  return std::static_pointer_cast<ConfigItemVector>(vec);
}

static void print_tree_level_string(std::ostream &out, std::string level_str, const std::string &tag, std::shared_ptr<ConfigItem> &tree)
{
  std::shared_ptr<ConfigItemString> str = std::static_pointer_cast<ConfigItemString>(tree);
  std::string::size_type pos = str->getValue().find("\n");
  if(pos == std::string::npos && strip(str->getValue()) == str->getValue()) {
    out << level_str << tag << ": " << str->getValue() << std::endl;
  } else {
    out << level_str << tag << ":" << std::endl;
    level_str += "\t";
    std::stringstream lines(str->getValue());
    std::string line;
    while(getline(lines, line))
      out << level_str << line << std::endl;
    // if the last line is empty, getline won't read the last line:
    if(str->getValue().size() > 0 && str->getValue()[str->getValue().size() - 1] == '\n')
      out << level_str << std::endl;
  }
}


static void print_tree_level(std::ostream &out, int level, std::shared_ptr<ConfigItem> tree)
{
  std::string level_str;
  for(int i = 0; i < level; i++)
    level_str += "\t";
  switch(tree->getType()) {
    case ConfigItemType::STRING: 
    {
      std::shared_ptr<ConfigItemString> str = std::static_pointer_cast<ConfigItemString>(tree);
      std::stringstream lines(str->getValue());
      std::string line;
      while(getline(lines, line))
        out << level_str << line << std::endl;
      break;
    }
    case ConfigItemType::VECTOR:
    {  
      std::shared_ptr<ConfigItemVector> vec = std::static_pointer_cast<ConfigItemVector>(tree);
      std::vector<std::tuple<std::string, std::shared_ptr<ConfigItem> > > items = vec->getValue();
      for(std::tuple<std::string, std::shared_ptr<ConfigItem> > item : items) {
        std::string tag;
        std::shared_ptr<ConfigItem> value;
        std::tie(tag, value) = item;
        switch(value->getType()) {
          case ConfigItemType::STRING:
            //out << level_str << tag << ":" << std::endl;
            //print_tree_level(out, level + 1, value);
            print_tree_level_string(out, level_str, tag, value);
            break;
          case ConfigItemType::VECTOR:
            out << level_str << tag << " +" << std::endl;
            print_tree_level(out, level + 1, value);
            break;
          case ConfigItemType::MAP:
            out << level_str << tag << " -" << std::endl;
            print_tree_level(out, level + 1, value);
            break;
          case ConfigItemType::NONE:
            break;
        }
      }
      break;
    }
    case ConfigItemType::MAP:
    {
      std::shared_ptr<ConfigItemMap> ptr = std::static_pointer_cast<ConfigItemMap>(tree);
      std::map<std::string, std::shared_ptr<ConfigItem> > map = ptr->getValue();
      for(std::map<std::string, std::shared_ptr<ConfigItem> >::iterator it = map.begin(); it != map.end(); ++it) {
        std::string tag = it->first;
        std::shared_ptr<ConfigItem> value = it->second;
        switch(value->getType()) {
          case ConfigItemType::STRING:
            //out << level_str << tag << ":" << std::endl;
            //print_tree_level(out, level + 1, value);
            print_tree_level_string(out, level_str, tag, value);
            break;
          case ConfigItemType::VECTOR:
            out << level_str << tag << " +" << std::endl;
            print_tree_level(out, level + 1, value);
            break;
          case ConfigItemType::MAP:
            out << level_str << tag << " -" << std::endl;
            print_tree_level(out, level + 1, value);
            break;
          case ConfigItemType::NONE:
            break;
        }
      }
      break;
    }
    case ConfigItemType::NONE:
      break;
  }
}


void ConfigFileParser::print_tree(std::ostream &out, std::shared_ptr<ConfigItemVector> tree)
{
  print_tree_level(out, 0, tree);
}


std::shared_ptr<ConfigItemMap> ConfigFileParser::getMap(std::shared_ptr<ConfigItem> ptr)
{
  if(ptr->getType() == ConfigItemType::MAP) {
    std::shared_ptr<ConfigItemMap> map = static_pointer_cast<ConfigItemMap>(ptr);
    return map;
  } else {
    throw(SimpleException("Error: No map type item."));
  }
  return nullptr;
}


std::string ConfigFileParser::getMapValue(std::shared_ptr<ConfigItemMap> map, std::string key)
{
  if(map->getValue().contains(key)) {
    std::shared_ptr<ConfigItem> ptr = map->getValue()[key];
    if(ptr->getType() == ConfigItemType::STRING) {
      std::shared_ptr<ConfigItemString> str = static_pointer_cast<ConfigItemString>(ptr);
      return str->getValue();
    } else {
      throw(SimpleException("Error: No string type item."));
    }
  }
  return std::string();
}
