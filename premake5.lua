workspace 'bea'
	configurations {'debug', 'release'}

-- Options --
newoption {
	trigger = 'enable-asan',
	description = 'Enable address sanitization (debug)',
}

-- Global configurations --
language 'C++'
cppdialect 'C++20'
includedirs {
	'include',
	'vendor/tomlc17/src',
	'vendor/reproc/reproc/include',
}
libdirs {
	'vendor/tomlc17/src',
	'vendor/reproc/build/reproc/lib',
}
objdir 'obj'

filter 'configurations:debug'
	defines {'DEBUG'}
	symbols 'on'

filter 'configurations:release'
	defines {'NDEBUG'}
	optimize 'on'

filter 'options:enable-asan'
	sanitize {'Address'}

-- Bea compiler / frontend --
project 'bea'
	kind 'ConsoleApp'
	files {'src/bea/**.cpp', 'include/bea/**.hpp'}
	links {'tomlc17:static', 'reproc:static'}
	targetdir 'bin'
