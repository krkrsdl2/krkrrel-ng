cmake_minimum_required(VERSION 3.0...3.10)
project(krkrrel-ng)

if((NOT DEFINED CMAKE_BUILD_TYPE) OR (NOT CMAKE_BUILD_TYPE) OR (CMAKE_BUILD_TYPE STREQUAL ""))
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-fno-delete-null-pointer-checks KRKR_MINIMAL_COMPILER_SUPPORTS_NO_DELETE_NULL_POINTER_CHECKS)

set(KRKR_MINIMAL_CXXFLAGS "")

if(KRKR_MINIMAL_COMPILER_SUPPORTS_NO_DELETE_NULL_POINTER_CHECKS)
list(APPEND KRKR_MINIMAL_CXXFLAGS
	-fno-delete-null-pointer-checks
)
endif()

set(KRKR_MINIMAL_SRC
	src/krkr_minimal/base/CharacterSet.cpp
	src/krkr_minimal/base/StorageIntf.cpp
	src/krkr_minimal/base/XP3Archive.cpp
	src/krkr_minimal/base/TextStream.cpp
	src/krkr_minimal/base/BinaryStream.cpp
	src/krkr_minimal/base/UtilStreams.cpp
	external/zlib/adler32.c
	external/zlib/compress.c
	external/zlib/crc32.c
	external/zlib/deflate.c
	external/zlib/infback.c
	external/zlib/inffast.c
	external/zlib/inflate.c
	external/zlib/inftrees.c
	external/zlib/trees.c
	external/zlib/uncompr.c
	external/zlib/zutil.c
	src/krkr_minimal/msg/android/MsgImpl.cpp
	src/krkr_minimal/msg/android/MsgLoad.cpp
	src/krkr_minimal/msg/MsgIntf.cpp
	src/krkr_minimal/tjs2/tjs.cpp
	src/krkr_minimal/tjs2/tjs.tab.cpp
	src/krkr_minimal/tjs2/tjsArray.cpp
	src/krkr_minimal/tjs2/tjsBinarySerializer.cpp
	src/krkr_minimal/tjs2/tjsByteCodeLoader.cpp
	src/krkr_minimal/tjs2/tjsCompileControl.cpp
	src/krkr_minimal/tjs2/tjsConfig.cpp
	src/krkr_minimal/tjs2/tjsConstArrayData.cpp
	src/krkr_minimal/tjs2/tjsDate.cpp
	src/krkr_minimal/tjs2/tjsDateParser.cpp
	src/krkr_minimal/tjs2/tjsDebug.cpp
	src/krkr_minimal/tjs2/tjsDictionary.cpp
	src/krkr_minimal/tjs2/tjsDisassemble.cpp
	src/krkr_minimal/tjs2/tjsError.cpp
	src/krkr_minimal/tjs2/tjsException.cpp
	src/krkr_minimal/tjs2/tjsGlobalStringMap.cpp
	src/krkr_minimal/tjs2/tjsInterCodeExec.cpp
	src/krkr_minimal/tjs2/tjsInterCodeGen.cpp
	src/krkr_minimal/tjs2/tjsInterface.cpp
	src/krkr_minimal/tjs2/tjsLex.cpp
	src/krkr_minimal/tjs2/tjsMT19937ar-cok.cpp
	src/krkr_minimal/tjs2/tjsMath.cpp
	src/krkr_minimal/tjs2/tjsMessage.cpp
	src/krkr_minimal/tjs2/tjsNamespace.cpp
	src/krkr_minimal/tjs2/tjsNative.cpp
	src/krkr_minimal/tjs2/tjsObject.cpp
	src/krkr_minimal/tjs2/tjsObjectExtendable.cpp
	src/krkr_minimal/tjs2/tjsOctPack.cpp
	src/krkr_minimal/tjs2/tjsRandomGenerator.cpp
	src/krkr_minimal/tjs2/tjsRegExp.cpp
	src/krkr_minimal/tjs2/tjsScriptBlock.cpp
	src/krkr_minimal/tjs2/tjsScriptCache.cpp
	src/krkr_minimal/tjs2/tjsSnprintf.cpp
	src/krkr_minimal/tjs2/tjsString.cpp
	src/krkr_minimal/tjs2/tjsUtils.cpp
	src/krkr_minimal/tjs2/tjsVariant.cpp
	src/krkr_minimal/tjs2/tjsVariantString.cpp
	src/krkr_minimal/tjs2/tjsdate.tab.cpp
	src/krkr_minimal/tjs2/tjspp.tab.cpp
	src/krkr_minimal/utils/DebugIntf.cpp
	src/krkr_minimal/utils/cp932_uni.cpp
	src/krkr_minimal/utils/uni_cp932.cpp
)

set(KRKR_MINIMAL_INCLUDES
	src/krkr_minimal/base
	external/zlib
	src/krkr_minimal/msg
	src/krkr_minimal/msg/android
	src/krkr_minimal/tjs2
	src/krkr_minimal/utils
)

set(KRKR_MINIMAL_DEFINITIONS
	TJS_NO_REGEXP
)

if(WIN32)
list(APPEND KRKR_MINIMAL_SRC
	src/krkr_minimal/base/win32/StorageImpl.cpp
)
list(APPEND KRKR_MINIMAL_INCLUDES
	src/krkr_minimal/base/win32
)
list(APPEND KRKR_MINIMAL_DEFINITIONS
	UNICODE
	_UNICODE
)
else()
list(APPEND KRKR_MINIMAL_SRC
	src/krkr_minimal/base/android/StorageImpl.cpp
)
list(APPEND KRKR_MINIMAL_INCLUDES
	src/krkr_minimal/base/android
)
endif()

add_library(krkr_minimal STATIC ${KRKR_MINIMAL_SRC})
set_target_properties(krkr_minimal PROPERTIES C_STANDARD 11)
set_target_properties(krkr_minimal PROPERTIES CXX_STANDARD 11)
target_compile_definitions(krkr_minimal PUBLIC ${KRKR_MINIMAL_DEFINITIONS})
target_compile_options(krkr_minimal PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${KRKR_MINIMAL_CXXFLAGS}>)
target_include_directories(krkr_minimal PUBLIC ${KRKR_MINIMAL_INCLUDES})

set(KRKRREL_SRC
	src/krkrrel.cpp
)

set(KRKRREL_DEFINITIONS
	${KRKR_MINIMAL_DEFINITIONS}
)

add_executable(krkrrel ${KRKRREL_SRC})
set_target_properties(krkrrel PROPERTIES CXX_STANDARD 11)
target_compile_definitions(krkrrel PUBLIC ${KRKRREL_DEFINITIONS})
target_link_libraries(krkrrel krkr_minimal)
if(WIN32)
target_link_options(krkrrel PUBLIC -static -static-libgcc -static-libstdc++ -municode)
endif()
