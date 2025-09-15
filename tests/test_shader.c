// Test suite for shader-related functionality using Check framework
#include <check.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

START_TEST(test_shader_generation)
{
    
    // Simulate building blur shader with constants
    float blur_kernel_size = 5.0f;
    float blur_weight_falloff = 0.15f;
    float blur_min_threshold = 0.001f;
    
    char shader[2048];
    int written = snprintf(shader, sizeof(shader),
        "precision highp float;\n"
        "uniform float u_blur_amount;\n"
        "void main() {\n"
        "    float blur = u_blur_amount;\n"
        "    if (blur < %.4f) {\n"
        "        return;\n"
        "    }\n"
        "}\n",
        blur_min_threshold);
    
    ck_assert_int_gt(written, 0);
    ck_assert_int_lt(written, sizeof(shader));
    ck_assert_ptr_nonnull(strstr(shader, "0.0010"));  // Check threshold was inserted
    ck_assert_ptr_nonnull(strstr(shader, "precision highp float"));
}
END_TEST

START_TEST(test_shader_uniforms)
{
    
    // Simulate uniform location retrieval
    struct {
        const char *name;
        int location;
    } uniforms[] = {
        {"u_texture", 0},
        {"u_opacity", 1},
        {"u_blur_amount", 2},
        {"u_resolution", 3},
        {"invalid_uniform", -1}
    };
    
    // All valid uniforms should have location >= 0
    for (int i = 0; i < 4; i++) {
        ck_assert_int_ge(uniforms[i].location, 0);
    }
    
    // Invalid uniform should be -1
    ck_assert_int_eq(uniforms[4].location, -1);
}
END_TEST

START_TEST(test_shader_program_state)
{
    
    struct {
        int standard_shader;
        int blur_shader;
        int current_shader;
    } state = {0, 0, 0};
    
    // Initially no shaders
    ck_assert_int_eq(state.standard_shader, 0);
    ck_assert_int_eq(state.blur_shader, 0);
    
    // After initialization
    state.standard_shader = 4;
    state.blur_shader = 5;
    
    // Select shader based on blur
    float blur_amount = 2.0f;
    float threshold = 0.001f;
    
    if (blur_amount > threshold && state.blur_shader != 0) {
        state.current_shader = state.blur_shader;
    } else {
        state.current_shader = state.standard_shader;
    }
    
    ck_assert_int_eq(state.current_shader, 5);  // Should use blur shader
    
    // Test with no blur
    blur_amount = 0.0f;
    if (blur_amount > threshold && state.blur_shader != 0) {
        state.current_shader = state.blur_shader;
    } else {
        state.current_shader = state.standard_shader;
    }
    
    ck_assert_int_eq(state.current_shader, 4);  // Should use standard shader
}
END_TEST

START_TEST(test_shader_init_order)
{
    
    int init_order[10];
    int step = 0;
    
    // Simulate initialization sequence
    init_order[step++] = 1;  // Create context
    init_order[step++] = 2;  // Make current
    init_order[step++] = 3;  // Create shaders
    init_order[step++] = 4;  // Link programs
    init_order[step++] = 5;  // Get uniforms
    
    // Verify order
    ck_assert_int_lt(init_order[0], init_order[2]);  // Context before shaders
    ck_assert_int_lt(init_order[2], init_order[3]);  // Create before link
    ck_assert_int_lt(init_order[3], init_order[4]);  // Link before uniforms
}
END_TEST

// Create the test suite
Suite *shader_suite(void)
{
    Suite *s;
    TCase *tc_core;
    
    s = suite_create("Shader");
    
    // Core test case
    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_shader_generation);
    tcase_add_test(tc_core, test_shader_uniforms);
    tcase_add_test(tc_core, test_shader_program_state);
    tcase_add_test(tc_core, test_shader_init_order);
    suite_add_tcase(s, tc_core);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    s = shader_suite();
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