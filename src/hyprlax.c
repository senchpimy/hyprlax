#define _GNU_SOURCE
#define HYPRLAX_VERSION "1.0.0"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <getopt.h>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "../protocols/xdg-shell-client-protocol.h"
#include "../protocols/wlr-layer-shell-client-protocol.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Easing functions
typedef enum {
    EASE_LINEAR,
    EASE_QUAD_OUT,
    EASE_CUBIC_OUT,
    EASE_QUART_OUT,
    EASE_QUINT_OUT,
    EASE_SINE_OUT,
    EASE_EXPO_OUT,
    EASE_CIRC_OUT,
    EASE_BACK_OUT,
    EASE_ELASTIC_OUT,
    EASE_CUSTOM_SNAP  // Extra snappy custom easing
} easing_type_t;

// Configuration
struct config {
    float shift_per_workspace;
    float animation_duration;
    float animation_delay;  // Delay before starting animation
    float scale_factor;
    easing_type_t easing;
    int target_fps;
    int vsync;
    int debug;
} config = {
    .shift_per_workspace = 200.0f,  // More dramatic shift between workspaces
    .animation_duration = 1.0f,  // Longer duration - user can "feel" it settling
    .animation_delay = 0.0f,
    .scale_factor = 1.5f,  // Increased scale to accommodate larger shifts
    .easing = EASE_EXPO_OUT,  // Exponential ease out - fast then very gentle settling
    .target_fps = 144,
    .vsync = 1,
    .debug = 0
};

// Global state
struct {
    // Wayland objects
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_output *output;
    struct zwlr_layer_shell_v1 *layer_shell;
    struct wl_surface *surface;
    struct zwlr_layer_surface_v1 *layer_surface;
    struct wl_callback *frame_callback;
    
    // EGL/OpenGL
    struct wl_egl_window *egl_window;
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
    GLuint texture;
    GLuint shader_program;
    GLuint vbo, ebo;
    
    // Window dimensions
    int width, height;
    
    // Image data
    int img_width, img_height;
    
    // Animation state
    float current_offset;
    float target_offset;
    float start_offset;
    double animation_start;
    int animating;
    int current_workspace;
    
    // Performance tracking
    double last_frame_time;
    int frame_count;
    double fps_timer;
    
    // Hyprland IPC
    int ipc_fd;
    
    // Running state
    int running;
} state = {0};

// Easing functions implementation
float apply_easing(float t, easing_type_t type) {
    switch (type) {
        case EASE_LINEAR:
            return t;
        
        case EASE_QUAD_OUT:
            return 1.0f - (1.0f - t) * (1.0f - t);
        
        case EASE_CUBIC_OUT:
            return 1.0f - powf(1.0f - t, 3.0f);
        
        case EASE_QUART_OUT:
            return 1.0f - powf(1.0f - t, 4.0f);
        
        case EASE_QUINT_OUT:
            return 1.0f - powf(1.0f - t, 5.0f);
        
        case EASE_SINE_OUT:
            return sinf((t * M_PI) / 2.0f);
        
        case EASE_EXPO_OUT:
            return t == 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t);
        
        case EASE_CIRC_OUT:
            return sqrtf(1.0f - powf(t - 1.0f, 2.0f));
        
        case EASE_BACK_OUT: {
            float c1 = 1.70158f;
            float c3 = c1 + 1.0f;
            return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
        }
        
        case EASE_ELASTIC_OUT: {
            float c4 = (2.0f * M_PI) / 3.0f;
            return t == 0 ? 0 : t == 1 ? 1 : powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
        }
        
        case EASE_CUSTOM_SNAP: {
            // Custom extra snappy easing - fast start, very quick deceleration at the end
            if (t < 0.4f) {
                // Accelerate quickly for first 40% of time
                return 1.0f - powf(1.0f - (t * 2.5f), 6.0f);
            } else {
                // Then ease out more gently
                return 1.0f - powf(1.0f - t, 8.0f);
            }
        }
        
        default:
            return t;
    }
}

// Shader sources with better precision
const char *vertex_shader_src = 
    "precision highp float;\n"
    "attribute vec2 position;\n"
    "attribute vec2 texcoord;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "    v_texcoord = texcoord;\n"
    "}\n";

const char *fragment_shader_src = 
    "precision highp float;\n"
    "varying vec2 v_texcoord;\n"
    "uniform sampler2D texture;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(texture, v_texcoord);\n"
    "}\n";

// Helper: Get time in seconds with high precision
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

// Helper: Compile shader
GLuint compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "Shader compilation failed: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

// Initialize OpenGL with optimizations
int init_gl() {
    // Create shader program
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
    
    if (!vertex_shader || !fragment_shader) return -1;
    
    state.shader_program = glCreateProgram();
    glAttachShader(state.shader_program, vertex_shader);
    glAttachShader(state.shader_program, fragment_shader);
    glLinkProgram(state.shader_program);
    
    GLint status;
    glGetProgramiv(state.shader_program, GL_LINK_STATUS, &status);
    if (!status) {
        fprintf(stderr, "Shader linking failed\n");
        return -1;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    glUseProgram(state.shader_program);
    
    // Create VBO and EBO for better performance
    glGenBuffers(1, &state.vbo);
    glGenBuffers(1, &state.ebo);
    
    // Set up static index buffer
    GLushort indices[] = {0, 1, 2, 0, 2, 3};
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Disable depth testing and blending for better performance
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    
    return 0;
}

// Load image as texture with mipmaps
int load_image(const char *path) {
    int channels;
    unsigned char *data = stbi_load(path, &state.img_width, &state.img_height, &channels, 4);
    if (!data) {
        fprintf(stderr, "Failed to load image: %s\n", path);
        return -1;
    }
    
    glGenTextures(1, &state.texture);
    glBindTexture(GL_TEXTURE_2D, state.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, state.img_width, state.img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    // Use trilinear filtering for smoother animation
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Calculate proper scale factor based on max workspaces and shift distance
    // Scale factor determines how much larger the image is than the viewport
    // We need: image_width = viewport_width * scale_factor
    // And: (scale_factor - 1) * viewport_width >= total_shift_needed
    int max_workspaces = 10;
    float total_shift_needed = (max_workspaces - 1) * config.shift_per_workspace;
    
    // Initialize state.width if not set (use default screen width)
    if (state.width == 0) {
        state.width = 1920;  // Default, will be updated when surface is configured
    }
    
    float min_scale_factor = 1.0f + (total_shift_needed / (float)state.width);
    
    // Use the larger of the configured scale or the minimum required
    if (config.scale_factor < min_scale_factor) {
        config.scale_factor = min_scale_factor;
        if (config.debug) {
            printf("Adjusted scale factor to %.2f to accommodate %d workspaces with %.0fpx shifts\n", 
                   config.scale_factor, max_workspaces, config.shift_per_workspace);
        }
    }
    
    stbi_image_free(data);
    
    return 0;
}

// Forward declaration
void render_frame();

// Frame callback for smooth animation
static void frame_done(void *data, struct wl_callback *callback, uint32_t time) {
    (void)data;
    (void)time;
    if (callback) wl_callback_destroy(callback);
    state.frame_callback = NULL;
    
    // Render the next frame
    render_frame();
}

static const struct wl_callback_listener frame_listener = {
    .done = frame_done
};

// Render frame with optimizations
void render_frame() {
    double current_time = get_time();
    
    // Update animation
    if (state.animating) {
        double elapsed = current_time - state.animation_start;
        
        if (elapsed >= config.animation_duration) {
            state.current_offset = state.target_offset;
            state.animating = 0;
        } else {
            float t = elapsed / config.animation_duration;
            float eased = apply_easing(t, config.easing);
            state.current_offset = state.start_offset + (state.target_offset - state.start_offset) * eased;
        }
    }
    
    // Clear
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Calculate how much of the image we show (viewport width in texture space)
    float viewport_width_in_texture = 1.0f / config.scale_factor;
    
    // Calculate the offset in texture space (0.0 to 1.0)
    // Total texture space available for panning = 1.0 - viewport_width_in_texture
    float max_texture_offset = 1.0f - viewport_width_in_texture;
    
    // Map pixel offset to texture coordinate offset
    float max_pixel_offset = (config.scale_factor - 1.0f) * state.width;
    float tex_offset = 0.0f;
    
    if (max_pixel_offset > 0) {
        tex_offset = (state.current_offset / max_pixel_offset) * max_texture_offset;
    }
    
    // Clamp to valid range
    if (tex_offset > max_texture_offset) {
        tex_offset = max_texture_offset;
    }
    if (tex_offset < 0.0f) {
        tex_offset = 0.0f;
    }
    
    // Update vertex buffer
    float vertices[] = {
        -1.0f, -1.0f,  tex_offset, 1.0f,
         1.0f, -1.0f,  tex_offset + viewport_width_in_texture, 1.0f,
         1.0f,  1.0f,  tex_offset + viewport_width_in_texture, 0.0f,
        -1.0f,  1.0f,  tex_offset, 0.0f,
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, state.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    
    // Set up vertex attributes
    GLint pos_attrib = glGetAttribLocation(state.shader_program, "position");
    GLint tex_attrib = glGetAttribLocation(state.shader_program, "texcoord");
    
    glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(pos_attrib);
    
    glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(tex_attrib);
    
    // Draw
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.ebo);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    
    // Swap buffers with vsync control
    if (config.vsync) {
        eglSwapInterval(state.egl_display, 1);
    } else {
        eglSwapInterval(state.egl_display, 0);
    }
    eglSwapBuffers(state.egl_display, state.egl_surface);
    
    // FPS tracking for debug
    if (config.debug) {
        state.frame_count++;
        if (current_time - state.fps_timer >= 1.0) {
            printf("FPS: %d\n", state.frame_count);
            state.frame_count = 0;
            state.fps_timer = current_time;
        }
    }
    
    // Request next frame if animating
    if (state.animating && !state.frame_callback) {
        state.frame_callback = wl_surface_frame(state.surface);
        wl_callback_add_listener(state.frame_callback, &frame_listener, NULL);
        wl_surface_commit(state.surface);
    }
    
    state.last_frame_time = current_time;
}

// Connect to Hyprland IPC
int connect_hyprland_ipc() {
    const char *sig = getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (!sig) {
        fprintf(stderr, "Not running under Hyprland\n");
        return -1;
    }
    
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (!runtime_dir) {
        runtime_dir = "/run/user/1000";
    }
    
    char socket_path[256];
    snprintf(socket_path, sizeof(socket_path), "%s/hypr/%s/.socket2.sock", runtime_dir, sig);
    
    state.ipc_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (state.ipc_fd < 0) return -1;
    
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    if (connect(state.ipc_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(state.ipc_fd);
        return -1;
    }
    
    // Make non-blocking
    fcntl(state.ipc_fd, F_SETFL, O_NONBLOCK);
    
    return 0;
}

// Process Hyprland IPC events
void process_ipc_events() {
    char buffer[1024];
    ssize_t n = read(state.ipc_fd, buffer, sizeof(buffer) - 1);
    
    if (n > 0) {
        buffer[n] = '\0';
        
        // Parse workspace events
        char *line = strtok(buffer, "\n");
        while (line) {
            if (strncmp(line, "workspace>>", 11) == 0) {
                int workspace = atoi(line + 11);
                if (workspace != state.current_workspace) {
                    // If already animating, interrupt and start new animation from current position
                    if (state.animating) {
                        // Calculate current position in the interrupted animation
                        double elapsed = get_time() - state.animation_start;
                        float t = fminf(elapsed / config.animation_duration, 1.0f);
                        float eased = apply_easing(t, config.easing);
                        state.current_offset = state.start_offset + (state.target_offset - state.start_offset) * eased;
                    }
                    
                    // Start new animation from current position
                    state.start_offset = state.current_offset;
                    state.target_offset = (workspace - 1) * config.shift_per_workspace;
                    state.animation_start = get_time() + config.animation_delay;  // Add delay
                    state.animating = 1;
                    state.current_workspace = workspace;
                    
                    if (config.debug) {
                        printf("Workspace changed to %d (offset: %.2f -> %.2f)\n", 
                               workspace, state.start_offset, state.target_offset);
                    }
                    
                    // Request frame
                    if (!state.frame_callback) {
                        state.frame_callback = wl_surface_frame(state.surface);
                        wl_callback_add_listener(state.frame_callback, &frame_listener, NULL);
                        wl_surface_commit(state.surface);
                    }
                }
            }
            line = strtok(NULL, "\n");
        }
    }
}

// Layer surface configure
static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *layer_surface,
                                    uint32_t serial, uint32_t width, uint32_t height) {
    state.width = width;
    state.height = height;
    
    zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
    
    if (state.egl_window) {
        wl_egl_window_resize(state.egl_window, width, height, 0, 0);
    }
    
    glViewport(0, 0, width, height);
    render_frame();
}

static void layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *layer_surface) {
    state.running = 0;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_configure,
    .closed = layer_surface_closed,
};

// Registry handlers
static void registry_global(void *data, struct wl_registry *registry, uint32_t id,
                           const char *interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state.compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        state.output = wl_registry_bind(registry, id, &wl_output_interface, 1);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        state.layer_shell = wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, 1);
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t id) {}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS] <image_path>\n\n", prog);
    printf("Options:\n");
    printf("  -s, --shift <pixels>     Pixels to shift per workspace (default: 200)\n");
    printf("  -d, --duration <seconds> Animation duration (default: 1.0)\n");
    printf("  --delay <seconds>        Delay before animation starts (default: 0)\n");
    printf("  -e, --easing <type>      Easing function (default: cubic)\n");
    printf("                           Options: linear, quad, cubic, quart, quint,\n");
    printf("                                   sine, expo, circ, back, elastic\n");
    printf("  -f, --scale <factor>     Scale factor for panning room (default: 1.5)\n");
    printf("  -v, --vsync <0|1>        Enable vsync (default: 1)\n");
    printf("  --fps <rate>             Target FPS (default: 144)\n");
    printf("  --debug                  Enable debug output\n");
    printf("  --version                Show version information\n");
    printf("  -h, --help               Show this help\n");
}

void print_version() {
    printf("hyprlax %s\n", HYPRLAX_VERSION);
    printf("Smooth parallax wallpaper animations for Hyprland\n");
}

int main(int argc, char *argv[]) {
    // Parse arguments
    static struct option long_options[] = {
        {"shift", required_argument, 0, 's'},
        {"duration", required_argument, 0, 'd'},
        {"delay", required_argument, 0, 0},
        {"easing", required_argument, 0, 'e'},
        {"scale", required_argument, 0, 'f'},
        {"vsync", required_argument, 0, 'v'},
        {"fps", required_argument, 0, 0},
        {"debug", no_argument, 0, 0},
        {"version", no_argument, 0, 0},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "s:d:e:f:v:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 's':
                config.shift_per_workspace = atof(optarg);
                break;
            case 'd':
                config.animation_duration = atof(optarg);
                break;
            case 'e':
                if (strcmp(optarg, "linear") == 0) config.easing = EASE_LINEAR;
                else if (strcmp(optarg, "quad") == 0) config.easing = EASE_QUAD_OUT;
                else if (strcmp(optarg, "cubic") == 0) config.easing = EASE_CUBIC_OUT;
                else if (strcmp(optarg, "quart") == 0) config.easing = EASE_QUART_OUT;
                else if (strcmp(optarg, "quint") == 0) config.easing = EASE_QUINT_OUT;
                else if (strcmp(optarg, "sine") == 0) config.easing = EASE_SINE_OUT;
                else if (strcmp(optarg, "expo") == 0) config.easing = EASE_EXPO_OUT;
                else if (strcmp(optarg, "circ") == 0) config.easing = EASE_CIRC_OUT;
                else if (strcmp(optarg, "back") == 0) config.easing = EASE_BACK_OUT;
                else if (strcmp(optarg, "elastic") == 0) config.easing = EASE_ELASTIC_OUT;
                else if (strcmp(optarg, "snap") == 0) config.easing = EASE_CUSTOM_SNAP;
                break;
            case 'f':
                config.scale_factor = atof(optarg);
                break;
            case 'v':
                config.vsync = atoi(optarg);
                break;
            case 0:
                if (strcmp(long_options[option_index].name, "fps") == 0) {
                    config.target_fps = atoi(optarg);
                } else if (strcmp(long_options[option_index].name, "delay") == 0) {
                    config.animation_delay = atof(optarg);
                } else if (strcmp(long_options[option_index].name, "debug") == 0) {
                    config.debug = 1;
                } else if (strcmp(long_options[option_index].name, "version") == 0) {
                    print_version();
                    return 0;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    if (optind >= argc) {
        fprintf(stderr, "Error: Image path required\n");
        print_usage(argv[0]);
        return 1;
    }
    
    const char *image_path = argv[optind];
    
    // Print config if debug
    if (config.debug) {
        printf("Configuration:\n");
        printf("  Image: %s\n", image_path);
        printf("  Shift: %.1f pixels/workspace\n", config.shift_per_workspace);
        printf("  Duration: %.2f seconds\n", config.animation_duration);
        printf("  Scale factor: %.2f\n", config.scale_factor);
        printf("  VSync: %s\n", config.vsync ? "enabled" : "disabled");
        printf("  Target FPS: %d\n", config.target_fps);
    }
    
    // Connect to Wayland
    state.display = wl_display_connect(NULL);
    if (!state.display) {
        fprintf(stderr, "Failed to connect to Wayland\n");
        return 1;
    }
    
    // Get registry and bind interfaces
    state.registry = wl_display_get_registry(state.display);
    wl_registry_add_listener(state.registry, &registry_listener, NULL);
    wl_display_roundtrip(state.display);
    
    if (!state.compositor || !state.layer_shell) {
        fprintf(stderr, "Missing required Wayland interfaces\n");
        return 1;
    }
    
    // Create surface
    state.surface = wl_compositor_create_surface(state.compositor);
    
    // Create layer surface
    state.layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        state.layer_shell, state.surface, state.output,
        ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND, "hyprlax");
    
    zwlr_layer_surface_v1_add_listener(state.layer_surface, &layer_surface_listener, NULL);
    zwlr_layer_surface_v1_set_exclusive_zone(state.layer_surface, -1);
    zwlr_layer_surface_v1_set_anchor(state.layer_surface,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    
    wl_surface_commit(state.surface);
    wl_display_roundtrip(state.display);
    
    // Initialize EGL
    state.egl_display = eglGetDisplay((EGLNativeDisplayType)state.display);
    eglInitialize(state.egl_display, NULL, NULL);
    
    EGLint attributes[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    
    EGLConfig config_egl;
    EGLint num_configs;
    eglChooseConfig(state.egl_display, attributes, &config_egl, 1, &num_configs);
    
    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    
    state.egl_context = eglCreateContext(state.egl_display, config_egl, EGL_NO_CONTEXT, context_attribs);
    
    // Create EGL window
    state.egl_window = wl_egl_window_create(state.surface, 1920, 1080);
    state.egl_surface = eglCreateWindowSurface(state.egl_display, config_egl, state.egl_window, NULL);
    
    eglMakeCurrent(state.egl_display, state.egl_surface, state.egl_surface, state.egl_context);
    
    // Initialize OpenGL
    if (init_gl() < 0) return 1;
    
    // Load image
    if (load_image(image_path) < 0) return 1;
    
    // Connect to Hyprland IPC
    if (connect_hyprland_ipc() < 0) {
        fprintf(stderr, "Warning: Failed to connect to Hyprland IPC\n");
    }
    
    // Get initial workspace
    state.current_workspace = 1;
    state.current_offset = 0;
    state.target_offset = 0;
    
    // Initialize timers
    state.last_frame_time = get_time();
    state.fps_timer = state.last_frame_time;
    
    // Main loop
    state.running = 1;
    
    struct pollfd fds[2];
    fds[0].fd = wl_display_get_fd(state.display);
    fds[0].events = POLLIN;
    fds[1].fd = state.ipc_fd;
    fds[1].events = POLLIN;
    
    while (state.running) {
        // Dispatch Wayland events
        wl_display_dispatch_pending(state.display);
        wl_display_flush(state.display);
        
        // Calculate timeout for target FPS
        int timeout = -1;
        if (state.animating) {
            double frame_time = 1.0 / config.target_fps;
            double elapsed = get_time() - state.last_frame_time;
            timeout = (int)((frame_time - elapsed) * 1000);
            if (timeout < 0) timeout = 0;
        }
        
        // Poll for events
        if (poll(fds, 2, timeout) > 0) {
            if (fds[0].revents & POLLIN) {
                wl_display_dispatch(state.display);
            }
            if (fds[1].revents & POLLIN) {
                process_ipc_events();
            }
        }
        
        // Render if animating and enough time has passed
        if (state.animating) {
            double current_time = get_time();
            double frame_time = 1.0 / config.target_fps;
            if (current_time - state.last_frame_time >= frame_time) {
                render_frame();
            }
        }
    }
    
    // Cleanup
    if (state.frame_callback) wl_callback_destroy(state.frame_callback);
    if (state.texture) glDeleteTextures(1, &state.texture);
    if (state.shader_program) glDeleteProgram(state.shader_program);
    if (state.vbo) glDeleteBuffers(1, &state.vbo);
    if (state.ebo) glDeleteBuffers(1, &state.ebo);
    
    eglDestroySurface(state.egl_display, state.egl_surface);
    eglDestroyContext(state.egl_display, state.egl_context);
    eglTerminate(state.egl_display);
    
    wl_egl_window_destroy(state.egl_window);
    zwlr_layer_surface_v1_destroy(state.layer_surface);
    wl_surface_destroy(state.surface);
    
    if (state.ipc_fd >= 0) close(state.ipc_fd);
    
    wl_display_disconnect(state.display);
    
    return 0;
}