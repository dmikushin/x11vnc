/*
 * simple_server.c - Simple example of using libx11vnc API
 *
 * This example shows how to create and run an x11vnc server
 * using the library API instead of command line.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <libx11vnc.h>

static x11vnc_server_t* g_server = NULL;

/* Signal handler for graceful shutdown */
static void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    if (g_server) {
        x11vnc_server_stop(g_server);
    }
}

int main(int argc, char* argv[]) {
    int ret;
    
    printf("libx11vnc simple server example\n");
    printf("Version: %s\n", x11vnc_get_version());
    printf("========================================\n");
    
    /* Create server instance */
    g_server = x11vnc_server_create();
    if (!g_server) {
        fprintf(stderr, "Failed to create x11vnc server\n");
        return 1;
    }
    
    /* Install signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Prepare arguments */
    char* server_args[] = {
        "x11vnc",
        "-display", ":0",     /* Connect to display :0 */
        "-forever",           /* Keep running after clients disconnect */
        "-shared",            /* Allow multiple clients */
        "-nopw",              /* No password (WARNING: insecure) */
        "-once",              /* Exit after first client disconnects */
        NULL
    };
    int server_argc = sizeof(server_args)/sizeof(server_args[0]) - 1;
    
    /* Start server */
    ret = x11vnc_server_start(g_server, server_argc, server_args);
    if (ret != X11VNC_SUCCESS) {
        fprintf(stderr, "Failed to start x11vnc server: %d\n", ret);
        x11vnc_server_destroy(g_server);
        return 1;
    }
    
    printf("Server started successfully\n");
    printf("Port: %d\n", x11vnc_server_get_port(g_server));
    printf("Status: %s\n", x11vnc_server_is_running(g_server) ? "Running" : "Stopped");
    printf("Connect with: vncviewer localhost:%d\n", x11vnc_server_get_port(g_server) - 5900);
    printf("Press Ctrl+C to stop\n");
    
    /* Run main loop */
    ret = x11vnc_server_run(g_server);
    
    printf("Server stopped with code: %d\n", ret);
    printf("Final client count: %d\n", x11vnc_server_get_client_count(g_server));
    
    /* Clean up */
    x11vnc_server_destroy(g_server);
    
    return ret;
}