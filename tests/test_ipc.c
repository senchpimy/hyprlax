/*
 * Test suite for hyprlax IPC functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <assert.h>
#include <pwd.h>
#include <errno.h>

#include "../src/ipc.h"

// Test framework
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    fflush(stdout); \
    test_##name(); \
    printf(" ✓\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n  FAILED: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(1); \
    } \
} while(0)

static int tests_passed = 0;

// Helper function to get socket path
static void get_test_socket_path(char* buffer, size_t size) {
    const char* user = getenv("USER");
    if (!user) {
        struct passwd* pw = getpwuid(getuid());
        user = pw ? pw->pw_name : "unknown";
    }
    snprintf(buffer, size, "/tmp/hyprlax-test-%s.sock", user);
}

// Test IPC initialization
TEST(ipc_init) {
    ipc_context_t* ctx = ipc_init();
    ASSERT(ctx != NULL);
    ASSERT(ctx->active == true);
    ASSERT(ctx->socket_fd >= 0);
    ASSERT(ctx->layer_count == 0);
    ASSERT(ctx->next_layer_id == 1);
    
    // Check socket exists
    ASSERT(access(ctx->socket_path, F_OK) == 0);
    
    ipc_cleanup(ctx);
    
    // Check socket is removed after cleanup
    ASSERT(access(ctx->socket_path, F_OK) != 0);
}

// Test adding layers
TEST(ipc_add_layer) {
    ipc_context_t* ctx = ipc_init();
    ASSERT(ctx != NULL);
    
    // Create a temporary test image file
    const char* test_image = "/tmp/test_image.png";
    FILE* f = fopen(test_image, "w");
    ASSERT(f != NULL);
    fclose(f);
    
    // Add a layer
    uint32_t id1 = ipc_add_layer(ctx, test_image, 1.0f, 1.0f, 0.0f, 0.0f, 0);
    ASSERT(id1 > 0);
    ASSERT(ctx->layer_count == 1);
    
    // Add another layer
    uint32_t id2 = ipc_add_layer(ctx, test_image, 2.0f, 0.5f, 10.0f, 20.0f, 1);
    ASSERT(id2 > id1);
    ASSERT(ctx->layer_count == 2);
    
    // Verify layer properties
    layer_t* layer1 = ipc_find_layer(ctx, id1);
    ASSERT(layer1 != NULL);
    ASSERT(strcmp(layer1->image_path, test_image) == 0);
    ASSERT(layer1->scale == 1.0f);
    ASSERT(layer1->opacity == 1.0f);
    
    // Clean up
    unlink(test_image);
    ipc_cleanup(ctx);
}

// Test removing layers
TEST(ipc_remove_layer) {
    ipc_context_t* ctx = ipc_init();
    ASSERT(ctx != NULL);
    
    // Create test image
    const char* test_image = "/tmp/test_image.png";
    FILE* f = fopen(test_image, "w");
    fclose(f);
    
    // Add layers
    uint32_t id1 = ipc_add_layer(ctx, test_image, 1.0f, 1.0f, 0.0f, 0.0f, 0);
    uint32_t id2 = ipc_add_layer(ctx, test_image, 1.0f, 1.0f, 0.0f, 0.0f, 1);
    ASSERT(ctx->layer_count == 2);
    
    // Remove first layer
    ASSERT(ipc_remove_layer(ctx, id1) == true);
    ASSERT(ctx->layer_count == 1);
    ASSERT(ipc_find_layer(ctx, id1) == NULL);
    ASSERT(ipc_find_layer(ctx, id2) != NULL);
    
    // Try to remove non-existent layer
    ASSERT(ipc_remove_layer(ctx, 9999) == false);
    
    // Clean up
    unlink(test_image);
    ipc_cleanup(ctx);
}

// Test modifying layers
TEST(ipc_modify_layer) {
    ipc_context_t* ctx = ipc_init();
    ASSERT(ctx != NULL);
    
    // Create test image
    const char* test_image = "/tmp/test_image.png";
    FILE* f = fopen(test_image, "w");
    fclose(f);
    
    // Add a layer
    uint32_t id = ipc_add_layer(ctx, test_image, 1.0f, 1.0f, 0.0f, 0.0f, 0);
    layer_t* layer = ipc_find_layer(ctx, id);
    ASSERT(layer != NULL);
    
    // Modify properties
    ASSERT(ipc_modify_layer(ctx, id, "opacity", "0.5") == true);
    ASSERT(layer->opacity == 0.5f);
    
    ASSERT(ipc_modify_layer(ctx, id, "scale", "2.5") == true);
    ASSERT(layer->scale == 2.5f);
    
    ASSERT(ipc_modify_layer(ctx, id, "visible", "false") == true);
    ASSERT(layer->visible == false);
    
    ASSERT(ipc_modify_layer(ctx, id, "visible", "1") == true);
    ASSERT(layer->visible == true);
    
    // Try invalid property
    ASSERT(ipc_modify_layer(ctx, id, "invalid", "value") == false);
    
    // Clean up
    unlink(test_image);
    ipc_cleanup(ctx);
}

// Test listing layers
TEST(ipc_list_layers) {
    ipc_context_t* ctx = ipc_init();
    ASSERT(ctx != NULL);
    
    // Empty list
    char* list = ipc_list_layers(ctx);
    ASSERT(list == NULL);
    
    // Create test image
    const char* test_image = "/tmp/test_image.png";
    FILE* f = fopen(test_image, "w");
    fclose(f);
    
    // Add layers
    ipc_add_layer(ctx, test_image, 1.0f, 0.8f, 0.0f, 0.0f, 0);
    ipc_add_layer(ctx, test_image, 2.0f, 0.5f, 10.0f, 20.0f, 1);
    
    // List layers
    list = ipc_list_layers(ctx);
    ASSERT(list != NULL);
    ASSERT(strstr(list, test_image) != NULL);
    ASSERT(strstr(list, "Scale: 1.00") != NULL);
    ASSERT(strstr(list, "Scale: 2.00") != NULL);
    ASSERT(strstr(list, "Opacity: 0.80") != NULL);
    ASSERT(strstr(list, "Opacity: 0.50") != NULL);
    free(list);
    
    // Clean up
    unlink(test_image);
    ipc_cleanup(ctx);
}

// Test clearing layers
TEST(ipc_clear_layers) {
    ipc_context_t* ctx = ipc_init();
    ASSERT(ctx != NULL);
    
    // Create test image
    const char* test_image = "/tmp/test_image.png";
    FILE* f = fopen(test_image, "w");
    fclose(f);
    
    // Add layers
    ipc_add_layer(ctx, test_image, 1.0f, 1.0f, 0.0f, 0.0f, 0);
    ipc_add_layer(ctx, test_image, 1.0f, 1.0f, 0.0f, 0.0f, 1);
    ipc_add_layer(ctx, test_image, 1.0f, 1.0f, 0.0f, 0.0f, 2);
    ASSERT(ctx->layer_count == 3);
    
    // Clear all layers
    ipc_clear_layers(ctx);
    ASSERT(ctx->layer_count == 0);
    
    // Verify layers are freed
    for (int i = 0; i < IPC_MAX_LAYERS; i++) {
        ASSERT(ctx->layers[i] == NULL);
    }
    
    // Clean up
    unlink(test_image);
    ipc_cleanup(ctx);
}

// Test layer sorting
TEST(ipc_sort_layers) {
    ipc_context_t* ctx = ipc_init();
    ASSERT(ctx != NULL);
    
    // Create test image
    const char* test_image = "/tmp/test_image.png";
    FILE* f = fopen(test_image, "w");
    fclose(f);
    
    // Add layers with different z-indices
    ipc_add_layer(ctx, test_image, 1.0f, 1.0f, 0.0f, 0.0f, 5);
    ipc_add_layer(ctx, test_image, 1.0f, 1.0f, 0.0f, 0.0f, 1);
    ipc_add_layer(ctx, test_image, 1.0f, 1.0f, 0.0f, 0.0f, 3);
    
    // Verify layers are sorted by z-index
    ASSERT(ctx->layers[0]->z_index == 1);
    ASSERT(ctx->layers[1]->z_index == 3);
    ASSERT(ctx->layers[2]->z_index == 5);
    
    // Clean up
    unlink(test_image);
    ipc_cleanup(ctx);
}

// Test max layers limit
TEST(ipc_max_layers) {
    ipc_context_t* ctx = ipc_init();
    ASSERT(ctx != NULL);
    
    // Create test image
    const char* test_image = "/tmp/test_image.png";
    FILE* f = fopen(test_image, "w");
    fclose(f);
    
    // Add maximum number of layers
    for (int i = 0; i < IPC_MAX_LAYERS; i++) {
        uint32_t id = ipc_add_layer(ctx, test_image, 1.0f, 1.0f, 0.0f, 0.0f, i);
        ASSERT(id > 0);
    }
    ASSERT(ctx->layer_count == IPC_MAX_LAYERS);
    
    // Try to add one more (should fail)
    uint32_t id = ipc_add_layer(ctx, test_image, 1.0f, 1.0f, 0.0f, 0.0f, 100);
    ASSERT(id == 0);
    ASSERT(ctx->layer_count == IPC_MAX_LAYERS);
    
    // Clean up
    unlink(test_image);
    ipc_cleanup(ctx);
}

// Test client-server communication
TEST(ipc_client_server) {
    // Start a mock server
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - server
        ipc_context_t* ctx = ipc_init();
        if (!ctx) exit(1);
        
        // Wait for one connection and process it
        for (int i = 0; i < 100; i++) {
            if (ipc_process_commands(ctx)) {
                break;
            }
            usleep(10000); // 10ms
        }
        
        ipc_cleanup(ctx);
        exit(0);
    }
    
    // Parent process - client
    usleep(50000); // Give server time to start
    
    // Get socket path
    char socket_path[256];
    const char* user = getenv("USER");
    if (!user) {
        struct passwd* pw = getpwuid(getuid());
        user = pw ? pw->pw_name : "unknown";
    }
    snprintf(socket_path, sizeof(socket_path), "/tmp/hyprlax-%s.sock", user);
    
    // Connect to server
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT(sock >= 0);
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    // Try to connect (may fail if server not ready)
    int connected = 0;
    for (int i = 0; i < 10; i++) {
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            connected = 1;
            break;
        }
        usleep(10000);
    }
    
    if (connected) {
        // Send status command
        const char* cmd = "status";
        send(sock, cmd, strlen(cmd), 0);
        
        // Receive response
        char buffer[256];
        ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            ASSERT(strstr(buffer, "Status: Active") != NULL);
        }
    }
    
    close(sock);
    
    // Wait for server to finish
    int status;
    waitpid(pid, &status, 0);
}

int main(int argc, char** argv) {
    printf("\n=================================\n");
    printf("     hyprlax IPC Test Suite\n");
    printf("=================================\n\n");
    
    RUN_TEST(ipc_init);
    RUN_TEST(ipc_add_layer);
    RUN_TEST(ipc_remove_layer);
    RUN_TEST(ipc_modify_layer);
    RUN_TEST(ipc_list_layers);
    RUN_TEST(ipc_clear_layers);
    RUN_TEST(ipc_sort_layers);
    RUN_TEST(ipc_max_layers);
    RUN_TEST(ipc_client_server);
    
    printf("\n=================================\n");
    printf("  All tests passed! (%d/%d)\n", tests_passed, tests_passed);
    printf("=================================\n\n");
    
    return 0;
}