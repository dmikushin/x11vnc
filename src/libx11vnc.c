/*
 * libx11vnc.c - Library implementation for x11vnc API
 *
 * Copyright (C) 2002-2010 Karl J. Runge <runge@karlrunge.com>
 * Copyright (C) 2024 x11vnc contributors
 * All rights reserved.
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#include "libx11vnc.h"
#include "x11vnc.h"
#include "options.h"
#include "connections.h"
#include "cleanup.h"

/* Global state backup structure */
typedef struct {
    /* Save important global variables here */
    int saved_client_count;
    int saved_got_rfbport;
    char* saved_use_dpy;
    char* saved_auth_file;
    /* Add more globals as needed */
} global_state_backup_t;

/* Server context structure */
struct x11vnc_server {
    /* State */
    bool running;
    bool initialized;
    
    /* Threading */
    pthread_t server_thread;
    pthread_mutex_t mutex;
    
    /* Arguments storage */
    int argc;
    char** argv;
    
    /* Global state backup */
    global_state_backup_t saved_state;
    
    /* Exit flag for main loop */
    volatile bool should_exit;
};

/* Static functions */
static void* server_thread_main(void* arg);
static void save_global_state(x11vnc_server_t* server);
static void restore_global_state(x11vnc_server_t* server);
static void apply_server_context(x11vnc_server_t* server);
static char** copy_argv(int argc, char** argv);
static void free_argv(int argc, char** argv);

/* Create server instance */
x11vnc_server_t* x11vnc_server_create(void) {
    x11vnc_server_t* server = calloc(1, sizeof(x11vnc_server_t));
    if (!server) {
        return NULL;
    }
    
    /* Initialize mutex */
    if (pthread_mutex_init(&server->mutex, NULL) != 0) {
        free(server);
        return NULL;
    }
    
    /* Save current global state */
    save_global_state(server);
    
    return server;
}

/* Start server with command line arguments */
int x11vnc_server_start(x11vnc_server_t* server, int argc, char** argv) {
    if (!server) {
        return X11VNC_ERROR_INVALID_ARG;
    }
    
    pthread_mutex_lock(&server->mutex);
    
    if (server->running) {
        pthread_mutex_unlock(&server->mutex);
        return X11VNC_ERROR_ALREADY_RUNNING;
    }
    
    /* Copy arguments */
    if (argc > 0 && argv) {
        server->argc = argc;
        server->argv = copy_argv(argc, argv);
        if (!server->argv) {
            pthread_mutex_unlock(&server->mutex);
            return X11VNC_ERROR_NO_MEMORY;
        }
    } else {
        /* Default arguments for minimal setup */
        server->argc = 1;
        server->argv = calloc(2, sizeof(char*));
        if (!server->argv) {
            pthread_mutex_unlock(&server->mutex);
            return X11VNC_ERROR_NO_MEMORY;
        }
        server->argv[0] = strdup("x11vnc");
    }
    
    server->should_exit = false;
    server->running = true;
    
    pthread_mutex_unlock(&server->mutex);
    
    return X11VNC_SUCCESS;
}

/* Run server main loop */
int x11vnc_server_run(x11vnc_server_t* server) {
    if (!server) {
        return X11VNC_ERROR_INVALID_ARG;
    }
    
    pthread_mutex_lock(&server->mutex);
    
    if (!server->running) {
        pthread_mutex_unlock(&server->mutex);
        return X11VNC_ERROR_NOT_RUNNING;
    }
    
    /* Apply server context to globals */
    apply_server_context(server);
    
    pthread_mutex_unlock(&server->mutex);
    
    /* Call the original main function */
    int result = x11vnc_main_legacy(server->argc, server->argv);
    
    /* Mark as stopped */
    pthread_mutex_lock(&server->mutex);
    server->running = false;
    pthread_mutex_unlock(&server->mutex);
    
    return result;
}

/* Stop server */
void x11vnc_server_stop(x11vnc_server_t* server) {
    if (!server) {
        return;
    }
    
    pthread_mutex_lock(&server->mutex);
    
    if (!server->running) {
        pthread_mutex_unlock(&server->mutex);
        return;
    }
    
    /* Signal shutdown */
    server->should_exit = true;
    shut_down = 1;  /* Set global shutdown flag */
    
    pthread_mutex_unlock(&server->mutex);
    
    /* Wait for server thread if it exists */
    if (server->server_thread) {
        pthread_join(server->server_thread, NULL);
        server->server_thread = 0;
    }
}

/* Destroy server instance */
void x11vnc_server_destroy(x11vnc_server_t* server) {
    if (!server) {
        return;
    }
    
    /* Stop if running */
    x11vnc_server_stop(server);
    
    /* Restore global state */
    restore_global_state(server);
    
    /* Free arguments */
    if (server->argv) {
        free_argv(server->argc, server->argv);
    }
    
    /* Destroy mutex */
    pthread_mutex_destroy(&server->mutex);
    
    /* Free server */
    free(server);
}

/* Get version string */
const char* x11vnc_get_version(void) {
    return VERSION;
}

/* Get server port */
int x11vnc_server_get_port(x11vnc_server_t* server) {
    if (!server) {
        return -1;
    }
    
    pthread_mutex_lock(&server->mutex);
    
    if (!server->running) {
        pthread_mutex_unlock(&server->mutex);
        return -1;
    }
    
    /* Get port from global state */
    int port = got_rfbport ? got_rfbport : 5900;
    
    pthread_mutex_unlock(&server->mutex);
    
    return port;
}

/* Get client count */
int x11vnc_server_get_client_count(x11vnc_server_t* server) {
    if (!server) {
        return -1;
    }
    
    pthread_mutex_lock(&server->mutex);
    
    if (!server->running) {
        pthread_mutex_unlock(&server->mutex);
        return -1;
    }
    
    /* Get client count from global state */
    int count = client_count;
    
    pthread_mutex_unlock(&server->mutex);
    
    return count;
}

/* Check if server is running */
bool x11vnc_server_is_running(x11vnc_server_t* server) {
    if (!server) {
        return false;
    }
    
    pthread_mutex_lock(&server->mutex);
    bool running = server->running;
    pthread_mutex_unlock(&server->mutex);
    
    return running;
}

/* Static helper functions */

/* Save global state */
static void save_global_state(x11vnc_server_t* server) {
    server->saved_state.saved_client_count = client_count;
    server->saved_state.saved_got_rfbport = got_rfbport;
    server->saved_state.saved_use_dpy = use_dpy ? strdup(use_dpy) : NULL;
    server->saved_state.saved_auth_file = auth_file ? strdup(auth_file) : NULL;
}

/* Restore global state */
static void restore_global_state(x11vnc_server_t* server) {
    client_count = server->saved_state.saved_client_count;
    got_rfbport = server->saved_state.saved_got_rfbport;
    
    if (use_dpy) {
        free(use_dpy);
    }
    use_dpy = server->saved_state.saved_use_dpy;
    
    if (auth_file) {
        free(auth_file);
    }
    auth_file = server->saved_state.saved_auth_file;
}

/* Apply server context to globals */
static void apply_server_context(x11vnc_server_t* server) {
    /* This function can be used to set up global state
     * specific to this server instance before calling
     * the main function */
    (void)server; /* Currently unused */
}

/* Copy argument array */
static char** copy_argv(int argc, char** argv) {
    char** new_argv = calloc(argc + 1, sizeof(char*));
    if (!new_argv) {
        return NULL;
    }
    
    for (int i = 0; i < argc; i++) {
        new_argv[i] = strdup(argv[i]);
        if (!new_argv[i]) {
            /* Clean up on failure */
            for (int j = 0; j < i; j++) {
                free(new_argv[j]);
            }
            free(new_argv);
            return NULL;
        }
    }
    
    return new_argv;
}

/* Free argument array */
static void free_argv(int argc, char** argv) {
    if (!argv) {
        return;
    }
    
    for (int i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
}