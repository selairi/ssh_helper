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

#ifndef _CONFIGFILEPARSER_H_
#define _CONFIGFILEPARSER_H_

#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <memory>

enum ConfigItemType {
  NONE, STRING, MAP, VECTOR
};

class ConfigItem {
  public:
    ConfigItem();
    ConfigItem(ConfigItemType type);
    ConfigItemType getType();
  private:
      ConfigItemType mType;
};

template <class T>
class ConfigItemValue : public ConfigItem {
  public:
    ConfigItemValue(ConfigItemType type) : ConfigItem(type) {};
    T &getValue() {return mValue;};
    void setValue(T &value) {mValue = value;}
  private:
    T mValue;
};

class ConfigItemString : public ConfigItemValue<std::string> {
  public:
    ConfigItemString() : ConfigItemValue<std::string>(ConfigItemType::STRING) {}
};

class ConfigItemMap : public ConfigItemValue<std::map<std::string, std::shared_ptr<ConfigItem> > > {
  public:
    ConfigItemMap() : ConfigItemValue<std::map<std::string, std::shared_ptr<ConfigItem> > >(ConfigItemType::MAP) {}
    void setKey(std::string key, std::string value);
    void setKey(std::string key, std::shared_ptr<ConfigItem> value);
    std::string getKeyValue(std::string key);
    bool contains(std::string key);
};

class ConfigItemVector : public ConfigItemValue<std::vector<std::tuple<std::string, std::shared_ptr<ConfigItem> > > > {
  public:
    ConfigItemVector() : ConfigItemValue<std::vector<std::tuple<std::string, std::shared_ptr<ConfigItem> > > >(ConfigItemType::VECTOR) {}
};

class ConfigFileParser {
  public:
    
    static std::shared_ptr<ConfigItemVector> parser(std::string filename, const std::set<std::string> &allowed_tags);
    static std::shared_ptr<ConfigItemVector> parser(std::istream &in, const std::set<std::string> &allowed_tags);
 
    static void print_tree(std::ostream &out, std::shared_ptr<ConfigItemVector> tree);

    static std::shared_ptr<ConfigItemMap> getMap(std::shared_ptr<ConfigItem> ptr);
    static std::string getMapValue(std::shared_ptr<ConfigItemMap> map, std::string key);

};

// Remove trailing and starting spaces 
std::string strip(const std::string &str);

#endif
