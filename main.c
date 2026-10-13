#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include "functions.h"

#define RUNS 30

static int correctness_failed = 0;

//Checks if a kernel's result is within 1e-9 of the C result
#define CLOSE(a, b) (fabs((a) - (b)) <= 1e-9 * fabs(b))

//Function pointer type for the kernels, so all 4 can be stored in one array
typedef double (*kernel_fn)(int n, double* arr);

static kernel_fn kernels[4] = { euclidean_norm_c, euclidean_norm_asm, euclidean_norm_xmm, euclidean_norm_ymm };
static const char* names[4] = { "C", "x86-64", "XMM", "YMM" };


int main(void)
{
#ifdef _DEBUG
    printf("Build: DEBUG\n\n");
#else
    printf("Build: RELEASE\n\n");
#endif

    //Checks the C kernel against a known closed-form answer
    run_c_check();

    //Checks that each kernel handles leftover (boundary) elements correctly
    printf("\n------------------------------------------------------------\n");
    run_boundary_checks();

    //Times each kernel over 30 runs for each vector size
    printf("\n------------------------------------------------------------\n");
    printf("Initialization: A[i] = (i %% 100) + 1\n");
    printf("Expected Z = sqrt((n / 100) * 338350 + r(r+1)(2r+1)/6), r = n %% 100\n");
    printf("Each kernel runs %d times\n\n", RUNS);

    run_large_checks(20);
    run_large_checks(26);
    run_large_checks(30);

    if (correctness_failed) {
        printf("\nRESULT: FAIL - one or more correctness checks failed.\n");
        return 1;
    }

    printf("\nRESULT: PASS - all correctness checks passed.\n");
    return 0;
}

//C kernel, used as the reference result
double euclidean_norm_c(int n, double* arr)
{
    double sum = 0.0;

    for (int i = 0; i < n; i++) {
        double value = arr[i];
        sum += value * value;
    }

    return sqrt(sum);
}


//Closed-form Z for A[i] = (i % 100) + 1: each full block of 100 adds 1^2 + ... + 100^2 = 338350, then the r leftover elements add 1^2 + ... + r^2
static double expected_norm(int n)
{
    long long blocks = n / 100;
    long long r = n % 100;
    return sqrt((double)(blocks * 338350 + r * (r + 1) * (2 * r + 1) / 6));
}


//Runs a kernel 30 times and returns its average execution time
static double time_kernel(kernel_fn kernel, int n, double* arr, double* result)
{
    LARGE_INTEGER freq, start, end;
    double total = 0.0;

    QueryPerformanceFrequency(&freq);

    for (int run = 0; run < RUNS; run++) {
        QueryPerformanceCounter(&start);
        *result = kernel(n, arr);
        QueryPerformanceCounter(&end);
        total += (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
    }

    return total / RUNS;
}


//C-only check initialization A[i] = i + 1; the sum of squares is n(n+1)(2n+1)/6
void run_c_check(void)
{
    int n = 1000;
    double arr[1000];

    for (int i = 0; i < n; i++) {
        arr[i] = i + 1;
    }

    double expected = sqrt((double)n * (n + 1) * (2 * n + 1) / 6.0);
    double z = euclidean_norm_c(n, arr);

    printf("C correctness test\n\n");
    printf("Initialization: A[i] = i + 1, n = %d\n", n);
    printf("Values: ");
    for (int i = 0; i < 10; i++) {
        printf("%g ", arr[i]);
    }
    printf("... ");
    for (int i = n - 10; i < n; i++) {
        printf("%g ", arr[i]);
    }
    printf("\n\n");

    printf("Expected Z = sqrt(n(n+1)(2n+1)/6) = %.6f\n", expected);
    printf("C Z        = %.6f\n\n", z);

    int ok = CLOSE(z, expected);

    printf("C output is %s\n", ok ? "correct" : "incorrect");

    if (!ok) {
        correctness_failed = 1;
    }
}


//Times all kernels, checks C against the expected value and the other kernels against C
void run_checks(int n, double* arr)
{
    double expected = expected_norm(n);
    double z[4], t[4];

    printf("Expected Z: %.6f\n\n", expected);

    for (int k = 0; k < 4; k++) {
        t[k] = time_kernel(kernels[k], n, arr, &z[k]);
    }

    printf("%-8s %14s %20s %8s\n", "Kernel", "Avg time (ms)", "Z", "vs C");
    for (int k = 0; k < 4; k++) {
        printf("%-8s %14.6f %20.6f %7.2fx\n", names[k], t[k] * 1000.0, z[k], t[0] / t[k]);
    }

    printf("\n");

    int c_ok = CLOSE(z[0], expected);

    printf("C output is %s\n", c_ok ? "correct" : "incorrect");

    if (!c_ok) {
        correctness_failed = 1;
    }

    for (int k = 1; k < 4; k++) {
        int ok = CLOSE(z[k], z[0]);

        printf("%s output is %s\n", names[k], ok ? "correct" : "incorrect");

        if (!ok) {
            correctness_failed = 1;
        }
    }
}


//Boundary test: sizes 1001, 1002, 1003 leave YMM with 1, 2, 3 leftover elements and XMM with 1, 0, 1
void run_boundary_checks(void)
{
    int sizes[3] = { 1001, 1002, 1003 };
    double arr[1003];
    double z[4][3];

    for (int i = 0; i < 1003; i++) {
        arr[i] = (i % 100) + 1;
    }

    printf("SIMD boundary and tail-handling test\n\n");
    printf("Initialization: A[i] = (i %% 100) + 1\n");
    printf("Values: ");
    for (int i = 0; i < 10; i++) {
        printf("%g ", arr[i]);
    }
    printf("...\n\n");

    for (int k = 0; k < 4; k++) {
        for (int s = 0; s < 3; s++) {
            z[k][s] = kernels[k](sizes[s], arr);
        }
    }

    printf("%-8s", "Kernel");
    for (int s = 0; s < 3; s++) {
        printf("     Z (length = %d)", sizes[s]);
    }
    printf("\n");

    printf("%-8s", "Expected");
    for (int s = 0; s < 3; s++) {
        printf("%22.6f", expected_norm(sizes[s]));
    }
    printf("\n");

    for (int k = 0; k < 4; k++) {
        printf("%-8s", names[k]);
        for (int s = 0; s < 3; s++) {
            printf("%22.6f", z[k][s]);
        }
        printf("\n");
    }

    printf("\n");
    for (int k = 0; k < 4; k++) {
        int ok = 1;
        for (int s = 0; s < 3; s++) {
            //C is checked against the expected value, the other kernels against C
            double reference = (k == 0) ? expected_norm(sizes[s]) : z[0][s];
            if (!CLOSE(z[k][s], reference)) {
                ok = 0;
            }
        }
        printf("%s output is %s\n", names[k], ok ? "correct" : "incorrect");

        if (!ok) {
            correctness_failed = 1;
        }
    }
}

// Tests all kernels with n = 2^exponent.
// If the vector does not fit in free memory, tries smaller sizes down to 2^28.
void run_large_checks(int exponent)
{
    const int min_exponent = 28;

    MEMORYSTATUSEX mem;
    int n = 0;
    size_t bytes = 0;
    double* arr = NULL;

    while (1) {
        n = 1 << exponent;
        bytes = (size_t)n * sizeof(double);

        mem.dwLength = sizeof(mem);

        // Check available physical memory before attempting allocation.
        if (GlobalMemoryStatusEx(&mem) &&
            (unsigned long long)bytes <= mem.ullAvailPhys) {
            arr = malloc(bytes);
        }
        else {
            arr = NULL;
        }

        if (arr != NULL || exponent <= min_exponent) {
            break;
        }

        printf("Could not allocate 2^%d elements "
            "(%.1f GiB required, %.1f GiB free). Trying 2^%d.\n\n",
            exponent,
            bytes / (1024.0 * 1024.0 * 1024.0),
            mem.ullAvailPhys / (1024.0 * 1024.0 * 1024.0),
            exponent - 1);

        exponent--;
    }

    if (arr == NULL) {
        printf("Could not allocate 2^%d elements (%.1f GiB required). Skipping.\n\n",
            exponent, bytes / (1024.0 * 1024.0 * 1024.0));
        return;
    }

    for (int i = 0; i < n; i++) {
        arr[i] = (i % 100) + 1;
    }

    printf("Length: %d (2^%d)\n\n", n, exponent);

    printf("Values: ");
    for (int i = 0; i < 10; i++) {
        printf("%g ", arr[i]);
    }

    printf("... ");
    for (int i = n - 10; i < n; i++) {
        printf("%g ", arr[i]);
    }
    printf("\n\n");

    run_checks(n, arr);

    printf("\n");
    free(arr);
}