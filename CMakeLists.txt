cmake_minimum_required(VERSION 3.12)

project(x11vnc 
    VERSION 0.9.17
    DESCRIPTION "a VNC server for real X displays"
    HOMEPAGE_URL "https://github.com/LibVNC/x11vnc"
    LANGUAGES C)

# Build options
option(BUILD_X11VNC "Build x11vnc" ON)
option(WITH_SSL "Enable SSL/TLS support" ON)
option(WITH_CRYPTO "Enable crypto support" ON)
option(WITH_CRYPT "Enable crypt support" ON)
option(WITH_DRM "Enable DRM support" ON)
option(WITH_XKEYBOARD "Enable XKEYBOARD extension support" ON)
option(WITH_XINERAMA "Enable XINERAMA extension support" ON)
option(WITH_XRANDR "Enable XRANDR extension support" ON)
option(WITH_XFIXES "Enable XFIXES extension support" ON)
option(WITH_XDAMAGE "Enable XDAMAGE extension support" ON)
option(WITH_XCOMPOSITE "Enable XCOMPOSITE extension support" ON)
option(WITH_XTRAP "Enable XTRAP extension support" ON)
option(WITH_XRECORD "Enable XRECORD extension support" ON)
option(WITH_FBPM "Enable FBPM extension support" ON)
option(WITH_DPMS "Enable DPMS extension support" ON)
option(WITH_V4L "Enable video4linux support" ON)
option(WITH_FBDEV "Enable linux fb device support" ON)
option(WITH_UINPUT "Enable linux uinput device support" ON)
option(WITH_MACOSX_NATIVE "Enable MacOS X native display support" ON)
option(WITH_COLORMULTIPOINTER "Enable color support for multiple pointers" ON)
option(WITH_AVAHI "Enable Avahi/mDNS support" ON)
option(WITH_CAIRO "Enable cairo graphics library support" ON)

# CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Standard settings
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform detection
if(APPLE)
    set(OSX TRUE)
endif()

if(WIN32 AND MINGW)
    set(MINGW TRUE)
endif()

if(EXISTS "/usr/include/linux")
    set(LINUX TRUE)
endif()

if(EXISTS "/usr/include/android/api-level.h")
    set(ANDROID TRUE)
    add_definitions(-DHAVE_ANDROID=1)
endif()

# Configuration header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    @ONLY
)

# Include directories
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# Required dependencies
find_package(PkgConfig REQUIRED)

# Find libvncserver/libvncclient
pkg_check_modules(LIBVNCSERVER REQUIRED libvncserver>=0.9.8)
pkg_check_modules(LIBVNCCLIENT REQUIRED libvncclient>=0.9.8)

# X11 dependencies (if not disabled)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR WITH_X11)
    find_package(X11 REQUIRED)
    if(X11_FOUND)
        set(HAVE_X11 TRUE)
        
        # Check for X11 extensions
        if(WITH_XTEST AND X11_XTest_FOUND)
            set(HAVE_XTEST TRUE)
        endif()
        
        if(WITH_XINERAMA AND X11_Xinerama_FOUND)
            set(HAVE_LIBXINERAMA TRUE)
        endif()
        
        if(WITH_XRANDR AND X11_Xrandr_FOUND)
            set(HAVE_LIBXRANDR TRUE)
        endif()
        
        if(WITH_XFIXES AND X11_Xfixes_FOUND)
            set(HAVE_LIBXFIXES TRUE)
        endif()
        
        if(WITH_XDAMAGE AND X11_Xdamage_FOUND)
            set(HAVE_LIBXDAMAGE TRUE)
        endif()
    endif()
endif()

# SSL/TLS support
if(WITH_SSL)
    find_package(OpenSSL 1.0.0)
    if(OpenSSL_FOUND)
        set(HAVE_LIBSSL TRUE)
        if(WITH_CRYPTO)
            set(HAVE_LIBCRYPTO TRUE)
        endif()
    endif()
endif()

# Crypt support
if(WITH_CRYPT)
    find_library(CRYPT_LIBRARY crypt)
    if(CRYPT_LIBRARY)
        set(HAVE_LIBCRYPT TRUE)
    endif()
endif()

# DRM support
if(WITH_DRM)
    find_library(DRM_LIBRARY drm)
    if(DRM_LIBRARY)
        set(HAVE_LIBDRM TRUE)
    endif()
endif()

# Avahi support
if(WITH_AVAHI)
    pkg_check_modules(AVAHI avahi-client>=0.6.4)
    if(AVAHI_FOUND)
        set(HAVE_AVAHI TRUE)
    endif()
endif()

# Cairo support (for XI2 color cursor support)
if(WITH_CAIRO)
    pkg_check_modules(CAIRO cairo)
    if(CAIRO_FOUND)
        set(HAVE_CAIRO TRUE)
    endif()
endif()

# XI2 support
pkg_check_modules(XI2 xi>=1.2.99 inputproto>=1.9.99.9)
if(XI2_FOUND)
    set(HAVE_XI2 TRUE)
    
    # Xcursor support for color multipointer
    if(WITH_COLORMULTIPOINTER)
        find_library(XCURSOR_LIBRARY Xcursor)
        if(XCURSOR_LIBRARY)
            set(HAVE_LIBXCURSOR TRUE)
        endif()
    endif()
endif()

# Platform-specific settings
if(OSX)
    # MacOS X native display support
    if(WITH_MACOSX_NATIVE)
        set(HAVE_MACOSX_NATIVE_DISPLAY TRUE)
    endif()
    
    # Check for OpenGL header
    find_path(OPENGL_INCLUDE_DIR OpenGL/OpenGL.h)
    if(OPENGL_INCLUDE_DIR)
        set(HAVE_MACOSX_OPENGL_H TRUE)
    endif()
endif()

if(MINGW)
    # Windows socket library
    set(WSOCKLIB ws2_32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined")
endif()

# HP-UX specific
if(CMAKE_SYSTEM_NAME STREQUAL "HP-UX")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lsec")
endif()

# Header checks
include(CheckIncludeFile)

# Standard headers
check_include_file("arpa/inet.h" HAVE_ARPA_INET_H)
check_include_file("endian.h" HAVE_ENDIAN_H)
check_include_file("fcntl.h" HAVE_FCNTL_H)
check_include_file("netdb.h" HAVE_NETDB_H)
check_include_file("netinet/in.h" HAVE_NETINET_IN_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("sys/endian.h" HAVE_SYS_ENDIAN_H)
check_include_file("sys/socket.h" HAVE_SYS_SOCKET_H)
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
check_include_file("sys/timeb.h" HAVE_SYS_TIMEB_H)
check_include_file("syslog.h" HAVE_SYSLOG_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("ws2tcpip.h" HAVE_WS2TCPIP_H)
check_include_file("pwd.h" HAVE_PWD_H)
check_include_file("sys/wait.h" HAVE_SYS_WAIT_H)
check_include_file("utmpx.h" HAVE_UTMPX_H)
check_include_file("termios.h" HAVE_TERMIOS_H)
check_include_file("sys/ioctl.h" HAVE_SYS_IOCTL_H)
check_include_file("sys/stropts.h" HAVE_SYS_STROPTS_H)

# Linux-specific headers
if(WITH_V4L)
    check_include_file("libv4l1-videodev.h" HAVE_LIBV4L1_VIDEODEV_H)
    check_include_file("linux/videodev.h" HAVE_LINUX_VIDEODEV_H)
    check_include_file("linux/videodev2.h" HAVE_LINUX_VIDEODEV2_H)
endif()

if(WITH_FBDEV)
    check_include_file("linux/fb.h" HAVE_LINUX_FB_H)
endif()

if(WITH_UINPUT)
    check_include_file("linux/input.h" HAVE_LINUX_INPUT_H)
    if(HAVE_LINUX_INPUT_H)
        check_include_file("linux/uinput.h" HAVE_LINUX_UINPUT_H)
    endif()
endif()

# Function checks
include(CheckFunctionExists)
check_function_exists(crypt HAVE_LIBC_CRYPT)

# Type checks
include(CheckTypeSize)
check_type_size("socklen_t" SOCKLEN_T)
if(NOT HAVE_SOCKLEN_T)
    set(NEED_SOCKLEN_T TRUE)
endif()

# Solaris workaround for write() returning ENOENT
if(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_SYSTEM_VERSION MATCHES "5.7")
    set(ENOENT_WORKAROUND TRUE)
endif()

# Configure subdirectories
if(BUILD_X11VNC)
    add_subdirectory(src)
endif()

add_subdirectory(misc)

# Install files
install(FILES x11vnc.1 DESTINATION share/man/man1)
install(FILES x11vnc.desktop DESTINATION share/applications)

# CPack configuration
set(CPACK_PACKAGE_NAME "x11vnc")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_CONTACT "https://github.com/LibVNC/x11vnc")

include(CPack)