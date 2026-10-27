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

# macOS/Linux service implementations and service registration. The UI services are created
# through ra::ui::PlatformUi, which the linked UI backend (RAIntegrationHeadless, ...) provides.
if(NOT WIN32)
  find_package(CURL REQUIRED)
  target_sources(RAIntegrationCore PRIVATE
    src/Exports.cpp
    src/RA_Core.cpp
    src/services/Initialization.cpp
    src/services/impl/CurlHttpRequester.cpp
    src/services/impl/PosixDebuggerDetector.cpp
    src/services/impl/PosixFileSystem.cpp
  )
  target_link_libraries(RAIntegrationCore PUBLIC CURL::libcurl)

  # UI backend without any windows: logs dialogs, keeps surfaces in memory.
  add_library(RAIntegrationHeadless STATIC
    src/ui/headless/HeadlessDesktop.cpp
    src/ui/headless/HeadlessImageRepository.cpp
    src/ui/headless/PlatformUi.cpp
  )
  target_link_libraries(RAIntegrationHeadless PUBLIC RAIntegrationCore)
  target_precompile_headers(RAIntegrationHeadless REUSE_FROM RAIntegrationCore)
  ra_apply_compat(RAIntegrationHeadless)

  # starts the integration headless and exercises the platform services
  add_executable(RAHeadlessSmoke tools/HeadlessSmoke.cpp)
  target_link_libraries(RAHeadlessSmoke PRIVATE RAIntegrationHeadless)
  target_precompile_headers(RAHeadlessSmoke REUSE_FROM RAIntegrationCore)
  ra_apply_compat(RAHeadlessSmoke)
endif()

# ---- unit tests ---------------------------------------------------------------
# The tests are written against Microsoft's CppUnitTest framework. On other platforms
# cmake/unittest provides a compatible header and a small runner (TestMain.cpp).
option(RA_BUILD_TESTS "Build the unit tests" ON)
if(RA_BUILD_TESTS AND NOT MSVC)
  enable_testing()
  set(RA_UNITTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake/unittest)

  add_executable(RADevKit.Tests
  tests/devkit/context/impl/ConsoleContext_Tests.cpp
  tests/devkit/context/impl/EmulatorMemoryContext_Tests.cpp
  tests/devkit/context/impl/RcClient_Tests.cpp
  tests/devkit/context/mocks/MockEmulatorMemoryContext.cpp
  tests/devkit/context/mocks/MockRcClient.cpp
  tests/devkit/data/DataModelBase_Tests.cpp
  tests/devkit/data/Memory_Tests.cpp
  tests/devkit/data/ModelPropertyContainer_Tests.cpp
  tests/devkit/data/ModelProperty_Tests.cpp
  tests/devkit/data/models/AchievementModel_Tests.cpp
  tests/devkit/data/models/AchievementSetModel_Tests.cpp
  tests/devkit/data/models/ArrayMemoryNoteModel_Tests.cpp
  tests/devkit/data/models/GameAssets_Tests.cpp
  tests/devkit/data/models/LeaderboardModel_Tests.cpp
  tests/devkit/data/models/LocalBadgesModel_Tests.cpp
  tests/devkit/data/models/MemoryNotesModel_Tests.cpp
  tests/devkit/data/models/PointerMemoryNoteModel_Tests.cpp
  tests/devkit/data/models/RichPresenceModel_Tests.cpp
  tests/devkit/data/models/ValueMemoryNoteModel_Tests.cpp
  tests/devkit/data/Requirement_Tests.cpp
  tests/devkit/data/util/AchievementLogicSerializer_Tests.cpp
  tests/devkit/data/util/IndirectNoteResolver_Tests.cpp
  tests/devkit/data/util/TriggerValidation_Tests.cpp
  tests/devkit/util/Tokenizer_Tests.cpp
  tests/devkit/data/models/AssetModelBase_Tests.cpp
  tests/devkit/data/models/MemoryNoteModel_Tests.cpp
  tests/devkit/data/models/MemoryRegionsModel_Tests.cpp
  tests/devkit/data/NotifyTargetSet_Tests.cpp
  tests/devkit/services/Http_Tests.cpp
  tests/devkit/services/StringTextReader_Tests.cpp
  tests/devkit/services/StringTextWriter_Tests.cpp
  tests/devkit/util/StringBuilder_Tests.cpp
  tests/devkit/util/Strings_Tests.cpp
    cmake/unittest/TestMain.cpp
  )
  target_include_directories(RADevKit.Tests PRIVATE tests/devkit ${RA_UNITTEST_DIR})
  target_link_libraries(RADevKit.Tests PRIVATE RADevKit)
  ra_apply_compat(RADevKit.Tests)
  add_test(NAME RADevKit.Tests COMMAND RADevKit.Tests)

  # Like RA_Integration.Tests.vcxproj, this compiles the integration sources itself with
  # RA_UTEST defined so the tests can substitute mocks.
  add_executable(RA_Integration.Tests
  src/api/ApiCall.cpp
  src/api/impl/ConnectedServer.cpp
  src/data/context/EmulatorContext.cpp
  src/data/context/GameContext.cpp
  src/data/context/SessionTracker.cpp
  src/Exports.cpp
  src/RA_md5factory.cpp
  src/services/AchievementRuntime.cpp
  src/services/AchievementRuntimeExports.cpp
  src/services/FrameEventQueue.cpp
  src/services/GameIdentifier.cpp
  src/services/impl/FileLocalStorage.cpp
  src/services/impl/JsonFileConfiguration.cpp
  src/services/impl/LoginService.cpp
  src/services/impl/OfflineRcClient.cpp
  src/services/PointerFinder.cpp
  src/services/SearchResults.cpp
  src/services/Search/SearchImpl.cpp
  src/ui/Theme.cpp
  src/ui/TransactionalViewModelBase.cpp
  src/ui/ViewModelCollection.cpp
  src/ui/viewmodels/AssetEditorViewModel.cpp
  src/ui/viewmodels/AssetListViewModel.cpp
  src/ui/viewmodels/AssetUploadViewModel.cpp
  src/ui/viewmodels/BrokenAchievementsViewModel.cpp
  src/ui/viewmodels/ChallengeIndicatorViewModel.cpp
  src/ui/viewmodels/EmulatorViewModel.cpp
  src/ui/viewmodels/FileDialogViewModel.cpp
  src/ui/viewmodels/IntegrationMenuViewModel.cpp
  src/ui/viewmodels/LookupItemViewModel.cpp
  src/ui/viewmodels/MemoryBookmarksViewModel.cpp
  src/ui/viewmodels/MemoryInspectorViewModel.cpp
  src/ui/viewmodels/MemoryNotesViewModel.cpp
  src/ui/viewmodels/MemoryRegionsViewModel.cpp
  src/ui/viewmodels/MemorySearchViewModel.cpp
  src/ui/viewmodels/MemoryViewerViewModel.cpp
  src/ui/viewmodels/MemoryWatchListViewModel.cpp
  src/ui/viewmodels/MemoryWatchViewModel.cpp
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
  src/ui/viewmodels/LoginViewModel.cpp
  src/ui/viewmodels/PopupViewModelBase.cpp
  src/ui/viewmodels/ProgressTrackerViewModel.cpp
  src/ui/viewmodels/ProgressViewModel.cpp
  src/ui/viewmodels/ScoreboardViewModel.cpp
  src/ui/viewmodels/ScoreTrackerViewModel.cpp
  src/ui/viewmodels/TriggerConditionViewModel.cpp
  src/ui/viewmodels/TriggerSummaryViewModel.cpp
  src/ui/viewmodels/TriggerViewModel.cpp
  src/ui/viewmodels/UnknownGameViewModel.cpp
  src/ui/ViewModelBase.cpp
  src/ui/viewmodels/GameChecksumViewModel.cpp
  src/ui/viewmodels/MessageBoxViewModel.cpp
  src/ui/viewmodels/RichPresenceMonitorViewModel.cpp
  src/ui/WindowViewModelBase.cpp
  tests/data/context/EmulatorContext_Tests.cpp
  tests/data/context/GameContext_Tests.cpp
  tests/data/context/SessionTracker_Tests.cpp
  tests/devkit/context/mocks/MockEmulatorMemoryContext.cpp
  tests/devkit/context/mocks/MockRcClient.cpp
  tests/Exports_Tests.cpp
  tests/mocks/MockAchievementRuntime.cpp
  tests/services/AchievementRuntimeExports_Tests.cpp
  tests/services/AchievementRuntime_Tests.cpp
  tests/services/FileLocalStorage_Tests.cpp
  tests/services/FrameEventQueue_Tests.cpp
  tests/services/GameIdentifier_Tests.cpp
  tests/services/LoginService_Tests.cpp
  tests/ui/OverlayTheme_Tests.cpp
  tests/ui/ViewModelBase_Tests.cpp
  tests/services/FileLogger_Tests.cpp
  tests/services/JsonFileConfiguration_Tests.cpp
  tests/services/SearchResults_Tests.cpp
  src/RA_Defs.cpp
  tests/ui/ViewModelCollection_Tests.cpp
  tests/ui/viewmodels/AssetEditorViewModel_Tests.cpp
  tests/ui/viewmodels/AssetListViewModel_Tests.cpp
  tests/ui/viewmodels/AssetUploadViewModel_Tests.cpp
  tests/ui/viewmodels/BrokenAchievementsViewModel_Tests.cpp
  tests/ui/viewmodels/MemoryNotesViewModel_Tests.cpp
  tests/ui/viewmodels/GameChecksumViewModel_Tests.cpp
  tests/ui/viewmodels/IntegrationMenuViewModel_Tests.cpp
  tests/ui/viewmodels/LoginViewModel_Tests.cpp
  tests/ui/viewmodels/MemoryBookmarksViewModel_Tests.cpp
  tests/ui/viewmodels/MemoryInspectorViewModel_Tests.cpp
  tests/ui/viewmodels/MemoryRegionsViewModel_Tests.cpp
  tests/ui/viewmodels/MemorySearchViewModel_Tests.cpp
  tests/ui/viewmodels/MemoryViewerViewModel_Tests.cpp
  tests/ui/viewmodels/MemoryWatchListViewModel_Tests.cpp
  tests/ui/viewmodels/MessageBoxViewModel_Tests.cpp
  tests/ui/viewmodels/OverlayAchievementsPageViewModel_Tests.cpp
  tests/ui/viewmodels/OverlayFriendsPageViewModel_Tests.cpp
  tests/ui/viewmodels/OverlayLeaderboardsPageViewModel_Tests.cpp
  tests/ui/viewmodels/OverlayListPageViewModel_Tests.cpp
  tests/ui/viewmodels/OverlayManager_Tests.cpp
  tests/ui/viewmodels/OverlayRecentGamesPageViewModel_Tests.cpp
  tests/ui/viewmodels/OverlaySettingsViewModel_Tests.cpp
  tests/ui/viewmodels/PointerFinderViewModel_Tests.cpp
  tests/ui/viewmodels/PointerInspectorViewModel_Tests.cpp
  tests/ui/viewmodels/ProgressViewModel_Tests.cpp
  tests/ui/viewmodels/RichPresenceMonitorViewModel_Tests.cpp
  tests/ui/viewmodels/TriggerConditionViewModel_Tests.cpp
  tests/ui/viewmodels/TriggerSummaryViewModel_Tests.cpp
  tests/ui/viewmodels/TriggerViewModel_Tests.cpp
  tests/ui/viewmodels/UnknownGameViewModel_Tests.cpp
  tests/ui/WindowViewModelBase_Tests.cpp
  tests/RA_UnitTestHelpers.cpp
    cmake/unittest/TestMain.cpp
  )
  target_compile_definitions(RA_Integration.Tests PRIVATE RA_UTEST)
  target_include_directories(RA_Integration.Tests PRIVATE
    src RAInterface tests tests/devkit ${RA_UNITTEST_DIR} cmake/posix_shims)
  target_link_libraries(RA_Integration.Tests PRIVATE RADevKit)
  target_precompile_headers(RA_Integration.Tests PRIVATE src/pch.h)
  ra_apply_compat(RA_Integration.Tests)
  add_test(NAME RA_Integration.Tests COMMAND RA_Integration.Tests)

  # tests for the macOS/Linux service implementations
  target_sources(RA_Integration.Tests PRIVATE
    src/services/impl/PosixFileSystem.cpp
    tests/services/impl/PosixServices_Tests.cpp
  )
endif()
