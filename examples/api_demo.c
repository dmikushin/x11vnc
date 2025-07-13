/*
 * api_demo.c - Demonstration of libx11vnc API usage
 *
 * This example shows how to use all the basic API functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <libx11vnc.h>

static x11vnc_server_t* g_server = NULL;

static void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    if (g_server) {
        x11vnc_server_stop(g_server);
    }
}

int main(int argc, char* argv[]) {
    printf("=== libx11vnc API Demo ===\n");
    printf("Version: %s\n\n", x11vnc_get_version());
    
    /* Test 1: Create server */
    printf("1. Creating server...\n");
    g_server = x11vnc_server_create();
    if (!g_server) {
        printf("   FAILED to create server\n");
        return 1;
    }
    printf("   SUCCESS: Server created\n");
    
    /* Test 2: Check initial state */
    printf("\n2. Checking initial state...\n");
    printf("   Running: %s\n", x11vnc_server_is_running(g_server) ? "YES" : "NO");
    printf("   Port: %d\n", x11vnc_server_get_port(g_server));
    printf("   Clients: %d\n", x11vnc_server_get_client_count(g_server));
    
    /* Test 3: Start server with custom arguments */
    printf("\n3. Starting server...\n");
    char* args[] = {
        "x11vnc",
        "-display", ":0",
        "-viewonly",     /* Read-only mode for safety */
        "-nopw",         /* No password (insecure but simple) */
        "-once",         /* Exit after first client */
        "-localhost",    /* Only local connections */
        "-quiet"         /* Less verbose output */
    };
    int arg_count = sizeof(args)/sizeof(args[0]);
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    int ret = x11vnc_server_start(g_server, arg_count, args);
    if (ret != X11VNC_SUCCESS) {
        printf("   FAILED to start server: error %d\n", ret);
        x11vnc_server_destroy(g_server);
        return 1;
    }
    printf("   SUCCESS: Server started\n");
    
    /* Test 4: Check running state */
    printf("\n4. Checking running state...\n");
    printf("   Running: %s\n", x11vnc_server_is_running(g_server) ? "YES" : "NO");
    printf("   Port: %d\n", x11vnc_server_get_port(g_server));
    printf("   Clients: %d\n", x11vnc_server_get_client_count(g_server));
    
    printf("\n=== Server Ready ===\n");
    printf("Connect with: vncviewer localhost:%d\n", x11vnc_server_get_port(g_server) - 5900);
    printf("Note: View-only mode (read-only)\n");
    printf("Press Ctrl+C to stop or wait for client connection\n\n");
    
    /* Test 5: Run main loop */
    printf("5. Running main loop...\n");
    ret = x11vnc_server_run(g_server);
    printf("   Main loop exited with code: %d\n", ret);
    
    /* Test 6: Check final state */
    printf("\n6. Checking final state...\n");
    printf("   Running: %s\n", x11vnc_server_is_running(g_server) ? "YES" : "NO");
    printf("   Final client count: %d\n", x11vnc_server_get_client_count(g_server));
    
    /* Test 7: Clean up */
    printf("\n7. Cleaning up...\n");
    x11vnc_server_destroy(g_server);
    printf("   SUCCESS: Server destroyed\n");
    
    printf("\n=== Demo Complete ===\n");
    return ret;
}