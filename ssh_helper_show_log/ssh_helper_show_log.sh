#!/usr/bin/bash

# (c)GPL3
#
# Copyright: 2022 P.L. Lucas <selairi@gmail.com>
# 
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along with 
# this program. If not, see <https://www.gnu.org/licenses/>. 

function list_files() {
  for file_path in `ls "$1"/*.txt` ; do
    printf "\033[1m "
    printf `basename "$file_path" ".txt"` 
    printf "\033[0m"
    printf ":\n"
    cat "$file_path" | eval `printf "sed 's/OK/\033[1;32mOk\033[0m/g'"` | eval `printf "sed 's/Error:/\033[1;31mError:\033[0m/g'"` | eval `printf "sed 's/FAIL/\033[1;31mFAIL\033[0m/g'"`

    echo
  done
}

list_files "$1" > "$1"/log

mv "$1"/log "$1"/log.txt
# TODO: Replace vte-2.91 as terminal
qterminal -e less -R "$1"/log.txt
rm "$1"/log.txt

if [ "$2" == "--remove" ] ; then
  echo "Remove " $1
  rm -Rf "$1"
fi

