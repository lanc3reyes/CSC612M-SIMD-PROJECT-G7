double euclidean_norm_c(int n, double* arr);
extern double euclidean_norm_asm(int n, double* arr);
extern double euclidean_norm_xmm(int n, double* arr);
extern double euclidean_norm_ymm(int n, double* arr);


void run_checks(int n, double* arr);
void run_boundary_checks(void);
void run_large_checks(int n);
void run_c_check(void);
