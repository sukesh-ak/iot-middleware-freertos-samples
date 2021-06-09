# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.14)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)


set(CMAKE_C_STANDARD 99)

# set(AZURE_IOT_MQTT_PORT $ENV{IDF_PATH}/components/mqtt)
set(FREERTOS_INC $ENV{IDF_PATH}/components/freertos/include/freertos)
set(AZ_INC ${CMAKE_CURRENT_LIST_DIR}/libraries/azure-sdk-for-c/sdk)
set(ESP_INC ${CMAKE_CURRENT_LIST_DIR}/ports)


idf_component_register(SRC_DIRS "source" "$ESP_INC" 
                      INCLUDE_DIRS "." "source/include" "source/interface" "${AZ_INC}/inc"
                      PRIV_INCLUDE_DIRS "${FREERTOS_INC}"
                      REQUIRES lwip freertos nghttp mbedtls tcp_transport)

idf_component_get_property(dir freertos COMPONENT_DIR)
message(STATUS "The 'freertos' component directory is: ${dir}")     

project(az_iot_middleware_freertos LANGUAGES C)
# target_compile_definitions(${COMPONENT_TARGET} PUBLIC "-DAZURE_IOT_NO_CUSTOM_CONFIG")
