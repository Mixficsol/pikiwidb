# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

INCLUDE_GUARD()

FetchContent_Declare(
        rocksdb
        GIT_REPOSITORY https://github.com/facebook/rocksdb.git
        GIT_TAG v8.3.3
)

SET(BUILD_TYPE OFF CACHE BOOL "" FORCE)
SET(WITH_TESTS OFF CACHE BOOL "" FORCE)
SET(WITH_BENCHMARK OFF CACHE BOOL "" FORCE)
SET(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "" FORCE)
SET(WITH_TOOLS OFF CACHE BOOL "" FORCE)
SET(WITH_CORE_TOOLS OFF CACHE BOOL "" FORCE)
SET(WITH_TRACE_TOOLS OFF CACHE BOOL "" FORCE)
SET(WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
SET(ROCKSDB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
SET(WITH_LIBURING OFF CACHE BOOL "" FORCE)
SET(WITH_LZ4 OFF CACHE BOOL "" FORCE)
SET(WITH_SNAPPY OFF CACHE BOOL "" FORCE)
SET(WITH_ZLIB ON CACHE BOOL "" FORCE)
SET(WITH_ZSTD OFF CACHE BOOL "" FORCE)
SET(WITH_GFLAGS OFF CACHE BOOL "" FORCE)
SET(USE_RTTI ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(rocksdb)
