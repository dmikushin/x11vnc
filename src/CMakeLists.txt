# src/CMakeLists.txt for x11vnc library and executable

# Source files for x11vnc library
set(X11VNC_SOURCES
    8to24.c
    appshare.c
    avahi.c
    cleanup.c
    connections.c
    cursor.c
    gui.c
    help.c
    inet.c
    keyboard.c
    linuxfb.c
    macosx.c
    macosxCG.c
    macosxCGP.c
    macosxCGS.c
    macosx_opengl.c
    options.c
    pm.c
    pointer.c
    rates.c
    remote.c
    scan.c
    screen.c
    selection.c
    solid.c
    sslcmds.c
    sslhelper.c
    uinput.c
    unixpw.c
    user.c
    userinput.c
    util.c
    v4l.c
    win_utils.c
    x11vnc.c
    x11vnc_defs.c
    xdamage.c
    xevents.c
    xi2_devices.c
    xinerama.c
    xkb_bell.c
    xrandr.c
    xrecord.c
    xwrappers.c
    libx11vnc.c
)

# Header files
set(X11VNC_HEADERS
    8to24.h
    allowed_input_t.h
    avahi.h
    blackout_t.h
    cleanup.h
    connections.h
    cursor.h
    default8x16.h
    enc.h
    gui.h
    help.h
    inet.h
    keyboard.h
    linuxfb.h
    macosx.h
    macosxCG.h
    macosxCGP.h
    macosxCGS.h
    macosx_opengl.h
    nox11.h
    nox11_funcs.h
    options.h
    params.h
    pm.h
    pointer.h
    rates.h
    remote.h
    scan.h
    screen.h
    scrollevent_t.h
    selection.h
    solid.h
    sslcmds.h
    sslhelper.h
    ssltools.h
    tkx11vnc.h
    uinput.h
    unixpw.h
    user.h
    userinput.h
    util.h
    v4l.h
    win_utils.h
    winattr_t.h
    x11vnc.h
    xdamage.h
    xevents.h
    xi2_devices.h
    xinerama.h
    xkb_bell.h
    xrandr.h
    xrecord.h
    xwrappers.h
)

# Create the x11vnc shared library
add_library(x11vnc_lib SHARED ${X11VNC_SOURCES} ${X11VNC_HEADERS})
set_target_properties(x11vnc_lib PROPERTIES OUTPUT_NAME "x11vnc")
set_target_properties(x11vnc_lib PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 0)

# Set public include directories for the library
target_include_directories(x11vnc_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Create the x11vnc executable
add_executable(x11vnc x11vnc_main.c)

# Include directories for both library and executable
foreach(target x11vnc_lib x11vnc)
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${LIBVNCSERVER_INCLUDE_DIRS}
        ${LIBVNCCLIENT_INCLUDE_DIRS}
    )
    
    # Compiler flags
    target_compile_definitions(${target} PRIVATE
        ${LIBVNCSERVER_CFLAGS_OTHER}
        ${LIBVNCCLIENT_CFLAGS_OTHER}
    )
endforeach()

# Libraries to link
set(X11VNC_LIBRARIES
    ${LIBVNCSERVER_LIBRARIES}
    ${LIBVNCCLIENT_LIBRARIES}
)

# Platform-specific settings
if(OSX)
    # macOS frameworks for both targets
    foreach(target x11vnc_lib x11vnc)
        target_link_libraries(${target} PRIVATE
            "-framework ApplicationServices"
            "-framework Carbon"
            "-framework IOKit"
            "-framework Cocoa"
        )
        
        if(HAVE_MACOSX_OPENGL_H)
            target_link_libraries(${target} PRIVATE "-framework OpenGL")
            
            if(HAVE_X11)
                # Special OpenGL library handling for X11 on macOS
                set(GL_LIB "/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib")
                target_link_libraries(${target} PRIVATE
                    "-dylib_file ${GL_LIB}:${GL_LIB}"
                )
            endif()
        endif()
        
        # Objective-C compilation for macOS sources
        target_compile_options(${target} PRIVATE -ObjC)
    endforeach()
endif()

# Windows/MinGW specific
if(MINGW)
    list(APPEND X11VNC_LIBRARIES ws2_32)
endif()

# CygIPC support
if(CYGIPC)
    list(APPEND X11VNC_LIBRARIES cygipc)
endif()

# X11 libraries
if(HAVE_X11)
    foreach(target x11vnc_lib x11vnc)
        target_include_directories(${target} PRIVATE ${X11_INCLUDE_DIR})
    endforeach()
    list(APPEND X11VNC_LIBRARIES ${X11_LIBRARIES})
    
    # Add X11 extension libraries if available
    if(HAVE_LIBXINERAMA)
        list(APPEND X11VNC_LIBRARIES ${X11_Xinerama_LIB})
    endif()
    
    if(HAVE_LIBXRANDR)
        list(APPEND X11VNC_LIBRARIES ${X11_Xrandr_LIB})
    endif()
    
    if(HAVE_LIBXFIXES)
        list(APPEND X11VNC_LIBRARIES ${X11_Xfixes_LIB})
    endif()
    
    if(HAVE_LIBXDAMAGE)
        list(APPEND X11VNC_LIBRARIES ${X11_Xdamage_LIB})
    endif()
    
    if(HAVE_LIBXCOMPOSITE)
        find_library(XCOMPOSITE_LIB Xcomposite)
        if(XCOMPOSITE_LIB)
            list(APPEND X11VNC_LIBRARIES ${XCOMPOSITE_LIB})
        endif()
    endif()
    
    if(HAVE_LIBXCURSOR)
        list(APPEND X11VNC_LIBRARIES ${XCURSOR_LIBRARY})
    endif()
endif()

# SSL libraries
if(HAVE_LIBSSL)
    foreach(target x11vnc_lib x11vnc)
        target_include_directories(${target} PRIVATE ${OPENSSL_INCLUDE_DIR})
    endforeach()
    list(APPEND X11VNC_LIBRARIES ${OPENSSL_LIBRARIES})
endif()

# Crypt library
if(HAVE_LIBCRYPT)
    list(APPEND X11VNC_LIBRARIES ${CRYPT_LIBRARY})
endif()

# DRM library
if(HAVE_LIBDRM)
    list(APPEND X11VNC_LIBRARIES ${DRM_LIBRARY})
    foreach(target x11vnc_lib x11vnc)
        target_include_directories(${target} PRIVATE /usr/include/drm)
    endforeach()
endif()

# Avahi libraries
if(HAVE_AVAHI)
    foreach(target x11vnc_lib x11vnc)
        target_include_directories(${target} PRIVATE ${AVAHI_INCLUDE_DIRS})
        target_compile_definitions(${target} PRIVATE ${AVAHI_CFLAGS_OTHER})
    endforeach()
    list(APPEND X11VNC_LIBRARIES ${AVAHI_LIBRARIES})
endif()

# XI2 libraries
if(HAVE_XI2)
    foreach(target x11vnc_lib x11vnc)
        target_include_directories(${target} PRIVATE ${XI2_INCLUDE_DIRS})
        target_compile_definitions(${target} PRIVATE ${XI2_CFLAGS_OTHER})
    endforeach()
    list(APPEND X11VNC_LIBRARIES ${XI2_LIBRARIES})
endif()

# Cairo libraries
if(HAVE_CAIRO)
    foreach(target x11vnc_lib x11vnc)
        target_include_directories(${target} PRIVATE ${CAIRO_INCLUDE_DIRS})
        target_compile_definitions(${target} PRIVATE ${CAIRO_CFLAGS_OTHER})
    endforeach()
    list(APPEND X11VNC_LIBRARIES ${CAIRO_LIBRARIES})
endif()

# Network libraries (for some platforms)
find_library(NSL_LIBRARY nsl)
if(NSL_LIBRARY)
    list(APPEND X11VNC_LIBRARIES ${NSL_LIBRARY})
endif()

find_library(SOCKET_LIBRARY socket)
if(SOCKET_LIBRARY)
    list(APPEND X11VNC_LIBRARIES ${SOCKET_LIBRARY})
endif()

# Link all libraries
target_link_libraries(x11vnc_lib PRIVATE ${X11VNC_LIBRARIES})
target_link_libraries(x11vnc PRIVATE x11vnc_lib)

# Install targets
install(TARGETS x11vnc_lib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(TARGETS x11vnc
    RUNTIME DESTINATION bin
)

# Install public header
install(FILES ${CMAKE_SOURCE_DIR}/include/libx11vnc.h
    DESTINATION include
)

# Special handling for large switch statements that compile slowly
if(CMAKE_COMPILER_IS_GNUCC)
    # Disable optimization for files with large switch statements to speed up compilation
    set_source_files_properties(
        remote.c
        x11vnc.c
        PROPERTIES COMPILE_FLAGS "-O0"
    )
endif()