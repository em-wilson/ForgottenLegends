cmake_minimum_required(VERSION 3.5)
project(ForgottenLegends)

# Set default C++ version to 17 (required for filesystem services)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add include path for cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(CMAKE_BUILD_TYPE Debug)

set(LIB_LIST cryptlib.lib
            cryptlibd.lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    src/act_clan.cpp
    src/act_comm.cpp
    src/act_enter.cpp
    src/act_info.cpp
    src/act_move.cpp
    src/act_obj.cpp
    src/act_wiz.cpp
    src/alias.cpp
    src/ban.cpp
    src/bank.cpp
    src/BanManager.cpp src/BanManager.h
    src/bit.cpp
    src/board.cpp src/board.h
    src/bow.cpp
    src/Character.cpp src/Character.h
    src/ConnectedStateManager.cpp src/ConnectedStateManager.h
    src/clans/Clan.cpp src/clans/Clan.h
    src/clans/ClanManager.cpp src/clans/ClanManager.h
    src/clans/ClanReader.cpp src/clans/ClanReader.h
    src/clans/ClanWriter.cpp src/clans/ClanWriter.h
    src/clans/IClanReader.h src/clans/IClanWriter.h
    src/connected_state_handlers/AbstractStateHandler.cpp src/connected_state_handlers/AbstractStateHandler.h
    src/connected_state_handlers/BreakConnectStateHandler.cpp src/connected_state_handlers/BreakConnectStateHandler.h
    src/connected_state_handlers/ConfirmNewNameStateHandler.cpp src/connected_state_handlers/ConfirmNewNameStateHandler.h
    src/connected_state_handlers/ConfirmNewPasswordStateHandler.cpp src/connected_state_handlers/ConfirmNewPasswordStateHandler.h
    src/connected_state_handlers/GetNameStateHandler.cpp src/connected_state_handlers/GetNameStateHandler.h
    src/connected_state_handlers/GetNewClassStateHandler.cpp src/connected_state_handlers/GetNewClassStateHandler.h
    src/connected_state_handlers/GetNewPasswordStateHandler.cpp src/connected_state_handlers/GetNewPasswordStateHandler.h
    src/connected_state_handlers/GetNewRaceStateHandler.cpp src/connected_state_handlers/GetNewRaceStateHandler.h
    src/connected_state_handlers/GetNewSexStateHandler.cpp src/connected_state_handlers/GetNewSexStateHandler.h
    src/connected_state_handlers/GetOldPasswordStateHandler.cpp src/connected_state_handlers/GetOldPasswordStateHandler.h
    src/connected_state_handlers/GetReclassStateHandler.cpp src/connected_state_handlers/GetReclassStateHandler.h
    src/connected_state_handlers/ReadMotdStateHandler.cpp src/connected_state_handlers/ReadMotdStateHandler.h
    src/comm.cpp
    src/const.cpp
    src/db.cpp src/db.h
    src/db2.cpp
    src/Descriptor.h
    src/effects.cpp
    src/EquipmentListGenerator.cpp src/EquipmentListGenerator.h
    src/ExtraDescription.cpp src/ExtraDescription.h
    src/enums/ActionTarget.h src/enums/ConnectedState.h src/enums/ExitFlag.h src/enums/FlagConversions.h
    src/FileLogger.cpp src/FileLogger.h
    src/fight.cpp
    src/flags.cpp
    src/Game.cpp src/Game.h
    src/handler.cpp
    src/healer.cpp
    src/helpers/FlagHelper.cpp src/helpers/FlagHelper.h
    src/helpers/SocketHelper.cpp src/helpers/SocketHelper.h
    src/helpers/StringHelper.cpp src/helpers/StringHelper.h
    src/interfaces/IConnectedStateHandler.h
    src/interfaces/ILogger.h
    src/interp.cpp src/interp.h
    src/lookup.cpp src/lookup.h
    src/magic.cpp src/magic.h
    src/magic2.cpp
    src/map.cpp
    src/mem.cpp
    src/merc.h
    src/mob_cmds.cpp src/mob_cmds.h
    src/mob_prog.cpp
    src/music.cpp src/music.h
    src/NonPlayerCharacter.cpp src/NonPlayerCharacter.h
    src/Note.cpp src/Note.h
    src/Object.cpp src/Object.h
    src/ObjectHelper.cpp src/ObjectHelper.h
    src/PlayerCharacter.cpp src/PlayerCharacter.h
    src/PlayerRace.cpp src/PlayerRace.h
    src/Portal.cpp src/Portal.h
    src/Race.cpp src/Race.h
    src/RaceReader.cpp src/RaceReader.h
    src/RaceWriter.cpp src/RaceWriter.h
    src/RaceManager.cpp src/RaceManager.h
    src/recycle.cpp src/recycle.h
    src/Room.cpp src/Room.h
    src/RoomManager.cpp src/RoomManager.h
    src/save.cpp
    src/scan.cpp
    src/search.cpp
    src/skills.cpp
    src/ShopKeeper.cpp src/ShopKeeper.h
    src/special.cpp
    src/tables.cpp src/tables.h
    src/telnet.h
    src/update.cpp
    src/Wiznet.cpp src/Wiznet.h
    src/olc_legacy.cpp

)

set(TEST_FILES
    test/main.cpp
    test/ClanManager.cpp
    test/NonPlayerCharacter.cpp
    test/PlayerCharacter.cpp
    test/StringHelper.cpp
  )

include_directories(src src/enums src/helpers src/interfaces)
add_library(librom STATIC ${SOURCE_FILES})
add_executable(ForgottenLegends src/main.cpp)
target_link_libraries(ForgottenLegends librom)

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)
add_executable(ForgottenLegendsTest ${TEST_FILES})
target_link_libraries(ForgottenLegendsTest PRIVATE Catch2::Catch2WithMain librom)
list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")
enable_testing()
add_test(fl_tests ForgottenLegendsTest)