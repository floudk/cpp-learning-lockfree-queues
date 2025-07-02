cmake_minimum_required(VERSION 3.15)
project(disruptor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
  src/main.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

# ----------- unit test -----------
add_executable(ringbuffer_test
    test/lock_vyukov_test.cpp
)
target_include_directories(ringbuffer_test PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(ringbuffer_test PRIVATE pthread)
enable_testing()
add_test(NAME RingBufferTest COMMAND ringbuffer_test)