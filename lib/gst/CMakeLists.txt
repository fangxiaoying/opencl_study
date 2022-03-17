set(MODULE_GST  GST)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${CMAKE_SYSROOT}/usr/include/imx
)


aux_source_directory(. GST_SOURCE)
add_library(${MODULE_GST} STATIC ${GST_SOURCE})
target_link_libraries(${MODULE_GST} g2d)