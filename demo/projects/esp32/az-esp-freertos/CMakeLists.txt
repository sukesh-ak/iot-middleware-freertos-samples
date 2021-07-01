#
# Component Makefile
#

set (includes)
set (srcs)
set (priv_includes)
set (srcs_dirs)
#set (az_sdk_path "azure-iot-middleware-freertos/libraries/azure-sdk-for-c")


#list(APPEND az_sdk_srcs
#    ${az_sdk_path}sdk/src/azure/core
#    ${az_sdk_path}sdk/src/azure/iot
#    ${az_sdk_path}sdk/src/azure/platform
#    )
message(STATUS "starting base component register")

list(APPEND includes
    "port/exponential_backoff"
    "port/logging"
    "port/platform"
    "port/network_transport"
    "config"
    #"azure-iot-middleware-freertos/source/include"
    #"azure-iot-middleware-freertos/source/interface"
    #"azure-iot-middleware-freertos/libraries/coreMQTT/source/include"
    #"azure-iot-middleware-freertos/libraries/coreMQTT/source/interface"
    #"${az_sdk_path}/sdk/inc/azure"
    )

#list(APPEND priv_includes
#     "azure-iot-middleware-freertos/libraries/azure-sdk-for-c/sdk/inc/azure"   
#        )

list(APPEND srcs
    "port/exponential_backoff/exponential_backoff.c"
    "port/network_transport/tls_freertos.c"
    "port/platform/platform.c"
    )

list(APPEND srcs_dirs
    "port/exponential_backoff/"
    "port/platform"
    "port/network_transport"
    #"azure-iot-middleware-freertos/source"
    #"azure-iot-middleware-freertos/libraries/coreMQTT/source"
    )

idf_component_register( SRC_DIRS ${srcs_dirs}
                        INCLUDE_DIRS ${includes}
                        PRIV_INCLUDE_DIRS ${priv_includes}
                        REQUIRES lwip nghttp mbedtls tcp_transport
                      )
idf_component_get_property(esp_freertos_path freertos COMPONENT_DIR)
message(STATUS "freertos-${esp_freertos_path}")

include_directories("port/logging")
include_directories("port/platform")

message(STATUS "cmake dir: ${CMAKE_CURRENT_LIST_DIR}")

set(FREERTOS_DIRECTORY ${esp_freertos_path} CACHE PATH "esp freertos")
set(CONFIG_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/config CACHE PATH "esp config")
set(FREERTOS_PORT_DIRECTORY ${esp_freertos_path}/xtensa/include/freertos CACHE PATH "esp port")

add_subdirectory(azure-iot-middleware-freertos)
#target_link_libraries(${COMPONENT_LIB} PUBLIC azure-az-iot-middleware-freertos)
target_link_libraries(${COMPONENT_LIB} PUBLIC
    az::iot_middleware::freertos
    )

#add_subdirectory(${az_sdk_path} azure-sdk-for-c)

#include_directories(BEFORE ${az_sdk_path})
#include_directories(BEFORE azure-iot-middleware-freertos)

#target_link_libraries(azure-iot-middleware-freertos PUBLIC azure-sdk-for-c)
#target_include_directories(${COMPONENT_LIB} BEFORE PUBLIC
#                           "azure-az-iot-middleware-freertos"
#                           "${az_sdk_path}"
#                           )



# SRCS "${srcs}"
