cmake_minimum_required(VERSION 3.20)
project(SIGSServerV3 VERSION 3.0.3.0 LANGUAGES CXX)
if(NOT WIN32)
  message(FATAL_ERROR "The server account backend currently requires Windows BCrypt")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(FD_SETSIZE=256 NOMINMAX WIN32_LEAN_AND_MEAN)
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS src/core/*.cpp)
add_library(sigscore ${CORE_SOURCES})
target_include_directories(sigscore PUBLIC src/core)
target_link_libraries(sigscore PUBLIC ws2_32)
add_executable(sigs-server src/main.cpp src/server/Server.cpp src/server/Services.cpp src/server/Accounts.cpp src/server/Storage.cpp src/server/Updates.cpp src/server/Protocol.cpp)
target_include_directories(sigs-server PRIVATE src/server)
target_link_libraries(sigs-server PRIVATE sigscore bcrypt)
if(MSVC)
  target_compile_options(sigs-server PRIVATE /W4 /EHsc)
endif()
enable_testing()
add_executable(core-tests tests/core.cpp)
target_link_libraries(core-tests PRIVATE sigscore)
add_test(NAME core COMMAND core-tests)
if(NOT DEFINED Python3_EXECUTABLE AND DEFINED ENV{USERPROFILE})
  set(BUNDLED_PYTHON "$ENV{USERPROFILE}/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe")
  if(EXISTS "${BUNDLED_PYTHON}")
    set(Python3_EXECUTABLE "${BUNDLED_PYTHON}")
  endif()
endif()
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME build-layout COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/build_layout.py ${CMAKE_COMMAND})
  add_test(NAME services COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration.py $<TARGET_FILE:sigs-server>)
  set(RESEARCH_INPUTS
    tests/research.py
    tools/research.py
    references/binaries/SIERRANW.DLL
    references/binaries/snwvalid.dll
    third_party/research/pefile.py
    third_party/research/ordlookup/__init__.py
    third_party/research/capstone/__init__.py
    third_party/research/capstone/x86.py
    third_party/research/capstone/x86_const.py
    third_party/research/capstone/lib/capstone.dll
  )
  set(RESEARCH_AVAILABLE TRUE)
  foreach(RESEARCH_INPUT IN LISTS RESEARCH_INPUTS)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${RESEARCH_INPUT}")
      set(RESEARCH_AVAILABLE FALSE)
    endif()
  endforeach()
  if(RESEARCH_AVAILABLE)
    add_test(NAME research COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/research.py)
  else()
    message(STATUS "Skipping research tests: local research scripts, libraries or reference binaries are missing")
  endif()
endif()
