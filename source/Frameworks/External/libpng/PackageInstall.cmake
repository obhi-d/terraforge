

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(TARGET_NAME libpng)

install(TARGETS png_static EXPORT libpng16Target)
write_basic_package_version_file("${TARGET_NAME}ConfigVersion.cmake"
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY SameMajorVersion)

file(WRITE "${PROJECT_BINARY_DIR}/${TARGET_NAME}Config.cmake.in"
"
@PACKAGE_INIT@
include(\"\${CMAKE_CURRENT_LIST_DIR}/@TARGET_NAME@Targets.cmake\")
check_required_components(\"@TARGET_NAME@\")
"
)

configure_package_config_file(
    "${PROJECT_BINARY_DIR}/${TARGET_NAME}Config.cmake.in"
    "${PROJECT_BINARY_DIR}/${TARGET_NAME}Config.cmake"
    INSTALL_DESTINATION
     cmake)


install(EXPORT libpng16Target
    FILE ${TARGET_NAME}Targets.cmake
    DESTINATION cmake)

install(FILES "${PROJECT_BINARY_DIR}/${TARGET_NAME}Config.cmake"
              "${PROJECT_BINARY_DIR}/${TARGET_NAME}ConfigVersion.cmake"
              DESTINATION cmake)

