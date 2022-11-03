

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(TARGET_NAME zlib)

install(TARGETS zlib zlibstatic EXPORT ZlibTargets)
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


install(EXPORT ZlibTargets
    FILE ${TARGET_NAME}Targets.cmake
    DESTINATION cmake)

install(FILES "${PROJECT_BINARY_DIR}/${TARGET_NAME}Config.cmake"
              "${PROJECT_BINARY_DIR}/${TARGET_NAME}ConfigVersion.cmake"
              DESTINATION cmake)

