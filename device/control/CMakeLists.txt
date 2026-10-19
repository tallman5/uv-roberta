cmake_minimum_required(VERSION 3.20)
project(RobertaControl LANGUAGES CXX)

option(ROBERTA_BUILD_DRIVE_RUNTIME "Build the later-phase motor-driving runtime" OFF)

enable_testing()

add_library(roberta_control
  src/controller.cpp
  src/hardware.cpp
  src/rc_config.cpp
  src/rc_mapping.cpp
  src/sbus.cpp)
target_include_directories(roberta_control PUBLIC include)
target_compile_features(roberta_control PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(roberta_control PRIVATE /W4 /WX)
else()
  target_compile_options(roberta_control PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(roberta_control_tests tests/controller_tests.cpp)
target_link_libraries(roberta_control_tests PRIVATE roberta_control)
add_test(NAME roberta_control_tests COMMAND roberta_control_tests)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_package(Threads REQUIRED)
  find_path(PIGPIO_INCLUDE_DIR pigpiod_if2.h REQUIRED)
  find_library(PIGPIOD_IF2_LIBRARY pigpiod_if2 REQUIRED)
  add_executable(roberta_rc_monitor src/linux_rc_monitor.cpp)
  target_include_directories(roberta_rc_monitor PRIVATE ${PIGPIO_INCLUDE_DIR})
  target_link_libraries(
    roberta_rc_monitor
    PRIVATE roberta_control ${PIGPIOD_IF2_LIBRARY} Threads::Threads)
  target_compile_features(roberta_rc_monitor PRIVATE cxx_std_20)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(roberta_rc_monitor PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()

  if(ROBERTA_BUILD_DRIVE_RUNTIME)
    add_executable(roberta_rc_drive
      src/linux_main.cpp
      src/linux_sbus_receiver.cpp
      src/linux_roboteq_driver.cpp)
    target_include_directories(roberta_rc_drive PRIVATE ${PIGPIO_INCLUDE_DIR})
    target_link_libraries(roberta_rc_drive PRIVATE roberta_control)
    target_link_libraries(roberta_rc_drive PRIVATE ${PIGPIOD_IF2_LIBRARY} Threads::Threads)
    target_compile_features(roberta_rc_drive PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(roberta_rc_drive PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
  endif()
endif()
