cmake_minimum_required(VERSION 3.24)

string(CONCAT FOO_DESCRIPTION
  "Give a million monkeys a million keyboards, and eventually they'll write "
  "the complete works of Shakespeare... or at least a valid C++ program.")

project(
  foo
  VERSION 0.1.0
  LANGUAGES CXX C
  DESCRIPTION "${FOO_DESCRIPTION}"
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR}/cmake)

### Options
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_LTO "Enable LTO" OFF)
if(ENABLE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

### CPM
set(CPM_DOWNLOAD_VERSION 0.40.8)
set(CPM_HASH_SUM "78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791")
set(CPM_DOWNLOAD_URL "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake")

if(CPM_SOURCE_CACHE)
  set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
  set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
  set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

get_filename_component(CPM_DOWNLOAD_LOCATION ${CPM_DOWNLOAD_LOCATION} ABSOLUTE)

if (NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
  file(DOWNLOAD ${CPM_DOWNLOAD_URL} ${CPM_DOWNLOAD_LOCATION}
       EXPECTED_HASH SHA256=${CPM_HASH_SUM})
endif()

include(${CPM_DOWNLOAD_LOCATION})

### Libraries
CPMAddPackage("gh:fmtlib/fmt#11.1.4")
set(SPDLOG_FMT_EXTERNAL ON)
CPMAddPackage("gh:gabime/spdlog@1.15.1")
set(ABSL_ENABLE_INSTALL ON)
CPMAddPackage("gh:abseil/abseil-cpp#20250127.1")
CPMAddPackage("gh:google/re2#2024-07-02")
CPMAddPackage("gh:google/googletest@1.16.0")
CPMAddPackage("gh:stephenberry/glaze@5.0.1")
CPMAddPackage("gh:uxlfoundation/oneTBB@2022.0.0")
set(YAML_CPP_BUILD_TESTS Off)
set(YAML_CPP_BUILD_CONTRIB Off)
set(YAML_CPP_BUILD_TOOLS Off)
CPMAddPackage("gh:jbeder/yaml-cpp#39f737443b05e4135e697cb91c2b7b18095acd53")
CPMAddPackage("gh:zhscn/outcome@0.1.0")
CPMAddPackage("gh:simdjson/simdjson@3.12.2")
find_package(Boost 1.87 REQUIRED)
find_package(Clang REQUIRED)
find_package(PkgConfig)

pkg_check_modules(CURSES REQUIRED IMPORTED_TARGET ncurses)

### Full interface target
add_library(full)
target_link_libraries(full INTERFACE
  absl::algorithm
  absl::base
  absl::debugging
  absl::flat_hash_map
  absl::flags
  absl::flags_parse
  absl::log_initialize
  absl::log
  # requires CMake 3.24
  $<LINK_LIBRARY:WHOLE_ARCHIVE,absl::log_flags>
  absl::memory
  absl::meta
  absl::numeric
  absl::random_random
  absl::strings
  absl::synchronization
  absl::time
  absl::utility
  Boost::boost
  fmt::fmt
  glaze::glaze
  outcome::outcome
  outcome::std_error_code
  outcome::exception
  PkgConfig::CURSES
  re2::re2
  simdjson::simdjson
  spdlog::spdlog
  TBB::tbb
)

if(APPLE)
  target_link_options(full INTERFACE -Wl,-no_warn_duplicate_libraries)
endif()

target_include_directories(full PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

configure_file(
  src/version.cc.in
  ${CMAKE_BINARY_DIR}/src/version.cc
  @ONLY
)

target_sources(full PRIVATE ${CMAKE_BINARY_DIR}/src/version.cc)

### LLVM settings
if(CLANG_LINK_CLANG_DYLIB)
  target_link_libraries(full PRIVATE clang-cpp)
else()
  target_link_libraries(full PRIVATE
    clangAST
    clangBasic
    clangDriver
    clangFrontend
    clangFormat
    clangIndex
    clangLex
    clangParse
    clangSema
    clangSerialization
    clangTooling
    clangToolingCore
    clangToolingInclusions)
endif()

if(LLVM_LINK_LLVM_DYLIB)
  target_link_libraries(full PRIVATE LLVM)
else()
  target_link_libraries(full PRIVATE LLVMOption LLVMSupport LLVMTargetParser)
endif()

if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(full PRIVATE -fno-rtti)
  target_compile_definitions(full PRIVATE BOOST_NO_TYPEID)
endif()

foreach(include_dir ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
  get_filename_component(real_dir ${include_dir} REALPATH)
  if(NOT ${real_dir} IN_LIST CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
    target_include_directories(full SYSTEM PRIVATE ${real_dir})
  endif()
endforeach()

### src
enable_testing()
add_subdirectory(src)
