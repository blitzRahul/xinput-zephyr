# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(xinput-middleware)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/usb)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
