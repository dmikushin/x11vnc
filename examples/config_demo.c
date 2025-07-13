/*
 * config_demo.c - Demonstration of Phase 2 configuration API
 *
 * This example shows how to use the configuration-based API
 * instead of command-line arguments.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <libx11vnc.h>

static x11vnc_server_t* g_server = NULL;

/* Event callback function */
static void on_server_event(x11vnc_server_t* server, 
                           x11vnc_event_type_t event_type,
                           const char* message,
                           void* user_data) {
    const char* event_name = "UNKNOWN";
    
    switch (event_type) {
        case X11VNC_EVENT_STARTED:
            event_name = "STARTED";
            break;
        case X11VNC_EVENT_STOPPED:
            event_name = "STOPPED";
            break;
        case X11VNC_EVENT_CLIENT_CONNECTED:
            event_name = "CLIENT_CONNECTED";
            break;
        case X11VNC_EVENT_CLIENT_DISCONNECTED:
            event_name = "CLIENT_DISCONNECTED";
            break;
        case X11VNC_EVENT_ERROR:
            event_name = "ERROR";
            break;
    }
    
    printf("[EVENT %s] %s\n", event_name, message ? message : "");
}

static void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    if (g_server) {
        x11vnc_server_stop(g_server);
    }
}

static void print_config(const x11vnc_simple_config_t* config) {
    printf("Configuration:\n");
    printf("  Display: %s\n", config->display ?: "default");
    printf("  Port: %d\n", config->port);
    printf("  View-only: %s\n", config->view_only ? "YES" : "NO");
    printf("  Shared: %s\n", config->shared ? "YES" : "NO");
    printf("  Localhost only: %s\n", config->localhost_only ? "YES" : "NO");
    printf("  Use XDAMAGE: %s\n", config->use_xdamage ? "YES" : "NO");
    printf("  Show cursor: %s\n", config->show_cursor ? "YES" : "NO");
    printf("  Password: %s\n", config->password ? "SET" : "NONE");
    printf("  Allow hosts: %s\n", config->allow_hosts ?: "ANY");
    printf("\n");
}

int main(int argc, char* argv[]) {
    printf("=== libx11vnc Configuration API Demo ===\n");
    printf("Version: %s\n\n", x11vnc_get_version());
    
    /* Step 1: Create server */
    printf("1. Creating server...\n");
    g_server = x11vnc_server_create();
    if (!g_server) {
        printf("   FAILED to create server\n");
        return 1;
    }
    printf("   SUCCESS: Server created\n\n");
    
    /* Step 2: Set up event callback */
    printf("2. Setting up event callback...\n");
    int ret = x11vnc_server_set_event_callback(g_server, on_server_event, NULL);
    if (ret != X11VNC_SUCCESS) {
        printf("   FAILED to set event callback: %d\n", ret);
    } else {
        printf("   SUCCESS: Event callback set\n");
    }
    printf("\n");
    
    /* Step 3: Initialize configuration with defaults */
    printf("3. Initializing configuration...\n");
    x11vnc_simple_config_t config;
    x11vnc_config_init_defaults(&config);
    
    /* Customize configuration */
    config.display = ":0";
    config.port = 5901;           /* Use non-standard port */
    config.view_only = true;      /* Read-only for safety */
    config.shared = true;         /* Allow multiple clients */
    config.localhost_only = true; /* Only local connections */
    config.forever = false;       /* Exit after clients disconnect */
    config.once = true;           /* Exit after first client */
    config.password = NULL;       /* No password (insecure) */
    config.use_xdamage = true;    /* Use XDAMAGE for efficiency */
    config.show_cursor = true;    /* Show cursor */
    config.poll_interval_ms = 50; /* 50ms polling */
    
    print_config(&config);
    
    /* Step 4: Configure server */
    printf("4. Configuring server...\n");
    ret = x11vnc_server_configure(g_server, &config);
    if (ret != X11VNC_SUCCESS) {
        printf("   FAILED to configure server: %d\n", ret);
        x11vnc_server_destroy(g_server);
        return 1;
    }
    printf("   SUCCESS: Server configured\n\n");
    
    /* Step 5: Verify configuration */
    printf("5. Verifying configuration...\n");
    x11vnc_simple_config_t retrieved_config;
    ret = x11vnc_server_get_config(g_server, &retrieved_config);
    if (ret != X11VNC_SUCCESS) {
        printf("   FAILED to get configuration: %d\n", ret);
    } else {
        printf("   SUCCESS: Configuration retrieved\n");
        print_config(&retrieved_config);
    }
    
    /* Step 6: Start server using configuration */
    printf("6. Starting server with configuration...\n");
    ret = x11vnc_server_start_configured(g_server);
    if (ret != X11VNC_SUCCESS) {
        printf("   FAILED to start server: %d\n", ret);
        x11vnc_server_destroy(g_server);
        return 1;
    }
    printf("   SUCCESS: Server started\n\n");
    
    /* Step 7: Show server status */
    printf("7. Server status:\n");
    printf("   Running: %s\n", x11vnc_server_is_running(g_server) ? "YES" : "NO");
    printf("   Port: %d\n", x11vnc_server_get_port(g_server));
    printf("   Clients: %d\n", x11vnc_server_get_client_count(g_server));
    printf("\n");
    
    /* Install signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("=== Server Ready ===\n");
    printf("Connect with: vncviewer localhost:%d\n", x11vnc_server_get_port(g_server) - 5900);
    printf("Note: View-only mode, localhost only, port %d\n", x11vnc_server_get_port(g_server));
    printf("Press Ctrl+C to stop or wait for client connection\n\n");
    
    /* Step 8: Test runtime configuration update */
    printf("8. Testing runtime configuration update...\n");
    x11vnc_simple_config_t new_config = config;
    new_config.view_only = false;  /* Change to read-write */
    new_config.shared = false;     /* Change to single client */
    
    bool restart_needed = false;
    ret = x11vnc_server_update_config(g_server, &new_config, &restart_needed);
    if (ret != X11VNC_SUCCESS) {
        printf("   FAILED to update configuration: %d\n", ret);
    } else {
        printf("   SUCCESS: Configuration updated\n");
        printf("   Restart needed: %s\n", restart_needed ? "YES" : "NO");
    }
    printf("\n");
    
    /* Step 9: Run main loop */
    printf("9. Running main loop...\n");
    ret = x11vnc_server_run(g_server);
    printf("   Main loop exited with code: %d\n", ret);
    
    /* Step 10: Clean up */
    printf("\n10. Cleaning up...\n");
    x11vnc_server_destroy(g_server);
    printf("    SUCCESS: Server destroyed\n");
    
    printf("\n=== Configuration Demo Complete ===\n");
    return ret;
}