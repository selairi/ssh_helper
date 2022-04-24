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

#include <QMessageBox>
#include <fstream>
#include <sstream>
#include <QFileDialog>
#include <QToolButton>
#include <QErrorMessage>
#include <simpleexception.h>
#include <QTemporaryDir> 
#include "mainwindow.h"
#include <QDebug>
#include "newhost.h"
#include "configfileparser.h"
#include "scriptdialog.h"
#include "monitordialog.h"
#include "guiscriptdialog.h"
#include "copyfiletoclientdialog.h"
#include "config.h"
#include "ui_ssh.h"

static int get_next_id()
{
  static int id_count = 0;
  return ++id_count;
}


MainWindow::MainWindow(QString menuFilePath, QWidget *parent, Qt::WindowFlags flags) : QMainWindow(parent, flags)
{
  ui.setupUi(this);

  loadMenuFile(SHARE_PATH "/menu.txt");
  if(!menuFilePath.isEmpty())
    loadMenuFile(menuFilePath);

  listenMenuActions();
}

QTreeWidgetItem *MainWindow::insertTreeItem(std::shared_ptr<ScriptGeneric> script, QTreeWidgetItem *parent)
{
  QTreeWidgetItem *item = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), QStringList(script->name));
  item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
  QVariant var;
  int script_id = get_next_id();
  var.setValue(script_id);
  item->setData(0, Qt::UserRole, var);
  if(parent == nullptr) {
    bool inserted = false;
    for(QTreeWidgetItem *selected : ui.scriptsTreeWidget->selectedItems()) {
      if(selected != nullptr) {
        QTreeWidgetItem *p = selected->parent();
        if(p != nullptr) {
          p->addChild(item);
          inserted = true;
        }
      } 
      break;
    }
    if(! inserted)
      ui.scriptsTreeWidget->addTopLevelItem(item);
  } else
    parent->addChild(item);
  scripts[script_id] = script;
  return item;
}

void MainWindow::onAddHost()
{
  NewHost dialog(this);
  if(QDialog::Accepted == dialog.exec()) {
    Host host;
    while(host.host.isEmpty() || host.user.isEmpty()) {
      host = dialog.getHost();
      if(host.host.isEmpty() || host.user.isEmpty()) {
        QMessageBox::StandardButton answer;
        answer = QMessageBox::warning(this, tr("Incomplete Form"),
            tr("The form does not contain all the necessary information.\n"
              "Do you want to discard it?"),
            QMessageBox::Yes | QMessageBox::No);
        if(answer == QMessageBox::Yes)
          break;
        if(QDialog::Accepted != dialog.exec())
          break;
      }
    }
    if(! host.host.isEmpty() && ! host.user.isEmpty()) {
      QListWidgetItem *item = new QListWidgetItem(host.user + "@" + host.host, ui.hostsList, Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
      QVariant var;
      int host_id = get_next_id();
      var.setValue(host_id);
      item->setData(Qt::UserRole, var);
      hosts[host_id] = host;
    }  
  }
}


void MainWindow::onDeleteHost()
{
  for(QListWidgetItem *item : ui.hostsList->selectedItems()) {
    int n = item->data(Qt::UserRole).toInt();
    hosts.remove(n);
    ui.hostsList->takeItem(ui.hostsList->row(item));
  }
}


void MainWindow::onSaveHosts()
{
  std::shared_ptr<ConfigItemVector> vec = std::make_shared<ConfigItemVector>();
  for(QListWidgetItem *item : ui.hostsList->findItems("*", Qt::MatchWildcard)) {
    int n = item->data(Qt::UserRole).toInt();
    Host host = hosts[n];
    std::shared_ptr<ConfigItemMap> map = host.getConfig();
    vec->getValue().push_back(std::make_tuple(std::string("host"), map));
  }
  QString file = QFileDialog::getSaveFileName(this, tr("Save hosts"), QString(), tr("Text files (*.txt)"));
  if(!file.isEmpty()) {
    std::ofstream stream(file.toLocal8Bit().data());
    ConfigFileParser::print_tree(stream, vec);
  }
}


void MainWindow::onOpenHosts()
{
  QString file = QFileDialog::getOpenFileName(this, tr("Open hosts"), QString(), tr("Text files (*.txt)"));
  if(!file.isEmpty()) {
    std::ifstream stream(file.toLocal8Bit().data());
    const std::set<std::string> tags = {"host", "user", "password"}; 
    std::shared_ptr<ConfigItemVector> vec = ConfigFileParser::parser(stream, tags);
    for(auto hostItem : vec->getValue()) {
      Host host;
      std::shared_ptr<ConfigItem> value;
      std::string tag;
      std::tie(tag, value) = hostItem;
      std::shared_ptr<ConfigItemMap> map = ConfigFileParser::getMap(value);
      if(map->getValue().contains(std::string("user"))) {
        std::string str = ConfigFileParser::getMapValue(map, std::string("user"));
        host.user = QString(str.c_str());
      }
      if(map->getValue().contains(std::string("host"))) {
        std::string str = ConfigFileParser::getMapValue(map, std::string("host"));
        host.host = QString(str.c_str());
      }
      if(map->getValue().contains(std::string("password"))) {
        std::string str = ConfigFileParser::getMapValue(map, std::string("password"));
        host.password = QString(str.c_str());
      }
      QListWidgetItem *item = new QListWidgetItem(host.user + "@" + host.host, ui.hostsList, Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
      QVariant var;
      int host_id = get_next_id();
      var.setValue(host_id);
      item->setData(Qt::UserRole, var);
      hosts[host_id] = host;
    }
  }
}

void MainWindow::onItemHostDoubleClicked(QListWidgetItem *item)
{
  int n = item->data(Qt::UserRole).toInt();
  NewHost dialog(this);
  dialog.setHost(hosts[n]);
  if(QDialog::Accepted == dialog.exec()) {
    Host host;
    while(host.host.isEmpty() || host.user.isEmpty()) {
      host = dialog.getHost();
      if(host.host.isEmpty() || host.user.isEmpty()) {
        QMessageBox::StandardButton answer;
        answer = QMessageBox::warning(this, tr("Incomplete Form"),
            tr("The form does not contain all the necessary information.\n"
              "Do you want to discard it?"),
            QMessageBox::Yes | QMessageBox::No);
        if(answer == QMessageBox::Yes)
          break;
        if(QDialog::Accepted != dialog.exec())
          break;
      }
    }
    if(! host.host.isEmpty() && ! host.user.isEmpty()) {
      item->setText(host.user + "@" + host.host);
      hosts[n] = host;
    }  
  }
}

void MainWindow::onEditHosts()
{
  for(QListWidgetItem *item : ui.hostsList->selectedItems()) {
    onItemHostDoubleClicked(item); 
    break;
  }
}


void MainWindow::onNewScript()
{
  ScriptDialog dialog(this);
  if(QDialog::Accepted == dialog.exec()) {
    std::shared_ptr<Script> script = dialog.getScript();
    insertTreeItem(script);
  }
}


void MainWindow::onEditScript()
{
  for(QTreeWidgetItem *selected : ui.scriptsTreeWidget->selectedItems()) {
    if(selected != nullptr) {
      int n = selected->data(0, Qt::UserRole).toInt();
      if(! scripts.contains(n))
        break; // Usually on Monitor lock and Non-loch childs
      std::shared_ptr<ScriptGeneric> script_generic = scripts[n];
      if(script_generic->getType() == ScriptType::SCRIPT) {
        ScriptDialog dialog(this);
        dialog.setScript( std::dynamic_pointer_cast<Script>(script_generic) );
        if(QDialog::Accepted == dialog.exec()) {
          scripts[n] = dialog.getScript();
        }
      } else if(script_generic->getType() == ScriptType::GUI_SCRIPT) {
        GuiScriptDialog dialog(std::dynamic_pointer_cast<GuiScript>(script_generic), script_generic->name, this);
        if(QDialog::Accepted == dialog.exec()) {
          scripts[n] = dialog.getGuiScript();
        }
      } else if(script_generic->getType() == ScriptType::MONITOR) {
        MonitorDialog dialog(this);
        dialog.setMonitor(std::dynamic_pointer_cast<Monitor>(script_generic));
        if(QDialog::Accepted == dialog.exec()) {
          scripts[n] = dialog.getMonitor();
        }
      } else if(script_generic->getType() == ScriptType::SCP) {
        CopyFileToClientDialog dialog(std::dynamic_pointer_cast<SCPScript>(script_generic), this);
        if(QDialog::Accepted == dialog.exec()) {
          scripts[n] = dialog.getSCP();
        }
      }
      selected->setText(0, script_generic->name);
    }
    break;
  }
}


void MainWindow::onScriptDoubleClicked(QTreeWidgetItem *item, int column)
{
  onEditScript();
}


void MainWindow::onNewMonitor()
{
  MonitorDialog dialog(this);
  if(QDialog::Accepted == dialog.exec()) {
    std::shared_ptr<Monitor> script = dialog.getMonitor();
    QTreeWidgetItem *item = insertTreeItem(script);
    QTreeWidgetItem *lock_item = new QTreeWidgetItem(item, QStringList("Lock"));
    lock_item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
    item->addChild(lock_item);
    QTreeWidgetItem *non_lock_item = new QTreeWidgetItem(item, QStringList("Non-lock"));
    non_lock_item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
    item->addChild(non_lock_item);
  }
}


void MainWindow::onNewProject()
{
  for(
      QTreeWidgetItem *item = ui.scriptsTreeWidget->takeTopLevelItem(0); 
      item != nullptr; 
      item = ui.scriptsTreeWidget->takeTopLevelItem(0)
      ) {
    int n = item->data(0, Qt::UserRole).toInt();
    scripts.remove(n);
    delete item;
  }
  for(
      QListWidgetItem *item = ui.hostsList->takeItem(0); 
      item != nullptr; 
      item = ui.hostsList->takeItem(0)
      ) {
    int n = item->data(Qt::UserRole).toInt();
    hosts.remove(n);
    delete item;
  }
  mFileProjectName = QString();
}


void MainWindow::onNewUser()
{
  std::shared_ptr<GuiScript> script = std::make_shared<GuiScript>(); 
  script->sudo = true;
  script->args["User"] = "";
  script->args["Password"] = "";
  script->args["help"] = "Add new user or change password.";
  script->args["command"] = R"(
    useradd -m '{{User}}' && chmod g-rwx '/home/{{User}}' && chmod o-rwx '/home/{{User}}'
    printf '{{Password}}\n{{Password}}' | passwd '{{User}}'
    if [[ $? -ne 0 ]] ; then
      echo "##log: Error: Adding user or changing password {{User}}."
    else
      echo "##log: OK {{User}} has been added."
    fi
    )"; 
  GuiScriptDialog dialog(script, tr("New User"), this);
  dialog.setWindowTitle(tr("New User"));
  if(QDialog::Accepted == dialog.exec()) {
    std::shared_ptr<GuiScript> script = dialog.getGuiScript();
    insertTreeItem(script);
  }
}


void MainWindow::deleteScriptChilds(QTreeWidgetItem *item)
{
  for(int i = 0; i < item->childCount(); i++) {
    QTreeWidgetItem *child = item->child(i);
    int n = child->data(0, Qt::UserRole).toInt();
    if(scripts.contains(n))
      scripts.remove(n);
    deleteScriptChilds(child);
  }
}


void MainWindow::onDeleteScript()
{
  for(QTreeWidgetItem *item : ui.scriptsTreeWidget->selectedItems()) {
    deleteScriptChilds(item);
    int n = item->data(0, Qt::UserRole).toInt();
    scripts.remove(n);
    ui.scriptsTreeWidget->takeTopLevelItem(ui.scriptsTreeWidget->indexOfTopLevelItem(item));
    delete item;
  }
}

void MainWindow::onSaveAs()
{
  QString dir = mFileProjectName.isEmpty() ? "." : mFileProjectName;
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save as"), dir, "Text files (*.txt)");
  if(! fileName.isEmpty()) {
    mFileProjectName = fileName;
    onSave();
  }
}


std::shared_ptr<ConfigItemVector> MainWindow::getScriptsTreeFromTreeWidget(QTreeWidgetItem *item)
{
  std::shared_ptr<ConfigItemVector> scripts_vec = std::make_shared<ConfigItemVector>();
  int count = 0;
  if(item == nullptr)
    count = ui.scriptsTreeWidget->topLevelItemCount();
  else
    count = item->childCount();

  for(int i = 0; i < count; i++) {
    QTreeWidgetItem *child = (item == nullptr) ? ui.scriptsTreeWidget->topLevelItem(i) : item->child(i);
    int n = child->data(0, Qt::UserRole).toInt();
    if(scripts.contains(n)) {
      std::shared_ptr<ScriptGeneric> script = scripts[n];
      if(script->getType() == ScriptType::GUI_SCRIPT || script->getType() == ScriptType::SCRIPT) {
        scripts_vec->getValue().push_back(std::make_tuple(std::string("script"), script->getConfig()));
      } else if(script->getType() == ScriptType::SCP) {
        scripts_vec->getValue().push_back(std::make_tuple(std::string("upload"), script->getConfig()));
      } else if(script->getType() == ScriptType::MONITOR) {
        std::shared_ptr<Monitor> monitor = std::dynamic_pointer_cast<Monitor>(script);
        // Lock scripts
        QTreeWidgetItem *lock = child->child(0);
        std::shared_ptr<ConfigItemVector> lock_scripts = getScriptsTreeFromTreeWidget(lock);
        QTreeWidgetItem *nonLock = child->child(1);
        std::shared_ptr<ConfigItemVector> non_lock_scripts = getScriptsTreeFromTreeWidget(nonLock);
        std::shared_ptr<ConfigItemMap> map = monitor->getConfig();
        map->setKey("scripts", non_lock_scripts);
        map->setKey("scripts_lock", lock_scripts);
        scripts_vec->getValue().push_back(std::make_tuple(std::string("monitor"), map));
      }
    }
  }
  return scripts_vec;
}


void MainWindow::onSave()
{
  if(mFileProjectName.isEmpty()) {
    onSaveAs();
    return;
  }
  std::shared_ptr<ConfigItemVector> tree = std::make_shared<ConfigItemVector>();
  // Get Hosts tree
  std::shared_ptr<ConfigItemVector> hosts_vec = std::make_shared<ConfigItemVector>();
  for(QListWidgetItem *item : ui.hostsList->findItems("*", Qt::MatchWildcard)) {
    int n = item->data(Qt::UserRole).toInt();
    Host host = hosts[n];
    std::shared_ptr<ConfigItemMap> map = host.getConfig();
    hosts_vec->getValue().push_back(std::make_tuple(std::string("host"), map));
  }
  
  // Get scripts tree
  std::shared_ptr<ConfigItemVector> scripts_vec = getScriptsTreeFromTreeWidget();

  // Save tree
  tree->getValue().push_back(std::make_tuple(std::string("hosts"), hosts_vec));
  tree->getValue().push_back(std::make_tuple(std::string("scripts"), scripts_vec));

  std::ofstream stream(mFileProjectName.toLocal8Bit().data());
  ConfigFileParser::print_tree(stream, tree);
}

void MainWindow::buildScriptsTree(std::shared_ptr<ConfigItemVector> tree, QTreeWidgetItem *parent)
{
  try {
    for(auto node : tree->getValue()) {
      std::string tag;
      std::shared_ptr<ConfigItem> leaf;
      std::tie(tag, leaf) = node;
      if(tag == "script") {
        if(leaf->getType() == ConfigItemType::MAP) {
          std::shared_ptr<ConfigItemMap> map = std::static_pointer_cast<ConfigItemMap>(leaf);
          if(map->contains("args")) {
            // GuiScript
            std::shared_ptr<GuiScript> script = std::make_shared<GuiScript>();
            script->name = QString::fromStdString(map->getKeyValue("name"));
            std::string args = map->getKeyValue("args");
            std::set<std::string> tags = {};
            std::stringstream args_stream(args);
            std::shared_ptr<ConfigItemVector> args_vec = ConfigFileParser::parser(args_stream, tags);
            QMap<QString, QString> args_map;
            std::string tag;
            std::shared_ptr<ConfigItem> value;
            for(auto item : args_vec->getValue()) {
              std::tie(tag, value) = item;
              if(value->getType() == ConfigItemType::STRING) {
                std::shared_ptr<ConfigItemString> value_str = static_pointer_cast<ConfigItemString>(value); 
                args_map[QString::fromStdString(tag)] = QString::fromStdString(value_str->getValue());
              }
            }
            script->args = args_map;
            if(map->contains("sudo"))
              script->sudo = map->getKeyValue("sudo") == "yes";
            insertTreeItem(script, parent);
          } else {
            // Simple script
            std::shared_ptr<Script> script = std::make_shared<Script>();
            if(map->contains("name"))
              script->name = QString::fromStdString(map->getKeyValue("name"));
            if(map->contains("sudo"))
              script->sudo = map->getKeyValue("sudo") == "yes";
            if(map->contains("command"))
              script->script = QString::fromStdString(map->getKeyValue("command"));
            insertTreeItem(script, parent);
          }
        }
      } else if(tag == "monitor") {
        if(leaf->getType() == ConfigItemType::MAP) {
          std::shared_ptr<ConfigItemMap> map = std::static_pointer_cast<ConfigItemMap>(leaf);
          std::shared_ptr<Monitor> script = std::make_shared<Monitor>();
          if(map->contains("threads"))
            script->nThreads = std::stoi(map->getKeyValue("threads"));
          if(map->contains("name"))
            script->name = QString::fromStdString(map->getKeyValue("name"));
          QTreeWidgetItem *item = insertTreeItem(script, parent);
          QTreeWidgetItem *lock_item = new QTreeWidgetItem(item, QStringList("Lock"));
          lock_item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
          item->addChild(lock_item);
          QTreeWidgetItem *non_lock_item = new QTreeWidgetItem(item, QStringList("Non-lock"));
          non_lock_item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
          item->addChild(non_lock_item);
          if(map->contains("scripts")) {
            if(map->getValue()["scripts"]->getType() == ConfigItemType::VECTOR) {
              std::shared_ptr<ConfigItemVector> tree = static_pointer_cast<ConfigItemVector>(map->getValue()["scripts"]);
              buildScriptsTree(tree, non_lock_item);
            }
          }
          if(map->contains("scripts_lock")) {
            if(map->getValue()["scripts_lock"]->getType() == ConfigItemType::VECTOR) {
              std::shared_ptr<ConfigItemVector> tree = static_pointer_cast<ConfigItemVector>(map->getValue()["scripts_lock"]);
              buildScriptsTree(tree, lock_item);
            }
          }
        }
      } else if(tag == "upload") {
        if(leaf->getType() == ConfigItemType::MAP) {
          std::shared_ptr<ConfigItemMap> map = std::static_pointer_cast<ConfigItemMap>(leaf);
          std::shared_ptr<SCPScript> script = std::make_shared<SCPScript>();
          if(map->contains("name"))
            script->name = QString::fromStdString(map->getKeyValue("name"));
          if(map->contains("orig"))
            script->orig = QString::fromStdString(map->getKeyValue("orig"));
          if(map->contains("dest"))
            script->dest = QString::fromStdString(map->getKeyValue("dest"));
          if(map->contains("md5"))
            script->md5 = QString::fromStdString(map->getKeyValue("md5"));
          if(map->contains("user"))
            script->user = QString::fromStdString(map->getKeyValue("user"));
          insertTreeItem(script, parent);
        }
      }
    }
  } catch(SimpleException &e) {
    std::cout << "Error: " << e.what() << std::endl;
    exit(0);
  }
}

void MainWindow::onOpen()
{
  QString dir = mFileProjectName.isEmpty() ? "." : mFileProjectName;
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open"), dir, "Text files (*.txt)");
  if(! fileName.isEmpty()) {
    onNewProject();
    mFileProjectName = fileName;
    const std::set<std::string> tags = {"hosts", "scripts", "user", "host", "password", "script", "name", "command", "sudo", "stop_on_error", "args", "orig", "dest", "md5", "threads", "scripts_lock", "type", "upload", "download", "monitor"};
    std::shared_ptr<ConfigItemVector> scripts_and_host = nullptr;
    try {
      scripts_and_host = ConfigFileParser::parser(fileName.toStdString(), tags);
    } catch(SimpleException &e) {
      	QErrorMessage dialog(this);
        dialog.setWindowTitle(tr("Error loading file"));
        dialog.showMessage(QString(tr("Error:\n")) + e.what());
        dialog.exec();
        return;
    }
    std::string tag;
    std::shared_ptr<ConfigItem> value;
    for(auto item : scripts_and_host->getValue()) {
      std::tie(tag, value) = item;
      if(tag == "hosts") {
        // Load hosts list
        if(value->getType() == ConfigItemType::VECTOR) {
          std::shared_ptr<ConfigItemVector> hosts_list = std::static_pointer_cast<ConfigItemVector>(value);
          // Load hosts
          for(auto host_item : hosts_list->getValue()) {
            std::string host_tag;
            std::shared_ptr<ConfigItem> host_value;
            std::tie(host_tag, host_value) = host_item;
            if(host_tag == "host" && host_value->getType() == ConfigItemType::MAP) {
              std::shared_ptr<ConfigItemMap> map_ptr = std::static_pointer_cast<ConfigItemMap>(host_value);
              std::map<std::string, std::shared_ptr<ConfigItem> > map = map_ptr->getValue();
              Host host;
              if(map.contains("user"))
                host.user = QString::fromStdString(ConfigFileParser::getMapValue(map_ptr, "user"));
              if(map.contains("host"))
                host.host = QString::fromStdString(ConfigFileParser::getMapValue(map_ptr, "host"));
              if(map.contains("password"))
                host.password = QString::fromStdString(ConfigFileParser::getMapValue(map_ptr, "password"));
              QListWidgetItem *item = new QListWidgetItem(host.user + "@" + host.host, ui.hostsList, Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
              QVariant var;
              int host_id = get_next_id();
              var.setValue(host_id);
              item->setData(Qt::UserRole, var);
              hosts[host_id] = host;
            }
          }
        }
      } else if(tag == "scripts") {
        // Load scripts tree
        if(value->getType() == ConfigItemType::VECTOR) {
          std::shared_ptr<ConfigItemVector> scripts_tree = std::static_pointer_cast<ConfigItemVector>(value);
          buildScriptsTree(scripts_tree, nullptr);
        }
      }
    }
  }
}


void MainWindow::onCopyFileToClient()
{
  std::shared_ptr<SCPScript> script = std::make_shared<SCPScript>(); 
  CopyFileToClientDialog dialog(script, this);
  dialog.setWindowTitle(tr("Copy file to client"));
  if(QDialog::Accepted == dialog.exec()) {
    script = dialog.getSCP();
    insertTreeItem(script);
  }
}


void MainWindow::loadMenuFile(QString path)
{
  QFile file(path);
  if(!file.exists()) {
    // File not found
    qDebug() << "Menu files '" + path + "' not found.";
  } else {
    // Parse menus
    const std::set<std::string> tags = {"hosts", "scripts", "user", "host", "password", "script", "name", "command", "sudo", "stop_on_error", "args", "orig", "dest", "md5", "threads", "scripts_lock", "type", "upload", "download", "monitor"};
    std::shared_ptr<ConfigItemVector> menu_scripts = nullptr;
    try {
      menu_scripts = ConfigFileParser::parser(path.toStdString(), tags);
    } catch(SimpleException &e) {
      QErrorMessage dialog(this);
      dialog.setWindowTitle(tr("Error loading file"));
      dialog.showMessage(QString(tr("Error:\n")) + e.what());
      dialog.exec();
      return;
    }
    // Menus are loaded. Build GUI
    std::string tag;
    std::shared_ptr<ConfigItem> value;
    for(auto menuItem : menu_scripts->getValue()) {
      std::tie(tag, value) = menuItem;
      if(tag == "scripts") {
        if(value->getType() == ConfigItemType::VECTOR) {
          std::shared_ptr<ConfigItemVector> vec = std::static_pointer_cast<ConfigItemVector>(value);
          for(auto node : vec->getValue()) {
            std::tie(tag, value) = node;
            if(tag == "script") {
              if(value->getType() == ConfigItemType::MAP) {
                std::shared_ptr<ConfigItemMap> map = std::static_pointer_cast<ConfigItemMap>(value);
                if(map->contains("args")) {
                  // Parse GuiScript
                  std::shared_ptr<GuiScript> script = std::make_shared<GuiScript>();
                  script->name = QString::fromStdString(map->getKeyValue("name")).trimmed();
                  std::string args = map->getKeyValue("args");
                  std::set<std::string> tags = {};
                  std::stringstream args_stream(args);
                  std::shared_ptr<ConfigItemVector> args_vec = ConfigFileParser::parser(args_stream, tags);
                  QMap<QString, QString> args_map;
                  std::string tag;
                  std::shared_ptr<ConfigItem> value;
                  for(auto item : args_vec->getValue()) {
                    std::tie(tag, value) = item;
                    if(value->getType() == ConfigItemType::STRING) {
                      std::shared_ptr<ConfigItemString> value_str = static_pointer_cast<ConfigItemString>(value); 
                      args_map[QString::fromStdString(tag)] = QString::fromStdString(value_str->getValue());
                    }
                  }
                  script->args = args_map;
                  if(map->contains("sudo"))
                    script->sudo = map->getKeyValue("sudo") == "yes";
                  // Insert new item in menu
                  QAction *action = ui.menuScripts->addAction(script->name);
                  connect(action, &QAction::triggered, [=, this]() {
                      GuiScriptDialog dialog(script, script->name, this);
                      dialog.setWindowTitle(script->name);
                      if(QDialog::Accepted == dialog.exec()) {
                        std::shared_ptr<GuiScript> script = dialog.getGuiScript();
                        insertTreeItem(script);
                      }
                  });
                }
              }
            }
          }
        }

      }
    }
  }
}

void MainWindow::onRunSSH()
{
  Ui::SSHDialog ui;
  QDialog dialog(this);
  ui.setupUi(&dialog);
  dialog.setWindowTitle(tr("Save and Run"));
  if(QDialog::Accepted == dialog.exec()) {
    // Save script
    onSave();
    if(mFileProjectName.isEmpty())
      return;
    // Build log path
    QTemporaryDir temp;
    temp.setAutoRemove(false);
    QString logPath = "log";
    if(temp.isValid())
      logPath = temp.path();
    // Build command to exec on terminal
    QString password = ui.passwordLineEdit->text(); 
    bool concurrent = ui.concurrentCheckBox->checkState() == Qt::Checked;
    // Build log path
    QString command = QString(BIN_PATH) + "/ssh_helper_cli '" + mFileProjectName + "' --log_path '" + logPath + "' ";
    if(!concurrent)
      command += " --no-multi ";
    if(! password.isEmpty())
      command += " --password '" + password + "' ";
    // Launch terminal
    // TODO: Replace qterminal as terminal
    QString terminal_command = "qterminal -e " + command;
    setVisible(false);
    system(terminal_command.toLocal8Bit().data());
    setVisible(true);
    // Show log
    command = QString(BIN_PATH) + "/ssh_helper_show_log.sh '" + logPath + "' --remove &";
    system(command.toLocal8Bit().data());
  }
}

void MainWindow::listenMenuActions()
{
  // Hosts menu actions
  connect(ui.actionNewHost, &QAction::triggered, this, &MainWindow::onAddHost);
  connect(ui.actionDelete_host, &QAction::triggered, this, &MainWindow::onDeleteHost);
  connect(ui.actionEdit_host, &QAction::triggered, this, &MainWindow::onEditHosts);
  connect(ui.hostsList, &QListWidget::itemDoubleClicked, this, &MainWindow::onItemHostDoubleClicked);
  connect(ui.removeHost, &QToolButton::clicked, this, &MainWindow::onDeleteHost);
  connect(ui.actionOpen_hosts, &QAction::triggered, this, &MainWindow::onOpenHosts);
  connect(ui.actionSave_hosts, &QAction::triggered, this, &MainWindow::onSaveHosts);

  // Script menu actions
  connect(ui.actionNew_script, &QAction::triggered, this, &MainWindow::onNewScript);
  connect(ui.actionEdit_script, &QAction::triggered, this, &MainWindow::onEditScript);
  connect(ui.scriptsTreeWidget, &QTreeWidget::itemDoubleClicked, this, &MainWindow::onScriptDoubleClicked);
  connect(ui.actionNew_monitor, &QAction::triggered, this, &MainWindow::onNewMonitor);
  connect(ui.actionNew_User, &QAction::triggered, this, &MainWindow::onNewUser);
  connect(ui.removeScript, &QToolButton::clicked, this, &MainWindow::onDeleteScript);
  connect(ui.actionCopy_file_to_client, &QAction::triggered, this, &MainWindow::onCopyFileToClient);

  // SSH menu actions
  connect(ui.actionRun, &QAction::triggered, this, &MainWindow::onRunSSH);

  // File menu actions
  connect(ui.actionClose, &QAction::triggered, this, &MainWindow::closeWindow);
  connect(ui.actionNew, &QAction::triggered, this, &MainWindow::onNewProject);
  connect(ui.actionSave, &QAction::triggered, this, &MainWindow::onSave);
  connect(ui.actionSave_as, &QAction::triggered, this, &MainWindow::onSaveAs);
  connect(ui.actionOpen, &QAction::triggered, this, &MainWindow::onOpen);
}

void MainWindow::closeWindow()
{
  close();
}


