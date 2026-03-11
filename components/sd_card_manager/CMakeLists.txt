idf_component_register(SRCS "sd_card_manager.c"
                      INCLUDE_DIRS "include"
                      REQUIRES fatfs sdmmc esp_driver_gpio)
