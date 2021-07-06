---@diagnostic disable: undefined-global

local ProjectName = "Slimes"
local OutBinDst = _MAIN_SCRIPT_DIR .. "/Example/%{cfg.system}/"

local function GetBasePath()
	return debug.getinfo(1).source:match("@?(.*/)")
end

local function CopyAssetsToOutput(Configuration, BinaryOutputDir, ProjectDir)
    filter ("configurations:" .. Configuration)
		if Configuration == "Debug" or Configuration == "Release" then
			local resFrom = GetBasePath() .. AstFol
            local resBinTo = BinaryOutputDir .. AstFol
            local resProjTo = ProjectDir .. AstFol
            postbuildcommands {
                CopyCmd(resFrom, resBinTo),
                CopyCmd(resFrom, resProjTo)
            }
		elseif Configuration == "Dist" then
            local resFrom = GetBasePath() .. AstFol
            local resBinTo = BinaryOutputDir .. AstFol
            local resProjTo = ProjectDir .. AstFol
            postbuildcommands {
                CopyCmd(resFrom, resBinTo),
                CopyCmd(resFrom, resProjTo)
            }
		end
end


workspace "Saffron"
	architecture "x64"

	configurations {
		"Debug",
		"Release",
		"Dist"
	}

	flags {
		"MultiProcessorCompile"
	}

    location (WrkLoc)

	startproject (ProjectName)

local SaffronEngine2D = require("ThirdParty.SaffronEngine2D.premake5")

project (ProjectName)
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
	staticruntime "On"

	objdir (OutObj)
	location (OutLoc)

    filter "configurations:Debug or Release"
	    targetdir (OutBin)
        SaffronEngine2D.PreBuild("Debug", OutBin, PrjLoc)
        SaffronEngine2D.PostBuild("Debug", OutBin, PrjLoc)
        CopyAssetsToOutput("Debug", OutBin, PrjLoc)

        SaffronEngine2D.PreBuild("Release", OutBin, PrjLoc)
        SaffronEngine2D.PostBuild("Release", OutBin, PrjLoc)
        CopyAssetsToOutput("Release", OutBin, PrjLoc)

    filter "configurations:Dist"
        targetdir (OutBinDst)
        SaffronEngine2D.PreBuild("Dist", OutBinDst, PrjLoc)
        SaffronEngine2D.PostBuild("Dist", OutBinDst, PrjLoc)
        CopyAssetsToOutput("Dist", OutBinDst, PrjLoc)

    filter "configurations:Debug or Release or Dist"

    files {
        "Source/**.h",
		"Source/**.c",
		"Source/**.hpp",
		"Source/**.cpp",
    }

    includedirs {
        "Source",
    }
    
    SaffronEngine2D.Include()
    SaffronEngine2D.Link()
    SaffronEngine2D.AddDefines()



    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"

    filter "configurations:Dist"
        optimize "On"
