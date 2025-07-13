/*
 * Phase 3 Demo - Advanced x11vnc API Features
 * 
 * This example demonstrates:
 * - Advanced event callbacks with typed data
 * - Performance monitoring and statistics
 * - Client management (list, disconnect, permissions)
 * - Input injection (mouse, keyboard, text)
 * - Clipboard management
 * - Remote control commands
 * - Non-blocking event processing
 * - Screen update control
 * - Bandwidth limiting
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <libx11vnc.h>

static volatile bool keep_running = true;
static x11vnc_server_t* global_server = NULL;

/* Signal handler for graceful shutdown */
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    keep_running = false;
    if (global_server) {
        x11vnc_server_stop(global_server);
    }
}

/* Advanced event callback */
void advanced_event_callback(x11vnc_server_t* server, 
                           x11vnc_event_type_t event_type,
                           void* event_data,
                           void* user_data) {
    const char* context = (const char*)user_data;
    
    switch (event_type) {
        case X11VNC_EVENT_FRAME_SENT: {
            printf("[%s] Frame sent to client\n", context);
            break;
        }
        
        case X11VNC_EVENT_INPUT_RECEIVED: {
            /* We need to determine input type from context */
            printf("[%s] Input event received\n", context);
            break;
        }
        
        case X11VNC_EVENT_CLIPBOARD_CHANGED: {
            x11vnc_clipboard_event_t* clip = (x11vnc_clipboard_event_t*)event_data;
            printf("[%s] Clipboard changed: %d bytes from client %s\n", 
                   context, (int)clip->length, clip->client_id);
            break;
        }
        
        case X11VNC_EVENT_SCREEN_CHANGED: {
            x11vnc_screen_event_t* screen = (x11vnc_screen_event_t*)event_data;
            printf("[%s] Screen resolution changed: %dx%d -> %dx%d\n",
                   context, screen->old_width, screen->old_height,
                   screen->new_width, screen->new_height);
            break;
        }
        
        case X11VNC_EVENT_PERFORMANCE_WARNING: {
            x11vnc_performance_event_t* perf = (x11vnc_performance_event_t*)event_data;
            printf("[%s] Performance warning: %s (%.2f > %.2f)\n",
                   context, perf->description, perf->value, perf->threshold);
            break;
        }
        
        case X11VNC_EVENT_CLIENT_CONNECTED: {
            printf("[%s] Client connected\n", context);
            break;
        }
        
        case X11VNC_EVENT_CLIENT_DISCONNECTED: {
            printf("[%s] Client disconnected\n", context);
            break;
        }
        
        default:
            printf("[%s] Event: %d\n", context, event_type);
            break;
    }
}

/* Demonstrate advanced statistics */
void demo_statistics(x11vnc_server_t* server) {
    printf("\n=== Advanced Statistics Demo ===\n");
    
    x11vnc_advanced_stats_t stats;
    int ret = x11vnc_server_get_advanced_stats(server, &stats);
    
    if (ret == X11VNC_SUCCESS) {
        printf("Server uptime: %lu seconds\n", stats.uptime_seconds);
        printf("Screen: %dx%d @ %d bpp\n", 
               stats.screen_width, stats.screen_height, stats.bits_per_pixel);
        printf("Clients: %d current, %d max reached, %lu total connections\n",
               stats.current_clients, stats.max_clients_reached, stats.total_connections);
        printf("Performance: %.1f fps current, %.1f fps average\n",
               stats.fps_current, stats.fps_average);
        printf("CPU: %.1f%%, Memory: %.1f MB\n",
               stats.cpu_usage_percent, stats.memory_usage_mb);
        printf("Network: %.1f KB/s in, %.1f KB/s out\n",
               stats.bandwidth_in_kbps, stats.bandwidth_out_kbps);
        printf("Frames: %lu sent, %d dropped\n", 
               stats.total_frames_sent, stats.dropped_frames);
    } else {
        printf("Failed to get statistics: %d\n", ret);
    }
}

/* Demonstrate client management */
void demo_client_management(x11vnc_server_t* server) {
    printf("\n=== Client Management Demo ===\n");
    
    /* Get list of connected clients */
    x11vnc_client_info_t clients[10];
    int actual_count;
    
    int ret = x11vnc_server_get_clients(server, clients, 10, &actual_count);
    if (ret == X11VNC_SUCCESS) {
        printf("Found %d connected clients:\n", actual_count);
        
        for (int i = 0; i < actual_count; i++) {
            x11vnc_client_info_t* client = &clients[i];
            printf("  Client %d:\n", i + 1);
            printf("    ID: %s\n", client->client_id);
            printf("    Host: %s:%d\n", client->hostname, client->port);
            printf("    User: %s\n", client->username);
            printf("    Authenticated: %s\n", client->authenticated ? "Yes" : "No");
            printf("    View-only: %s\n", client->view_only ? "Yes" : "No");
            printf("    Encoding: %s\n", client->encoding);
            printf("    Data: %lu sent, %lu received\n", 
                   client->bytes_sent, client->bytes_received);
            printf("    Frames sent: %u\n", client->frames_sent);
            
            /* Demonstrate setting permissions */
            if (i == 0 && !client->view_only) {
                printf("    Setting first client to view-only...\n");
                x11vnc_server_set_client_permissions(server, client->client_id, true);
            }
        }
        
        /* Demonstrate disconnecting a client */
        if (actual_count > 1) {
            printf("  Disconnecting second client...\n");
            x11vnc_server_disconnect_client(server, clients[1].client_id, 
                                          "Demonstration disconnect");
        }
    } else {
        printf("Failed to get client list: %d\n", ret);
    }
}

/* Demonstrate input injection */
void demo_input_injection(x11vnc_server_t* server) {
    printf("\n=== Input Injection Demo ===\n");
    
    /* Inject mouse movement and click */
    printf("Injecting mouse movement to center of screen...\n");
    x11vnc_server_inject_pointer(server, 400, 300, 0); /* Move to 400,300 */
    
    printf("Injecting left mouse click...\n");
    x11vnc_server_inject_pointer(server, 400, 300, 1); /* Press left button */
    usleep(100000); /* 100ms */
    x11vnc_server_inject_pointer(server, 400, 300, 0); /* Release button */
    
    /* Inject keyboard events */
    printf("Injecting keyboard events (Ctrl+A)...\n");
    x11vnc_server_inject_key(server, 0xffe3, true);  /* Ctrl down */
    x11vnc_server_inject_key(server, 0x0061, true);  /* 'a' down */
    usleep(50000); /* 50ms */
    x11vnc_server_inject_key(server, 0x0061, false); /* 'a' up */
    x11vnc_server_inject_key(server, 0xffe3, false); /* Ctrl up */
    
    /* Inject text */
    printf("Injecting text: 'Hello from libx11vnc!'...\n");
    x11vnc_server_inject_text(server, "Hello from libx11vnc!");
}

/* Demonstrate clipboard management */
void demo_clipboard_management(x11vnc_server_t* server) {
    printf("\n=== Clipboard Management Demo ===\n");
    
    /* Set clipboard content */
    const char* test_text = "This is test clipboard content from libx11vnc Phase 3 API!";
    printf("Setting clipboard content...\n");
    int ret = x11vnc_server_set_clipboard(server, test_text, -1);
    if (ret == X11VNC_SUCCESS) {
        printf("Clipboard set successfully\n");
    } else {
        printf("Failed to set clipboard: %d\n", ret);
    }
    
    /* Get clipboard content */
    printf("Getting clipboard content...\n");
    char buffer[1024];
    size_t actual_size;
    ret = x11vnc_server_get_clipboard(server, buffer, sizeof(buffer), &actual_size);
    if (ret == X11VNC_SUCCESS) {
        printf("Clipboard content (%zu bytes): %s\n", actual_size, buffer);
    } else {
        printf("Failed to get clipboard: %d\n", ret);
    }
}

/* Demonstrate remote control */
void demo_remote_control(x11vnc_server_t* server) {
    printf("\n=== Remote Control Demo ===\n");
    
    char response[256];
    const char* commands[] = {
        "ping",
        "version", 
        "clients",
        "set shared:1",
        "set viewonly:0"
    };
    
    for (int i = 0; i < 5; i++) {
        printf("Executing command: %s\n", commands[i]);
        int ret = x11vnc_server_remote_control(server, commands[i], 
                                              response, sizeof(response));
        if (ret == X11VNC_SUCCESS) {
            printf("  Response: %s\n", response);
        } else {
            printf("  Failed: %d\n", ret);
        }
    }
}

/* Demonstrate performance monitoring */
void demo_performance_monitoring(x11vnc_server_t* server) {
    printf("\n=== Performance Monitoring Demo ===\n");
    
    /* Enable performance monitoring */
    printf("Enabling performance monitoring (threshold: 80%%)...\n");
    int ret = x11vnc_server_set_performance_monitoring(server, true, 0.8);
    if (ret == X11VNC_SUCCESS) {
        printf("Performance monitoring enabled\n");
    } else {
        printf("Failed to enable monitoring: %d\n", ret);
    }
    
    /* Set bandwidth limit */
    printf("Setting bandwidth limit to 1000 KB/s per client...\n");
    ret = x11vnc_server_set_bandwidth_limit(server, 1000);
    if (ret == X11VNC_SUCCESS) {
        printf("Bandwidth limit set\n");
    } else {
        printf("Failed to set bandwidth limit: %d\n", ret);
    }
}

/* Demonstrate non-blocking event processing */
void demo_non_blocking_events(x11vnc_server_t* server) {
    printf("\n=== Non-blocking Event Processing Demo ===\n");
    
    for (int i = 0; i < 5; i++) {
        printf("Processing events (timeout: 100ms)...\n");
        int events = x11vnc_server_process_events(server, 100);
        if (events >= 0) {
            printf("  Processed %d events\n", events);
        } else {
            printf("  Error processing events: %d\n", events);
        }
        usleep(200000); /* 200ms between calls */
    }
}

/* Demonstrate screen update control */
void demo_screen_updates(x11vnc_server_t* server) {
    printf("\n=== Screen Update Control Demo ===\n");
    
    /* Force full screen update */
    printf("Forcing full screen update...\n");
    int ret = x11vnc_server_update_screen(server, 0, 0, 0, 0);
    if (ret == X11VNC_SUCCESS) {
        printf("Full screen update initiated\n");
    }
    
    /* Force partial screen update */
    printf("Forcing partial screen update (100x100 at 50,50)...\n");
    ret = x11vnc_server_update_screen(server, 50, 50, 100, 100);
    if (ret == X11VNC_SUCCESS) {
        printf("Partial screen update initiated\n");
    }
}

int main(int argc, char* argv[]) {
    printf("=== x11vnc Phase 3 API Demo ===\n");
    
    /* Set up signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Create server */
    printf("Creating x11vnc server...\n");
    x11vnc_server_t* server = x11vnc_server_create();
    if (!server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }
    global_server = server;
    
    /* Configure server */
    printf("Configuring server...\n");
    x11vnc_simple_config_t config;
    x11vnc_config_init_defaults(&config);
    config.port = 5901; /* Use different port for demo */
    config.shared = true;
    config.forever = true;
    config.localhost_only = true; /* Safe for demo */
    
    int ret = x11vnc_server_configure(server, &config);
    if (ret != X11VNC_SUCCESS) {
        fprintf(stderr, "Failed to configure server: %d\n", ret);
        x11vnc_server_destroy(server);
        return 1;
    }
    
    /* Set up advanced event callback */
    printf("Setting up advanced event callback...\n");
    ret = x11vnc_server_set_advanced_event_callback(server, 
                                                   advanced_event_callback, 
                                                   (void*)"Phase3Demo");
    if (ret != X11VNC_SUCCESS) {
        fprintf(stderr, "Failed to set event callback: %d\n", ret);
        x11vnc_server_destroy(server);
        return 1;
    }
    
    /* Start server */
    printf("Starting server on port %d...\n", config.port);
    ret = x11vnc_server_start_configured(server);
    if (ret != X11VNC_SUCCESS) {
        fprintf(stderr, "Failed to start server: %d\n", ret);
        x11vnc_server_destroy(server);
        return 1;
    }
    
    printf("Server started! Connect with: vncviewer localhost:%d\n", config.port - 5900);
    printf("Running demonstrations...\n\n");
    
    /* Run demonstrations */
    int demo_count = 0;
    while (keep_running && demo_count < 3) {
        printf("\n--- Demo Round %d ---\n", demo_count + 1);
        
        /* Demonstrate all Phase 3 features */
        demo_statistics(server);
        demo_performance_monitoring(server);
        demo_client_management(server);
        demo_input_injection(server);
        demo_clipboard_management(server);
        demo_remote_control(server);
        demo_non_blocking_events(server);
        demo_screen_updates(server);
        
        demo_count++;
        
        if (keep_running && demo_count < 3) {
            printf("\nWaiting 10 seconds before next demo round...\n");
            sleep(10);
        }
    }
    
    printf("\nDemo completed. Server will continue running until Ctrl+C...\n");
    printf("Try connecting with a VNC client to see the server in action!\n");
    
    /* Keep server running until interrupted */
    while (keep_running) {
        /* Process events occasionally */
        x11vnc_server_process_events(server, 1000);
        
        /* Update statistics periodically */
        static int stats_counter = 0;
        if (++stats_counter >= 30) { /* Every 30 seconds */
            demo_statistics(server);
            stats_counter = 0;
        }
    }
    
    /* Cleanup */
    printf("Stopping server...\n");
    x11vnc_server_stop(server);
    x11vnc_server_destroy(server);
    
    printf("Demo finished.\n");
    return 0;
}