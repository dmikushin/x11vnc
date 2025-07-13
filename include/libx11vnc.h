/*
 * libx11vnc.h - Public API for x11vnc library
 *
 * Copyright (C) 2002-2010 Karl J. Runge <runge@karlrunge.com>
 * All rights reserved.
 */

#ifndef LIBX11VNC_H
#define LIBX11VNC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

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

/* Event types for Phase 2 & 3 */
typedef enum {
    X11VNC_EVENT_STARTED,              /* Server started successfully */
    X11VNC_EVENT_STOPPED,              /* Server stopped */
    X11VNC_EVENT_CLIENT_CONNECTED,     /* Client connected */
    X11VNC_EVENT_CLIENT_DISCONNECTED,  /* Client disconnected */
    X11VNC_EVENT_ERROR,                /* Error occurred */
    /* Phase 3 events */
    X11VNC_EVENT_FRAME_SENT,           /* Frame update sent to client */
    X11VNC_EVENT_INPUT_RECEIVED,       /* Input event received from client */
    X11VNC_EVENT_CLIPBOARD_CHANGED,    /* Clipboard content changed */
    X11VNC_EVENT_SCREEN_CHANGED,       /* Screen resolution/layout changed */
    X11VNC_EVENT_CLIENT_AUTH,          /* Client authentication attempt */
    X11VNC_EVENT_PERFORMANCE_WARNING   /* Performance issue detected */
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

/* Phase 3 Advanced Structures */

/* Client information structure */
typedef struct {
    char client_id[64];          /* Unique client identifier */
    char hostname[256];          /* Client hostname/IP */
    int port;                    /* Client port */
    char username[64];           /* Authenticated username (if any) */
    bool authenticated;          /* Authentication status */
    bool view_only;              /* Client is view-only */
    uint64_t connected_time;     /* Connection timestamp */
    uint64_t bytes_sent;         /* Bytes sent to client */
    uint64_t bytes_received;     /* Bytes received from client */
    uint32_t frames_sent;        /* Video frames sent */
    double last_activity;        /* Last activity timestamp */
    char encoding[32];           /* Current encoding (Tight, Raw, etc.) */
} x11vnc_client_info_t;

/* Advanced server statistics */
typedef struct {
    /* Server uptime and state */
    uint64_t uptime_seconds;
    uint64_t total_connections;
    int current_clients;
    int max_clients_reached;
    
    /* Performance metrics */
    double fps_current;          /* Current frames per second */
    double fps_average;          /* Average FPS over time */
    uint64_t total_frames_sent;
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
    
    /* Screen information */
    int screen_width;
    int screen_height;
    int bits_per_pixel;
    double screen_update_rate;   /* Screen changes per second */
    
    /* Input statistics */
    uint64_t pointer_events;
    uint64_t key_events;
    uint64_t clipboard_events;
    
    /* Performance indicators */
    double cpu_usage_percent;    /* Estimated CPU usage */
    double memory_usage_mb;      /* Memory usage in MB */
    int dropped_frames;          /* Frames dropped due to load */
    
    /* Network statistics */
    double bandwidth_in_kbps;    /* Incoming bandwidth */
    double bandwidth_out_kbps;   /* Outgoing bandwidth */
    int compression_ratio;       /* Average compression ratio */
    
} x11vnc_advanced_stats_t;

/* Input event structures */
typedef struct {
    int x, y;                    /* Pointer coordinates */
    int button_mask;             /* Button state bitmask */
    double timestamp;            /* Event timestamp */
    char client_id[64];          /* Source client */
} x11vnc_pointer_event_t;

typedef struct {
    uint32_t keysym;             /* X11 keysym */
    bool down;                   /* True for press, false for release */
    double timestamp;            /* Event timestamp */
    char client_id[64];          /* Source client */
} x11vnc_key_event_t;

typedef struct {
    char* text;                  /* Clipboard text content */
    size_t length;               /* Text length */
    char* format;                /* MIME type/format */
    double timestamp;            /* Event timestamp */
    char client_id[64];          /* Source client */
} x11vnc_clipboard_event_t;

/* Screen change event */
typedef struct {
    int old_width, old_height;
    int new_width, new_height;
    int old_depth, new_depth;
    double timestamp;
} x11vnc_screen_event_t;

/* Performance warning event */
typedef struct {
    char warning_type[64];       /* Warning category */
    char description[256];       /* Detailed description */
    double severity;             /* Severity level (0.0-1.0) */
    double value;                /* Metric value that triggered warning */
    double threshold;            /* Warning threshold */
} x11vnc_performance_event_t;

/* Advanced event callback with typed data */
typedef void (*x11vnc_advanced_event_callback_t)(
    x11vnc_server_t* server,
    x11vnc_event_type_t event_type,
    void* event_data,            /* Typed event data based on event_type */
    void* user_data
);

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

/* Phase 3 Advanced API Functions */

/**
 * Set advanced event callback with typed event data
 * @param server Server handle
 * @param callback Advanced callback function (NULL to disable)
 * @param user_data User data passed to callback
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_set_advanced_event_callback(x11vnc_server_t* server,
                                             x11vnc_advanced_event_callback_t callback,
                                             void* user_data);

/**
 * Get advanced server statistics and metrics
 * @param server Server handle
 * @param stats Statistics structure to fill
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_get_advanced_stats(x11vnc_server_t* server, 
                                    x11vnc_advanced_stats_t* stats);

/**
 * Get list of connected clients
 * @param server Server handle
 * @param clients Array to fill with client info
 * @param max_clients Maximum number of clients to return
 * @param actual_count Actual number of clients returned
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_get_clients(x11vnc_server_t* server,
                             x11vnc_client_info_t* clients,
                             int max_clients,
                             int* actual_count);

/**
 * Disconnect a specific client
 * @param server Server handle
 * @param client_id Client identifier
 * @param reason Disconnect reason message
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_disconnect_client(x11vnc_server_t* server,
                                   const char* client_id,
                                   const char* reason);

/**
 * Set client permissions
 * @param server Server handle
 * @param client_id Client identifier
 * @param view_only Set to true for view-only mode
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_set_client_permissions(x11vnc_server_t* server,
                                        const char* client_id,
                                        bool view_only);

/**
 * Inject pointer/mouse event
 * @param server Server handle
 * @param x Pointer X coordinate
 * @param y Pointer Y coordinate
 * @param button_mask Button state bitmask
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_inject_pointer(x11vnc_server_t* server,
                                int x, int y, int button_mask);

/**
 * Inject keyboard event
 * @param server Server handle
 * @param keysym X11 keysym
 * @param down True for key press, false for release
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_inject_key(x11vnc_server_t* server,
                            uint32_t keysym, bool down);

/**
 * Send text as keyboard events
 * @param server Server handle
 * @param text Text to type
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_inject_text(x11vnc_server_t* server, const char* text);

/**
 * Get current clipboard content
 * @param server Server handle
 * @param buffer Buffer to store clipboard text
 * @param buffer_size Size of buffer
 * @param actual_size Actual size of clipboard content
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_get_clipboard(x11vnc_server_t* server,
                               char* buffer, size_t buffer_size,
                               size_t* actual_size);

/**
 * Set clipboard content
 * @param server Server handle
 * @param text Text to set in clipboard
 * @param length Text length (-1 for null-terminated)
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_set_clipboard(x11vnc_server_t* server,
                               const char* text, int length);

/**
 * Execute remote control command
 * @param server Server handle
 * @param command Remote control command
 * @param response Buffer for response (can be NULL)
 * @param response_size Size of response buffer
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_remote_control(x11vnc_server_t* server,
                                const char* command,
                                char* response, size_t response_size);

/**
 * Process events in non-blocking mode
 * @param server Server handle
 * @param timeout_ms Maximum time to wait for events (0 for immediate return)
 * @return Number of events processed, or negative error code
 */
int x11vnc_server_process_events(x11vnc_server_t* server, int timeout_ms);

/**
 * Force screen update for specific region
 * @param server Server handle
 * @param x Region X coordinate
 * @param y Region Y coordinate  
 * @param width Region width (0 for full screen)
 * @param height Region height (0 for full screen)
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_update_screen(x11vnc_server_t* server,
                               int x, int y, int width, int height);

/**
 * Enable/disable performance monitoring
 * @param server Server handle
 * @param enable True to enable monitoring
 * @param warning_threshold Performance warning threshold (0.0-1.0)
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_set_performance_monitoring(x11vnc_server_t* server,
                                            bool enable,
                                            double warning_threshold);

/**
 * Set bandwidth limits for clients
 * @param server Server handle
 * @param max_kbps_per_client Maximum KB/s per client (0 for unlimited)
 * @return 0 on success, negative error code on failure
 */
int x11vnc_server_set_bandwidth_limit(x11vnc_server_t* server,
                                     int max_kbps_per_client);

/* Legacy interface - for backwards compatibility */
int x11vnc_main_legacy(int argc, char* argv[]);

#ifdef __cplusplus
}
#endif

#endif /* LIBX11VNC_H */