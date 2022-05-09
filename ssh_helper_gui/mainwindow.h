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

#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QMainWindow>
#include <QMenu>
#include <QHash>
#include <memory>
#include "ui_mainwindow.h"
#include "host.h"
#include "script.h"


class MainWindow : public QMainWindow 
{
  Q_OBJECT
  public:
    MainWindow(QString menuFilePath, QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    void loadMenuFile(QString path);

  private slots:
    void onNewProject();
    void onSave();
    void onSaveAs();
    void onOpen();
    void closeWindow();

    void onAddHost();
    void onDeleteHost();
    void onEditHosts();
    void onItemHostDoubleClicked(QListWidgetItem *item);
    void onOpenHosts();
    void onSaveHosts();

    void onNewScript();
    void onEditScript();
    void onDeleteScript();
    void onScriptDoubleClicked(QTreeWidgetItem *item, int column);
    void onNewMonitor();
    void onNewUser();
    void onCopyFileToClient();

    void onRunSSH();

    void about();

  private:
    void listenMenuActions();
    void deleteScriptChilds(QTreeWidgetItem *item);
    void buildScriptsTree(std::shared_ptr<ConfigItemVector> tree, QTreeWidgetItem *parent);
    QTreeWidgetItem *insertTreeItem(std::shared_ptr<ScriptGeneric> script, QTreeWidgetItem *parent = nullptr);

    std::shared_ptr<ConfigItemVector> getScriptsTreeFromTreeWidget(QTreeWidgetItem *item = nullptr); 

    Ui::MainWindow ui;
    QHash<int, Host> hosts;
    QHash<int, std::shared_ptr<ScriptGeneric> > scripts;

    QString mFileProjectName;
};


#endif
