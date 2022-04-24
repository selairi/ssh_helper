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

#include "newhost.h"

NewHost::NewHost(QWidget *parent, Qt::WindowFlags f)
{
  ui.setupUi(this);
}

Host NewHost::getHost()
{
  Host host;
  host.user = ui.userLineEdit->text().trimmed();
  host.host = ui.hostLineEdit->text().trimmed();
  if(ui.passwordCheckBox->checkState() == Qt::Checked)
      host.password = ui.passwordLineEdit->text().trimmed();
  return host;
}

void NewHost::setHost(const Host &host)
{
  ui.userLineEdit->setText(host.user);
  ui.hostLineEdit->setText(host.host);
  if(! host.password.isEmpty()) {
    ui.passwordLineEdit->setText(host.password);
    ui.passwordLineEdit->setEnabled(true);
    ui.passwordCheckBox->setCheckState(Qt::Checked);
  }
}
