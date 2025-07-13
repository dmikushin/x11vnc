# FindX11Extensions.cmake - Enhanced X11 extension finding for x11vnc

include(FindPkgConfig)
include(CheckLibraryExists)
include(CheckIncludeFile)

# Find standard X11 first
find_package(X11 REQUIRED)

if(X11_FOUND)
    # Set up common X11 variables
    set(X11_EXTENSION_LIBRARIES)
    set(X11_EXTENSION_INCLUDE_DIRS ${X11_INCLUDE_DIR})

    # Helper macro to find X11 extensions
    macro(find_x11_extension name header library function)
        string(TOUPPER ${name} NAME_UPPER)
        
        # Check for header
        check_include_file("X11/extensions/${header}" HAVE_${NAME_UPPER}_H)
        
        if(HAVE_${NAME_UPPER}_H)
            # Check for library
            check_library_exists(${library} ${function} "${X11_LIB_SEARCH_PATH}" HAVE_LIB${NAME_UPPER})
            
            if(HAVE_LIB${NAME_UPPER})
                find_library(${NAME_UPPER}_LIBRARY 
                    NAMES ${library}
                    PATHS ${X11_LIB_SEARCH_PATH}
                    NO_DEFAULT_PATH
                )
                
                if(${NAME_UPPER}_LIBRARY)
                    set(X11_${name}_FOUND TRUE)
                    set(X11_${name}_LIB ${${NAME_UPPER}_LIBRARY})
                    list(APPEND X11_EXTENSION_LIBRARIES ${${NAME_UPPER}_LIBRARY})
                endif()
            endif()
        endif()
    endmacro()

    # Find X11 extensions used by x11vnc
    find_x11_extension(XTest XTest.h Xtst XTestFakeKeyEvent)
    find_x11_extension(Xinerama Xinerama.h Xinerama XineramaQueryScreens)
    find_x11_extension(Xrandr Xrandr.h Xrandr XRRSelectInput)
    find_x11_extension(Xfixes Xfixes.h Xfixes XFixesGetCursorImage)
    find_x11_extension(Xdamage Xdamage.h Xdamage XDamageQueryExtension)
    find_x11_extension(Xcomposite Xcomposite.h Xcomposite XCompositeQueryExtension)
    find_x11_extension(Xcursor X11/Xcursor/Xcursor.h Xcursor XcursorImageLoadCursor)

    # Special handling for XKEYBOARD (built into libX11)
    check_include_file("X11/XKBlib.h" HAVE_XKEYBOARD_H)
    if(HAVE_XKEYBOARD_H)
        check_library_exists(X11 XkbSelectEvents "${X11_LIB_SEARCH_PATH}" HAVE_XKEYBOARD)
        if(HAVE_XKEYBOARD)
            set(X11_XKeyboard_FOUND TRUE)
        endif()
    endif()

    # XTRAP support
    find_x11_extension(XTrap XTrap.h XTrap XETrapSetGrabServer)
    if(NOT X11_XTrap_FOUND)
        # Try alternative library name for Tru64
        find_x11_extension(XETrap XTrap.h XETrap XETrapSetGrabServer)
        if(X11_XETrap_FOUND)
            set(X11_XTrap_FOUND TRUE)
            set(X11_XTrap_LIB ${XETRAP_LIBRARY})
        endif()
    endif()

    # RECORD extension (part of Xtst)
    if(X11_XTest_FOUND)
        check_library_exists(Xtst XRecordEnableContextAsync "${X11_LIB_SEARCH_PATH}" HAVE_RECORD)
        if(HAVE_RECORD)
            set(X11_XRecord_FOUND TRUE)
            set(X11_XRecord_LIB ${X11_XTest_LIB})
        endif()
    endif()

    # DPMS support (usually in Xext)
    check_library_exists(Xext DPMSForceLevel "${X11_LIB_SEARCH_PATH}" HAVE_DPMS)
    if(HAVE_DPMS)
        set(X11_DPMS_FOUND TRUE)
        set(X11_DPMS_LIB ${X11_Xext_LIB})
    endif()

    # FBPM support (usually in Xext)
    check_library_exists(Xext FBPMForceLevel "${X11_LIB_SEARCH_PATH}" HAVE_FBPM)
    if(HAVE_FBPM)
        set(X11_FBPM_FOUND TRUE)
        set(X11_FBPM_LIB ${X11_Xext_LIB})
    endif()

    # MIT-SHM (usually in Xext)
    check_library_exists(Xext XShmGetImage "${X11_LIB_SEARCH_PATH}" HAVE_XSHM)
    if(HAVE_XSHM)
        set(X11_Xshm_FOUND TRUE)
        set(X11_Xshm_LIB ${X11_Xext_LIB})
    endif()

    # Platform-specific extensions
    if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
        # Solaris XReadScreen
        check_library_exists(Xext XReadScreen "${X11_LIB_SEARCH_PATH}" HAVE_SOLARIS_XREADSCREEN)
        if(HAVE_SOLARIS_XREADSCREEN)
            set(X11_SolarisXReadScreen_FOUND TRUE)
        endif()
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "IRIX")
        # IRIX XReadDisplay
        check_include_file("X11/extensions/readdisplay.h" HAVE_IRIX_XREADDISPLAY)
        if(HAVE_IRIX_XREADDISPLAY)
            set(X11_IrixXReadDisplay_FOUND TRUE)
        endif()
    endif()

    # Summary
    message(STATUS "X11 Extensions found:")
    foreach(ext XTest Xinerama Xrandr Xfixes Xdamage Xcomposite Xcursor XKeyboard XTrap XRecord DPMS FBPM Xshm)
        if(X11_${ext}_FOUND)
            message(STATUS "  ${ext}: YES")
        else()
            message(STATUS "  ${ext}: NO")
        endif()
    endforeach()

endif()

# Set the standard CMake variables
set(X11Extensions_FOUND ${X11_FOUND})
set(X11Extensions_LIBRARIES ${X11_LIBRARIES} ${X11_EXTENSION_LIBRARIES})
set(X11Extensions_INCLUDE_DIRS ${X11_EXTENSION_INCLUDE_DIRS})