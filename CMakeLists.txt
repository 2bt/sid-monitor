cmake_minimum_required(VERSION 3.7)

project(sid-monitor)

add_compile_options(-std=c++17 -Wall -O2)
#add_compile_options(-std=c++17 -Wall -Og -g)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

file(GLOB SRC
    "src/*.cpp"
    "src/resid-0.16/*.cc"
    )

add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
