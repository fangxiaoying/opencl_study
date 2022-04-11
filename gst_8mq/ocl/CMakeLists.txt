set(MODULE_GPU  GPU)

aux_source_directory(. GPU_SOURCE)
add_library(${MODULE_GPU} STATIC ${GPU_SOURCE})
target_link_libraries(${MODULE_GPU} OpenCL)