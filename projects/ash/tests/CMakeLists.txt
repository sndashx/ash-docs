# Test target. Catch2 will be added via CPM once the user fills in
# CPM.cmake; for the scaffold we declare the executable so the configure
# step succeeds even offline.
function(ash_add_test_dir dir)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt)
        add_subdirectory(${dir})
    endif()
endfunction()

set(ASH_TEST_SOURCES
    test_main.cpp
    render/test_cell.cpp
    render/test_ansi.cpp
    render/test_buffer.cpp
    render/test_camera.cpp
    render/test_light.cpp
    render/test_compositor.cpp
    render/test_animation.cpp
    rexpaint/test_xp_loader.cpp
    rexpaint/test_xp_writer.cpp
    rexpaint/test_layer_spec.cpp
    world/test_components.cpp
    world/test_query.cpp
    world/test_trigger.cpp
    editor/test_undo.cpp
    editor/test_clipboard.cpp
    input/test_key.cpp
    input/test_keybinds.cpp
    character/test_attributes.cpp
    character/test_skills.cpp
    character/test_inventory.cpp
    character/test_equipment.cpp
    character/test_derived.cpp
    combat/test_attack.cpp
    combat/test_damage.cpp
    combat/test_status.cpp
    combat/test_path.cpp
    combat/test_los.cpp
    npc/test_schedule.cpp
    npc/test_disposition.cpp
    npc/test_dialogue_run.cpp
    dialogue/test_dlg_lexer.cpp
    dialogue/test_dlg_parser.cpp
    dialogue/test_dlg_eval.cpp
    quest/test_flag_store.cpp
    quest/test_qst_parser.cpp
    quest/test_qst_engine.cpp
    save/test_save.cpp
    save/test_load.cpp
    save/test_migrate.cpp
    content/test_content_load.cpp
    ui/test_ui.cpp
)

add_executable(ash_tests ${ASH_TEST_SOURCES})
target_link_libraries(ash_tests PRIVATE ash_lib)
target_include_directories(ash_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Hook Catch2 in once CPM provides it. Without Catch2 we still build a
# runnable test executable (the stubs return immediately).
if(TARGET Catch2::Catch2WithMain)
    target_link_libraries(ash_tests PRIVATE Catch2::Catch2WithMain)
    include(Catch)
    catch_discover_tests(ash_tests)
endif()
