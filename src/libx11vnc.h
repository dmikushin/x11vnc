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
#define X11VNC_ERROR_INTERNAL      -5

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

/* Legacy interface - for backwards compatibility */
int x11vnc_main_legacy(int argc, char* argv[]);

#ifdef __cplusplus
}
#endif

#endif /* LIBX11VNC_H */