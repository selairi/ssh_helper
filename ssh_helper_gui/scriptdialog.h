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

#ifndef __SCRIPTDIALOG_H_
#define __SCRIPTDIALOG_H_

#include <QDialog>
#include <memory>
#include "ui_scriptdialog.h"
#include "script.h"

class ScriptDialog : public QDialog
{
  Q_OBJECT 
  public:
    ScriptDialog(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    std::shared_ptr<Script> getScript();
    void setScript(std::shared_ptr<Script> script);

  private:
    Ui::ScriptDialog ui;
};

#endif
