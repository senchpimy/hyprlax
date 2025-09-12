#define _GNU_SOURCE
#define HYPRLAX_VERSION "1.2.0"
#define MAX_LAYERS 8

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

// Layer structure for multi-layer parallax
struct layer {
    GLuint texture;
    int width, height;
    float shift_multiplier;  // How much this layer moves relative to base (0.0 = static, 1.0 = normal, 2.0 = double)
    float opacity;           // Layer opacity (0.0 - 1.0)
    char *image_path;
    
    // Per-layer animation state
    float current_offset;
    float target_offset;
    float start_offset;
    
    // Phase 3: Advanced per-layer settings
    easing_type_t easing;    // Per-layer easing function
    float animation_delay;   // Per-layer animation delay
    float animation_duration; // Per-layer animation duration
    double animation_start;  // When this layer's animation started
    int animating;          // Is this layer currently animating
    float blur_amount;      // Blur amount for depth (0.0 = no blur)
};

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
    int multi_layer_mode;  // Whether we're using multiple layers
} config = {
    .shift_per_workspace = 200.0f,  // More dramatic shift between workspaces
    .animation_duration = 1.0f,  // Longer duration - user can "feel" it settling
    .animation_delay = 0.0f,
    .scale_factor = 1.5f,  // Increased scale to accommodate larger shifts
    .easing = EASE_EXPO_OUT,  // Exponential ease out - fast then very gentle settling
    .target_fps = 144,
    .vsync = 1,
    .debug = 0,
    .multi_layer_mode = 0
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
    GLuint texture;  // Single texture for backward compatibility
    GLuint shader_program;
    GLuint blur_shader_program;  // Blur shader for depth effects
    GLuint vbo, ebo;
    
    // Standard shader uniforms
    GLint u_opacity;  // Uniform location for opacity
    GLint u_texture;  // Uniform location for texture
    
    // Blur shader uniforms
    GLint blur_u_texture;  // Uniform location for texture in blur shader
    GLint blur_u_opacity;  // Uniform location for opacity in blur shader
    GLint blur_u_blur_amount;  // Uniform location for blur amount
    GLint blur_u_resolution;  // Uniform location for resolution
    
    // Window dimensions
    int width, height;
    
    // Image data (for single layer mode)
    int img_width, img_height;
    
    // Multi-layer support
    struct layer *layers;
    int layer_count;
    int max_layers;
    
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
    "uniform sampler2D u_texture;\n"
    "uniform float u_opacity;\n"
    "void main() {\n"
    "    vec4 color = texture2D(u_texture, v_texcoord);\n"
    "    gl_FragColor = vec4(color.rgb, color.a * u_opacity);\n"
    "}\n";

// Blur fragment shader for depth perception
const char *blur_fragment_shader_src = 
    "precision highp float;\n"
    "varying vec2 v_texcoord;\n"
    "uniform sampler2D u_texture;\n"
    "uniform float u_opacity;\n"
    "uniform float u_blur_amount;\n"
    "uniform vec2 u_resolution;\n"
    "void main() {\n"
    "    float blur = u_blur_amount;\n"
    "    if (blur < 0.001) {\n"
    "        vec4 color = texture2D(u_texture, v_texcoord);\n"
    "        gl_FragColor = vec4(color.rgb, color.a * u_opacity);\n"
    "        return;\n"
    "    }\n"
    "    \n"
    "    // 9-tap box blur for performance\n"
    "    vec2 texelSize = 1.0 / u_resolution;\n"
    "    vec4 result = vec4(0.0);\n"
    "    float total = 0.0;\n"
    "    \n"
    "    for (float x = -1.0; x <= 1.0; x += 1.0) {\n"
    "        for (float y = -1.0; y <= 1.0; y += 1.0) {\n"
    "            vec2 offset = vec2(x, y) * texelSize * blur * 3.0;\n"
    "            float weight = 1.0 - length(vec2(x, y)) * 0.2;\n"
    "            result += texture2D(u_texture, v_texcoord + offset) * weight;\n"
    "            total += weight;\n"
    "        }\n"
    "    }\n"
    "    \n"
    "    result /= total;\n"
    "    gl_FragColor = vec4(result.rgb, result.a * u_opacity);\n"
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
    // Create standard shader program
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
    
    // Create blur shader program
    GLuint blur_fragment = compile_shader(GL_FRAGMENT_SHADER, blur_fragment_shader_src);
    if (!blur_fragment) return -1;
    
    state.blur_shader_program = glCreateProgram();
    glAttachShader(state.blur_shader_program, vertex_shader);
    glAttachShader(state.blur_shader_program, blur_fragment);
    glLinkProgram(state.blur_shader_program);
    
    glGetProgramiv(state.blur_shader_program, GL_LINK_STATUS, &status);
    if (!status) {
        fprintf(stderr, "Blur shader linking failed\n");
        return -1;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    glDeleteShader(blur_fragment);
    
    glUseProgram(state.shader_program);
    
    // Get uniform locations for standard shader
    state.u_texture = glGetUniformLocation(state.shader_program, "u_texture");
    state.u_opacity = glGetUniformLocation(state.shader_program, "u_opacity");
    
    // Get uniform locations for blur shader
    glUseProgram(state.blur_shader_program);
    state.blur_u_texture = glGetUniformLocation(state.blur_shader_program, "u_texture");
    state.blur_u_opacity = glGetUniformLocation(state.blur_shader_program, "u_opacity");
    state.blur_u_blur_amount = glGetUniformLocation(state.blur_shader_program, "u_blur_amount");
    state.blur_u_resolution = glGetUniformLocation(state.blur_shader_program, "u_resolution");
    
    // Switch back to standard shader
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

// Load image as layer for multi-layer mode
int load_layer(struct layer *layer, const char *path, float shift_multiplier, float opacity) {
    int channels;
    unsigned char *data = stbi_load(path, &layer->width, &layer->height, &channels, 4);
    if (!data) {
        fprintf(stderr, "Failed to load layer image: %s\n", path);
        return -1;
    }
    
    glGenTextures(1, &layer->texture);
    glBindTexture(GL_TEXTURE_2D, layer->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, layer->width, layer->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    // Use trilinear filtering for smoother animation
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    layer->shift_multiplier = shift_multiplier;
    layer->opacity = opacity;
    layer->image_path = strdup(path);
    layer->current_offset = 0.0f;
    layer->target_offset = 0.0f;
    layer->start_offset = 0.0f;
    
    // Initialize Phase 3 fields with defaults
    layer->easing = config.easing;  // Use global easing by default
    layer->animation_delay = 0.0f;  // Can be overridden per layer
    layer->animation_duration = config.animation_duration;  // Use global duration by default
    layer->animation_start = 0.0;
    layer->animating = 0;
    layer->blur_amount = 0.0f;
    
    stbi_image_free(data);
    
    if (config.debug) {
        printf("Loaded layer: %s (%.0fx%.0f) shift=%.2f opacity=%.2f\n", 
               path, (float)layer->width, (float)layer->height, shift_multiplier, opacity);
    }
    
    return 0;
}

// Add a layer to the state
int add_layer(const char *path, float shift_multiplier, float opacity) {
    if (state.layer_count >= state.max_layers) {
        fprintf(stderr, "Maximum number of layers (%d) reached\n", state.max_layers);
        return -1;
    }
    
    struct layer *layer = &state.layers[state.layer_count];
    if (load_layer(layer, path, shift_multiplier, opacity) == 0) {
        state.layer_count++;
        return 0;
    }
    
    return -1;
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
    
    // Update animation for all layers
    if (config.multi_layer_mode) {
        // Per-layer animation with individual timing
        int any_animating = 0;
        for (int i = 0; i < state.layer_count; i++) {
            struct layer *layer = &state.layers[i];
            
            if (layer->animating) {
                double elapsed = current_time - layer->animation_start;
                
                // Check if we're still in delay period
                if (elapsed < layer->animation_delay) {
                    any_animating = 1;
                    continue;
                }
                
                // Adjust elapsed time for delay
                elapsed -= layer->animation_delay;
                
                if (elapsed >= layer->animation_duration) {
                    // This layer's animation is complete
                    layer->current_offset = layer->target_offset;
                    layer->animating = 0;
                } else {
                    float t = elapsed / layer->animation_duration;
                    float eased = apply_easing(t, layer->easing);
                    layer->current_offset = layer->start_offset + 
                        (layer->target_offset - layer->start_offset) * eased;
                    any_animating = 1;
                }
            }
        }
        state.animating = any_animating;
    } else {
        // Single layer mode (backward compatible)
        if (state.animating) {
            double elapsed = current_time - state.animation_start;
            
            if (elapsed >= config.animation_duration) {
                state.current_offset = state.target_offset;
                state.animating = 0;
            } else {
                float t = elapsed / config.animation_duration;
                float eased = apply_easing(t, config.easing);
                state.current_offset = state.start_offset + 
                    (state.target_offset - state.start_offset) * eased;
            }
        }
    }
    
    // Clear
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Enable blending for multi-layer mode
    if (config.multi_layer_mode && state.layer_count > 1) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    
    // Get attribute locations once
    GLint pos_attrib = glGetAttribLocation(state.shader_program, "position");
    GLint tex_attrib = glGetAttribLocation(state.shader_program, "texcoord");
    
    if (config.multi_layer_mode) {
        // Render each layer
        for (int i = 0; i < state.layer_count; i++) {
            struct layer *layer = &state.layers[i];
            
            // Calculate layer-specific texture offset
            float viewport_width_in_texture = 1.0f / config.scale_factor;
            float max_texture_offset = 1.0f - viewport_width_in_texture;
            float max_pixel_offset = (config.scale_factor - 1.0f) * state.width;
            float tex_offset = 0.0f;
            
            if (max_pixel_offset > 0) {
                tex_offset = (layer->current_offset / max_pixel_offset) * max_texture_offset;
            }
            
            // Clamp to valid range
            if (tex_offset > max_texture_offset) tex_offset = max_texture_offset;
            if (tex_offset < 0.0f) tex_offset = 0.0f;
            
            // Update vertex buffer for this layer
            float vertices[] = {
                -1.0f, -1.0f,  tex_offset, 1.0f,
                 1.0f, -1.0f,  tex_offset + viewport_width_in_texture, 1.0f,
                 1.0f,  1.0f,  tex_offset + viewport_width_in_texture, 0.0f,
                -1.0f,  1.0f,  tex_offset, 0.0f,
            };
            
            glBindBuffer(GL_ARRAY_BUFFER, state.vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
            
            // Set up vertex attributes
            glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(pos_attrib);
            glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
            glEnableVertexAttribArray(tex_attrib);
            
            // Select shader based on blur amount
            if (layer->blur_amount > 0.001f) {
                glUseProgram(state.blur_shader_program);
                
                // Bind layer texture and set uniforms using pre-cached locations
                glBindTexture(GL_TEXTURE_2D, layer->texture);
                glUniform1i(state.blur_u_texture, 0);
                glUniform1f(state.blur_u_opacity, layer->opacity);
                glUniform1f(state.blur_u_blur_amount, layer->blur_amount);
                glUniform2f(state.blur_u_resolution, (float)state.width, (float)state.height);
            } else {
                glUseProgram(state.shader_program);
                
                // Bind layer texture and set uniforms
                glBindTexture(GL_TEXTURE_2D, layer->texture);
                glUniform1i(state.u_texture, 0);
                glUniform1f(state.u_opacity, layer->opacity);
            }
            
            // Draw layer
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.ebo);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
        }
    } else {
        // Single layer mode (backward compatible)
        float viewport_width_in_texture = 1.0f / config.scale_factor;
        float max_texture_offset = 1.0f - viewport_width_in_texture;
        float max_pixel_offset = (config.scale_factor - 1.0f) * state.width;
        float tex_offset = 0.0f;
        
        if (max_pixel_offset > 0) {
            tex_offset = (state.current_offset / max_pixel_offset) * max_texture_offset;
        }
        
        if (tex_offset > max_texture_offset) tex_offset = max_texture_offset;
        if (tex_offset < 0.0f) tex_offset = 0.0f;
        
        float vertices[] = {
            -1.0f, -1.0f,  tex_offset, 1.0f,
             1.0f, -1.0f,  tex_offset + viewport_width_in_texture, 1.0f,
             1.0f,  1.0f,  tex_offset + viewport_width_in_texture, 0.0f,
            -1.0f,  1.0f,  tex_offset, 0.0f,
        };
        
        glBindBuffer(GL_ARRAY_BUFFER, state.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(pos_attrib);
        glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(tex_attrib);
        
        // Bind single texture
        glBindTexture(GL_TEXTURE_2D, state.texture);
        glUniform1i(state.u_texture, 0);
        glUniform1f(state.u_opacity, 1.0f);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.ebo);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    }
    
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
                    if (config.multi_layer_mode) {
                        // Multi-layer mode: update each layer's animation state
                        double now = get_time();
                        
                        // Set new targets for each layer with individual timing
                        float base_target = (workspace - 1) * config.shift_per_workspace;
                        for (int i = 0; i < state.layer_count; i++) {
                            struct layer *layer = &state.layers[i];
                            
                            // If currently animating, update current position
                            if (layer->animating) {
                                double elapsed = now - layer->animation_start - layer->animation_delay;
                                if (elapsed > 0) {
                                    float t = fminf(elapsed / layer->animation_duration, 1.0f);
                                    float eased = apply_easing(t, layer->easing);
                                    layer->current_offset = layer->start_offset + 
                                        (layer->target_offset - layer->start_offset) * eased;
                                }
                            }
                            
                            // Set new animation parameters
                            layer->start_offset = layer->current_offset;
                            layer->target_offset = base_target * layer->shift_multiplier;
                            layer->animation_start = now;
                            layer->animating = 1;
                        }
                    } else {
                        // Single layer mode (backward compatible)
                        if (state.animating) {
                            double elapsed = get_time() - state.animation_start;
                            float t = fminf(elapsed / config.animation_duration, 1.0f);
                            float eased = apply_easing(t, config.easing);
                            state.current_offset = state.start_offset + (state.target_offset - state.start_offset) * eased;
                        }
                        
                        state.start_offset = state.current_offset;
                        state.target_offset = (workspace - 1) * config.shift_per_workspace;
                    }
                    
                    state.animation_start = get_time() + config.animation_delay;
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
    printf("Usage: %s [OPTIONS] <image_path>\n", prog);
    printf("   or: %s [OPTIONS] --layer <image:shift:opacity> [...]\n", prog);
    printf("   or: %s [OPTIONS] --config <config_file>\n\n", prog);
    printf("Options:\n");
    printf("  -s, --shift <pixels>     Pixels to shift per workspace (default: 200)\n");
    printf("  -d, --duration <seconds> Animation duration (default: 1.0)\n");
    printf("  --delay <seconds>        Delay before animation starts (default: 0)\n");
    printf("  -e, --easing <type>      Easing function (default: expo)\n");
    printf("                           Options: linear, quad, cubic, quart, quint,\n");
    printf("                                   sine, expo, circ, back, elastic, snap\n");
    printf("  -f, --scale <factor>     Scale factor for panning room (default: 1.5)\n");
    printf("  -v, --vsync <0|1>        Enable vsync (default: 1)\n");
    printf("  --fps <rate>             Target FPS (default: 144)\n");
    printf("  --debug                  Enable debug output\n");
    printf("  --version                Show version information\n");
    printf("  -h, --help               Show this help\n");
    printf("\nMulti-layer Mode:\n");
    printf("  --layer <spec>           Add a layer with format: image:shift:opacity[:easing[:delay[:duration[:blur]]]]\n");
    printf("                           shift: 0.0=static, 1.0=normal, 2.0=double speed\n");
    printf("                           opacity: 0.0-1.0 (default 1.0)\n");
    printf("                           easing: per-layer easing function (optional)\n");
    printf("                           delay: per-layer animation delay in seconds (optional)\n");
    printf("                           duration: per-layer animation duration (optional)\n");
    printf("                           blur: blur amount for depth (0.0-10.0, default 0.0)\n");
    printf("  --config <file>          Load layers from config file\n");
    printf("\nExamples:\n");
    printf("  # Single image (classic mode)\n");
    printf("  %s wallpaper.jpg\n", prog);
    printf("\n  # Multi-layer parallax\n");
    printf("  %s --layer sky.png:0.3:1.0 --layer mountains.png:0.6:1.0 --layer trees.png:1.0:1.0\n", prog);
    printf("\n  # Multi-layer with blur for depth\n");
    printf("  %s --layer background.png:0.3:1.0:expo:0:1.0:3.0 --layer midground.png:0.6:0.8:expo:0.1:1.0:1.0 --layer foreground.png:1.0:1.0\n", prog);
}

void print_version() {
    printf("hyprlax %s\n", HYPRLAX_VERSION);
    printf("Smooth parallax wallpaper animations for Hyprland\n");
}

// Parse config file
int parse_config_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open config file: %s\n", filename);
        return -1;
    }
    
    char line[512];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        // Remove newline
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        
        // Parse tokens
        char *cmd = strtok(line, " \t");
        if (!cmd) continue;
        
        if (strcmp(cmd, "layer") == 0) {
            char *image = strtok(NULL, " \t");
            char *shift_str = strtok(NULL, " \t");
            char *opacity_str = strtok(NULL, " \t");
            char *blur_str = strtok(NULL, " \t");
            
            if (!image) {
                fprintf(stderr, "Config line %d: layer requires image path\n", line_num);
                continue;
            }
            
            float shift = shift_str ? atof(shift_str) : 1.0f;
            float opacity = opacity_str ? atof(opacity_str) : 1.0f;
            float blur = blur_str ? atof(blur_str) : 0.0f;
            
            // Initialize layer array if needed
            if (!state.layers) {
                state.max_layers = MAX_LAYERS;
                state.layers = calloc(state.max_layers, sizeof(struct layer));
                state.layer_count = 0;
                config.multi_layer_mode = 1;
            }
            
            if (state.layer_count < state.max_layers) {
                struct layer *layer = &state.layers[state.layer_count];
                layer->image_path = strdup(image);
                layer->shift_multiplier = shift;
                layer->opacity = opacity;
                layer->blur_amount = blur;
                // Set defaults for Phase 3 features
                layer->easing = config.easing;
                layer->animation_delay = 0.0f;
                layer->animation_duration = config.animation_duration;
                state.layer_count++;
            }
        } else if (strcmp(cmd, "duration") == 0) {
            char *val = strtok(NULL, " \t");
            if (val) config.animation_duration = atof(val);
        } else if (strcmp(cmd, "shift") == 0) {
            char *val = strtok(NULL, " \t");
            if (val) config.shift_per_workspace = atof(val);
        } else if (strcmp(cmd, "easing") == 0) {
            char *val = strtok(NULL, " \t");
            if (val) {
                if (strcmp(val, "linear") == 0) config.easing = EASE_LINEAR;
                else if (strcmp(val, "quad") == 0) config.easing = EASE_QUAD_OUT;
                else if (strcmp(val, "cubic") == 0) config.easing = EASE_CUBIC_OUT;
                else if (strcmp(val, "quart") == 0) config.easing = EASE_QUART_OUT;
                else if (strcmp(val, "quint") == 0) config.easing = EASE_QUINT_OUT;
                else if (strcmp(val, "sine") == 0) config.easing = EASE_SINE_OUT;
                else if (strcmp(val, "expo") == 0) config.easing = EASE_EXPO_OUT;
                else if (strcmp(val, "circ") == 0) config.easing = EASE_CIRC_OUT;
                else if (strcmp(val, "back") == 0) config.easing = EASE_BACK_OUT;
                else if (strcmp(val, "elastic") == 0) config.easing = EASE_ELASTIC_OUT;
                else if (strcmp(val, "snap") == 0) config.easing = EASE_CUSTOM_SNAP;
            }
        }
    }
    
    fclose(file);
    return 0;
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
        {"layer", required_argument, 0, 0},
        {"config", required_argument, 0, 0},
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
                } else if (strcmp(long_options[option_index].name, "layer") == 0) {
                    // Parse extended layer specification: image:shift:opacity[:easing[:delay[:duration[:blur]]]]
                    char *spec = strdup(optarg);
                    char *image = strtok(spec, ":");
                    char *shift_str = strtok(NULL, ":");
                    char *opacity_str = strtok(NULL, ":");
                    char *easing_str = strtok(NULL, ":");
                    char *delay_str = strtok(NULL, ":");
                    char *duration_str = strtok(NULL, ":");
                    char *blur_str = strtok(NULL, ":");
                    
                    if (!image) {
                        fprintf(stderr, "Error: Invalid layer specification: %s\n", optarg);
                        free(spec);
                        return 1;
                    }
                    
                    float shift = shift_str ? atof(shift_str) : 1.0f;
                    float opacity = opacity_str ? atof(opacity_str) : 1.0f;
                    
                    // Initialize layer array if needed
                    if (!state.layers) {
                        state.max_layers = MAX_LAYERS;
                        state.layers = calloc(state.max_layers, sizeof(struct layer));
                        state.layer_count = 0;
                        config.multi_layer_mode = 1;
                    }
                    
                    // Store layer info for later loading
                    if (state.layer_count < state.max_layers) {
                        struct layer *layer = &state.layers[state.layer_count];
                        layer->image_path = strdup(image);
                        layer->shift_multiplier = shift;
                        layer->opacity = opacity;
                        
                        // Phase 3: Parse optional per-layer settings
                        layer->easing = config.easing;  // Default to global
                        if (easing_str) {
                            if (strcmp(easing_str, "linear") == 0) layer->easing = EASE_LINEAR;
                            else if (strcmp(easing_str, "quad") == 0) layer->easing = EASE_QUAD_OUT;
                            else if (strcmp(easing_str, "cubic") == 0) layer->easing = EASE_CUBIC_OUT;
                            else if (strcmp(easing_str, "quart") == 0) layer->easing = EASE_QUART_OUT;
                            else if (strcmp(easing_str, "quint") == 0) layer->easing = EASE_QUINT_OUT;
                            else if (strcmp(easing_str, "sine") == 0) layer->easing = EASE_SINE_OUT;
                            else if (strcmp(easing_str, "expo") == 0) layer->easing = EASE_EXPO_OUT;
                            else if (strcmp(easing_str, "circ") == 0) layer->easing = EASE_CIRC_OUT;
                            else if (strcmp(easing_str, "back") == 0) layer->easing = EASE_BACK_OUT;
                            else if (strcmp(easing_str, "elastic") == 0) layer->easing = EASE_ELASTIC_OUT;
                            else if (strcmp(easing_str, "snap") == 0) layer->easing = EASE_CUSTOM_SNAP;
                        }
                        
                        layer->animation_delay = delay_str ? atof(delay_str) : 0.0f;
                        layer->animation_duration = duration_str ? atof(duration_str) : config.animation_duration;
                        layer->blur_amount = blur_str ? atof(blur_str) : 0.0f;
                        
                        state.layer_count++;
                    }
                    
                    free(spec);
                } else if (strcmp(long_options[option_index].name, "config") == 0) {
                    if (parse_config_file(optarg) < 0) {
                        return 1;
                    }
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
    
    const char *image_path = NULL;
    
    // Check if we have layers or a single image
    if (config.multi_layer_mode) {
        if (state.layer_count == 0) {
            fprintf(stderr, "Error: At least one layer must be specified\n");
            print_usage(argv[0]);
            return 1;
        }
    } else {
        // Single image mode
        if (optind >= argc) {
            fprintf(stderr, "Error: Image path required\n");
            print_usage(argv[0]);
            return 1;
        }
        image_path = argv[optind];
    }
    
    // Print config if debug
    if (config.debug) {
        printf("Configuration:\n");
        if (config.multi_layer_mode) {
            printf("  Mode: Multi-layer (%d layers)\n", state.layer_count);
            for (int i = 0; i < state.layer_count; i++) {
                printf("    Layer %d: %s (shift=%.2f, opacity=%.2f, blur=%.2f)\n", 
                       i, state.layers[i].image_path, 
                       state.layers[i].shift_multiplier, 
                       state.layers[i].opacity,
                       state.layers[i].blur_amount);
            }
        } else {
            printf("  Mode: Single image\n");
            printf("  Image: %s\n", image_path);
        }
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
    
    // Load images
    if (config.multi_layer_mode) {
        // Load all layers
        for (int i = 0; i < state.layer_count; i++) {
            struct layer *layer = &state.layers[i];
            if (load_layer(layer, layer->image_path, layer->shift_multiplier, layer->opacity) < 0) {
                fprintf(stderr, "Failed to load layer %d: %s\n", i, layer->image_path);
                return 1;
            }
        }
        if (config.debug) {
            printf("Loaded %d layers successfully\n", state.layer_count);
        }
    } else {
        // Load single image
        if (load_image(image_path) < 0) return 1;
    }
    
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
    if (state.blur_shader_program) glDeleteProgram(state.blur_shader_program);
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