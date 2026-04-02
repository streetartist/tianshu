idf_component_register(SRCS "audio_player.c"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "../../main"
                       PRIV_REQUIRES esp_http_client esp-tls driver esp_psram
                                     fatfs sdmmc vfs)
