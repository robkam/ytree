#include <check.h>
#include <stdlib.h>
#include <string.h>

/* Include the actual production header */
#include "src/ui/attr_actions.h"

START_TEST(test_UI_ParseModeInput_buffer_bounds)
{
    /* Invariant: UI_ParseModeInput must never write beyond out_mode[10] and preview_mode[9] */
    const char *payloads[] = {
        "7777777777",      /* Exact exploit case: 10 chars, triggers full out_mode[10] write */
        "777",            /* Boundary case: 3 chars, triggers memcpy(&out_mode[1], ..., 9) */
        "rwxr-xr-x",      /* Valid 9-char input */
        "0777",           /* Valid 4-char octal */
        "777777777"       /* Valid 9-char octal (boundary) */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        char out_mode[11] = {0};      /* Exactly size 11 as expected by function */
        char preview_mode[10] = {0};  /* Exactly size 10 as expected by function */
        int result;

        /* Poison buffers with sentinel values */
        memset(out_mode, 0xAA, sizeof(out_mode));
        memset(preview_mode, 0xBB, sizeof(preview_mode));
        out_mode[10] = '\0';          /* Ensure null terminator at expected index */
        preview_mode[9] = '\0';       /* Ensure null terminator at expected index */

        result = UI_ParseModeInput(payloads[i], out_mode, preview_mode);

        /* Security property: No writes beyond allocated bounds */
        ck_assert_msg(out_mode[10] == '\0', 
            "Buffer overflow detected in out_mode for payload '%s'", payloads[i]);
        ck_assert_msg(preview_mode[9] == '\0', 
            "Buffer overflow detected in preview_mode for payload '%s'", payloads[i]);
        
        /* Additional safety: null-terminators preserved */
        if (result == 0) {
            ck_assert_str_eq(&out_mode[10], "");
            ck_assert_str_eq(&preview_mode[9], "");
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_UI_ParseModeInput_buffer_bounds);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}