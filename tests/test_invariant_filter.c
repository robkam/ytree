#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

START_TEST(test_buffer_reads_never_exceed_declared_length)
{
    // Invariant: Buffer reads never exceed the declared length
    const char *payloads[] = {
        "normal",                    // Valid input
        "A",                         // Boundary: single char
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",  // 100 chars - exceeds typical buffer
        "X"  // Will be replaced with exploit payload
    };
    
    // Replace X with actual exploit payload from vulnerability context
    char exploit_payload[256];
    memset(exploit_payload, 'A', 255);
    exploit_payload[255] = '\0';
    payloads[3] = exploit_payload;
    
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        // Create a test file with the payload
        FILE *test_file = fopen("test_input.txt", "w");
        ck_assert_ptr_nonnull(test_file);
        fprintf(test_file, "%s\n", payloads[i]);
        fclose(test_file);
        
        // Fork and exec the actual filter program with the test input
        pid_t pid = fork();
        if (pid == 0) {
            // Child process: execute the actual filter.c program
            freopen("test_input.txt", "r", stdin);
            execl("./filter", "filter", NULL);
            perror("execl failed");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            // Parent process: wait for child and check exit status
            int status;
            waitpid(pid, &status, 0);
            
            // The security property: program should not crash (SIGSEGV, SIGABRT)
            ck_assert_msg(!WIFSIGNALED(status), 
                         "Program crashed with signal %d for payload %d: %s",
                         WTERMSIG(status), i, payloads[i]);
            
            // Clean up
            unlink("test_input.txt");
        } else {
            ck_abort_msg("fork failed");
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

    tcase_add_test(tc_core, test_buffer_reads_never_exceed_declared_length);
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