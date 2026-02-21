cmake_minimum_required(VERSION 3.16)
project(poco_template_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(Poco REQUIRED COMPONENTS Net Foundation Util Prometheus JWT)

add_executable(poco_template_server
    src/main.cpp
)

target_include_directories(poco_template_server PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(poco_template_server
    PRIVATE
        Poco::Net
        Poco::Foundation
        Poco::Util
        Poco::Prometheus
        Poco::JWT
)

install(TARGETS poco_template_server DESTINATION bin)
