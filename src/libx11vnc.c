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
    bool configured;
    
    /* Threading */
    pthread_t server_thread;
    pthread_mutex_t mutex;
    
    /* Arguments storage (for Phase 1 compatibility) */
    int argc;
    char** argv;
    
    /* Phase 2: Configuration storage */
    x11vnc_simple_config_t config;
    bool config_valid;
    
    /* Phase 2: Event callback */
    x11vnc_event_callback_t event_callback;
    void* event_user_data;
    
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

/* Phase 2 static functions */
static void emit_event(x11vnc_server_t* server, x11vnc_event_type_t type, const char* message);
static int config_to_argv(const x11vnc_simple_config_t* config, int* argc, char*** argv);
static void apply_config_to_globals(const x11vnc_simple_config_t* config);
static void config_copy_strings(x11vnc_simple_config_t* dest, const x11vnc_simple_config_t* src);

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

/* Phase 2 API Implementation */

/* Initialize configuration with defaults */
void x11vnc_config_init_defaults(x11vnc_simple_config_t* config) {
    if (!config) return;
    
    memset(config, 0, sizeof(x11vnc_simple_config_t));
    
    /* Set sensible defaults */
    config->display = ":0";
    config->port = 5900;
    config->localhost_only = false;
    config->ipv6 = false;
    config->view_only = false;
    config->shared = true;
    config->forever = false;
    config->once = false;
    config->poll_interval_ms = 30;
    config->use_shm = true;
    config->use_xdamage = true;
    config->wireframe = false;
    config->show_cursor = true;
    config->accept_bell = true;
    config->accept_clipboard = true;
}

/* Configure server */
int x11vnc_server_configure(x11vnc_server_t* server, const x11vnc_simple_config_t* config) {
    if (!server || !config) {
        return X11VNC_ERROR_INVALID_ARG;
    }
    
    pthread_mutex_lock(&server->mutex);
    
    if (server->running) {
        pthread_mutex_unlock(&server->mutex);
        return X11VNC_ERROR_ALREADY_RUNNING;
    }
    
    /* Copy configuration */
    config_copy_strings(&server->config, config);
    server->config_valid = true;
    server->configured = true;
    
    pthread_mutex_unlock(&server->mutex);
    
    emit_event(server, X11VNC_EVENT_STARTED, "Server configured");
    
    return X11VNC_SUCCESS;
}

/* Start server using configuration */
int x11vnc_server_start_configured(x11vnc_server_t* server) {
    if (!server) {
        return X11VNC_ERROR_INVALID_ARG;
    }
    
    pthread_mutex_lock(&server->mutex);
    
    if (!server->configured || !server->config_valid) {
        pthread_mutex_unlock(&server->mutex);
        return X11VNC_ERROR_INVALID_ARG;
    }
    
    if (server->running) {
        pthread_mutex_unlock(&server->mutex);
        return X11VNC_ERROR_ALREADY_RUNNING;
    }
    
    /* Convert configuration to argv format */
    int argc;
    char** argv;
    int ret = config_to_argv(&server->config, &argc, &argv);
    if (ret != X11VNC_SUCCESS) {
        pthread_mutex_unlock(&server->mutex);
        return ret;
    }
    
    /* Store argv for cleanup */
    server->argc = argc;
    server->argv = argv;
    
    server->should_exit = false;
    server->running = true;
    
    pthread_mutex_unlock(&server->mutex);
    
    emit_event(server, X11VNC_EVENT_STARTED, "Server started with configuration");
    
    return X11VNC_SUCCESS;
}

/* Set event callback */
int x11vnc_server_set_event_callback(x11vnc_server_t* server, 
                                    x11vnc_event_callback_t callback, 
                                    void* user_data) {
    if (!server) {
        return X11VNC_ERROR_INVALID_ARG;
    }
    
    pthread_mutex_lock(&server->mutex);
    server->event_callback = callback;
    server->event_user_data = user_data;
    pthread_mutex_unlock(&server->mutex);
    
    return X11VNC_SUCCESS;
}

/* Get current configuration */
int x11vnc_server_get_config(x11vnc_server_t* server, x11vnc_simple_config_t* config) {
    if (!server || !config) {
        return X11VNC_ERROR_INVALID_ARG;
    }
    
    pthread_mutex_lock(&server->mutex);
    
    if (!server->config_valid) {
        pthread_mutex_unlock(&server->mutex);
        return X11VNC_ERROR_INVALID_ARG;
    }
    
    config_copy_strings(config, &server->config);
    
    pthread_mutex_unlock(&server->mutex);
    
    return X11VNC_SUCCESS;
}

/* Update configuration at runtime */
int x11vnc_server_update_config(x11vnc_server_t* server, 
                               const x11vnc_simple_config_t* config,
                               bool* restart_needed) {
    if (!server || !config) {
        return X11VNC_ERROR_INVALID_ARG;
    }
    
    pthread_mutex_lock(&server->mutex);
    
    /* Check what changes require restart */
    if (restart_needed) {
        *restart_needed = false;
        
        if (server->config_valid) {
            /* These changes require restart */
            if (strcmp(config->display ?: "", server->config.display ?: "") != 0 ||
                config->port != server->config.port ||
                config->localhost_only != server->config.localhost_only ||
                config->ipv6 != server->config.ipv6) {
                *restart_needed = true;
            }
        }
    }
    
    /* Update configuration */
    config_copy_strings(&server->config, config);
    server->config_valid = true;
    
    /* Apply some settings immediately if running */
    if (server->running) {
        apply_config_to_globals(config);
    }
    
    pthread_mutex_unlock(&server->mutex);
    
    return X11VNC_SUCCESS;
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

/* Phase 2 static helper functions */

/* Emit event to callback */
static void emit_event(x11vnc_server_t* server, x11vnc_event_type_t type, const char* message) {
    if (!server || !server->event_callback) {
        return;
    }
    
    server->event_callback(server, type, message, server->event_user_data);
}

/* Convert configuration to argv format */
static int config_to_argv(const x11vnc_simple_config_t* config, int* argc, char*** argv) {
    if (!config || !argc || !argv) {
        return X11VNC_ERROR_INVALID_ARG;
    }
    
    /* Calculate maximum number of arguments we might need */
    int max_args = 50;  /* Generous estimate */
    char** args = calloc(max_args, sizeof(char*));
    if (!args) {
        return X11VNC_ERROR_NO_MEMORY;
    }
    
    int arg_count = 0;
    
    /* Program name */
    args[arg_count++] = strdup("x11vnc");
    
    /* Display */
    if (config->display) {
        args[arg_count++] = strdup("-display");
        args[arg_count++] = strdup(config->display);
    }
    
    /* Auth file */
    if (config->auth_file) {
        args[arg_count++] = strdup("-auth");
        args[arg_count++] = strdup(config->auth_file);
    }
    
    /* Port */
    if (config->port != 5900 && config->port > 0) {
        args[arg_count++] = strdup("-rfbport");
        char port_str[16];
        snprintf(port_str, sizeof(port_str), "%d", config->port);
        args[arg_count++] = strdup(port_str);
    }
    
    /* Network options */
    if (config->localhost_only) {
        args[arg_count++] = strdup("-localhost");
    }
    if (config->ipv6) {
        args[arg_count++] = strdup("-6");
    }
    
    /* Security */
    if (config->password) {
        args[arg_count++] = strdup("-passwd");
        args[arg_count++] = strdup(config->password);
    } else if (config->password_file) {
        args[arg_count++] = strdup("-passwdfile");
        args[arg_count++] = strdup(config->password_file);
    } else {
        args[arg_count++] = strdup("-nopw");
    }
    
    if (config->view_only) {
        args[arg_count++] = strdup("-viewonly");
    }
    
    if (config->allow_hosts) {
        args[arg_count++] = strdup("-allow");
        args[arg_count++] = strdup(config->allow_hosts);
    }
    
    /* Behavior */
    if (config->shared) {
        args[arg_count++] = strdup("-shared");
    } else {
        args[arg_count++] = strdup("-noshared");
    }
    
    if (config->forever) {
        args[arg_count++] = strdup("-forever");
    }
    
    if (config->once) {
        args[arg_count++] = strdup("-once");
    }
    
    /* Performance */
    if (config->poll_interval_ms != 30) {
        args[arg_count++] = strdup("-wait");
        char wait_str[16];
        snprintf(wait_str, sizeof(wait_str), "%d", config->poll_interval_ms);
        args[arg_count++] = strdup(wait_str);
    }
    
    if (!config->use_shm) {
        args[arg_count++] = strdup("-noshm");
    }
    
    if (!config->use_xdamage) {
        args[arg_count++] = strdup("-noxdamage");
    }
    
    if (config->wireframe) {
        args[arg_count++] = strdup("-wireframe");
    }
    
    /* Features */
    if (!config->show_cursor) {
        args[arg_count++] = strdup("-nocursor");
    }
    
    if (!config->accept_bell) {
        args[arg_count++] = strdup("-nobell");
    }
    
    if (config->geometry) {
        args[arg_count++] = strdup("-geometry");
        args[arg_count++] = strdup(config->geometry);
    }
    
    if (config->clip) {
        args[arg_count++] = strdup("-clip");
        args[arg_count++] = strdup(config->clip);
    }
    
    /* Always add quiet flag for library usage */
    args[arg_count++] = strdup("-quiet");
    
    *argc = arg_count;
    *argv = args;
    
    return X11VNC_SUCCESS;
}

/* Apply configuration to global variables */
static void apply_config_to_globals(const x11vnc_simple_config_t* config) {
    if (!config) return;
    
    /* Apply settings that can be changed at runtime */
    view_only = config->view_only ? 1 : 0;
    shared = config->shared ? 1 : 0;
    
    if (config->allow_hosts) {
        if (allow_list) free(allow_list);
        allow_list = strdup(config->allow_hosts);
    }
    
    /* Update display settings */
    if (config->display) {
        if (use_dpy) free(use_dpy);
        use_dpy = strdup(config->display);
    }
    
    if (config->auth_file) {
        if (auth_file) free(auth_file);
        auth_file = strdup(config->auth_file);
    }
}

/* Copy configuration strings */
static void config_copy_strings(x11vnc_simple_config_t* dest, const x11vnc_simple_config_t* src) {
    if (!dest || !src) return;
    
    /* Copy the structure first */
    *dest = *src;
    
    /* Then duplicate all string pointers */
    dest->display = src->display ? strdup(src->display) : NULL;
    dest->auth_file = src->auth_file ? strdup(src->auth_file) : NULL;
    dest->password = src->password ? strdup(src->password) : NULL;
    dest->password_file = src->password_file ? strdup(src->password_file) : NULL;
    dest->allow_hosts = src->allow_hosts ? strdup(src->allow_hosts) : NULL;
    dest->geometry = src->geometry ? strdup(src->geometry) : NULL;
    dest->clip = src->clip ? strdup(src->clip) : NULL;
}