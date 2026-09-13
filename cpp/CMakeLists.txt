cmake_minimum_required(VERSION 3.20)
project(ma2a_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ma2a_core INTERFACE)
target_include_directories(ma2a_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(ma2a_core_contract_test tests/contract_test.cpp)
target_link_libraries(ma2a_core_contract_test PRIVATE ma2a_core)

add_executable(ma2a_job_framing_test tests/job_framing_test.cpp)
target_link_libraries(ma2a_job_framing_test PRIVATE ma2a_core)

enable_testing()
add_test(NAME ma2a_core_contract_test COMMAND ma2a_core_contract_test)
add_test(NAME ma2a_job_framing_test COMMAND ma2a_job_framing_test)
