# Cross-platform (macOS/Linux) build. The Windows build still uses RA_Integration.sln.
cmake_minimum_required(VERSION 3.20)
project(RAIntegration C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
  message(WARNING "On Windows, build with RA_Integration.sln instead.")
endif()

find_package(Git QUIET)

# ---- generated version header -------------------------------------------------
set(RA_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${RA_GENERATED_DIR})
add_custom_target(RA_BuildVer
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/RAInterface/MakeBuildVer.sh
          ${RA_GENERATED_DIR}/RA_BuildVer.h RAIntegration RA_INTEGRATION
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  BYPRODUCTS ${RA_GENERATED_DIR}/RA_BuildVer.h
  COMMENT "Generating RA_BuildVer.h"
)

# Shims for MSVC-only annotations (SAL, _NODISCARD) when building with clang/gcc.
set(RA_COMPAT_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/cmake/msvc_compat.h)
function(ra_apply_compat target)
  if(NOT MSVC)
    target_compile_options(${target} PRIVATE
      -Wno-extra-tokens
      "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include ${RA_COMPAT_HEADER}>")
  endif()
endfunction()

# ---- rcheevos ---------------------------------------------------------------
file(GLOB RCHEEVOS_SOURCES CONFIGURE_DEPENDS
  rcheevos/src/api/*.c
  rcheevos/src/hash/*.c
  rcheevos/src/runtime/*.c
  rcheevos/src/util/*.c
  rcheevos/src/client/rc_client.c
  rcheevos/src/client/rc_client_external.c
)
add_library(rcheevos STATIC ${RCHEEVOS_SOURCES})
target_include_directories(rcheevos PUBLIC rcheevos/include)
target_compile_definitions(rcheevos PUBLIC RC_CLIENT_SUPPORTS_HASH)

# ---- RADevKit ---------------------------------------------------------------
file(GLOB_RECURSE RADEVKIT_SOURCES CONFIGURE_DEPENDS src/devkit/*.cpp)
add_library(RADevKit STATIC ${RADEVKIT_SOURCES})
target_include_directories(RADevKit PUBLIC
  src/devkit
  src
  ${CMAKE_CURRENT_SOURCE_DIR}          # for <rcheevos/src/...> internal headers
  GSL/include
  rapidjson/include
  ${RA_GENERATED_DIR}
)
target_link_libraries(RADevKit PUBLIC rcheevos)
add_dependencies(RADevKit RA_BuildVer)
ra_apply_compat(RADevKit)
