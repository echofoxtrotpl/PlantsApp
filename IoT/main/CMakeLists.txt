idf_component_register(SRCS "main.cpp" "nvs.c" "ble.c" "wifi.c" "am2320.cpp" "bh1750.cpp"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES certificate.pem)
