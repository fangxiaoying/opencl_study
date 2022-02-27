find_package(PkgConfig)
pkg_check_modules(GLIB REQUIRED glib-2.0)
link_directories(${GLIB_LIBRARY_DIRS} ${GSTREAMER_LIBRARY_DIRS})

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ../lib)

add_executable(copy_base copy_base.cc)
target_link_libraries(copy_base  ${OpenCV_LIBS} GPU)

add_executable(zero_cpy zero_copy.cc)
target_link_libraries(zero_cpy  ${OpenCV_LIBS} GPU )

add_executable(zero-copy-cma zero_cpy_g2d.cc )
target_link_libraries(zero-copy-cma  ${OpenCV_LIBS} GPU g2d)

# add_executable(gst_debug gst_pipeline_debug.cc)
# target_link_libraries(gst_debug  ${GSTREAMER_LIBRARIES} cairo ${OpenCV_LIBS} GPU)

# add_executable(motor_debug motor_test.cc)
# target_link_libraries(motor_debug )

