set(component_srcs)

idf_component_register(SRCS "font.c" "ssd1306.c" "${component_srcs}"
                       PRIV_REQUIRES driver
                       INCLUDE_DIRS ".")
