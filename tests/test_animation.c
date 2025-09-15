// Test suite for animation logic using Check framework
#include <check.h>
#include <stdlib.h>
#include <math.h>

START_TEST(test_animation_timing)
{
    
    float duration = 1.0f;
    float delay = 0.5f;
    float current_time = 2.0f;
    float animation_start = 1.0f;
    
    // Calculate progress
    float elapsed = current_time - animation_start - delay;
    float progress = elapsed / duration;
    
    // Should be at 50% progress (0.5 seconds into 1.0 second animation)
    ck_assert_float_eq_tol(progress, 0.5f, 0.001f);
    
    // Test clamping
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;
    
}
END_TEST

START_TEST(test_workspace_offset)
{
    
    int workspace = 3;
    float shift_pixels = 200.0f;
    float shift_multiplier = 0.5f;
    
    float expected_offset = workspace * shift_pixels * shift_multiplier;
    ck_assert_float_eq_tol(expected_offset, 300.0f, 0.001f);
    
    // Test negative workspace
    workspace = -2;
    expected_offset = workspace * shift_pixels * shift_multiplier;
    ck_assert_float_eq_tol(expected_offset, -200.0f, 0.001f);
    
}
END_TEST

START_TEST(test_animation_state)
{
    
    struct {
        int animating;
        float current_offset;
        float target_offset;
        float animation_progress;
    } state = {0, 0.0f, 0.0f, 0.0f};
    
    // Start animation
    float new_target = 100.0f;
    if (state.target_offset != new_target) {
        state.animating = 1;
        state.target_offset = new_target;
        state.animation_progress = 0.0f;
    }
    
    ck_assert_int_eq(state.animating, 1);
    ck_assert_float_eq(state.target_offset, 100.0f);
    
    // Complete animation
    state.animation_progress = 1.0f;
    state.current_offset = state.target_offset;
    state.animating = 0;
    
    ck_assert_int_eq(state.animating, 0);
    ck_assert_float_eq(state.current_offset, 100.0f);
    
}
END_TEST

START_TEST(test_layer_animation)
{
    
    struct layer {
        float shift_multiplier;
        float current_offset;
        float target_offset;
        float animation_delay;
        float animation_duration;
        int animating;
    } layers[3] = {
        {0.3f, 0.0f, 0.0f, 0.0f, 1.0f, 0},
        {0.6f, 0.0f, 0.0f, 0.2f, 1.5f, 0},
        {1.0f, 0.0f, 0.0f, 0.4f, 2.0f, 0}
    };
    
    // Trigger animation to workspace 2
    int workspace = 2;
    float shift = 100.0f;
    
    for (int i = 0; i < 3; i++) {
        layers[i].target_offset = workspace * shift * layers[i].shift_multiplier;
        layers[i].animating = 1;
    }
    
    // Check different speeds
    ck_assert_float_eq_tol(layers[0].target_offset, 60.0f, 0.001f);
    ck_assert_float_eq_tol(layers[1].target_offset, 120.0f, 0.001f);
    ck_assert_float_eq_tol(layers[2].target_offset, 200.0f, 0.001f);
    
    // Check different delays
    ck_assert_float_eq(layers[0].animation_delay, 0.0f);
    ck_assert_float_eq(layers[1].animation_delay, 0.2f);
    ck_assert_float_eq(layers[2].animation_delay, 0.4f);
    
}
END_TEST

// Create the test suite
Suite *animation_suite(void)
{
    Suite *s;
    TCase *tc_core;
    
    s = suite_create("Animation");
    
    // Core test case
    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_animation_timing);
    tcase_add_test(tc_core, test_workspace_offset);
    tcase_add_test(tc_core, test_animation_state);
    tcase_add_test(tc_core, test_layer_animation);
    suite_add_tcase(s, tc_core);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    s = animation_suite();
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