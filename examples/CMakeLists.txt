# Examples CMakeLists.txt

# Simple server example
add_executable(simple_server simple_server.c)
target_link_libraries(simple_server PRIVATE x11vnc_lib)
target_include_directories(simple_server PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
)

# API demo example
add_executable(api_demo api_demo.c)
target_link_libraries(api_demo PRIVATE x11vnc_lib)
target_include_directories(api_demo PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
)