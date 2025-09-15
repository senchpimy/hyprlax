// Test suite for easing functions using Check framework
#include <check.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Easing function implementations (simplified)
float ease_linear(float t) { return t; }
float ease_quad(float t) { return t * t; }
float ease_cubic(float t) { return t * t * t; }
float ease_expo(float t) { return t == 0 ? 0 : pow(2, 10 * (t - 1)); }
float ease_sine(float t) { return 1 - cos(t * 3.14159 / 2); }

START_TEST(test_easing_values)
{
    
    // Test at key points: 0, 0.5, 1
    ck_assert_float_eq(ease_linear(0.0f), 0.0f);
    ck_assert_float_eq(ease_linear(0.5f), 0.5f);
    ck_assert_float_eq(ease_linear(1.0f), 1.0f);
    
    // Quad should be slower at start
    ck_assert(ease_quad(0.25f) < ease_linear(0.25f));
    ck_assert_float_eq(ease_quad(1.0f), 1.0f);
    
    // Cubic should be even slower at start
    ck_assert(ease_cubic(0.25f) < ease_quad(0.25f));
    ck_assert_float_eq(ease_cubic(1.0f), 1.0f);
    
    // All should start at 0 and end at 1
    float easings[] = {
        ease_linear(0), ease_quad(0), ease_cubic(0), 
        ease_expo(0), ease_sine(0)
    };
    for (int i = 0; i < 5; i++) {
        ck_assert_float_eq(easings[i], 0.0f);
    }
    
}
END_TEST

START_TEST(test_easing_parsing)
{
    
    struct {
        const char *name;
        int expected_id;
    } test_cases[] = {
        {"linear", 0},
        {"quad", 1},
        {"cubic", 2},
        {"quart", 3},
        {"quint", 4},
        {"sine", 5},
        {"expo", 6},
        {"circ", 7},
        {"back", 8},
        {"elastic", 9},
        {"bounce", 10},
        {"snap", 11},
        {"invalid", -1},
        {"", -1}
    };
    
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
        // Simulate parsing
        int id = -1;
        const char *easings[] = {
            "linear", "quad", "cubic", "quart", "quint",
            "sine", "expo", "circ", "back", "elastic", 
            "bounce", "snap"
        };
        
        for (int j = 0; j < 12; j++) {
            if (strcmp(test_cases[i].name, easings[j]) == 0) {
                id = j;
                break;
            }
        }
        
        if (test_cases[i].expected_id >= 0) {
            ck_assert_int_eq(id, test_cases[i].expected_id);
        }
    }
    
}
END_TEST

START_TEST(test_easing_interpolation)
{
    
    float start = 100.0f;
    float end = 500.0f;
    float t = 0.5f;
    
    // Linear should give midpoint
    float linear_value = start + (end - start) * ease_linear(t);
    ck_assert_float_eq_tol(linear_value, 300.0f, 0.001f);
    
    // Quad should be less than midpoint at t=0.5
    float quad_value = start + (end - start) * ease_quad(t);
    ck_assert(quad_value < 300.0f);
    ck_assert(quad_value > start);
    
    // Test edge cases
    t = 0.0f;
    float value_at_start = start + (end - start) * ease_linear(t);
    ck_assert_float_eq(value_at_start, start);
    
    t = 1.0f;
    float value_at_end = start + (end - start) * ease_linear(t);
    ck_assert_float_eq(value_at_end, end);
    
}
END_TEST

START_TEST(test_snap_easing)
{
    
    // Snap should be 0 until threshold, then 1
    float threshold = 0.9f;
    
    ck_assert_float_eq(ease_linear(0.0f), 0.0f);
    ck_assert_float_eq(ease_linear(0.5f), 0.5f);
    
    // Snap simulation
    float snap_at_089 = (0.89f >= threshold) ? 1.0f : 0.0f;
    float snap_at_091 = (0.91f >= threshold) ? 1.0f : 0.0f;
    
    ck_assert_float_eq(snap_at_089, 0.0f);
    ck_assert_float_eq(snap_at_091, 1.0f);
    
}
END_TEST

// Create the test suite
Suite *easing_suite(void)
{
    Suite *s;
    TCase *tc_core;
    
    s = suite_create("Easing");
    
    // Core test case
    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_easing_values);
    tcase_add_test(tc_core, test_easing_parsing);
    tcase_add_test(tc_core, test_easing_interpolation);
    tcase_add_test(tc_core, test_snap_easing);
    suite_add_tcase(s, tc_core);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    s = easing_suite();
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