cmake_minimum_required(VERSION 3.16)
project(SFMLToolkit)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

find_package(SFML 2.5 REQUIRED system window graphics network audio)
find_package(Graphviz 2.43 REQUIRED)

add_executable(main src/main.cpp src/constants.cpp)

if(NOT SFML_FOUND)
	message(FATAL_ERROR "SFML Required")
endif()

if(NOT GRAPHVIZ_FOUND)
	message(FATAL_ERROR "GraphViz Required")
endif()


target_link_libraries(main ${SFML_LIBRARIES} ${GRAPHVIZ_CGRAPH_LIBRARY} frontier-phoenix)


