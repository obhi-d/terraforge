

include(GNUInstallDirs)

find_package(SDL2 REQUIRED PATHS ${fetch_sdk_dir} NO_DEFAULT_PATH)
find_package(freetype REQUIRED PATHS ${fetch_sdk_dir} NO_DEFAULT_PATH)

add_library(${PROJECT_NAME} STATIC "${fetch_src_dir}/imgui.cpp"  
          "${fetch_src_dir}/imgui_draw.cpp" 
          "${fetch_src_dir}/imgui_tables.cpp"
          "${fetch_src_dir}/imgui_widgets.cpp"
          "${fetch_src_dir}/misc/cpp/imgui_stdlib.cpp"
          "${fetch_src_dir}/misc/freetype/imgui_freetype.cpp"
          )

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC freetype SDL2::SDL2)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${fetch_src_dir}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>
)

target_compile_definitions(
  ${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:IMGUI_ENABLE_FREETYPE>
  $<INSTALL_INTERFACE:IMGUI_ENABLE_FREETYPE>
)

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}_Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CMakePackageConfigHelpers)
write_basic_package_version_file("${PROJECT_NAME}ConfigVersion.cmake"
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY SameMajorVersion)

file(WRITE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake.in"
"
@PACKAGE_INIT@

include(\"\${CMAKE_CURRENT_LIST_DIR}/@PROJECT_NAME@Targets.cmake\")
check_required_components(\"@PROJECT_NAME@\")

"
)

configure_package_config_file(
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake.in"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION
     cmake)


install(EXPORT ${PROJECT_NAME}_Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION cmake)

install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
              "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
              DESTINATION cmake)

install(DIRECTORY ${fetch_src_dir}/misc DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

install(FILES 
        ${fetch_src_dir}/imconfig.h
        ${fetch_src_dir}/imgui.h
        ${fetch_src_dir}/imgui_internal.h
        ${fetch_src_dir}/imstb_rectpack.h
        ${fetch_src_dir}/imstb_textedit.h
        ${fetch_src_dir}/imstb_truetype.h
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
