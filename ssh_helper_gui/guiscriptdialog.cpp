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

#include "guiscriptdialog.h"
#include <QLabel>
#include <QLineEdit>

GuiScriptDialog::GuiScriptDialog(std::shared_ptr<GuiScript> guiScript, QString name, QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
  ui.setupUi(this);
  mGuiScript = guiScript;
  for(QString key : mGuiScript->args.keys()) {
    if(key != "command" && key != "help") {
      QLineEdit *lineEdit = new QLineEdit(this);
      lineEdit->setText(mGuiScript->args[key]);
      ui.formLayout->addRow(key + ":", lineEdit);
      values[key] = lineEdit;
    }
  }
  if(mGuiScript->args.contains("help"))
    ui.helpText->setHtml(mGuiScript->args["help"]);
  setWindowTitle(name);
  ui.nameLineEdit->setText(name);
}


std::shared_ptr<GuiScript> GuiScriptDialog::getGuiScript()
{
  for(QString key : mGuiScript->args.keys()) {
    if(key != "command" && key != "help")
      mGuiScript->args[key] = values[key]->text();
  }
  mGuiScript->name = ui.nameLineEdit->text();
  return mGuiScript;
}


