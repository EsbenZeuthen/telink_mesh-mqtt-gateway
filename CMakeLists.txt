cmake_minimum_required(VERSION 3.10)

# Project name
project(ProjectName)

# C++ standard
set(CMAKE_CXX_STANDARD 17)

#add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
#add_link_options(-fsanitize=address,undefined)


# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(GLIBMM REQUIRED glibmm-2.4)
pkg_check_modules(GIOMM REQUIRED giomm-2.4)
pkg_check_modules(GIO REQUIRED gio-2.0 giomm-2.4)
pkg_check_modules(JSON REQUIRED jsoncpp)
#pkg_check_modules(BLUETOOTH REQUIRED bluetooth)

# Include directories for libraries
include_directories(
  ${GLIB2_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
  ${GIOMM_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
  ${MQTT_INCLUDE_DIRS}
  ${JSON_INCLUDE_DIRS}
  /usr/include
  #${BLUETOOTH_INCLUDE_DIRS}
)

link_directories(/usr/lib/x86_64-linux-gnu)

add_executable(meshgateway
  src/main.cpp
  src/crypto/crypto.cpp
  src/ble_stack/bluezproxy.cpp
  src/ble_stack/telink_mesh.cpp
)

target_link_libraries(meshgateway
  ${GLIB2_LIBRARIES}
  ${GLIBMM_LIBRARIES}
  ${BLUETOOTH_LIBRARIES}
  ${GIO_LIBRARIES}
  ${MQTT_LIBRARIES}
  ${JSON_LIBRARIES}
  paho-mqtt3c
  paho-mqttpp3
  crypto  
)

# Set environment variable for debug logging
set_target_properties(meshgateway PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "G_MESSAGES_DEBUG=all"
)

# Bluetooth PoC executable
#add_executable(ble_poc
#    src/ble_poc.cpp
#    src/crypto/crypto.cpp
#)

# Link libraries for Bluetooth PoC
#target_link_libraries(ble_poc
#  ${GLIB2_LIBRARIES}
#  ${GLIBMM_LIBRARIES}
#  ${BLUETOOTH_LIBRARIES}
#  ${GIO_LIBRARIES}
#  crypto
#)

if(BUILD_TESTS)

  # Enable testing
  enable_testing()

  # Add Google Test library
  find_package(GTest REQUIRED)

  # Crypto test executable
  add_executable(crypto_tests
      tests/test_crypto.cpp
      src/crypto/crypto.cpp
  )

  # Link libraries for Crypto test
  target_link_libraries(crypto_tests 
      GTest::GTest 
      GTest::Main 
      pthread 
      ssl 
      crypto
  )

  # Include directories for test sources
  target_include_directories(crypto_tests PRIVATE src/crypto)

  # Add tests to CTest
  add_test(NAME CryptoTests COMMAND crypto_tests)


endif()