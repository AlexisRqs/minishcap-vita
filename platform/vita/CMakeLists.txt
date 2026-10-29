cmake_minimum_required(VERSION 3.20)
project(tmc_vita C CXX)
set(TMC_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
file(GLOB TMC_ACTOR_SOURCES CONFIGURE_DEPENDS
    "${TMC_ROOT}/src/engine/src/playerItem/*.c"
    "${TMC_ROOT}/src/engine/src/enemy/*.c"
    "${TMC_ROOT}/src/engine/src/npc/*.c"
    "${TMC_ROOT}/src/engine/src/object/*.c"
    "${TMC_ROOT}/src/engine/src/projectile/*.c"
    "${TMC_ROOT}/src/engine/src/manager/*.c"
    "${TMC_ROOT}/src/engine/src/item/*.c"
    "${TMC_ROOT}/src/engine/src/subtask/*.c"
    "${TMC_ROOT}/src/engine/src/menu/*.c"
    "${TMC_ROOT}/src/engine/src/worldEvent/*.c"
)

file(GLOB AGBPLAY_SOURCES CONFIGURE_DEPENDS "${TMC_ROOT}/src/third_party/agbplay_core/*.cpp")

set(TMC_ENGINE_SOURCES
    src/engine/src/main.c src/engine/src/common.c src/engine/src/interrupts.c src/engine/src/game.c src/engine/src/title.c src/engine/src/fileselect.c
    src/engine/src/flags.c src/engine/src/save.c src/engine/src/fade.c src/engine/src/color.c src/engine/src/vram.c src/engine/src/screenTileMap.c
    src/engine/src/message.c src/engine/src/text.c src/engine/src/sound.c
    src/engine/src/script.c src/engine/src/scroll.c src/engine/src/room.c src/engine/src/roomInit.c src/engine/src/entity.c src/engine/src/physics.c
    src/engine/src/collision.c src/engine/src/movement.c src/engine/src/affine.c src/engine/src/sineTable.c src/engine/src/ui.c
    src/engine/src/player.c src/engine/src/playerUtils.c src/engine/src/playerHitbox.c src/engine/src/playerItem.c
    src/engine/src/playerItemUtils.c src/engine/src/playerItemDefinitions.c src/engine/src/enemy.c src/engine/src/enemyUtils.c
    src/engine/src/npc.c src/engine/src/npcUtils.c src/engine/src/npcFunctions.c src/engine/src/npcDefinitions.c src/engine/src/object.c
    src/engine/src/objectUtils.c src/engine/src/objectDefinitions.c src/engine/src/enemyUpdate.c src/engine/src/projectile.c
    src/engine/src/projectileUtils.c src/engine/src/projectileUpdate.c src/engine/src/manager.c src/engine/src/item.c
    src/engine/src/itemUtils.c src/engine/src/itemDefinitions.c src/engine/src/itemMetaData.c src/engine/src/subtask.c
    src/engine/src/cutscene.c src/engine/src/backgroundAnimations.c src/engine/src/beanstalkSubtask.c
    src/engine/src/enterPortalSubtask.c src/engine/src/gameOverTask.c src/engine/src/gameUtils.c src/engine/src/gameData.c
    src/engine/src/kinstone.c src/engine/src/droptables.c src/engine/src/demo.c src/engine/src/debug.c src/engine/src/flagDebug.c
    src/engine/src/staffroll.c src/engine/src/data/figurineMenuData.c src/engine/src/data/hitbox.c
    src/engine/src/data/areaMetadata.c src/engine/src/data/caveBorderMapData.c src/engine/src/data/data_080046A4.c
    src/engine/src/data/mapActTileToSurfaceType.c src/engine/src/data/objPalettes.c
    src/engine/src/data/screenTransitions.c src/engine/src/data/transitions.c src/engine/src/code_08049CD4.c
    src/engine/src/code_08049DF4.c src/engine/src/code_0805EC04.c
)
list(TRANSFORM TMC_ENGINE_SOURCES PREPEND "${TMC_ROOT}/")

set(TMC_PORT_SOURCES
    src/port/port_bios.c src/port/port_debug_verbose.c src/port/port_inline_ptrs.c
    src/port/port_script_addrs.c src/port/port_offset_remap.c src/port/port_rom.c
    src/port/port_region_data.c
    src/port/port_bottle_compat.c
    src/port/port_bomb_compat.c
    src/port/port_cloud_tops_fight.c
    src/port/port_vaati_progress.c
    src/port/port_stubs.c src/port/stubs_autogen.c src/port/data_stubs_autogen.c
    src/port/data_const_stubs.c src/port/flag_remap_generated.c src/port/port_rom_tables.c
    src/port/port_linked_stubs.c src/port/port_widescreen.c src/port/port_figurines.c src/port/port_draw.c
    src/port/port_gba_mem.c src/port/port_hdma.c src/port/port_save.c src/port/port_softslots.c
    src/port/port_roll_attack_macro.c
    src/port/port_filter.c src/port/port_animation.c src/port/port_math.c
    src/port/port_text_render.c src/port/port_gameplay_stubs.c src/port/port_m4a_stubs.c
    src/port/port_room_funcs.c src/port/port_script_funcs.c src/port/port_analog_movement.c
    src/port/port_m4a_backend.cpp src/port/port_m4a_mixdown.c src/port/generated_sounds_embed.cpp
    src/port/ppu/src/virtuappu.c src/port/ppu/src/mode1.c
)
list(TRANSFORM TMC_PORT_SOURCES PREPEND "${TMC_ROOT}/")

set(TMC_GENERATED_DIR "" CACHE PATH "Local directory containing generated game-data sources")
if(NOT TMC_GENERATED_DIR)
  message(FATAL_ERROR "Set TMC_GENERATED_DIR to your local generated source directory; game data is not included in this repository")
endif()
foreach(name data_const_stubs.c port_rom_tables.c data_stubs_autogen.c generated_sounds_embed.cpp)
  if(NOT EXISTS "${TMC_GENERATED_DIR}/${name}")
    message(FATAL_ERROR "Missing local generated source: ${TMC_GENERATED_DIR}/${name}")
  endif()
  list(REMOVE_ITEM TMC_PORT_SOURCES "${TMC_ROOT}/src/port/${name}")
  list(APPEND TMC_PORT_SOURCES "${TMC_GENERATED_DIR}/${name}")
endforeach()



add_executable(tmc-vita ${TMC_ENGINE_SOURCES} ${TMC_ACTOR_SOURCES} ${TMC_PORT_SOURCES} ${AGBPLAY_SOURCES}
  vita_livearea.c vita_menu.c port-menu/port_menu.c port-menu/port_model.c port-menu/port_backup.c port-menu/port_runtime.c vita_main.c vita_platform.c vita_ppu.c vita_audio.c vita_optional.c
  ../3ds/source/port_asset_stubs_3ds.c vita_config.c ../3ds/source/port_optional_stubs_3ds.c)
target_include_directories(tmc-vita PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${TMC_ROOT}
 ${TMC_ROOT}/src/engine/include ${TMC_ROOT}/src/port ${TMC_ROOT}/src/port/ppu/include
 ${TMC_ROOT}/src/third_party/agbplay_core ${TMC_ROOT}/src/offsets/USA
 ${TMC_ROOT}/platform/3ds/source)
target_compile_definitions(tmc-vita PRIVATE PC_PORT NON_MATCHING USE_HDMA TMC_3DS TMC_VITA MULTI_REGION USA ENGLISH REVISION=0
 MODE1_GBA_WIDTH=240 MODE1_GBA_HEIGHT=160 TMC_PORT_VERSION="0.06" TMC_PC_VERSION="0.06")
target_compile_options(tmc-vita PRIVATE -include region.h -O2 -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections -Wno-unused-parameter -Wno-missing-field-initializers)
set_target_properties(tmc-vita PROPERTIES C_STANDARD 11 CXX_STANDARD 20)
if(VITA)
 target_link_options(tmc-vita PRIVATE -Wl,-q -Wl,-z,nocopyreloc -Wl,--gc-sections -Wl,--defsym=__sce_headroom=0x1000)
 target_link_libraries(tmc-vita PRIVATE vita2d SceDisplay_stub SceCtrl_stub SceTouch_stub SceAudio_stub ScePower_stub SceGxm_stub SceSysmodule_stub ScePgf_stub SceCommonDialog_stub SceAppMgr_stub SceLiveArea_stub m)
else()
 find_package(SDL2 REQUIRED CONFIG PATHS /opt/homebrew/lib/cmake/SDL2)
 target_link_libraries(tmc-vita PRIVATE SDL2::SDL2)
endif()
