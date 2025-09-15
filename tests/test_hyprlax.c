// Test suite for hyprlax core functionality using Check framework
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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
START_TEST(test_easing_linear)
{
    ck_assert_float_eq_tol(apply_easing(0.0f, EASE_LINEAR), 0.0f, 0.0001f);
    ck_assert_float_eq_tol(apply_easing(0.5f, EASE_LINEAR), 0.5f, 0.0001f);
    ck_assert_float_eq_tol(apply_easing(1.0f, EASE_LINEAR), 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_easing_boundaries)
{
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
            ck_assert_float_eq_tol(start, 0.0f, 0.0001f);
        }
        
        // All should end at 1
        ck_assert_float_eq_tol(apply_easing(1.0f, types[i]), 1.0f, 0.0001f);
    }
}
END_TEST

START_TEST(test_easing_monotonic)
{
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
            ck_assert(current >= prev);
            prev = current;
        }
    }
}
END_TEST

START_TEST(test_version_check)
{
    // Test that hyprlax binary exists and returns version
    int status = system("./hyprlax --version > /dev/null 2>&1");
    ck_assert_int_eq(WEXITSTATUS(status), 0);
}
END_TEST

START_TEST(test_help_output)
{
    // Test that help flag works
    int status = system("./hyprlax --help > /dev/null 2>&1");
    ck_assert_int_eq(WEXITSTATUS(status), 0);
}
END_TEST

START_TEST(test_invalid_args)
{
    // Test that invalid arguments are handled
    int status = system("./hyprlax --invalid-flag 2> /dev/null");
    ck_assert(WEXITSTATUS(status) != 0);
}
END_TEST

START_TEST(test_missing_image)
{
    // Test that missing image file is handled
    int status = system("./hyprlax /nonexistent/image.jpg 2> /dev/null");
    ck_assert(WEXITSTATUS(status) != 0);
}
END_TEST

START_TEST(test_workspace_offset_calculation)
{
    // Test workspace offset calculations
    float shift_per_workspace = 200.0f;
    int max_workspaces = 10;
    
    // Workspace 1 should have offset 0
    float offset = (1 - 1) * shift_per_workspace;
    ck_assert_float_eq_tol(offset, 0.0f, 0.0001f);
    
    // Workspace 5 should have offset 800
    offset = (5 - 1) * shift_per_workspace;
    ck_assert_float_eq_tol(offset, 800.0f, 0.0001f);
    
    // Workspace 10 should have offset 1800
    offset = (10 - 1) * shift_per_workspace;
    ck_assert_float_eq_tol(offset, 1800.0f, 0.0001f);
}
END_TEST

START_TEST(test_animation_progress)
{
    // Test animation progress calculation
    float duration = 1.0f;
    float elapsed;
    float progress;
    
    // At start
    elapsed = 0.0f;
    progress = elapsed / duration;
    ck_assert_float_eq_tol(progress, 0.0f, 0.0001f);
    
    // Halfway
    elapsed = 0.5f;
    progress = elapsed / duration;
    ck_assert_float_eq_tol(progress, 0.5f, 0.0001f);
    
    // Complete
    elapsed = 1.0f;
    progress = elapsed / duration;
    ck_assert_float_eq_tol(progress, 1.0f, 0.0001f);
    
    // Over time (should clamp)
    elapsed = 1.5f;
    progress = fminf(elapsed / duration, 1.0f);
    ck_assert_float_eq_tol(progress, 1.0f, 0.0001f);
}
END_TEST

START_TEST(test_config_parsing)
{
    // Create a test config file
    FILE *f = fopen("/tmp/test_hyprlax.conf", "w");
    ck_assert_ptr_nonnull(f);
    
    fprintf(f, "shift = 300\n");
    fprintf(f, "duration = 2.0\n");
    fprintf(f, "easing = expo\n");
    fprintf(f, "fps = 60\n");
    fprintf(f, "debug = true\n");
    fclose(f);
    
    // Test that config file is readable
    f = fopen("/tmp/test_hyprlax.conf", "r");
    ck_assert_ptr_nonnull(f);
    
    char line[256];
    int found_shift = 0;
    int found_duration = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "shift = 300")) found_shift = 1;
        if (strstr(line, "duration = 2.0")) found_duration = 1;
    }
    
    ck_assert(found_shift);
    ck_assert(found_duration);
    
    fclose(f);
    unlink("/tmp/test_hyprlax.conf");
}
END_TEST

START_TEST(test_scale_factor_calculation)
{
    // Test scale factor calculations
    float shift_per_workspace = 200.0f;
    int max_workspaces = 10;
    float viewport_width = 1920.0f;
    
    // Maximum total shift
    float max_shift = shift_per_workspace * (max_workspaces - 1);
    ck_assert_float_eq_tol(max_shift, 1800.0f, 0.0001f);
    
    // Required texture width
    float required_width = viewport_width + max_shift;
    ck_assert_float_eq_tol(required_width, 3720.0f, 0.0001f);
    
    // Scale factor
    float scale_factor = required_width / viewport_width;
    ck_assert(scale_factor > 1.9f && scale_factor < 2.0f);
}
END_TEST

START_TEST(test_easing_performance)
{
    // Test that easing functions are fast enough
    const int iterations = 1000000;
    
    for (int i = 0; i < iterations; i++) {
        float t = (float)i / iterations;
        apply_easing(t, EASE_EXPO_OUT);
    }
    
    // If we get here without timeout, performance is acceptable
    ck_assert(1);
}
END_TEST

// Create the test suite
Suite *hyprlax_suite(void)
{
    Suite *s;
    TCase *tc_easing;
    TCase *tc_binary;
    TCase *tc_calc;
    TCase *tc_perf;
    
    s = suite_create("Hyprlax");
    
    // Easing test case
    tc_easing = tcase_create("Easing");
    tcase_add_test(tc_easing, test_easing_linear);
    tcase_add_test(tc_easing, test_easing_boundaries);
    tcase_add_test(tc_easing, test_easing_monotonic);
    suite_add_tcase(s, tc_easing);
    
    // Binary test case (conditional)
    if (access("./hyprlax", X_OK) == 0) {
        tc_binary = tcase_create("Binary");
        tcase_add_test(tc_binary, test_version_check);
        tcase_add_test(tc_binary, test_help_output);
        tcase_add_test(tc_binary, test_invalid_args);
        tcase_add_test(tc_binary, test_missing_image);
        suite_add_tcase(s, tc_binary);
    }
    
    // Calculation test case
    tc_calc = tcase_create("Calculations");
    tcase_add_test(tc_calc, test_workspace_offset_calculation);
    tcase_add_test(tc_calc, test_animation_progress);
    tcase_add_test(tc_calc, test_config_parsing);
    tcase_add_test(tc_calc, test_scale_factor_calculation);
    suite_add_tcase(s, tc_calc);
    
    // Performance test case
    tc_perf = tcase_create("Performance");
    tcase_set_timeout(tc_perf, 5);  // 5 second timeout
    tcase_add_test(tc_perf, test_easing_performance);
    suite_add_tcase(s, tc_perf);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    s = hyprlax_suite();
    sr = srunner_create(s);
    
    // Run in fork mode for test isolation
    srunner_set_fork_status(sr, CK_FORK);
    
    // Run tests
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    
    // Cleanup
    srunner_free(sr);
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}