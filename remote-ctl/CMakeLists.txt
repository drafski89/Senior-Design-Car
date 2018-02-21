include_directories(
    ${catkin_INCLUDE_DIRS}
    inc/)

set(ROVER_SRC
    src/rover.cpp
    src/RoverApp.cpp
    src/ArduinoPWM.cpp)

add_executable(rover ${ROVER_SRC})

target_link_libraries(rover ${catkin_LIBRARIES})

set(GCC_ADDITIONAL_COMPILE_FLAGS "-Wall -std=c++11 -pthread")
set(GCC_ADDITIONAL_LINK_FLAGS "-lSDL2 -pthread")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_ADDITIONAL_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_ADDITIONAL_LINK_FLAGS}")
