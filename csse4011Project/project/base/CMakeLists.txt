cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(project/base)

set(CMAKE_C_COMPILE_FEATURES ${compile_features_c11})
list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)

# zephyr_nanopb_sources()

target_sources(app PRIVATE src/main.c src/bluetooth.c src/bluetoothtransmit.c)
