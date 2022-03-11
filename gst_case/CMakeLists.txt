include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS})


add_executable(open_camera_0 open_camera_base.cc)
target_link_libraries(open_camera_0  ${OpenCV_LIBS} )


add_executable(open_camera_1 open_camera_gst.cc)
target_link_libraries(open_camera_1  ${GSTREAMER_LIBRARIES} ${OpenCV_LIBS} gstallocators-1.0 cairo )

