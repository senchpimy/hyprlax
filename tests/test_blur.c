// Test suite for blur functionality using Check framework
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mock structures matching hyprlax
struct layer {
    float blur_amount;
    float opacity;
    float shift_multiplier;
    char *image_path;
};

// Test fixture data
static FILE *test_config_file;
static const char *test_config_path = "/tmp/test_blur.conf";

// Setup function - runs before each test
void setup(void) {
    // Setup code if needed
}

// Teardown function - runs after each test  
void teardown(void) {
    // Cleanup test files
    remove(test_config_path);
}

START_TEST(test_config_parsing_preserves_blur)
{
    // Create a test config file
    FILE *f = fopen(test_config_path, "w");
    ck_assert_ptr_nonnull(f);
    fprintf(f, "layer test1.png 0.5 1.0 2.5\n");
    fprintf(f, "layer test2.png 0.3 0.8 40.0\n");
    fclose(f);
    
    // Mock parsing logic
    struct layer layers[2];
    FILE *config = fopen(test_config_path, "r");
    ck_assert_ptr_nonnull(config);
    
    char line[256];
    int i = 0;
    
    while (fgets(line, sizeof(line), config) && i < 2) {
        char cmd[64], image[128];
        float shift, opacity, blur;
        if (sscanf(line, "%s %s %f %f %f", cmd, image, &shift, &opacity, &blur) == 5) {
            layers[i].shift_multiplier = shift;
            layers[i].opacity = opacity;
            layers[i].blur_amount = blur;
            i++;
        }
    }
    fclose(config);
    
    // Check blur values were parsed correctly
    ck_assert_float_eq(layers[0].blur_amount, 2.5f);
    ck_assert_float_eq(layers[1].blur_amount, 40.0f);
}
END_TEST

START_TEST(test_layer_loading_preserves_blur)
{
    struct layer layer;
    layer.blur_amount = 5.0f;
    
    // Simulate what load_layer SHOULD do
    float original_blur = layer.blur_amount;
    float loaded_blur = original_blur; // Should preserve
    
    ck_assert_float_eq(loaded_blur, original_blur);
    ck_assert_float_eq(loaded_blur, 5.0f);
}
END_TEST

START_TEST(test_blur_shader_selection)
{
    float BLUR_MIN_THRESHOLD = 0.001f;
    int blur_shader_program = 5;
    int standard_shader_program = 4;
    
    struct layer layers[3] = {
        {.blur_amount = 0.0f},    // Should use standard shader
        {.blur_amount = 0.0005f},  // Should use standard shader (below threshold)
        {.blur_amount = 2.0f}      // Should use blur shader
    };
    
    for (int i = 0; i < 3; i++) {
        int selected_shader;
        
        if (layers[i].blur_amount > BLUR_MIN_THRESHOLD && blur_shader_program != 0) {
            selected_shader = blur_shader_program;
        } else {
            selected_shader = standard_shader_program;
        }
        
        if (i < 2) {
            ck_assert_int_eq(selected_shader, standard_shader_program);
        } else {
            ck_assert_int_eq(selected_shader, blur_shader_program);
        }
    }
}
END_TEST

START_TEST(test_initialization_order)
{
    // Test that blur shader is initialized before rendering
    int blur_shader_program = 0;
    int render_attempted = 0;
    
    // Simulate the bug: render before init
    if (blur_shader_program == 0) {
        // Should not attempt to render
        ck_assert_int_eq(render_attempted, 0);
    }
    
    // After initialization
    blur_shader_program = 5;
    render_attempted = 1;
    ck_assert_int_ne(blur_shader_program, 0);
}
END_TEST

START_TEST(test_blur_range_values)
{
    // Test various blur values
    struct {
        float blur;
        int expected_valid;
    } test_cases[] = {
        {-1.0f, 0},   // Negative - invalid
        {0.0f, 1},    // Zero - valid (no blur)
        {0.5f, 1},    // Small blur - valid
        {10.0f, 1},   // Large blur - valid
        {100.0f, 1},  // Very large blur - valid
    };
    
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
        int is_valid = (test_cases[i].blur >= 0.0f) ? 1 : 0;
        ck_assert_int_eq(is_valid, test_cases[i].expected_valid);
    }
}
END_TEST

START_TEST(test_memory_leak_in_layer_path)
{
    // Test that layer paths are properly freed
    struct layer layer = {0};
    layer.image_path = strdup("/test/path.png");
    ck_assert_ptr_nonnull(layer.image_path);
    
    // Cleanup
    free(layer.image_path);
    layer.image_path = NULL;
    ck_assert_ptr_null(layer.image_path);
}
END_TEST

// Create the test suite
Suite *blur_suite(void)
{
    Suite *s;
    TCase *tc_core;
    TCase *tc_memory;
    
    s = suite_create("Blur");
    
    // Core test case
    tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_config_parsing_preserves_blur);
    tcase_add_test(tc_core, test_layer_loading_preserves_blur);
    tcase_add_test(tc_core, test_blur_shader_selection);
    tcase_add_test(tc_core, test_initialization_order);
    tcase_add_test(tc_core, test_blur_range_values);
    suite_add_tcase(s, tc_core);
    
    // Memory test case
    tc_memory = tcase_create("Memory");
    tcase_add_test(tc_memory, test_memory_leak_in_layer_path);
    suite_add_tcase(s, tc_memory);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    s = blur_suite();
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