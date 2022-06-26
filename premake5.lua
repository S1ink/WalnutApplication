-- premake5.lua
workspace "Test Applicaiton"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "Test Application"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
include "Walnut/WalnutExternal.lua"

include "Test Application"