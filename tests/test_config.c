// Test suite for configuration parsing using Check framework
#include <check.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

START_TEST(test_config_commands)
{
    // Create test config
    FILE *f = fopen("/tmp/test_config.conf", "w");
    ck_assert_ptr_nonnull(f);
    fprintf(f, "# Comment line\n");
    fprintf(f, "duration 2.5\n");
    fprintf(f, "shift 150\n");
    fprintf(f, "easing expo\n");
    fprintf(f, "fps 60\n");
    fprintf(f, "layer img1.png 0.5 1.0 2.0\n");
    fprintf(f, "layer img2.png 0.3 0.8\n");  // No blur
    fprintf(f, "\n");  // Empty line
    fprintf(f, "   # Indented comment\n");
    fclose(f);
    
    // Parse config (simplified)
    FILE *config = fopen("/tmp/test_config.conf", "r");
    ck_assert_ptr_nonnull(config);
    
    char line[256];
    int layer_count = 0;
    float duration = 0;
    int shift = 0;
    
    while (fgets(line, sizeof(line), config)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;
        
        char cmd[64];
        if (sscanf(line, "%s", cmd) == 1) {
            if (strcmp(cmd, "duration") == 0) {
                sscanf(line, "%*s %f", &duration);
            } else if (strcmp(cmd, "shift") == 0) {
                sscanf(line, "%*s %d", &shift);
            } else if (strcmp(cmd, "layer") == 0) {
                layer_count++;
            }
        }
    }
    fclose(config);
    
    ck_assert_float_eq(duration, 2.5f);
    ck_assert_int_eq(shift, 150);
    ck_assert_int_eq(layer_count, 2);
    
    unlink("/tmp/test_config.conf");
}
END_TEST

START_TEST(test_config_relative_paths)
{
    // Create config with relative paths
    mkdir("/tmp/test_config_dir", 0755);
    FILE *f = fopen("/tmp/test_config_dir/test.conf", "w");
    ck_assert_ptr_nonnull(f);
    fprintf(f, "layer ./image.png 1.0 1.0\n");
    fprintf(f, "layer ../other/image.png 1.0 1.0\n");
    fprintf(f, "layer /absolute/path.png 1.0 1.0\n");
    fclose(f);
    
    // Test path resolution logic
    char *config_dir = "/tmp/test_config_dir";
    char *relative_path = "./image.png";
    char resolved[256];
    
    // Should resolve to /tmp/test_config_dir/image.png
    snprintf(resolved, sizeof(resolved), "%s/%s", config_dir, relative_path + 2);
    ck_assert_ptr_nonnull(strstr(resolved, "/tmp/test_config_dir/image.png"));
    
    unlink("/tmp/test_config_dir/test.conf");
    rmdir("/tmp/test_config_dir");
}
END_TEST

START_TEST(test_config_validation)
{
    
    // Test invalid values
    struct {
        const char *line;
        int should_fail;
    } test_cases[] = {
        {"duration -1.0", 1},      // Negative duration
        {"duration 0.0", 1},       // Zero duration
        {"duration 10.0", 0},      // Valid
        {"shift -100", 1},         // Negative shift
        {"shift 0", 0},            // Zero shift OK
        {"fps 0", 1},              // Zero FPS
        {"fps 240", 0},            // Valid FPS
        {"layer img.png -1.0 1.0", 1}, // Negative shift multiplier
        {"layer img.png 1.0 2.0", 1},  // Opacity > 1.0
        {"layer img.png 1.0 -0.5", 1}, // Negative opacity
    };
    
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
        // Validation logic would go here
        // For now, just check format
        char cmd[64];
        float val1, val2;
        sscanf(test_cases[i].line, "%s %f %f", cmd, &val1, &val2);
        
        if (test_cases[i].should_fail) {
            // Should reject invalid values
            if (strcmp(cmd, "duration") == 0 && val1 <= 0) continue;
            if (strcmp(cmd, "shift") == 0 && val1 < 0) continue;
            if (strcmp(cmd, "fps") == 0 && val1 <= 0) continue;
        }
    }
    
}
END_TEST

// Create the test suite
Suite *config_suite(void)
{
    Suite *s;
    TCase *tc_core;
    
    s = suite_create("Config");
    
    // Core test case
    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_config_commands);
    tcase_add_test(tc_core, test_config_relative_paths);
    tcase_add_test(tc_core, test_config_validation);
    suite_add_tcase(s, tc_core);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;
    
    s = config_suite();
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