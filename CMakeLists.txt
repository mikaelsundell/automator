# Copyright 2022-present Contributors to the colorpicker project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/automator

cmake_minimum_required( VERSION 3.27 )
set( project_name "Automator" )

project( ${project_name} )

# packages
set (qt6_modules Core Gui Widgets)
find_package(Qt6 COMPONENTS ${qt6_modules} CONFIG REQUIRED)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)

# sources
set (app_sources
    automator.h
    automator.cpp
    eventfilter.h
    eventfilter.cpp
    mac.h
    mac.mm
    main.cpp
    about.ui
    automator.ui
    automator.qrc
)

# resources
file( GLOB app_resources
    "resources/*.icns" 
    "resources/*.icc"
    "resources/*.css"
    "resources/App*.png"
)

# presets
file( GLOB app_presets
    "presets/*.json" 
)

# bundle
set (bundle_sources
    "resources/MacOSXBundle.plist.in"
)

if (APPLE)
    set (MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.mikaelsundell.automator")
    set (MACOSX_BUNDLE_EXECUTABLE_NAME ${project_name})
    set (MACOSX_BUNDLE_INFO_STRING ${project_name})
    set (MACOSX_BUNDLE_BUNDLE_NAME ${project_name})
    set (MACOSX_BUNDLE_ICON_FILE AppIcon.icns)
    set (MACOSX_BUNDLE_LONG_VERSION_STRING "1.0.0")
    set (MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")
    set (MACOSX_BUNDLE_BUNDLE_VERSION "1.0")
    set (MACOSX_BUNDLE_COPYRIGHT "Copyright 2022-present Contributors to the ${project_name} project")
    set (MACOSX_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET})
    set (CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "-o linker-signed")
    set_source_files_properties(${app_resources} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set_source_files_properties(${app_presets} PROPERTIES MACOSX_PACKAGE_LOCATION "Presets")
    add_executable (${project_name} MACOSX_BUNDLE ${app_sources} ${app_resources} ${app_presets})
    # definitions
    add_definitions(-DMACOSX_BUNDLE_COPYRIGHT="${MACOSX_BUNDLE_COPYRIGHT}")
    add_definitions(-DMACOSX_BUNDLE_LONG_VERSION_STRING="${MACOSX_BUNDLE_LONG_VERSION_STRING}")
    add_definitions(-DGITHUBURL="https://github.com/mikaelsundell/automator")
    # code signing
    if (SIGN_CODE)
        if (CODE_SIGN_IDENTITY STREQUAL "" OR DEVELOPMENT_TEAM_ID STREQUAL "")
            message (FATAL_ERROR "Code signing is enabled, but CODE_SIGN_IDENTITY or DEVELOPMENT_TEAM_ID is not set.")
        else ()
            message ("Code signing enabled for Xcode")
            message ("  Sign identify: ${CODE_SIGN_IDENTITY}")
            message ("  Development team: ${DEVELOPMENT_TEAM_ID}")
        endif()
        set_target_properties(${project_name} PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${CODE_SIGN_IDENTITY}"
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DEVELOPMENT_TEAM_ID}"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${MACOSX_BUNDLE_GUI_IDENTIFIER}"
        )
    endif()
    set_target_properties(${project_name} PROPERTIES
        OUTPUT_NAME ${project_name}
        MACOSX_FRAMEWORK_INFO_PLIST ${bundle_sources})
    target_compile_options (${project_name} PRIVATE -Wno-deprecated-register)
    target_include_directories (${project_name} PRIVATE ${LCMS2_INCLUDE_DIR})
    target_link_libraries (${project_name} 
        Qt6::Core Qt6::Gui Qt6::Widgets
        "-framework CoreFoundation"
        "-framework AppKit")
else ()
    message (WARNING "${project_name} is a Mac program, will not be built.")
endif ()
