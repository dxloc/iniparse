cmake_minimum_required(VERSION 3.0.0)
project(iniparse VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

add_library(${PROJECT_NAME} iniparse.c)
set_target_properties(${PROJECT_NAME} PROPERTIES
  PUBLIC_HEADER iniparse.h
)

add_executable(${PROJECT_NAME}-test test.c)

target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})

add_test(NAME test-${PROJECT_NAME} COMMAND ${PROJECT_NAME}-test ${CMAKE_BINARY_DIR}/Testing/test.ini)

add_custom_command(
  TARGET ${PROJECT_NAME}-test POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/tests/test.ini
          ${CMAKE_BINARY_DIR}/Testing/test.ini
)

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/../release)

install(TARGETS ${PROJECT_NAME})
install(TARGETS ${PROJECT_NAME}-test)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "TGZ;ZIP;7Z")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
