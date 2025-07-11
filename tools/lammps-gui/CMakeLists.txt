cmake_minimum_required(VERSION 3.16)

project(lammps-gui VERSION 1.6.14 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(LAMMPS_GUI_USE_PLUGIN "Load LAMMPS library dynamically at runtime" OFF)
mark_as_advanced(LAMMPS_GUI_USE_PLUGIN)
option(LAMMPS_GUI_USE_QT5 "Prefer using Qt5 over Qt6" OFF)

include(CheckIncludeFileCXX)
# helper function to check for usable omp.h header
function(check_omp_h_include)
  find_package(OpenMP COMPONENTS CXX QUIET)
  if(OpenMP_CXX_FOUND)
    set(CMAKE_REQUIRED_FLAGS ${OpenMP_CXX_FLAGS})
    set(CMAKE_REQUIRED_INCLUDES ${OpenMP_CXX_INCLUDE_DIRS})
    set(CMAKE_REQUIRED_LINK_OPTIONS ${OpenMP_CXX_FLAGS})
    set(CMAKE_REQUIRED_LIBRARIES ${OpenMP_CXX_LIBRARIES})
    # there are all kinds of problems with finding omp.h
    # for Clang and derived compilers so we pretend it is there.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      set(_have_omp_h TRUE)
    else()
      check_include_file_cxx(omp.h _have_omp_h)
    endif()
  else()
    set(_have_omp_h FALSE)
  endif()
  set(HAVE_OMP_H_INCLUDE ${_have_omp_h} PARENT_SCOPE)
endfunction()

# detect if we may enable OpenMP support by default
set(BUILD_OMP_DEFAULT OFF)
find_package(OpenMP COMPONENTS CXX QUIET)
if(OpenMP_CXX_FOUND)
  check_omp_h_include()
  if(HAVE_OMP_H_INCLUDE)
    set(BUILD_OMP_DEFAULT ON)
  endif()
endif()

option(BUILD_OMP "Build with OpenMP support" ${BUILD_OMP_DEFAULT})
if(BUILD_OMP)
  find_package(OpenMP COMPONENTS CXX REQUIRED)
  check_omp_h_include()
  if(NOT HAVE_OMP_H_INCLUDE)
    message(FATAL_ERROR "Cannot find the 'omp.h' header file required for full OpenMP support")
  endif()
endif()

# checks
# when this file is included as subdirectory in the LAMMPS build, many settings are directly imported
if(LAMMPS_DIR)
  set(LAMMPS_HEADER_DIR ${LAMMPS_SOURCE_DIR})
  set(LAMMPS_LIBRARY lammps)
else()
  # NOTE: the next line should be commented out when used outside of the LAMMPS package
  get_filename_component(LAMMPS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src ABSOLUTE)
  set(LAMMPS_HEADER_DIR ${LAMMPS_SOURCE_DIR} CACHE PATH "Location of LAMMPS headers")
  if(NOT LAMMPS_HEADER_DIR)
    message(FATAL_ERROR "Must set LAMMPS_HEADER_DIR")
  endif()
  get_filename_component(LAMMPS_DIR ${LAMMPS_SOURCE_DIR}/.. ABSOLUTE)
  # find LAMMPS library
  find_library(LAMMPS_LIBRARY
    NAMES lammps lammps_serial
    HINTS ${LAMMPS_LIB_DIR} ${LAMMPS_HEADER_DIR}/../build ${LAMMPS_HEADER_DIR}/../build-clang ${LAMMPS_HEADER_DIR}/../build-test ${LAMMPS_HEADER_DIR}
    REQUIRED
  )
  # by default, install into $HOME/.local (not /usr/local),
  # so that no root access (and sudo) is needed
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Default install path" FORCE)
  endif()
  # ugly hacks for MSVC which by default always reports an old C++ standard in the __cplusplus macro
  # and prints lots of pointless warnings about "unsafe" functions
  if(MSVC)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      add_compile_options(/Zc:__cplusplus)
      add_compile_options(/wd4244)
      add_compile_options(/wd4267)
      add_compile_options(/wd4250)
      add_compile_options(/EHsc)
    endif()
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  endif()
endif()

option(BUILD_WHAM "Download and compile WHAM executable from Grossfield Lab" YES)
if(BUILD_WHAM)
  set(WHAM_URL "http://membrane.urmc.rochester.edu/sites/default/files/wham/wham-release-2.1.0.tgz" CACHE STRING "URL for WHAM tarball")
  set(WHAM_MD5 "4ed6e24254925ec124f44bb381c3b87f" CACHE STRING "MD5 checksum of WHAM tarball")
  mark_as_advanced(WHAM_URL)
  mark_as_advanced(WHAM_MD5)

  get_filename_component(archive ${WHAM_URL} NAME)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/src)
  if(EXISTS ${CMAKE_BINARY_DIR}/_deps/${archive})
    file(MD5 ${CMAKE_BINARY_DIR}/_deps/${archive} DL_MD5)
  endif()
  if(NOT "${DL_MD5}" STREQUAL "${WHAM_MD5}")
    message(STATUS "Downloading ${WHAM_URL}")
    file(DOWNLOAD ${WHAM_URL} ${CMAKE_BINARY_DIR}/_deps/${archive} STATUS DL_STATUS SHOW_PROGRESS)
    file(MD5 ${CMAKE_BINARY_DIR}/_deps/${archive} DL_MD5)
    if((NOT DL_STATUS EQUAL 0) OR (NOT "${DL_MD5}" STREQUAL "${WHAM_MD5}"))
      message(ERROR "Download of WHAM sources from ${WHAM_URL} failed")
    endif()
  else()
    message(STATUS "Using already downloaded archive ${CMAKE_BINARY_DIR}/_deps/${archive}")
  endif()
  message(STATUS "Unpacking and configuring ${archive}")

  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_BINARY_DIR}/_deps/${archive}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/src)
  find_package(Patch)
  if(PATCH_FOUND)
    message(STATUS "Apply patch to customize WHAM using ${Patch_EXECUTABLE}")
    execute_process(
      COMMAND ${Patch_EXECUTABLE} -p1 -i ${CMAKE_CURRENT_SOURCE_DIR}/update-wham-2.1.0.patch
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/src/wham/
    )
  endif()
  file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/_deps/wham-src)
  file(RENAME "${CMAKE_BINARY_DIR}/_deps/src/wham" ${CMAKE_BINARY_DIR}/_deps/wham-src)
  file(COPY CMakeLists.wham DESTINATION ${CMAKE_BINARY_DIR}/_deps/wham-src/)
  file(RENAME "${CMAKE_BINARY_DIR}/_deps/wham-src/CMakeLists.wham"
    "${CMAKE_BINARY_DIR}/_deps/wham-src/CMakeLists.txt")
  add_subdirectory("${CMAKE_BINARY_DIR}/_deps/wham-src" "${CMAKE_BINARY_DIR}/_deps/wham-build")
  set(WHAM_EXE wham wham-2d)
endif()

set(LAMMPS_PLUGINLIB_DIR ${LAMMPS_DIR}/examples/COUPLE/plugin)
if(LAMMPS_GUI_USE_PLUGIN)
  if(APPLE)
    message(FATAL_ERROR "Dynamic loading of the LAMMPS library currently not supported on this platform")
  endif()
  enable_language(C)
  set(PLUGIN_LOADER_SRC ${LAMMPS_PLUGINLIB_DIR}/liblammpsplugin.c)
endif()

# include resource compiler to embed icons into the executable on Windows
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  enable_language(RC)
  set(ICON_RC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/lmpicons.rc)
endif()

# we require Qt 5 and at least version 5.12 at that.
if(NOT LAMMPS_GUI_USE_QT5)
  find_package(Qt6 6.2 QUIET COMPONENTS Widgets Charts)
endif()
if(NOT Qt6_FOUND)
  find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets Charts)
  set(QT_VERSION_MAJOR 5)
else()
  set(QT_VERSION_MAJOR 6)
endif()
message(STATUS "Using Qt version ${Qt${QT_VERSION_MAJOR}_VERSION} for LAMMPS-GUI")

set(PROJECT_SOURCES
  main.cpp
  lammpsgui.cpp
  lammpsgui.h
  lammpsgui.ui
  chartviewer.cpp
  chartviewer.h
  codeeditor.cpp
  codeeditor.h
  findandreplace.cpp
  findandreplace.h
  flagwarnings.cpp
  flagwarnings.h
  helpers.cpp
  highlighter.cpp
  highlighter.h
  imageviewer.cpp
  imageviewer.h
  lammpsrunner.h
  lammpswrapper.cpp
  lammpswrapper.h
  linenumberarea.h
  logwindow.cpp
  logwindow.h
  fileviewer.cpp
  fileviewer.h
  preferences.cpp
  preferences.h
  setvariables.cpp
  setvariables.h
  slideshow.h
  slideshow.cpp
  stdcapture.cpp
  ${PLUGIN_LOADER_SRC}
  ${ICON_RC_FILE}
)
if(QT_VERSION_MAJOR EQUAL 6)
  qt6_add_resources(PROJECT_SOURCES lammpsgui.qrc)
else()
  qt5_add_resources(PROJECT_SOURCES lammpsgui.qrc)
endif()

if(APPLE)
  set(MACOSX_ICON_FILE ${LAMMPS_DIR}/cmake/packaging/lammps.icns)
  set(MACOSX_README_FILE ${LAMMPS_DIR}/cmake/packaging/README.macos)
  set(MACOSX_BACKGROUND_FILE ${LAMMPS_DIR}/cmake/packaging/LAMMPS_DMG_Background.png)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
  qt_add_executable(lammps-gui
    MANUAL_FINALIZATION
    ${MACOSX_ICON_FILE}
    ${PROJECT_SOURCES}
  )
else()
  add_executable(lammps-gui
    ${MACOSX_ICON_FILE}
    ${PROJECT_SOURCES}
  )
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(lammps-gui)
endif()

# compilation settings
if(LAMMPS_GUI_USE_PLUGIN)
  target_compile_definitions(lammps-gui PRIVATE LAMMPS_GUI_USE_PLUGIN)
  target_include_directories(lammps-gui PRIVATE ${LAMMPS_PLUGINLIB_DIR})
else()
  target_include_directories(lammps-gui PRIVATE ${LAMMPS_HEADER_DIR})
  target_link_libraries(lammps-gui PRIVATE ${LAMMPS_LIBRARY})
endif()
target_include_directories(lammps-gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(lammps-gui PRIVATE LAMMPS_GUI_VERSION="${PROJECT_VERSION}")
target_link_libraries(lammps-gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Charts)
if(BUILD_OMP)
  find_package(OpenMP COMPONENTS CXX REQUIRED)
  target_link_libraries(lammps-gui PRIVATE OpenMP::OpenMP_CXX)
endif()

# build LAMMPS-GUI and LAMMPS as flatpak, if tools are installed
find_program(FLATPAK_COMMAND flatpak DOC "Path to flatpak command")
find_program(FLATPAK_BUILDER flatpak-builder DOC "Path to flatpak-builder command")
if(FLATPAK_COMMAND AND FLATPAK_BUILDER)
  file(STRINGS ${LAMMPS_DIR}/src/version.h line REGEX LAMMPS_VERSION)
  string(REGEX REPLACE "#define LAMMPS_VERSION \"([0-9]+) ([A-Za-z][A-Za-z][A-Za-z])[A-Za-z]* ([0-9]+)\""
                        "\\1\\2\\3" LAMMPS_RELEASE "${line}")
  set(FLATPAK_BUNDLE "LAMMPS-Linux-x86_64-GUI-${LAMMPS_RELEASE}.flatpak")
  add_custom_target(flatpak
    COMMAND ${FLATPAK_COMMAND} --user remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
    COMMAND ${FLATPAK_BUILDER} --force-clean --verbose --repo=${CMAKE_CURRENT_BINARY_DIR}/flatpak-repo
                               --install-deps-from=flathub --state-dir=${CMAKE_CURRENT_BINARY_DIR}
                               --user --ccache --default-branch=${LAMMPS_RELEASE}
                               flatpak-build ${LAMMPS_DIR}/tools/lammps-gui/org.lammps.lammps-gui.yml
    COMMAND ${FLATPAK_COMMAND} build-bundle --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo --verbose
                               ${CMAKE_CURRENT_BINARY_DIR}/flatpak-repo
                               ${FLATPAK_BUNDLE} org.lammps.lammps-gui ${LAMMPS_RELEASE}
    COMMENT "Create Flatpak bundle file of LAMMPS-GUI and LAMMPS"
    BYPRODUCT ${FLATPAK_BUNDLE}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
else()
  add_custom_target(flatpak
    COMMAND ${CMAKE_COMMAND} -E echo "The flatpak and flatpak-builder commands required to build a LAMMPS-GUI flatpak bundle were not found. Skipping.")
endif()

# when compiling on macOS, create an "app bundle"
if(APPLE)
  file(STRINGS ${LAMMPS_DIR}/src/version.h line REGEX LAMMPS_VERSION)
  string(REGEX REPLACE "#define LAMMPS_VERSION \"([0-9]+) ([A-Za-z][A-Za-z][A-Za-z])[A-Za-z]* ([0-9]+)\""
                        "\\1\\2\\3" LAMMPS_RELEASE "${line}")
  set_target_properties(lammps-gui PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${LAMMPS_DIR}/cmake/packaging/MacOSXBundleInfo.plist.in
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE_ICON_FILE lammps.icns
    MACOSX_BUNDLE_COPYRIGHT "(c) 2003 - 2025, The LAMMPS Developers"
    MACOSX_BUNDLE TRUE
  )
  # additional targets to populate the bundle tree and create the .dmg image file
  set(APP_CONTENTS ${CMAKE_BINARY_DIR}/lammps-gui.app/Contents)
  if(BUILD_TOOLS)
    add_custom_target(complete-bundle
      ${CMAKE_COMMAND} -E make_directory ${APP_CONTENTS}/bin
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/lmp ${APP_CONTENTS}/bin/
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/msi2lmp ${APP_CONTENTS}/bin/
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/binary2txt ${APP_CONTENTS}/bin/
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/stl_bin2txt ${APP_CONTENTS}/bin/
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/phana ${APP_CONTENTS}/bin/
      COMMAND ${CMAKE_COMMAND} -E create_symlink ../MacOS/lammps-gui ${APP_CONTENTS}/bin/lammps-gui
      COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_CONTENTS}/Resources
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LAMMPS_DIR}/cmake/packaging/README.macos ${APP_CONTENTS}/Resources/README.txt
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LAMMPS_DIR}/cmake/packaging/lammps.icns ${APP_CONTENTS}/Resources
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LAMMPS_DIR}/cmake/packaging/LAMMPS_DMG_Background.png ${APP_CONTENTS}/Resources
      COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_CONTENTS}/share/lammps
      COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_CONTENTS}/share/lammps/man/man1
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${LAMMPS_DIR}/potentials ${APP_CONTENTS}/share/lammps/potentials
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${LAMMPS_DIR}/bench ${APP_CONTENTS}/share/lammps/bench
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${LAMMPS_DIR}/tools/msi2lmp/frc_files ${APP_CONTENTS}/share/lammps/frc_files
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LAMMPS_DIR}/doc/lammps.1 ${APP_CONTENTS}/share/lammps/man/man1/
      COMMAND ${CMAKE_COMMAND} -E create_symlink lammps.1 ${APP_CONTENTS}/share/lammps/man/man1/lmp.1
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LAMMPS_DIR}/doc/msi2lmp.1 ${APP_CONTENTS}/share/lammps/man/man1
      DEPENDS lammps-gui lammps lmp binary2txt stl_bin2txt msi2lmp phana
      COMMENT "Copying additional files into macOS app bundle tree"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
  else()
    message(FATAL_ERROR "Must use -D BUILD_TOOLS=yes for building app bundle")
  endif()
  if(BUILD_WHAM)
    add_custom_target(copy-wham
      ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/wham ${APP_CONTENTS}/bin/
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/wham-2d ${APP_CONTENTS}/bin/
      DEPENDS complete-bundle wham wham-2d
      COMMENT "Copying WHAM executables into macOS app bundle tree"
    )
    set(WHAM_TARGET copy-wham)
  endif()
  if(FFMPEG_EXECUTABLE)
    add_custom_target(copy-ffmpeg
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFMPEG_EXECUTABLE} ${APP_CONTENTS}/bin/
      COMMENT "Copying FFMpeg into macOS app bundle tree"
      DEPENDS complete-bundle
    )
    set(FFMPEG_TARGET copy-ffmpeg)
  endif()
  add_custom_target(dmg
    COMMAND ${LAMMPS_DIR}/cmake/packaging/build_macos_dmg.sh ${LAMMPS_RELEASE}
    DEPENDS complete-bundle ${WHAM_TARGET} ${FFMPEG_TARGET}
    COMMENT "Create Drag-n-Drop installer disk image from app bundle"
    BYPRODUCT LAMMPS-macOS-multiarch-${LAMMPS_VERSION}.dmg
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  # settings or building on Windows with Visual Studio
elseif(MSVC)
  file(STRINGS ${LAMMPS_DIR}/src/version.h line REGEX LAMMPS_VERSION)
  string(REGEX REPLACE "#define LAMMPS_VERSION \"([0-9]+) ([A-Za-z][A-Za-z][A-Za-z])[A-Za-z]* ([0-9]+)\""
                        "\\1\\2\\3" LAMMPS_RELEASE "${line}")
  install(TARGETS lammps-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES $<TARGET_RUNTIME_DLLS:lammps-gui> TYPE BIN)
  if(BUILD_SHARED_LIBS)
    install(FILES $<TARGET_RUNTIME_DLLS:lammps> TYPE BIN)
  endif()
  install(FILES $<TARGET_RUNTIME_DLLS:lmp> TYPE BIN)
  # find path to VC++ init batch file
  get_filename_component(VC_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(VC_BASE_DIR "${VC_COMPILER_DIR}/../../../../../.." ABSOLUTE)
  set(VC_INIT "${VC_BASE_DIR}/Auxiliary/Build/vcvarsall.bat")
  get_filename_component(QT5_BIN_DIR "${Qt5Core_DIR}/../../../bin" ABSOLUTE)
  get_filename_component(INSTNAME ${CMAKE_INSTALL_PREFIX} NAME)
  install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -D INSTNAME=${INSTNAME} -D VC_INIT=\"${VC_INIT}\" -D QT5_BIN_DIR=\"${QT5_BIN_DIR}\" -P \"${CMAKE_SOURCE_DIR}/packaging/build_windows_vs.cmake\" WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/..\" COMMAND_ECHO STDOUT)")
elseif((CMAKE_SYSTEM_NAME STREQUAL "Windows") AND CMAKE_CROSSCOMPILING)
  file(STRINGS ${LAMMPS_DIR}/src/version.h line REGEX LAMMPS_VERSION)
  string(REGEX REPLACE "#define LAMMPS_VERSION \"([0-9]+) ([A-Za-z][A-Za-z][A-Za-z])[A-Za-z]* ([0-9]+)\""
                        "\\1\\2\\3" LAMMPS_RELEASE "${line}")
  install(TARGETS lammps-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES $<TARGET_RUNTIME_DLLS:lammps-gui> TYPE BIN)
  if(BUILD_SHARED_LIBS)
    install(FILES $<TARGET_RUNTIME_DLLS:lammps> TYPE BIN)
  endif()
  install(FILES $<TARGET_RUNTIME_DLLS:lmp> TYPE BIN)
  add_custom_target(zip
    COMMAND sh -vx ${LAMMPS_DIR}/cmake/packaging/build_windows_cross_zip.sh ${CMAKE_INSTALL_PREFIX} ${LAMMPS_RELEASE}
    DEPENDS lmp lammps-gui ${WHAM_EXE}
    COMMENT "Create zip file with windows binaries"
    BYPRODUCT LAMMPS-Win10-amd64-${LAMMPS_VERSION}.zip
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
elseif((CMAKE_SYSTEM_NAME STREQUAL "Linux") AND NOT LAMMPS_GUI_USE_PLUGIN)
  file(STRINGS ${LAMMPS_DIR}/src/version.h line REGEX LAMMPS_VERSION)
  string(REGEX REPLACE "#define LAMMPS_VERSION \"([0-9]+) ([A-Za-z][A-Za-z][A-Za-z])[A-Za-z]* ([0-9]+)\""
                        "\\1\\2\\3" LAMMPS_RELEASE "${line}")
  install(TARGETS lammps-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lammps-gui.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lammps-gui.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/appdata/)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lammps-input.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages/)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lammps-input.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/text/x-application-lammps.xml)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons/hicolor DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/)
  install(CODE [[
    file(GET_RUNTIME_DEPENDENCIES
      LIBRARIES $<TARGET_FILE:lammps>
      EXECUTABLES $<TARGET_FILE:lammps-gui> $<TARGET_FILE:lmp>
      RESOLVED_DEPENDENCIES_VAR _r_deps
      UNRESOLVED_DEPENDENCIES_VAR _u_deps
    )
    foreach(_file ${_r_deps})
      file(INSTALL
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
        TYPE SHARED_LIBRARY
        FOLLOW_SYMLINK_CHAIN
        FILES "${_file}"
      )
    endforeach()
    list(LENGTH _u_deps _u_length)
    if("${_u_length}" GREATER 0)
      message(WARNING "Unresolved dependencies detected: ${_u_deps}")
    endif() ]]
  )

  add_custom_target(tgz
    COMMAND ${LAMMPS_DIR}/cmake/packaging/build_linux_tgz.sh ${LAMMPS_RELEASE}
    DEPENDS lammps-gui ${WHAM_EXE}
    COMMENT "Create compressed tar file of LAMMPS-GUI with dependent libraries and wrapper"
    BYPRODUCT LAMMPS-Linux-amd64-${LAMMPS_RELEASE}.tar.gz
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endif()
