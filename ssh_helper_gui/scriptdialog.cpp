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

#include "scriptdialog.h"

ScriptDialog::ScriptDialog(QWidget *parent, Qt::WindowFlags f)
{
  ui.setupUi(this);
}

std::shared_ptr<Script> ScriptDialog::getScript()
{
  auto script = std::make_shared<Script>();
  script->name = ui.nameLineEdit->text().trimmed();
  script->script = ui.scriptEdit->toPlainText();
  script->sudo = ui.sudoCheckBox->checkState() == Qt::Checked;
  return script;
}

void ScriptDialog::setScript(std::shared_ptr<Script> script)
{
  ui.nameLineEdit->setText(script->name);
  ui.scriptEdit->setPlainText(script->script);
  ui.sudoCheckBox->setCheckState(script->sudo ? Qt::Checked : Qt::Unchecked);
}
