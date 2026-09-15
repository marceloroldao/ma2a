cmake_minimum_required(VERSION 3.20)
project(ma2a_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_library(ma2a_core INTERFACE)
target_include_directories(ma2a_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ma2a_core INTERFACE OpenSSL::Crypto)

add_executable(ma2a_core_contract_test tests/contract_test.cpp)
target_link_libraries(ma2a_core_contract_test PRIVATE ma2a_core)

add_executable(ma2a_job_framing_test tests/job_framing_test.cpp)
target_link_libraries(ma2a_job_framing_test PRIVATE ma2a_core)

add_executable(ma2a_python_interop_test tests/python_interop_test.cpp)
target_link_libraries(ma2a_python_interop_test PRIVATE ma2a_core)

add_executable(ma2a_wire_contract_test tests/wire_contract_test.cpp)
target_link_libraries(ma2a_wire_contract_test PRIVATE ma2a_core)

add_executable(ma2a_tcp_crosslang_client tests/tcp_crosslang_client.cpp)
target_link_libraries(ma2a_tcp_crosslang_client PRIVATE ma2a_core)

add_executable(ma2a_tcp_crosslang_signed_client tests/tcp_crosslang_signed_client.cpp)
target_link_libraries(ma2a_tcp_crosslang_signed_client PRIVATE ma2a_core)

add_executable(ma2a_failure_notice_test tests/failure_notice_test.cpp)
target_link_libraries(ma2a_failure_notice_test PRIVATE ma2a_core)

enable_testing()
add_test(NAME ma2a_core_contract_test COMMAND ma2a_core_contract_test)
add_test(NAME ma2a_job_framing_test COMMAND ma2a_job_framing_test)
add_test(NAME ma2a_python_interop_test COMMAND ma2a_python_interop_test)
add_test(NAME ma2a_wire_contract_test COMMAND ma2a_wire_contract_test)
add_test(NAME ma2a_failure_notice_test COMMAND ma2a_failure_notice_test)
add_test(
    NAME ma2a_tcp_crosslang_roundtrip
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/tcp_crosslang_roundtrip.py $<TARGET_FILE:ma2a_tcp_crosslang_client>
)
add_test(
    NAME ma2a_tcp_crosslang_signed_roundtrip
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/tcp_crosslang_signed_roundtrip.py $<TARGET_FILE:ma2a_tcp_crosslang_signed_client>
)
add_test(
    NAME ma2a_distributed_failover_three_nodes
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/distributed_failover_three_nodes.py
)
