idf_component_register(SRCS "nvs.c" "ble.c" "wifi.c" "am2320.cpp" "main3.cpp"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES certificate.pem)
