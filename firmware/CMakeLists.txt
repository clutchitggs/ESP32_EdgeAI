cmake_minimum_required(VERSION 3.16.0)

# ESP-IDF entry point. Set IDF_PATH in your environment, or source the
# ESP-IDF export.sh / export.ps1 before running idf.py from this directory.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(xiao_audio_analyzer)
