# CSC612M SIMD Programming Project - Group 7
# Euclidean Norm Kernels in C, x86-64 Assembly, and AVX2 (XMM and YMM)

## 1. Introduction

### 1.1 Objective

This report presents the implementation and evaluation of a kernel that computes the Euclidean norm of a vector. Given a vector A of n double-precision floating-point values, the kernel computes Z = √(Σ A[i]²) for i = 0, 1, …, n − 1 and stores the result in a double-precision scalar variable Z. The kernel was implemented in four versions, wherein each version was verified for correctness against the C version, and all four were timed across three vector sizes in both the Debug and Release configurations of Visual Studio.

In the Release configuration, the SIMD YMM kernel was 2.26 to 4.07 times faster than the optimized C kernel. It reached the theoretical speedup of four only at the smallest vector size. At the larger sizes, the speedup fell to about 2.3, a result that the analysis in Section 6 attributes to the memory bandwidth limit reached by the YMM kernel.

### 1.2 Kernel Implementations

The four versions differ in the number of elements processed per instruction and in how the running sum is accumulated, as summarized in Table 1.

| Kernel | Implementation | Elements per Instruction | Accumulation |
|---|---|---|---|
| C | C, compiled by Visual Studio | 1 | One running sum, in order |
| x86-64 | NASM, scalar SSE instructions | 1 | One running sum, in order |
| SIMD XMM | NASM, AVX2 on 128-bit XMM registers | 2 | Two partial sums |
| SIMD YMM | NASM, AVX2 on 256-bit YMM registers | 4 | Four partial sums |

*Table 1. Summary of the four kernel implementations*

The C kernel serves as the reference implementation. It iterates over the vector, squares each element, adds it to a running sum, and returns the square root of the sum. The x86-64 kernel performs the same steps in assembly using scalar instructions, which operate on a single double held in the low half of an XMM register.

The SIMD kernels load several adjacent elements into one register and square and accumulate them with a single instruction. The XMM kernel processes two doubles per instruction and therefore maintains two partial sums, while the YMM kernel processes four doubles per instruction and maintains four partial sums. When the main loop ends, each SIMD kernel processes any remaining elements individually, combines its partial sums into a single value, and takes the square root.

### 1.3 Experimental Setup

The specification requires vector sizes of n = 2^20, 2^26, and 2^30. A vector of 2^30 doubles requires 8 GiB of memory, which exceeded the 5.4 to 6.0 GiB of free memory on the test machine. The program therefore reduced the largest size to 2^29 (4 GiB) in both configurations, as permitted by the specification. Before allocating each vector, the program checks that it fits in the free physical memory, so that the operating system does not page the vector to disk during timing.

Each kernel was executed 30 times for each vector size, and the average execution time is reported. Only the kernel call is timed, using the Windows high-resolution performance counter (QueryPerformanceCounter). Memory allocation, initialization, and output are excluded from the measurements.

For the boundary test and the timed runs, each vector was initialized as A[i] = (i mod 100) + 1, so that its values run from 1 to 100 and then repeat. The standalone C correctness test uses A[i] = i + 1 with n = 1000. Both initializations have closed-form expected results, which are described in Section 4.

The program was built for the x64 platform in both the Debug and Release configurations of Visual Studio. The assembly kernels were assembled with NASM through a custom build step. The test machine specifications are as follows: AMD Ryzen 5 3600 (6 cores, 32 MB L3 cache), 16 GB DDR4-2666 RAM in dual-channel configuration, and Windows 11.

## 2. Program Output and Execution Time

The program was executed once in each build configuration. Each execution prints the build type, the result of the C correctness test, the result of the boundary test, and a timing table for each vector size. Each timing table lists the average execution time of every kernel, its result, and its speedup relative to the C kernel. Figures 1 and 2 show the complete output of the Debug and Release builds, respectively.

![Insert screenshot: program output, Debug build]()  
*Figure 1. Program output, Debug build*

![Insert screenshot: program output, Release build]()  
*Figure 2. Program output, Release build*

## 3. Comparative Execution Time

Table 2 compares the average execution times of all four kernels. The speedup of each kernel is computed as the execution time of the C kernel divided by the execution time of that kernel.

| Build | Size | C (ms) | x86-64 (ms) | XMM (ms) | YMM (ms) | x86-64 vs C | XMM vs C | YMM vs C |
|---|---|---|---|---|---|---|---|---|
| Release | 2^20 | 0.887 | 0.924 | 0.419 | 0.218 | 0.96× | 2.12× | 4.07× |
| Release | 2^26 | 57.806 | 56.250 | 36.211 | 25.544 | 1.03× | 1.60× | 2.26× |
| Release | 2^29 | 445.441 | 469.816 | 276.493 | 194.342 | 0.95× | 1.61× | 2.29× |
| Debug | 2^20 | 3.136 | 0.842 | 0.436 | 0.283 | 3.72× | 7.19× | 11.07× |
| Debug | 2^26 | 189.182 | 54.523 | 33.215 | 23.684 | 3.47× | 5.70× | 7.99× |
| Debug | 2^29 | 1516.663 | 453.523 | 284.133 | 206.996 | 3.34× | 5.34× | 7.33× |

*Table 2. Average execution time over 30 runs and speedup relative to C*

In the Release configuration, the SIMD YMM kernel is the fastest at every vector size, followed by the SIMD XMM kernel. The x86-64 and C kernels are effectively tied, with speedup ratios between 0.95 and 1.03. These small differences are attributed to run-to-run variation, since they do not favor either kernel consistently across vector sizes.

The large advantage of the x86-64 kernel in the Debug configuration is a consequence of the C kernel being compiled without optimization. In that configuration, the compiler stores the running sum to memory and reloads it on every iteration, whereas the hand-written kernel keeps the sum in a register. 
In the Release configuration, the compiler generates an equivalent scalar loop, which removes the difference.

The SIMD kernels are faster because each instruction squares and accumulates several elements at once. In the Release configuration at the two larger sizes, the XMM kernel is approximately 1.6 times faster than the x86-64 kernel, and the YMM kernel is approximately 1.4 times faster than the XMM kernel. At 2^20, the gains are larger, at approximately 2.2 and 1.9 times respectively, as discussed in Section 6.

The execution times of the three assembly kernels are similar in both configurations, because NASM assembles them the same way regardless of the build. Only the C kernel is affected by compiler optimization, and it runs approximately 3.3 to 3.5 times faster in the Release configuration. Consequently, the Debug speedups of up to 11.07 times overstate the benefit of SIMD, and the Release figures represent the appropriate comparison against optimized C.

## 4. Correctness Verification

All four kernels produced correct results in every test, in both configurations and at all vector sizes. The C kernel was verified against closed-form expected values, and the remaining three kernels were verified against the C kernel, which the specification designates as the reference result.

### 4.1 C Kernel

The C kernel is verified in two ways. First, a standalone test initializes A[i] = i + 1 with n = 1000. The sum of squares of the integers from 1 to n is given by n(n + 1)(2n + 1)/6, so the expected result is Z = √333,833,500 = 18271.111077, which the C kernel matched.

Second, every timed run computes an expected result for the vector initialized as A[i] = (i mod 100) + 1. Such a vector consists of complete blocks containing the values 1 to 100, followed by a partial block containing the values 1 to r. Each complete block contributes 1² + 2² + … + 100² = 338,350 to the sum of squares, which gives the following expected result:

Z = √( ⌊n / 100⌋ × 338,350 + r(r + 1)(2r + 1) / 6 ),   where r = n mod 100

The C kernel matched this expected value at every vector size, as shown in Figure 3.

![Insert screenshot: C correctness check]()  
*Figure 3. C correctness check*

### 4.2 x86-64 Kernel

The result of the x86-64 kernel was compared with the result of the C kernel at every vector size, and the program reported the output as correct in all cases (Figure 4). Because this kernel adds the elements in the same order as the C kernel, its results are expected to be identical to those of the C kernel.

![Insert screenshot: x86-64 correctness check]()  
*Figure 4. x86-64 correctness check*

### 4.3 SIMD XMM Kernel

The result of the SIMD XMM kernel was compared with the result of the C kernel at every vector size, and the program reported the output as correct in all cases (Figure 5).

![Insert screenshot: SIMD XMM correctness check]()  
*Figure 5. SIMD XMM correctness check*

### 4.4 SIMD YMM Kernel

The result of the SIMD YMM kernel was compared with the result of the C kernel at every vector size, and the program reported the output as correct in all cases (Figure 6).

![Insert screenshot: SIMD YMM correctness check]()  
*Figure 6. SIMD YMM correctness check*

### 4.5 Floating-Point Tolerance

Every floating-point addition is rounded to the nearest representable double-precision value. As a result, the order in which values are added can change the final digits of a sum. For example, (0.1 + 0.2) + 0.3 evaluates to 0.6000000000000001, while 0.1 + (0.2 + 0.3) evaluates to 0.6.

The C and x86-64 kernels add the squared elements in order: A[0]², then A[1]², then A[2]², and so on. The SIMD kernels do not. The YMM kernel, for instance, accumulates A[0]², A[4]², A[8]², and so on in one partial sum, and A[1]², A[5]², A[9]², and so on in another, combining the four partial sums only at the end. Because the additions occur in a different order, rounding errors accumulate differently, and the SIMD results may differ from the C result in the last decimal places even though both are correct.

For this reason, an exact comparison could reject a correct SIMD kernel. Instead, a result is considered correct if it differs from the C result by no more than 10^-9 times the magnitude of the C result. In a test with the decimal initialization A[i] = sin(0.001i), the observed relative differences between kernels were on the order of 10^-13, several orders of magnitude below the tolerance. A genuine error, such as an omitted or double-counted element, would change the result by far more than this tolerance.

The final program uses the initialization A[i] = (i mod 100) + 1 because its expected result has a simple closed form and because it initializes large vectors faster than evaluating a sine for every element. Since every value in this vector is an integer, all sums are computed exactly, and the four kernels produce identical results. The tolerance is retained so that the correctness check remains valid for other initializations.

## 5. Boundary Handling

Both SIMD kernels correctly handle vector sizes that are not a multiple of their register width. Each SIMD kernel first processes as many complete groups of elements as fit within the vector, two per iteration for the XMM kernel and four per iteration for the YMM kernel. A scalar loop then processes the remaining elements one at a time.

To test this behavior, the kernels were run on vectors of length 1001, 1002, and 1003. These lengths were chosen so that the YMM kernel is tested with every possible number of leftover elements, as shown in Table 3.

| Length | XMM Full Steps | XMM Leftover | YMM Full Steps | YMM Leftover | Expected Z |
|---|---|---|---|---|---|
| 1001 | 500 | 1 | 250 | 1 | 1839.429531 |
| 1002 | 501 | 0 | 250 | 2 | 1839.430618 |
| 1003 | 501 | 1 | 250 | 3 | 1839.433065 |

*Table 3. Leftover elements for each boundary test length*

If a kernel failed to process the leftover elements, its result would be smaller than the expected value, and the comparison against the C kernel would fail. Both SIMD kernels matched the C kernel and the expected value at all three lengths (Figure 7).

![Insert screenshot: boundary check for SIMD XMM and SIMD YMM]()  
*Figure 7. Boundary check for SIMD XMM and SIMD YMM*

## 6. Performance Analysis

### 6.1 Relative Performance of the Kernels

In the Release configuration, the kernels rank from fastest to slowest as YMM, XMM, and then x86-64 and C, which perform comparably. Wider registers allow more elements to be processed per instruction, which reduces the number of loop iterations from n for the scalar kernels to n/2 for the XMM kernel and n/4 for the YMM kernel.

### 6.2 Comparison with Optimized C

The SIMD YMM kernel was 2.26 to 4.07 times faster than the optimized C kernel. In the Release configuration, the C kernel ran at roughly the same speed as the hand-written scalar kernel, which indicates that the compiler did not vectorize the loop. By default, Visual Studio preserves the order of floating-point operations as written in the source code. Vectorizing this loop would change the order of the additions and potentially the final digits of the result, as discussed in Section 4.5, so the compiler retains the scalar loop.

### 6.3 Deviation from Theoretical Speedup

Since the XMM and YMM kernels process two and four elements per instruction, their theoretical speedups over the scalar kernels are two and four, respectively. The measured speedups reach these values at the smallest vector size but are lower at the larger sizes. To identify the limiting factor, the Release execution times were converted into data throughput, defined as the number of bytes read divided by the execution time (Table 4).

| Size | Bytes Read | C | x86-64 | XMM | YMM |
|---|---|---|---|---|---|
| 2^20 | 8 MiB | 9.5 GB/s | 9.1 GB/s | 20.0 GB/s | 38.5 GB/s |
| 2^26 | 512 MiB | 9.3 GB/s | 9.5 GB/s | 14.8 GB/s | 21.0 GB/s |
| 2^29 | 4 GiB | 9.6 GB/s | 9.1 GB/s | 15.5 GB/s | 22.1 GB/s |

*Table 4. Data throughput of each kernel in the Release configuration*

If all four kernels were limited only by memory bandwidth, they would read data at the same rate. Instead, the YMM kernel reads data more than twice as fast as the scalar kernels from the same memory, which shows that the scalar kernels are not limited by memory.

The scalar kernels are instead limited by their chain of dependent additions. Each addition to the running sum must wait for the previous addition to complete, which takes several clock cycles. During each such interval, the scalar kernels add one element, the XMM kernel adds two, and the YMM kernel adds four, so the ideal speed ratio is 1 : 2 : 4. At 2^20, the measured ratio relative to the x86-64 kernel is approximately 1 : 2.2 : 4.2, matching the ideal. At the two larger sizes, it drops to approximately 1 : 1.6 : 2.3. The SIMD kernels therefore reach their ideal speedups when the data is supplied quickly enough, but fall short of them at the larger sizes.

### 6.4 Memory Bandwidth Limitation

The throughput of the YMM kernel levels off at approximately 21 to 22 GB/s at the two larger vector sizes. This ceiling is most likely the practical memory bandwidth available to a single core, which means that further widening the arithmetic would yield little additional speedup. The test machine’s dual-channel DDR4-2666 memory has a theoretical peak bandwidth of 42.7 GB/s, but a single core typically achieves only around half of this peak, which is consistent with the observed ceiling. At 2^20, by contrast, the YMM kernel reaches 38.5 GB/s, taking 0.21 ns per element compared with 0.36 to 0.38 ns at the larger sizes. This is because the 8 MiB vector fits in the processor’s 32 MB L3 cache, which supplies data faster than main memory, while the 512 MiB and 4 GiB vectors must be read from main memory.

The kernel performs only two floating-point operations, one multiplication and one addition, for every 8 bytes it reads. Without SIMD, the latency of the dependent additions is the bottleneck. With the YMM kernel, the arithmetic becomes fast enough that memory bandwidth becomes the limiting factor. The vectorized Euclidean norm is therefore memory-bound at large vector sizes, while the scalar versions are limited by computation.

## 7. Discussion

### 7.1 Problems Encountered and Solutions

Development initially began on macOS, but code written there could not be carried over directly, because macOS and Windows pass function arguments in different registers (rdi and rsi on macOS, rcx and rdx on Windows). All development was therefore moved to Windows and Visual Studio to match the environment used in the course.

A vector of 2^30 doubles requires 8 GiB, which exceeded the 5.4 to 6.0 GiB of available memory. Windows may still satisfy such an allocation by paging memory to disk, which would severely distort the timing results. The program therefore checks the free physical memory before allocating each vector and reduces the size to 2^29, and then 2^28, if the vector does not fit.

Finally, an exact comparison of floating-point results proved unreliable for the SIMD kernels, because they add the elements in a different order. This was resolved by comparing each result with the C result using a relative tolerance of 10^-9, as described in Section 4.5. [Add any problems encountered while implementing the assembly kernels.]

### 7.2 Methodology

Several design choices were made to strengthen the reliability of the results. First, the C kernel was not trusted by default as the reference. It was verified against closed-form expected values before being used to judge the other kernels, so that an error in the reference could not go unnoticed.

Second, the boundary test lengths of 1001, 1002, and 1003 were chosen so that the YMM kernel is tested with one, two, and three leftover elements, which covers every possible case. 

Third, all four kernels are stored in a single array of function pointers and measured by the same timing routine, which ensures that every kernel is timed under identical conditions.

Fourth, the program determines the largest feasible vector size from the free physical memory of the machine, which avoids both allocation failures and timing distortion due to paging. Lastly, each run reports the build configuration, the vector size as a power of two, the first and last ten values of the vector, and the speedup of each kernel, which makes the output self-documenting.

### 7.3 Key Insights

The most significant finding of this project is that doubling the register width does not always double performance. At the two larger vector sizes, the YMM kernel was only about 1.4 times faster than the XMM kernel, compared with about 1.9 times at 2^20, because once the data no longer fits in cache, the processor spends most of its time waiting for data from memory rather than performing arithmetic.

A second insight is that floating-point rounding explains two separate observations. The same reordering of additions that requires a tolerance in the correctness check is also the reason the compiler does not vectorize the C kernel on its own. Hand-written SIMD code effectively accepts a small difference in rounding that the compiler, by default, does not.

A third insight is that Debug measurements overstate the benefit of SIMD. The assembly kernels run at similar speeds in both configurations, while the C kernel becomes approximately 3.3 to 3.5 times faster in Release. A comparison based only on the Debug build would suggest a speedup of up to 11.07 times for the YMM kernel, whereas the speedup over optimized C is 2.26 to 4.07 times.

Finally, hand-written scalar assembly offered no advantage over optimized C. In the Release configuration, the x86-64 kernel and the C kernel performed comparably, which shows that the performance gains in this project come entirely from SIMD.

## 8. Conclusion

The Euclidean norm kernel was successfully implemented in C, x86-64 assembly, and AVX2 assembly using XMM and YMM registers. All four kernels produced correct results at every vector size, including vector lengths that are not a multiple of the SIMD register width. In the Release configuration, the YMM kernel was the fastest, at 2.26 to 4.07 times the speed of the optimized C kernel. The speedup reaches the theoretical factor of four only when the vector fits in cache. At larger sizes, it falls to about 2.3 because the vectorized kernel is limited by the memory bandwidth of the machine. These results show that the benefit of SIMD for this kernel is ultimately bounded by how quickly data can be supplied from memory.
