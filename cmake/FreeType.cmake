# Desktop font service. Use FreeType's GPLv2 option; downloaded code stays local.
include(FetchContent)
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/build/_deps" CACHE PATH "Local downloaded source dependencies")
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
foreach(component ZLIB BZIP2 PNG HARFBUZZ BROTLI)
  set(FT_DISABLE_${component} ON CACHE BOOL "Unused compressed font/shaping dependency" FORCE)
endforeach()
set(FT_ENABLE_ERROR_STRINGS ON CACHE BOOL "Diagnose explicit font-provider failures" FORCE)
FetchContent_Declare(saga_freetype
  URL https://download.savannah.gnu.org/releases/freetype/freetype-2.13.3.tar.xz
  URL_HASH SHA256=0550350666d427c74daeb85d5ac7bb353acba5f76956395995311a9c6f063289)
FetchContent_MakeAvailable(saga_freetype)
