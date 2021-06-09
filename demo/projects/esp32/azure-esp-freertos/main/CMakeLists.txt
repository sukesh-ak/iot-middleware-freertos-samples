
set(COMPONENT_SRCS "main.c" 
                   "azure_esp_iot_task.c"
                   "esp_fr_common.c"
                   "esp32_tls_transport/esp_tls_transport.c" 
                   

list(APPEND COMPONENT_ADD_INCLUDEDIRS "."
                                      "esp32_tls_transport" 
                                      "include"
                                      )

register_component()