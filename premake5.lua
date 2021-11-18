workspace "compactxf"
	configurations { "Release", "Debug" }
	platforms { "x86", "x64" }
	location "build"

	files { "src/*.*" }

	includedirs { "src" }

project "compactxf"
	kind "ConsoleApp"
	language "C++"
	targetname "compactxf"
	targetdir "bin/%{cfg.platform}/%{cfg.buildcfg}"

	characterset ("MBCS")
	links { "legacy_stdio_definitions" }
	defines { "_WINDOWS", "WIN32", "WIN32_LEAN_AND_MEAN", "VC_EXTRALEAN", "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_NONSTDC_NO_WARNINGS" }
	staticruntime "on"

	filter "configurations:Debug"
		defines { "_DEBUG" }
		symbols "full"
		optimize "off"
		runtime "debug"

	filter "configurations:Release"
		defines { "NDEBUG" }
		symbols "on"
		optimize "speed"
		runtime "release"
		staticruntime "on"
		flags { "LinkTimeOptimization" }
