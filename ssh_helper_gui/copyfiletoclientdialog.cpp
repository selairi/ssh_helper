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

#include "copyfiletoclientdialog.h"
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QThread>
#include <QProgressDialog>
#include <QProcess>
#include <QErrorMessage>
#include <QDebug>

CopyFileToClientDialog::CopyFileToClientDialog(std::shared_ptr<SCPScript> scp, QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
  mThread = nullptr;
  mProgressDialog = nullptr;
  mWaitThreadMessageDialog = nullptr;
  mStopMD5Process = false;
  ui.setupUi(this);
  mScp = scp;
  setWindowTitle(scp->name);
  ui.nameLineEdit->setText(scp->name);
  ui.userLineEdit->setText(scp->user);
  ui.destLineEdit->setText(scp->dest);
  ui.origLineEdit->setText(scp->orig);

  connect(ui.openFileButton, &QPushButton::clicked, this, &CopyFileToClientDialog::onSelectFile);
  connect(this, &CopyFileToClientDialog::updateProgress, this, &CopyFileToClientDialog::onUpdateProgress);
}

std::shared_ptr<SCPScript> CopyFileToClientDialog::getSCP()
{
  mScp->name = ui.nameLineEdit->text();
  mScp->user = ui.userLineEdit->text();
  mScp->dest = ui.destLineEdit->text();
  mScp->orig = ui.origLineEdit->text();
  return mScp;
}

void CopyFileToClientDialog::waitChildThread()
{
  if(mThread) {
    mThread->wait();
    disconnect(mThread, &QThread::finished, this, &CopyFileToClientDialog::onFinishMD5);
    onFinishMD5();
  }
}

void CopyFileToClientDialog::onSelectFile()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"));
  if(! fileName.isEmpty()) {
    ui.origLineEdit->setText(fileName);
    QFile file(ui.origLineEdit->text());
    if(file.exists()) {
      if(mThread) {
        mStopMD5Process = true;
        waitChildThread();
      }
      // Get md5 of file
      mStopMD5Process = false;
      if(! mProgressDialog) {
        mProgressDialog = new QProgressDialog(tr("Running MD5"), tr("Cancel"), 0, 100, this);
        mProgressDialog->setWindowModality(Qt::WindowModal);
        connect(mProgressDialog, &QProgressDialog::canceled, this, &CopyFileToClientDialog::onCancelProgress);
      }
      mProgressDialog->setValue(0);
      mProgressDialog->show();
      mThread = QThread::create([this](){ CopyFileToClientDialog::getMD5(); }); 
      connect(mThread, &QThread::finished, this, &CopyFileToClientDialog::onFinishMD5);
      mThread->start();
    } else {
      // File doesn't exists
      QErrorMessage error(this);
      error.showMessage(tr("File not found."));
      error.exec();
    }
  }
}

void CopyFileToClientDialog::onCancelProgress()
{
  if(! mWaitThreadMessageDialog) {
    mWaitThreadMessageDialog = new QDialog(this);
    mWaitThreadMessageDialog->setModal(true);
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(new QLabel(tr("Please wait"), mWaitThreadMessageDialog));
    mWaitThreadMessageDialog->setLayout(layout);
    mWaitThreadMessageDialog->show();
  }
  mStopMD5Process = true;
  ui.origLineEdit->setText("");
  mThread->wait();
}

void CopyFileToClientDialog::onFinishMD5()
{
  if(mThread) {
    ui.nameLineEdit->setText(tr("Copy file to client: ") + ui.origLineEdit->text());
    delete mThread;
    mThread = nullptr;
    mProgressDialog->setVisible(false);
    delete mProgressDialog;
    mProgressDialog = nullptr;
    if(mWaitThreadMessageDialog) {
      mWaitThreadMessageDialog->setVisible(false);
      delete mWaitThreadMessageDialog;
      mWaitThreadMessageDialog = nullptr;
    }
  }
}

void CopyFileToClientDialog::onUpdateProgress(int value)
{
  if(mProgressDialog) {
    mProgressDialog->setValue(value < 100 ? value : 99);
  }
}

void CopyFileToClientDialog::getMD5()
{
  QFile file(ui.origLineEdit->text());

  if (!file.open(QIODevice::ReadOnly))
    return;
  qint64 size = file.size(), completed = 0;
  int percent = 0, last_percent = 0;
  QProcess md5;
  md5.start("md5sum", QStringList() << "-b" << "-");
  if(md5.waitForStarted()) {
    char buffer[1024*1024];
    while(!file.atEnd() && !mStopMD5Process) {
      qint64 read = file.read(buffer, sizeof(buffer));
      md5.write(buffer, read);
      completed += read;
      percent = (int)((completed * 100) / size);
      if(percent > last_percent) 
        emit updateProgress(percent);
      last_percent = percent;
      // Uncomment the next line for testing
      //QThread::sleep(10);
    }
    md5.closeWriteChannel();
    if(md5.waitForFinished()) {
      QByteArray result = md5.readAll();
      mScp->md5 = QString(result).split(" ")[0];
    }
  }
}
