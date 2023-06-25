cmake_minimum_required(VERSION 3.0.0)
project(iniparse VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

add_library(${PROJECT_NAME} iniparse.c)

add_executable(${PROJECT_NAME}-test main.c)
target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})

add_test(NAME test-${PROJECT_NAME} COMMAND ${PROJECT_NAME}-test ../test.ini)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
