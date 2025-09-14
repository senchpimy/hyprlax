/*
 * IPC implementation for hyprlax
 * Handles runtime layer management via Unix sockets
 */

#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>

static void get_socket_path(char* buffer, size_t size) {
    const char* user = getenv("USER");
    if (!user) {
        struct passwd* pw = getpwuid(getuid());
        user = pw ? pw->pw_name : "unknown";
    }
    snprintf(buffer, size, "%s%s.sock", IPC_SOCKET_PATH_PREFIX, user);
}

static ipc_command_t parse_command(const char* cmd) {
    if (strcmp(cmd, "add") == 0) return IPC_CMD_ADD_LAYER;
    if (strcmp(cmd, "remove") == 0 || strcmp(cmd, "rm") == 0) return IPC_CMD_REMOVE_LAYER;
    if (strcmp(cmd, "modify") == 0 || strcmp(cmd, "mod") == 0) return IPC_CMD_MODIFY_LAYER;
    if (strcmp(cmd, "list") == 0 || strcmp(cmd, "ls") == 0) return IPC_CMD_LIST_LAYERS;
    if (strcmp(cmd, "clear") == 0) return IPC_CMD_CLEAR_LAYERS;
    if (strcmp(cmd, "reload") == 0) return IPC_CMD_RELOAD_CONFIG;
    if (strcmp(cmd, "status") == 0) return IPC_CMD_GET_STATUS;
    return IPC_CMD_UNKNOWN;
}

ipc_context_t* ipc_init(void) {
    ipc_context_t* ctx = calloc(1, sizeof(ipc_context_t));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate IPC context\n");
        return NULL;
    }

    get_socket_path(ctx->socket_path, sizeof(ctx->socket_path));

    // Remove existing socket if it exists
    unlink(ctx->socket_path);

    // Create Unix domain socket
    ctx->socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (ctx->socket_fd < 0) {
        fprintf(stderr, "Failed to create IPC socket: %s\n", strerror(errno));
        free(ctx);
        return NULL;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ctx->socket_path, sizeof(addr.sun_path) - 1);

    if (bind(ctx->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Failed to bind IPC socket: %s\n", strerror(errno));
        close(ctx->socket_fd);
        free(ctx);
        return NULL;
    }

    if (listen(ctx->socket_fd, 5) < 0) {
        fprintf(stderr, "Failed to listen on IPC socket: %s\n", strerror(errno));
        close(ctx->socket_fd);
        unlink(ctx->socket_path);
        free(ctx);
        return NULL;
    }

    // Set socket permissions to user-only
    chmod(ctx->socket_path, 0600);

    ctx->active = true;
    ctx->next_layer_id = 1;

    printf("IPC socket listening at: %s\n", ctx->socket_path);
    return ctx;
}

void ipc_cleanup(ipc_context_t* ctx) {
    if (!ctx) return;

    ctx->active = false;

    // Clear all layers
    ipc_clear_layers(ctx);

    // Close and remove socket
    if (ctx->socket_fd >= 0) {
        close(ctx->socket_fd);
        unlink(ctx->socket_path);
    }

    free(ctx);
}

bool ipc_process_commands(ipc_context_t* ctx) {
    if (!ctx || !ctx->active) return false;

    // Accept new connections
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(ctx->socket_fd, (struct sockaddr*)&client_addr, &client_len);

    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            fprintf(stderr, "Failed to accept IPC connection: %s\n", strerror(errno));
        }
        return false;
    }

    // Read command
    char buffer[IPC_MAX_MESSAGE_SIZE];
    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
        close(client_fd);
        return false;
    }
    buffer[bytes] = '\0';

    // Parse and execute command
    char* cmd = strtok(buffer, " \n");
    if (!cmd) {
        const char* error = "Error: No command specified\n";
        send(client_fd, error, strlen(error), 0);
        close(client_fd);
        return false;
    }

    ipc_command_t command = parse_command(cmd);
    char response[IPC_MAX_MESSAGE_SIZE];
    bool success = false;

    switch (command) {
        case IPC_CMD_ADD_LAYER: {
            char* path = strtok(NULL, " \n");
            if (!path) {
                snprintf(response, sizeof(response), "Error: Image path required\n");
                break;
            }

            // Parse optional parameters
            float scale = 1.0f, opacity = 1.0f, x_offset = 0.0f, y_offset = 0.0f;
            int z_index = ctx->layer_count;

            char* param;
            while ((param = strtok(NULL, " \n"))) {
                if (strncmp(param, "scale=", 6) == 0) {
                    scale = atof(param + 6);
                } else if (strncmp(param, "opacity=", 8) == 0) {
                    opacity = atof(param + 8);
                } else if (strncmp(param, "x=", 2) == 0) {
                    x_offset = atof(param + 2);
                } else if (strncmp(param, "y=", 2) == 0) {
                    y_offset = atof(param + 2);
                } else if (strncmp(param, "z=", 2) == 0) {
                    z_index = atoi(param + 2);
                }
            }

            uint32_t id = ipc_add_layer(ctx, path, scale, opacity, x_offset, y_offset, z_index);
            if (id > 0) {
                snprintf(response, sizeof(response), "Layer added with ID: %u\n", id);
                success = true;
            } else {
                snprintf(response, sizeof(response), "Error: Failed to add layer\n");
            }
            break;
        }

        case IPC_CMD_REMOVE_LAYER: {
            char* id_str = strtok(NULL, " \n");
            if (!id_str) {
                snprintf(response, sizeof(response), "Error: Layer ID required\n");
                break;
            }

            uint32_t id = atoi(id_str);
            if (ipc_remove_layer(ctx, id)) {
                snprintf(response, sizeof(response), "Layer %u removed\n", id);
                success = true;
            } else {
                snprintf(response, sizeof(response), "Error: Layer %u not found\n", id);
            }
            break;
        }

        case IPC_CMD_MODIFY_LAYER: {
            char* id_str = strtok(NULL, " \n");
            char* property = strtok(NULL, " \n");
            char* value = strtok(NULL, " \n");

            if (!id_str || !property || !value) {
                snprintf(response, sizeof(response), "Error: Usage: modify <id> <property> <value>\n");
                break;
            }

            uint32_t id = atoi(id_str);
            if (ipc_modify_layer(ctx, id, property, value)) {
                snprintf(response, sizeof(response), "Layer %u modified\n", id);
                success = true;
            } else {
                snprintf(response, sizeof(response), "Error: Failed to modify layer %u\n", id);
            }
            break;
        }

        case IPC_CMD_LIST_LAYERS: {
            char* list = ipc_list_layers(ctx);
            if (list) {
                strncpy(response, list, sizeof(response) - 1);
                response[sizeof(response) - 1] = '\0';
                free(list);
                success = true;
            } else {
                snprintf(response, sizeof(response), "No layers\n");
                success = true;
            }
            break;
        }

        case IPC_CMD_CLEAR_LAYERS:
            ipc_clear_layers(ctx);
            snprintf(response, sizeof(response), "All layers cleared\n");
            success = true;
            break;

        case IPC_CMD_GET_STATUS:
            snprintf(response, sizeof(response),
                "Status: Active\nLayers: %d/%d\nSocket: %s\n",
                ctx->layer_count, IPC_MAX_LAYERS, ctx->socket_path);
            success = true;
            break;

        default:
            snprintf(response, sizeof(response), "Error: Unknown command '%s'\n", cmd);
            break;
    }

    // Send response
    send(client_fd, response, strlen(response), 0);
    close(client_fd);

    return success;
}

uint32_t ipc_add_layer(ipc_context_t* ctx, const char* image_path, float scale, float opacity, float x_offset, float y_offset, int z_index) {
    if (!ctx || !image_path || ctx->layer_count >= IPC_MAX_LAYERS) {
        return 0;
    }

    // Check if file exists
    if (access(image_path, R_OK) != 0) {
        fprintf(stderr, "Image file not found or not readable: %s\n", image_path);
        return 0;
    }

    layer_t* layer = calloc(1, sizeof(layer_t));
    if (!layer) return 0;

    layer->image_path = strdup(image_path);
    layer->scale = scale;
    layer->opacity = opacity;
    layer->x_offset = x_offset;
    layer->y_offset = y_offset;
    layer->z_index = z_index;
    layer->visible = true;
    layer->id = ctx->next_layer_id++;

    ctx->layers[ctx->layer_count++] = layer;
    ipc_sort_layers(ctx);

    return layer->id;
}

bool ipc_remove_layer(ipc_context_t* ctx, uint32_t layer_id) {
    if (!ctx) return false;

    for (int i = 0; i < ctx->layer_count; i++) {
        if (ctx->layers[i]->id == layer_id) {
            // Free layer resources
            free(ctx->layers[i]->image_path);
            free(ctx->layers[i]);

            // Shift remaining layers
            for (int j = i; j < ctx->layer_count - 1; j++) {
                ctx->layers[j] = ctx->layers[j + 1];
            }
            ctx->layers[--ctx->layer_count] = NULL;

            return true;
        }
    }

    return false;
}

bool ipc_modify_layer(ipc_context_t* ctx, uint32_t layer_id, const char* property, const char* value) {
    if (!ctx || !property || !value) return false;

    layer_t* layer = ipc_find_layer(ctx, layer_id);
    if (!layer) return false;

    bool needs_sort = false;

    if (strcmp(property, "scale") == 0) {
        layer->scale = atof(value);
    } else if (strcmp(property, "opacity") == 0) {
        layer->opacity = atof(value);
    } else if (strcmp(property, "x") == 0) {
        layer->x_offset = atof(value);
    } else if (strcmp(property, "y") == 0) {
        layer->y_offset = atof(value);
    } else if (strcmp(property, "z") == 0) {
        layer->z_index = atoi(value);
        needs_sort = true;
    } else if (strcmp(property, "visible") == 0) {
        layer->visible = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else {
        return false;
    }

    if (needs_sort) {
        ipc_sort_layers(ctx);
    }

    return true;
}

char* ipc_list_layers(ipc_context_t* ctx) {
    if (!ctx || ctx->layer_count == 0) return NULL;

    char* result = malloc(IPC_MAX_MESSAGE_SIZE);
    if (!result) return NULL;

    int offset = 0;
    for (int i = 0; i < ctx->layer_count; i++) {
        layer_t* layer = ctx->layers[i];
        int written = snprintf(result + offset, IPC_MAX_MESSAGE_SIZE - offset,
            "ID: %u | Path: %s | Scale: %.2f | Opacity: %.2f | Position: (%.2f, %.2f) | Z: %d | Visible: %s\n",
            layer->id, layer->image_path, layer->scale, layer->opacity,
            layer->x_offset, layer->y_offset, layer->z_index,
            layer->visible ? "yes" : "no");

        if (written < 0 || offset + written >= IPC_MAX_MESSAGE_SIZE) break;
        offset += written;
    }

    return result;
}

void ipc_clear_layers(ipc_context_t* ctx) {
    if (!ctx) return;

    for (int i = 0; i < ctx->layer_count; i++) {
        if (ctx->layers[i]) {
            free(ctx->layers[i]->image_path);
            free(ctx->layers[i]);
            ctx->layers[i] = NULL;
        }
    }
    ctx->layer_count = 0;
}

layer_t* ipc_find_layer(ipc_context_t* ctx, uint32_t layer_id) {
    if (!ctx) return NULL;

    for (int i = 0; i < ctx->layer_count; i++) {
        if (ctx->layers[i]->id == layer_id) {
            return ctx->layers[i];
        }
    }

    return NULL;
}

static int layer_compare(const void* a, const void* b) {
    layer_t* layer_a = *(layer_t**)a;
    layer_t* layer_b = *(layer_t**)b;
    return layer_a->z_index - layer_b->z_index;
}

void ipc_sort_layers(ipc_context_t* ctx) {
    if (!ctx || ctx->layer_count < 2) return;
    qsort(ctx->layers, ctx->layer_count, sizeof(layer_t*), layer_compare);
}

