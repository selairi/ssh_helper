# ssh_helper

## Overview

`ssh_helper` is a simple tool to administrate a network using SSH.

Each client on the network has to install an SSH server. A static IP has to be set.

A user with the same name and password makes everything easier on network, but if 
someone guesses the user's password, it will be a security hole. `ssh_helper` could 
manage a network with different users and passwords.

## Install

In order to compile `ssh_helper`, you will need:

- C++20 Compiler. Ex. g++
- [cmake](<https://cmake.org/>) 
- [qt5 libraries](<https://www.qt.io/>)
- [libssh](<https://www.libssh.org/>)
- [qterminal](<https://github.com/lxqt/qterminal>)

Then run:

`
cd build
cmake ..
make
sudo make install
`

Ubuntu users can run `install-packages-ubuntu.sh` script to install dependencies and install `ssh_helper`.

### `ssh_helper_gui`. The GUI tool

![`ssh_helper_gui`](ssh_helper_gui.png)

`ssh_helper_gui` is a simple tool to build your scripts. Those scripts will be run in selected hosts:

![`ssh_helper_gui`](ssh_helper_gui_script.png)

Hosts menu helps you to add, remove or edit the hosts list. The hosts list can be saved and loaded in other scripts.

![`ssh_helper_gui`](ssh_helper_gui_host.png)

You can use predefined scripts in Scripts menu or write your own scripts in "Scripts/New script" menu item. This script will be run as bash script in the hosts.

When you finish, use the SSH menu to run your script. If you check the "sudo" option, the script will be run as sudo. You can add `echo "##log: "` to show log messages:
`
rm -Rf 'Path'
if [[ $? -ne 0 ]] ; then
  echo "##log: Error: deleting Path"
else
  echo "##log: OK Path Deleted"
fi
`

You can add more items to menu script by editing the `INSTALL PATH/share/ssh_helper/menu.txt` or writing your own `menu.txt` file and running the command line:

`ssh_helper_gui -m menu.txt`

### `ssh_helper_cli`. The command line tool

