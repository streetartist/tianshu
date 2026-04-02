idf_component_register(
    SRCS "src/tianshu.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_client esp_websocket_client json
)
