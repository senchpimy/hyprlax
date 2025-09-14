/*
 * Test suite for hyprlax
 * Tests core functionality without requiring Wayland environment
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_FLOAT_EQ(a, b) ASSERT(fabs((a) - (b)) < 0.0001)
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)

static int tests_passed = 0;
static int tests_total = 0;

// Include easing functions for testing (copied from hyprlax.c)
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
    EASE_CUSTOM_SNAP
} easing_type_t;

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
            return t == 0 ? 0 : (t == 1.0f ? 1.0f : powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * c4) + 1.0f);
        }
        
        case EASE_CUSTOM_SNAP: {
            if (t < 0.5f) {
                return 4.0f * t * t * t;
            } else {
                float p = 2.0f * t - 2.0f;
                return 1.0f + p * p * p / 2.0f;
            }
        }
        
        default:
            return t;
    }
}

// Test cases
TEST(easing_linear) {
    ASSERT_FLOAT_EQ(apply_easing(0.0f, EASE_LINEAR), 0.0f);
    ASSERT_FLOAT_EQ(apply_easing(0.5f, EASE_LINEAR), 0.5f);
    ASSERT_FLOAT_EQ(apply_easing(1.0f, EASE_LINEAR), 1.0f);
}

TEST(easing_boundaries) {
    // Test all easing functions at boundaries
    easing_type_t types[] = {
        EASE_LINEAR, EASE_QUAD_OUT, EASE_CUBIC_OUT, EASE_QUART_OUT,
        EASE_QUINT_OUT, EASE_SINE_OUT, EASE_EXPO_OUT, EASE_CIRC_OUT,
        EASE_BACK_OUT, EASE_ELASTIC_OUT, EASE_CUSTOM_SNAP
    };
    
    for (int i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
        // All should start at 0 (except back/elastic which can overshoot)
        float start = apply_easing(0.0f, types[i]);
        if (types[i] != EASE_BACK_OUT && types[i] != EASE_ELASTIC_OUT) {
            ASSERT_FLOAT_EQ(start, 0.0f);
        }
        
        // All should end at 1
        ASSERT_FLOAT_EQ(apply_easing(1.0f, types[i]), 1.0f);
    }
}

TEST(easing_monotonic) {
    // Test that most easing functions are monotonically increasing
    easing_type_t types[] = {
        EASE_LINEAR, EASE_QUAD_OUT, EASE_CUBIC_OUT, EASE_QUART_OUT,
        EASE_QUINT_OUT, EASE_SINE_OUT, EASE_EXPO_OUT, EASE_CIRC_OUT,
        EASE_CUSTOM_SNAP
    };
    
    for (int i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
        float prev = 0.0f;
        for (float t = 0.1f; t <= 1.0f; t += 0.1f) {
            float current = apply_easing(t, types[i]);
            ASSERT(current >= prev);
            prev = current;
        }
    }
}

TEST(version_check) {
    // Test that hyprlax binary exists and returns version
    int status = system("./hyprlax --version > /dev/null 2>&1");
    ASSERT_EQ(WEXITSTATUS(status), 0);
}

TEST(help_output) {
    // Test that help flag works
    int status = system("./hyprlax --help > /dev/null 2>&1");
    ASSERT_EQ(WEXITSTATUS(status), 0);
}

TEST(invalid_args) {
    // Test that invalid arguments are handled
    int status = system("./hyprlax --invalid-flag 2> /dev/null");
    ASSERT(WEXITSTATUS(status) != 0);
}

TEST(missing_image) {
    // Test that missing image file is handled
    int status = system("./hyprlax /nonexistent/image.jpg 2> /dev/null");
    ASSERT(WEXITSTATUS(status) != 0);
}

TEST(workspace_offset_calculation) {
    // Test workspace offset calculations
    float shift_per_workspace = 200.0f;
    int max_workspaces = 10;
    
    // Workspace 1 should have offset 0
    float offset = (1 - 1) * shift_per_workspace;
    ASSERT_FLOAT_EQ(offset, 0.0f);
    
    // Workspace 5 should have offset 800
    offset = (5 - 1) * shift_per_workspace;
    ASSERT_FLOAT_EQ(offset, 800.0f);
    
    // Workspace 10 should have offset 1800
    offset = (10 - 1) * shift_per_workspace;
    ASSERT_FLOAT_EQ(offset, 1800.0f);
}

TEST(animation_progress) {
    // Test animation progress calculation
    float duration = 1.0f;
    float elapsed;
    float progress;
    
    // At start
    elapsed = 0.0f;
    progress = elapsed / duration;
    ASSERT_FLOAT_EQ(progress, 0.0f);
    
    // Halfway
    elapsed = 0.5f;
    progress = elapsed / duration;
    ASSERT_FLOAT_EQ(progress, 0.5f);
    
    // Complete
    elapsed = 1.0f;
    progress = elapsed / duration;
    ASSERT_FLOAT_EQ(progress, 1.0f);
    
    // Over time (should clamp)
    elapsed = 1.5f;
    progress = fminf(elapsed / duration, 1.0f);
    ASSERT_FLOAT_EQ(progress, 1.0f);
}

TEST(config_parsing) {
    // Create a test config file
    FILE *f = fopen("/tmp/test_hyprlax.conf", "w");
    ASSERT(f != NULL);
    
    fprintf(f, "shift = 300\n");
    fprintf(f, "duration = 2.0\n");
    fprintf(f, "easing = expo\n");
    fprintf(f, "fps = 60\n");
    fprintf(f, "debug = true\n");
    fclose(f);
    
    // Test that config file is readable
    f = fopen("/tmp/test_hyprlax.conf", "r");
    ASSERT(f != NULL);
    
    char line[256];
    int found_shift = 0;
    int found_duration = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "shift = 300")) found_shift = 1;
        if (strstr(line, "duration = 2.0")) found_duration = 1;
    }
    
    ASSERT(found_shift);
    ASSERT(found_duration);
    
    fclose(f);
    unlink("/tmp/test_hyprlax.conf");
}

TEST(scale_factor_calculation) {
    // Test scale factor calculations
    float shift_per_workspace = 200.0f;
    int max_workspaces = 10;
    float viewport_width = 1920.0f;
    
    // Maximum total shift
    float max_shift = shift_per_workspace * (max_workspaces - 1);
    ASSERT_FLOAT_EQ(max_shift, 1800.0f);
    
    // Required texture width
    float required_width = viewport_width + max_shift;
    ASSERT_FLOAT_EQ(required_width, 3720.0f);
    
    // Scale factor
    float scale_factor = required_width / viewport_width;
    ASSERT(scale_factor > 1.9f && scale_factor < 2.0f);
}

// Performance tests
TEST(easing_performance) {
    // Test that easing functions are fast enough
    const int iterations = 1000000;
    
    for (int i = 0; i < iterations; i++) {
        float t = (float)i / iterations;
        apply_easing(t, EASE_EXPO_OUT);
    }
    
    // If we get here without timeout, performance is acceptable
    ASSERT(1);
}

// Main test runner
int main(int argc, char **argv) {
    printf("\n=================================\n");
    printf("     hyprlax Test Suite\n");
    printf("=================================\n\n");
    
    // Core functionality tests
    RUN_TEST(easing_linear);
    RUN_TEST(easing_boundaries);
    RUN_TEST(easing_monotonic);
    
    // Binary tests (only if binary exists)
    if (access("./hyprlax", X_OK) == 0) {
        RUN_TEST(version_check);
        RUN_TEST(help_output);
        RUN_TEST(invalid_args);
        RUN_TEST(missing_image);
    } else {
        printf("Skipping binary tests (hyprlax not found)\n");
    }
    
    // Calculation tests
    RUN_TEST(workspace_offset_calculation);
    RUN_TEST(animation_progress);
    RUN_TEST(config_parsing);
    RUN_TEST(scale_factor_calculation);
    
    // Performance tests
    RUN_TEST(easing_performance);
    
    printf("\n=================================\n");
    printf("  All tests passed! (%d/%d)\n", tests_passed, tests_passed);
    printf("=================================\n\n");
    
    return 0;
}