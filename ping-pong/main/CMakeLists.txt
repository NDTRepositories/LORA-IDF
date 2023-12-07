set(component_srcs)
set(COMPONENT_ADD_INCLUDEDIRS "")
idf_component_register(SRCS "main.c" "${component_srcs}"
                       INCLUDE_DIRS ".")


