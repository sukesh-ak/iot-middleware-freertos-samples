cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp-freertos-sample)


target_add_binary_data(esp-freertos-sample.elf "certs/${CONFIG_AZ_BROKER_CERT}" TEXT)