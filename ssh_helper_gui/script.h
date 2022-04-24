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

#ifndef __SCRIPT_H__
#define __SCRIPT_H__

#include <QString>
#include <QMap>
#include "configfileparser.h"

enum ScriptType {
  SCRIPT, MONITOR, SCP, GUI_SCRIPT
};

class Script;
class Monitor;

class ScriptGeneric {
  public:
    ~ScriptGeneric() = default;

    QString name;

    ScriptType getType() {return type;}

    virtual std::shared_ptr<ConfigItemMap> getConfig()=0;

    Script *getScript();
    Monitor *getMonitor();

  protected:
    ScriptType type;
};

class Script : public ScriptGeneric {
  public:
    Script();
    virtual ~Script() = default;

    QString script;
    bool sudo;

    virtual std::shared_ptr<ConfigItemMap> getConfig();
};


class Monitor : public ScriptGeneric {
  public:
    Monitor();
    ~Monitor() = default;

    int nThreads;

    virtual std::shared_ptr<ConfigItemMap> getConfig();
};

class GuiScript : public ScriptGeneric {
  public:
    GuiScript();
    ~GuiScript() = default;

    bool sudo;

    QMap<QString, QString> args;

    virtual std::shared_ptr<ConfigItemMap> getConfig();
};

class SCPScript : public ScriptGeneric {
  public:
    SCPScript();
    ~SCPScript() = default;

    QString user, dest, orig, md5;

    virtual std::shared_ptr<ConfigItemMap> getConfig();
};



#endif
