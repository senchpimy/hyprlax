/*
 * hyprlax-ctl - Command-line client for hyprlax IPC
 * 
 * Usage:
 *   hyprlax-ctl add <image> [scale=1.0] [opacity=1.0] [x=0] [y=0] [z=0]
 *   hyprlax-ctl remove <id>
 *   hyprlax-ctl modify <id> <property> <value>
 *   hyprlax-ctl list
 *   hyprlax-ctl clear
 *   hyprlax-ctl status
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pwd.h>

#define IPC_SOCKET_PATH_PREFIX "/tmp/hyprlax-"
#define IPC_MAX_MESSAGE_SIZE 4096

static void get_socket_path(char* buffer, size_t size) {
    const char* user = getenv("USER");
    if (!user) {
        struct passwd* pw = getpwuid(getuid());
        user = pw ? pw->pw_name : "unknown";
    }
    snprintf(buffer, size, "%s%s.sock", IPC_SOCKET_PATH_PREFIX, user);
}

static void print_usage(const char* prog) {
    printf("Usage:\n");
    printf("  %s add <image> [scale=N] [opacity=N] [x=N] [y=N] [z=N]\n", prog);
    printf("  %s remove|rm <id>\n", prog);
    printf("  %s modify|mod <id> <property> <value>\n", prog);
    printf("  %s list|ls\n", prog);
    printf("  %s clear\n", prog);
    printf("  %s status\n", prog);
    printf("\nExamples:\n");
    printf("  %s add /path/to/image.png scale=1.5 opacity=0.8\n", prog);
    printf("  %s modify 1 opacity 0.5\n", prog);
    printf("  %s remove 1\n", prog);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Get socket path
    char socket_path[256];
    get_socket_path(socket_path, sizeof(socket_path));
    
    // Create socket
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Failed to create socket");
        return 1;
    }
    
    // Connect to hyprlax IPC socket
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Failed to connect to hyprlax IPC socket at %s\n", socket_path);
        fprintf(stderr, "Is hyprlax running?\n");
        close(sock);
        return 1;
    }
    
    // Build command string
    char command[IPC_MAX_MESSAGE_SIZE];
    int offset = 0;
    
    for (int i = 1; i < argc; i++) {
        int written = snprintf(command + offset, sizeof(command) - offset, 
                               "%s%s", (i > 1 ? " " : ""), argv[i]);
        if (written < 0 || offset + written >= sizeof(command)) {
            fprintf(stderr, "Command too long\n");
            close(sock);
            return 1;
        }
        offset += written;
    }
    
    // Send command
    if (send(sock, command, strlen(command), 0) < 0) {
        perror("Failed to send command");
        close(sock);
        return 1;
    }
    
    // Receive response
    char response[IPC_MAX_MESSAGE_SIZE];
    ssize_t bytes = recv(sock, response, sizeof(response) - 1, 0);
    if (bytes > 0) {
        response[bytes] = '\0';
        printf("%s", response);
    }
    
    close(sock);
    return 0;
}