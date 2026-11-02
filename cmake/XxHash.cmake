# Public xxHash v0.8.3, BSD-2-Clause. Keep upstream code and license in local build/.
set(SAGA_XXHASH_DIR "${CMAKE_SOURCE_DIR}/build/_deps/xxhash-0.8.3")
set(SAGA_XXHASH_SHA256 17973c0dc49d9854ca26caa191f0e12f7a424b68858d9a78de3860d959d85e4b)
file(MAKE_DIRECTORY "${SAGA_XXHASH_DIR}")
set(SAGA_XXHASH_ACTUAL missing)
if(EXISTS "${SAGA_XXHASH_DIR}/xxhash.h")
  file(SHA256 "${SAGA_XXHASH_DIR}/xxhash.h" SAGA_XXHASH_ACTUAL)
endif()
if(NOT SAGA_XXHASH_ACTUAL STREQUAL SAGA_XXHASH_SHA256)
  file(DOWNLOAD "https://raw.githubusercontent.com/Cyan4973/xxHash/v0.8.3/xxhash.h"
    "${SAGA_XXHASH_DIR}/xxhash.h" EXPECTED_HASH SHA256=${SAGA_XXHASH_SHA256} TLS_VERIFY ON)
endif()
add_library(saga_xxhash INTERFACE)
target_include_directories(saga_xxhash SYSTEM INTERFACE "${SAGA_XXHASH_DIR}")
