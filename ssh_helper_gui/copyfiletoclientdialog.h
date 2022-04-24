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

#ifndef __COPYFILETOCLIENT_H__
#define __COPYFILETOCLIENT_H__

#include <QDialog>
#include <QProgressDialog>
#include <QProcess>
#include <QThread>
#include <memory>
#include "ui_copyfiletoclientdialog.h"
#include "script.h"

class CopyFileToClientDialog : public QDialog
{
  Q_OBJECT 
  public:
    CopyFileToClientDialog(std::shared_ptr<SCPScript> scp, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    std::shared_ptr<SCPScript> getSCP();

  public slots:
    void onSelectFile();
    void getMD5();
    void onFinishMD5();
    void onUpdateProgress(int value);
    void onCancelProgress();
  
  signals:
    void updateProgress(int value);

  private:
    Ui::CopyFileToClientDialog ui;
    std::shared_ptr<SCPScript> mScp;
    QThread *mThread;
    QProgressDialog *mProgressDialog;
    bool mStopMD5Process;
    
};

#endif
