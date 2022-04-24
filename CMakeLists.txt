cmake_minimum_required(VERSION 3.16.0)

project(ssh_helper VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(configfileparser)
add_subdirectory(ssh_helper_cli)
add_subdirectory(ssh_helper_gui)
add_subdirectory(ssh_helper_show_log)
