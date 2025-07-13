/*
 * libx11vnc.h - Public API for x11vnc library
 *
 * Copyright (C) 2002-2010 Karl J. Runge <runge@karlrunge.com>
 * All rights reserved.
 */

#ifndef LIBX11VNC_H
#define LIBX11VNC_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Library version */
#define LIBX11VNC_VERSION_MAJOR 0
#define LIBX11VNC_VERSION_MINOR 9
#define LIBX11VNC_VERSION_PATCH 17

/* Opaque server handle */
typedef struct x11vnc_server x11vnc_server_t;

/* Error codes */
#define X11VNC_SUCCESS              0
#define X11VNC_ERROR_INVALID_ARG   -1
#define X11VNC_ERROR_NO_MEMORY     -2
#define X11VNC_ERROR_ALREADY_RUNNING -3
#define X11VNC_ERROR_NOT_RUNNING   -4
#define X11VNC_ERROR_DISPLAY_OPEN  -5
#define X11VNC_ERROR_AUTH_FAILED   -6
#define X11VNC_ERROR_INTERNAL      -99

/* Event types for Phase 2 */
typedef enum {
    X11VNC_EVENT_STARTED,           /* Server started successfully */
    X11VNC_EVENT_STOPPED,           /* Server stopped */
    X11VNC_EVENT_CLIENT_CONNECTED,  /* Client connected */
    X11VNC_EVENT_CLIENT_DISCONNECTED, /* Client disconnected */
    X11VNC_EVENT_ERROR              /* Error occurred */
} x11vnc_event_type_t;

/* Event callback function type */
typedef void (*x11vnc_event_callback_t)(
    x11vnc_server_t* server,
    x11vnc_event_type_t event_type,
    const char* message,
    void* user_data
);

/* Simple configuration structure for Phase 2 */
typedef struct {
    /* Display settings */
    const char* display;            /* X11 display (e.g., ":0") */
    const char* auth_file;          /* X authority file path */
    
    /* Network settings */
    int port;                       /* VNC port (0 for auto, default 5900) */
    bool localhost_only;            /* Only allow local connections */
    bool ipv6;                      /* Enable IPv6 support */
    
    /* Security settings */
    const char* password;           /* VNC password (NULL for none) */
    const char* password_file;      /* Path to password file */
    bool view_only;                 /* Read-only mode */
    const char* allow_hosts;        /* Comma-separated allowed IPs */
    
    /* Behavior settings */
    bool shared;                    /* Allow multiple clients */
    bool forever;                   /* Keep running after last client */
    bool once;                      /* Exit after first client disconnects */
    
    /* Performance settings */
    int poll_interval_ms;           /* Screen polling interval */
    bool use_shm;                   /* Use shared memory extension */
    bool use_xdamage;               /* Use XDAMAGE extension */
    bool wireframe;                 /* Wireframe mode for moving windows */
    
    /* Feature settings */
    bool show_cursor;               /* Show remote cursor */
    bool accept_bell;               /* Accept bell events */
    bool accept_clipboard;          /* Accept clipboard changes */
    const char* geometry;           /* Force screen geometry (WxH) */
    const char* clip;               /* Clip region (WxH+X+Y) */
    
} x11vnc_simple_config_t;

/**
 * Create a new x11vnc server instance
 * @return Server handle or NULL on error
 */
x11vnc_server_t* x11vnc_server_create(void);

/**
 * Start the VNC server with command line arguments
 * @param server Server handle
 * @param argc Number of arguments
 * @param argv Array of arguments (can be NULL for defaults)
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_start(x11vnc_server_t* server, int argc, char** argv);

/**
 * Run the VNC server main loop (blocking)
 * @param server Server handle
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_run(x11vnc_server_t* server);

/**
 * Stop the VNC server
 * @param server Server handle
 */
void x11vnc_server_stop(x11vnc_server_t* server);

/**
 * Destroy server instance and free resources
 * @param server Server handle
 */
void x11vnc_server_destroy(x11vnc_server_t* server);

/**
 * Get library version string
 * @return Version string
 */
const char* x11vnc_get_version(void);

/**
 * Get VNC server port
 * @param server Server handle
 * @return Port number or -1 if not running
 */
int x11vnc_server_get_port(x11vnc_server_t* server);

/**
 * Get number of connected clients
 * @param server Server handle
 * @return Number of clients or -1 if not running
 */
int x11vnc_server_get_client_count(x11vnc_server_t* server);

/**
 * Check if server is running
 * @param server Server handle
 * @return true if running, false otherwise
 */
bool x11vnc_server_is_running(x11vnc_server_t* server);

/* Phase 2 API Functions */

/**
 * Initialize configuration with defaults
 * @param config Configuration structure to initialize
 */
void x11vnc_config_init_defaults(x11vnc_simple_config_t* config);

/**
 * Configure server without command line arguments
 * @param server Server handle
 * @param config Configuration structure
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_configure(x11vnc_server_t* server, const x11vnc_simple_config_t* config);

/**
 * Start server using configuration (no command line args needed)
 * @param server Server handle (must be configured first)
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_start_configured(x11vnc_server_t* server);

/**
 * Set event callback for server events
 * @param server Server handle
 * @param callback Callback function (NULL to disable)
 * @param user_data User data passed to callback
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_set_event_callback(x11vnc_server_t* server, 
                                    x11vnc_event_callback_t callback, 
                                    void* user_data);

/**
 * Get current configuration
 * @param server Server handle
 * @param config Configuration structure to fill
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_get_config(x11vnc_server_t* server, x11vnc_simple_config_t* config);

/**
 * Update configuration at runtime (some changes may require restart)
 * @param server Server handle
 * @param config New configuration
 * @param restart_needed Set to true if restart is required for changes
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_update_config(x11vnc_server_t* server, 
                               const x11vnc_simple_config_t* config,
                               bool* restart_needed);

/* Legacy interface - for backwards compatibility */
int x11vnc_main_legacy(int argc, char* argv[]);

#ifdef __cplusplus
}
#endif

#endif /* LIBX11VNC_H */