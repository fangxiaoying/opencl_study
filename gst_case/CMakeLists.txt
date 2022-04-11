include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ../lib
    )


add_executable(open_camera_0 open_camera_base.cc)
target_link_libraries(open_camera_0  ${OpenCV_LIBS} )

add_executable(open_camera_x open_camera_gst.cc)
target_link_libraries(open_camera_x  ${GSTREAMER_LIBRARIES} ${OpenCV_LIBS} gstallocators-1.0 cairo )

add_executable(open_camera_1 open_camera_v1.cc)
target_link_libraries(open_camera_1  ${GSTREAMER_LIBRARIES} ${OpenCV_LIBS} gstallocators-1.0 cairo )

add_executable(open_camera_2 open_camera_v2.cc)
target_link_libraries(open_camera_2  ${GSTREAMER_LIBRARIES} ${OpenCV_LIBS} GPU GST gstallocators-1.0 cairo g2d)

add_executable(open_camera_3 open_camera_v3.cc)
target_link_libraries(open_camera_3  ${GSTREAMER_LIBRARIES} ${OpenCV_LIBS} GPU GST gstallocators-1.0 cairo g2d)

add_executable(open_camera_tmp open_camera_tmp.cc)
target_link_libraries(open_camera_tmp  ${GSTREAMER_LIBRARIES} ${OpenCV_LIBS} GPU GST gstallocators-1.0 cairo g2d)

add_executable(g2d_ g2d_test.cc)
target_link_libraries(g2d_  ${GSTREAMER_LIBRARIES} ${OpenCV_LIBS} GPU GST gstallocators-1.0 cairo g2d)

add_executable(g2d_resize g2d_resize.cc)
target_link_libraries(g2d_resize  ${GSTREAMER_LIBRARIES} ${OpenCV_LIBS} GPU GST gstallocators-1.0 cairo g2d)

