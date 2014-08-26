if (not _ACTION) then return end

-- A solution contains projects, and defines the available configurations
solution "lua_tinker"
   configurations { "Debug", "Release" }
   location ("build/" .. _ACTION)
   defines {"_WINDOWS", "WIN32"}
 
   -- A project defines one build target
   project "lua_tinker"
      targetname "lua_tinker"
      language "C++"

      -- project wise settings
      files 
      { 
          "lua_tinker.cpp"
      }

      includedirs 
      {
          ".."
      }

      flags 
      {
          "Unicode", 
          "WinMain",
          "Symbols" 
      }

      defines "_CRT_SECURE_NO_WARNINGS"
      warnings "Extra"

      kind "StaticLib"

      configuration "Debug*"
         defines 
         { 
             "_DEBUG", 
             "DEBUG" 
         }
         optimize "Off"
 
      configuration "Release*"
         defines { "NDEBUG" }
         optimize "Full"
         flags { 'LinkTimeOptimization', }

      for _, config in ipairs(configurations()) do
         configuration(config)
         targetdir ("lib/" .. config)
         objdir ("obj/" .. config)
      end

