# misc/CMakeLists.txt for x11vnc utilities and misc files

# Add turbovnc subdirectory
add_subdirectory(turbovnc)

# Scripts and utilities to install
set(MISC_SCRIPTS
    connect_switch
    deskshot
    desktop.cgi
    dtVncPopup
    inet6to4
    panner.pl
    qt_tslib_inject.pl
    ranfb.pl
    rx11vnc
    rx11vnc.pl
    shm_clear
    slide.pl
    uinput.pl
    ultravnc_repeater.pl
    vcinject.pl
    x11vnc_loop
    x11vnc_pw
)

# C source utilities (not compiled by default, but distributed)
set(MISC_C_SOURCES
    blockdpy.c
)

# X11-specific files (only if X11 is available)
if(HAVE_X11)
    # Generate Xdummy script from template
    set(XDUMMY_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/Xdummy")
    
    # Read Xdummy.in template
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Xdummy.in" XDUMMY_IN_CONTENT)
    
    # Read Xdummy.c content
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Xdummy.c" XDUMMY_C_CONTENT)
    
    # Replace the placeholder with actual C code
    string(REPLACE "\"XDUMMY_C\"" "${XDUMMY_C_CONTENT}" XDUMMY_CONTENT "${XDUMMY_IN_CONTENT}")
    
    # Remove the substitution marker line
    string(REGEX REPLACE ".*Will be substituted by Xdummy\\.c.*\n" "" XDUMMY_CONTENT "${XDUMMY_CONTENT}")
    
    # Write the generated script
    file(GENERATE OUTPUT "${XDUMMY_SCRIPT}" CONTENT "${XDUMMY_CONTENT}")
    
    # Install Xdummy script
    install(PROGRAMS "${XDUMMY_SCRIPT}"
        DESTINATION bin
    )
    
    # Add X11-specific files to distribution
    list(APPEND MISC_C_SOURCES
        Xdummy.c
        Xdummy.in
    )
endif()

# Install misc scripts
install(PROGRAMS ${MISC_SCRIPTS}
    DESTINATION bin
)

# Create a custom target for misc scripts (for dependencies)
add_custom_target(misc_scripts ALL)

# Install documentation files
install(FILES README
    DESTINATION share/doc/x11vnc/misc
    OPTIONAL
)

# Install LICENSE file if it exists
install(FILES LICENSE
    DESTINATION share/doc/x11vnc
    OPTIONAL
)

# Set up CPack to include misc files in source distribution
set(CPACK_SOURCE_IGNORE_FILES
    ${CPACK_SOURCE_IGNORE_FILES}
    "/misc/enhanced_tightvnc_viewer/"
)

# Add all files to the source distribution
foreach(script ${MISC_SCRIPTS})
    install(FILES ${script}
        DESTINATION share/doc/x11vnc/examples
        COMPONENT Development
    )
endforeach()

foreach(source ${MISC_C_SOURCES})
    install(FILES ${source}
        DESTINATION share/doc/x11vnc/examples
        COMPONENT Development
    )
endforeach()

# Optional: Build blockdpy utility (if requested)
option(BUILD_BLOCKDPY "Build blockdpy utility" OFF)

if(BUILD_BLOCKDPY AND HAVE_X11)
    add_executable(blockdpy blockdpy.c)
    target_include_directories(blockdpy PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(blockdpy PRIVATE ${X11_LIBRARIES})
    
    install(TARGETS blockdpy
        RUNTIME DESTINATION bin
        COMPONENT Utilities
    )
endif()

# Add enhanced TightVNC viewer files to source distribution
# (Note: these are quite large and contain pre-built binaries)
set(ENHANCED_TIGHTVNC_FILES
    enhanced_tightvnc_viewer/COPYING
    enhanced_tightvnc_viewer/README
    enhanced_tightvnc_viewer/build.unix
    enhanced_tightvnc_viewer/filelist.txt
    enhanced_tightvnc_viewer/ssvnc.desktop
)

# Install enhanced TightVNC viewer documentation
foreach(file ${ENHANCED_TIGHTVNC_FILES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
        get_filename_component(dir ${file} DIRECTORY)
        install(FILES ${file}
            DESTINATION share/doc/x11vnc/${dir}
            COMPONENT Documentation
            OPTIONAL
        )
    endif()
endforeach()

# Man pages from enhanced viewer (if they exist)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/enhanced_tightvnc_viewer/man/man1")
    install(DIRECTORY enhanced_tightvnc_viewer/man/man1/
        DESTINATION share/man/man1
        COMPONENT Documentation
        FILES_MATCHING PATTERN "*.1"
    )
endif()