include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ../lib)

add_executable(copy-base copy_base.cc)
target_link_libraries(copy-base  ${OpenCV_LIBS} GPU)

add_executable(zero-copy zero_copy.cc)
target_link_libraries(zero-copy  ${OpenCV_LIBS} GPU)

add_executable(zero-copy-cma zero_copy_g2d.cc )
target_link_libraries(zero-copy-cma  ${OpenCV_LIBS} GPU g2d)

add_executable(zero-copy-last zero_copy_last.cc )
target_link_libraries(zero-copy-last  ${OpenCV_LIBS} GPU g2d)

add_executable(bgra2rgb-ocl bgra2rgb.cc )
target_link_libraries(bgra2rgb-ocl  ${OpenCV_LIBS} GPU g2d)

add_executable(vec-ocl learning_vector.cc )
target_link_libraries(vec-ocl  ${OpenCV_LIBS} GPU g2d)

