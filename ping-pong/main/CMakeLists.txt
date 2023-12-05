set(component_srcs "main.c")
set(COMPONENT_ADD_INCLUDEDIRS "")
idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS ".")


