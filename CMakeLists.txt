cmake_minimum_required(VERSION 3.16)
project(HelloWorldSFML)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

find_package(SFML 2.5 REQUIRED system window graphics network audio)
find_package(Graphviz REQUIRED)

add_executable(main src/main.cpp src/constants.cpp)

if(SFML_FOUND)
	target_link_libraries(main ${SFML_LIBRARIES} cgraph gvc)
endif(SFML_FOUND)


