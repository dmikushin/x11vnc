# misc/turbovnc/CMakeLists.txt for TurboVNC support files

# TurboVNC scripts and utilities
set(TURBOVNC_SCRIPTS
    apply_turbovnc
    convert
    convert_rfbserver
    undo_turbovnc
)

# TurboVNC source files
set(TURBOVNC_SOURCES
    tight.c
    turbojpeg.h
)

# Documentation
set(TURBOVNC_DOCS
    README
)

# Install TurboVNC scripts
install(PROGRAMS ${TURBOVNC_SCRIPTS}
    DESTINATION bin
    COMPONENT TurboVNC
)

# Install TurboVNC source files for reference/development
install(FILES ${TURBOVNC_SOURCES}
    DESTINATION share/doc/x11vnc/turbovnc
    COMPONENT Development
)

# Install TurboVNC documentation
install(FILES ${TURBOVNC_DOCS}
    DESTINATION share/doc/x11vnc/turbovnc
    COMPONENT Documentation
)

# Optional: Build tight.c as a standalone library for testing
option(BUILD_TURBOVNC_TIGHT "Build TurboVNC tight encoding library for testing" OFF)

if(BUILD_TURBOVNC_TIGHT)
    # Note: This is primarily for development/testing purposes
    # The actual tight.c is integrated into the main x11vnc build
    
    add_library(turbovnc_tight STATIC tight.c)
    
    # Include TurboJPEG header directory
    target_include_directories(turbovnc_tight PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # This library would need TurboJPEG to be useful
    find_library(TURBOJPEG_LIBRARY turbojpeg)
    if(TURBOJPEG_LIBRARY)
        target_link_libraries(turbovnc_tight PRIVATE ${TURBOJPEG_LIBRARY})
        target_compile_definitions(turbovnc_tight PRIVATE HAVE_TURBOJPEG=1)
    endif()
    
    # Install the library if built
    install(TARGETS turbovnc_tight
        ARCHIVE DESTINATION lib
        COMPONENT Development
    )
endif()

# Add a custom target to make scripts executable
add_custom_target(turbovnc_scripts ALL
    COMMENT "Processing TurboVNC scripts"
)

# Ensure scripts are executable in the build directory
foreach(script ${TURBOVNC_SCRIPTS})
    add_custom_command(
        TARGET turbovnc_scripts POST_BUILD
        COMMAND chmod +x "${CMAKE_CURRENT_SOURCE_DIR}/${script}"
        COMMENT "Making ${script} executable"
    )
endforeach()

# CPack component descriptions
set(CPACK_COMPONENT_TURBOVNC_DISPLAY_NAME "TurboVNC Support")
set(CPACK_COMPONENT_TURBOVNC_DESCRIPTION "Scripts and utilities for TurboVNC integration")