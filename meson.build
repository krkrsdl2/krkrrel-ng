project(
	'krkrrel-ng',
	[
		'c',
		'cpp'
	],
	default_options: [
		'optimization=3',
		'debug=true',
		'warning_level=1'
	],
	version: 'unknown'
)

cxx = meson.get_compiler('cpp')

krkr_minimal_src = [
	'src/krkr_minimal/base/CharacterSet.cpp',
	'src/krkr_minimal/base/StorageIntf.cpp',
	'src/krkr_minimal/base/XP3Archive.cpp',
	'src/krkr_minimal/base/TextStream.cpp',
	'src/krkr_minimal/base/BinaryStream.cpp',
	'src/krkr_minimal/base/UtilStreams.cpp',
	'external/zlib/adler32.c',
	'external/zlib/compress.c',
	'external/zlib/crc32.c',
	'external/zlib/deflate.c',
	'external/zlib/gzclose.c',
	'external/zlib/gzlib.c',
	'external/zlib/gzread.c',
	'external/zlib/gzwrite.c',
	'external/zlib/infback.c',
	'external/zlib/inffast.c',
	'external/zlib/inflate.c',
	'external/zlib/inftrees.c',
	'external/zlib/trees.c',
	'external/zlib/uncompr.c',
	'external/zlib/zutil.c',
	'src/krkr_minimal/msg/android/MsgImpl.cpp',
	'src/krkr_minimal/msg/android/MsgLoad.cpp',
	'src/krkr_minimal/msg/MsgIntf.cpp',
	'src/krkr_minimal/tjs2/tjs.cpp',
	'src/krkr_minimal/tjs2/tjs.tab.cpp',
	'src/krkr_minimal/tjs2/tjsArray.cpp',
	'src/krkr_minimal/tjs2/tjsBinarySerializer.cpp',
	'src/krkr_minimal/tjs2/tjsByteCodeLoader.cpp',
	'src/krkr_minimal/tjs2/tjsCompileControl.cpp',
	'src/krkr_minimal/tjs2/tjsConfig.cpp',
	'src/krkr_minimal/tjs2/tjsConstArrayData.cpp',
	'src/krkr_minimal/tjs2/tjsDate.cpp',
	'src/krkr_minimal/tjs2/tjsDateParser.cpp',
	'src/krkr_minimal/tjs2/tjsDebug.cpp',
	'src/krkr_minimal/tjs2/tjsDictionary.cpp',
	'src/krkr_minimal/tjs2/tjsDisassemble.cpp',
	'src/krkr_minimal/tjs2/tjsError.cpp',
	'src/krkr_minimal/tjs2/tjsException.cpp',
	'src/krkr_minimal/tjs2/tjsGlobalStringMap.cpp',
	'src/krkr_minimal/tjs2/tjsInterCodeExec.cpp',
	'src/krkr_minimal/tjs2/tjsInterCodeGen.cpp',
	'src/krkr_minimal/tjs2/tjsInterface.cpp',
	'src/krkr_minimal/tjs2/tjsLex.cpp',
	'src/krkr_minimal/tjs2/tjsMT19937ar-cok.cpp',
	'src/krkr_minimal/tjs2/tjsMath.cpp',
	'src/krkr_minimal/tjs2/tjsMessage.cpp',
	'src/krkr_minimal/tjs2/tjsNamespace.cpp',
	'src/krkr_minimal/tjs2/tjsNative.cpp',
	'src/krkr_minimal/tjs2/tjsObject.cpp',
	'src/krkr_minimal/tjs2/tjsObjectExtendable.cpp',
	'src/krkr_minimal/tjs2/tjsOctPack.cpp',
	'src/krkr_minimal/tjs2/tjsRandomGenerator.cpp',
	'src/krkr_minimal/tjs2/tjsRegExp.cpp',
	'src/krkr_minimal/tjs2/tjsScriptBlock.cpp',
	'src/krkr_minimal/tjs2/tjsScriptCache.cpp',
	'src/krkr_minimal/tjs2/tjsSnprintf.cpp',
	'src/krkr_minimal/tjs2/tjsString.cpp',
	'src/krkr_minimal/tjs2/tjsUtils.cpp',
	'src/krkr_minimal/tjs2/tjsVariant.cpp',
	'src/krkr_minimal/tjs2/tjsVariantString.cpp',
	'src/krkr_minimal/tjs2/tjsdate.tab.cpp',
	'src/krkr_minimal/tjs2/tjspp.tab.cpp',
	'src/krkr_minimal/utils/DebugIntf.cpp',
	'src/krkr_minimal/utils/cp932_uni.cpp',
	'src/krkr_minimal/utils/uni_cp932.cpp',
]

krkr_minimal_includes = [
	'src/krkr_minimal/base',
	'external/zlib',
	'src/krkr_minimal/msg',
	'src/krkr_minimal/msg/android',
	'src/krkr_minimal/tjs2',
	'src/krkr_minimal/utils',
]

if target_machine.system() == 'windows'
	krkr_minimal_src += [
		'src/krkr_minimal/base/win32/StorageImpl.cpp',
	]
	krkr_minimal_includes += [
		'src/krkr_minimal/base/win32',
	]
else
	krkr_minimal_src += [
		'src/krkr_minimal/base/android/StorageImpl.cpp',
	]
	krkr_minimal_includes += [
		'src/krkr_minimal/base/android',
	]
endif

krkr_minimal_cflags = []
krkr_minimal_cflags += [
	'-DTJS_NO_REGEXP',
]
if target_machine.system() == 'windows'
	krkr_minimal_cflags += [
		'-DUNICODE',
		'-D_UNICODE',
	]
endif
krkr_minimal_cppflags = []
krkr_minimal_cppflags += krkr_minimal_cflags
krkr_minimal_cppflags += [
	'-std=c++11',
]
if cxx.get_id() == 'gcc'
	krkr_minimal_cppflags += ['-fno-delete-null-pointer-checks']
endif
krkr_minimal = static_library(
	'krkr_minimal',
	sources: krkr_minimal_src,
	include_directories: include_directories(krkr_minimal_includes),
	c_args: krkr_minimal_cflags,
	cpp_args: krkr_minimal_cppflags,
)

krkr_minimal_dep = declare_dependency(include_directories: krkr_minimal_includes, link_with: krkr_minimal)

common_exe_ldflags = []
if target_machine.system() == 'windows'
	common_exe_ldflags += [
		'-static',
		'-municode',
	]
endif

krkrrel_src = [
	'src/krkrrel.cpp',
]

krkrrel_dependson = [
	krkr_minimal_dep,
]

executable(
	'krkrrel',
	sources: krkrrel_src,
	cpp_args: krkr_minimal_cppflags,
	link_args: common_exe_ldflags,
	dependencies: krkrrel_dependson,
)
