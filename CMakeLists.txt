# Cross-platform (macOS/Linux) build. The Windows build still uses RA_Integration.sln.
cmake_minimum_required(VERSION 3.20)
project(RAIntegration C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
  message(WARNING "On Windows, build with RA_Integration.sln instead.")
endif()

find_package(Git QUIET)

# ---- generated version header -------------------------------------------------
set(RA_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${RA_GENERATED_DIR})
add_custom_target(RA_BuildVer
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/RAInterface/MakeBuildVer.sh
          ${RA_GENERATED_DIR}/RA_BuildVer.h RAIntegration RA_INTEGRATION
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  BYPRODUCTS ${RA_GENERATED_DIR}/RA_BuildVer.h
  COMMENT "Generating RA_BuildVer.h"
)

# Shims for MSVC-only annotations (SAL, _NODISCARD) when building with clang/gcc.
set(RA_COMPAT_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/cmake/msvc_compat.h)
function(ra_apply_compat target)
  if(NOT MSVC)
    target_compile_options(${target} PRIVATE
      -Wno-extra-tokens
      "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include ${RA_COMPAT_HEADER}>")
  endif()
endfunction()

# ---- rcheevos ---------------------------------------------------------------
file(GLOB RCHEEVOS_SOURCES CONFIGURE_DEPENDS
  rcheevos/src/api/*.c
  rcheevos/src/hash/*.c
  rcheevos/src/runtime/*.c
  rcheevos/src/util/*.c
  rcheevos/src/client/rc_client.c
  rcheevos/src/client/rc_client_external.c
)
add_library(rcheevos STATIC ${RCHEEVOS_SOURCES})
target_include_directories(rcheevos PUBLIC rcheevos/include)
target_compile_definitions(rcheevos PUBLIC RC_CLIENT_SUPPORTS_HASH)

# ---- RADevKit ---------------------------------------------------------------
file(GLOB_RECURSE RADEVKIT_SOURCES CONFIGURE_DEPENDS src/devkit/*.cpp)
add_library(RADevKit STATIC ${RADEVKIT_SOURCES})
target_include_directories(RADevKit PUBLIC
  src/devkit
  src
  ${CMAKE_CURRENT_SOURCE_DIR}          # for <rcheevos/src/...> internal headers
  GSL/include
  rapidjson/include
  ${RA_GENERATED_DIR}
)
target_link_libraries(RADevKit PUBLIC rcheevos)
add_dependencies(RADevKit RA_BuildVer)
ra_apply_compat(RADevKit)

# ---- RAIntegrationCore (non-UI part of RA_Integration) -------------------------
# Mirrors src/RA_Integration.vcxproj minus the Win32 views (ui/win32), GDI drawing,
# Windows service implementations and the DLL export layer.
add_library(RAIntegrationCore STATIC
  src/api/ApiCall.cpp
  src/api/impl/ConnectedServer.cpp
  src/data/context/EmulatorContext.cpp
  src/data/context/GameContext.cpp
  src/data/context/SessionTracker.cpp
  src/RA_Defs.cpp
  src/RA_md5factory.cpp
  src/services/AchievementRuntime.cpp
  src/services/AchievementRuntimeExports.cpp
  src/services/FrameEventQueue.cpp
  src/services/GameIdentifier.cpp
  src/services/impl/FileLocalStorage.cpp
  src/services/impl/JsonFileConfiguration.cpp
  src/services/impl/LoginService.cpp
  src/services/impl/OfflineRcClient.cpp
  src/services/impl/ThreadPool.cpp
  src/services/PerformanceCounter.cpp
  src/services/PointerFinder.cpp
  src/services/SearchResults.cpp
  src/services/Search/SearchImpl.cpp
  src/ui/Theme.cpp
  src/ui/TransactionalViewModelBase.cpp
  src/ui/ViewModelBase.cpp
  src/ui/ViewModelCollection.cpp
  src/ui/viewmodels/AssetEditorViewModel.cpp
  src/ui/viewmodels/AssetListViewModel.cpp
  src/ui/viewmodels/AssetUploadViewModel.cpp
  src/ui/viewmodels/BrokenAchievementsViewModel.cpp
  src/ui/viewmodels/ChallengeIndicatorViewModel.cpp
  src/ui/viewmodels/MemoryNotesViewModel.cpp
  src/ui/viewmodels/EmulatorViewModel.cpp
  src/ui/viewmodels/FileDialogViewModel.cpp
  src/ui/viewmodels/GameChecksumViewModel.cpp
  src/ui/viewmodels/IntegrationMenuViewModel.cpp
  src/ui/viewmodels/LoginViewModel.cpp
  src/ui/viewmodels/LookupItemViewModel.cpp
  src/ui/viewmodels/MemoryBookmarksViewModel.cpp
  src/ui/viewmodels/MemoryInspectorViewModel.cpp
  src/ui/viewmodels/MemoryRegionsViewModel.cpp
  src/ui/viewmodels/MemorySearchViewModel.cpp
  src/ui/viewmodels/MemoryViewerViewModel.cpp
  src/ui/viewmodels/MemoryWatchListViewModel.cpp
  src/ui/viewmodels/MemoryWatchViewModel.cpp
  src/ui/viewmodels/MessageBoxViewModel.cpp
  src/ui/viewmodels/NewAssetViewModel.cpp
  src/ui/viewmodels/OverlayAchievementsPageViewModel.cpp
  src/ui/viewmodels/OverlayFriendsPageViewModel.cpp
  src/ui/viewmodels/OverlayLeaderboardsPageViewModel.cpp
  src/ui/viewmodels/OverlayListPageViewModel.cpp
  src/ui/viewmodels/OverlayManager.cpp
  src/ui/viewmodels/OverlayRecentGamesPageViewModel.cpp
  src/ui/viewmodels/OverlaySettingsViewModel.cpp
  src/ui/viewmodels/OverlayViewModel.cpp
  src/ui/viewmodels/PointerFinderViewModel.cpp
  src/ui/viewmodels/PointerInspectorViewModel.cpp
  src/ui/viewmodels/PopupMessageViewModel.cpp
  src/ui/viewmodels/PopupViewModelBase.cpp
  src/ui/viewmodels/ProgressTrackerViewModel.cpp
  src/ui/viewmodels/ProgressViewModel.cpp
  src/ui/viewmodels/RichPresenceMonitorViewModel.cpp
  src/ui/viewmodels/ScoreboardViewModel.cpp
  src/ui/viewmodels/ScoreTrackerViewModel.cpp
  src/ui/viewmodels/TriggerConditionViewModel.cpp
  src/ui/viewmodels/TriggerSummaryViewModel.cpp
  src/ui/viewmodels/TriggerViewModel.cpp
  src/ui/viewmodels/UnknownGameViewModel.cpp
  src/ui/WindowViewModelBase.cpp
)
target_include_directories(RAIntegrationCore PUBLIC src RAInterface)
if(NOT WIN32)
  target_include_directories(RAIntegrationCore PUBLIC cmake/posix_shims)
endif()
target_link_libraries(RAIntegrationCore PUBLIC RADevKit)
target_precompile_headers(RAIntegrationCore PRIVATE src/pch.h)
ra_apply_compat(RAIntegrationCore)
