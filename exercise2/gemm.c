/*
 * authors   : Giuseppe Piero Brandino - eXact-lab s.r.l.
 * date      : October 2018
 * copyright : GNU Public License
 * modifiedby: Stefano Cozzini for DSSC usage
 */

#define min(x,y) (((x) < (y)) ? (x) : (y))

#include <stdio.h>
#include <stdlib.h>

#ifdef MKL
#include "mkl_cblas.h"
#endif

#ifdef OPENBLAS
#include "cblas.h"
#endif

#ifdef BLIS
#include "cblas.h"
#endif

#include <time.h>

#ifdef USE_FLOAT
#define MYFLOAT float
#define GEMMCPU cblas_sgemm
#endif

#ifdef USE_DOUBLE
#define MYFLOAT double
#define GEMMCPU cblas_dgemm
#endif

struct timespec diff(struct timespec start, struct timespec end)
{
        struct timespec temp;
        if ((end.tv_nsec - start.tv_nsec) < 0) {
            temp.tv_sec = end.tv_sec - start.tv_sec - 1;
            temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
        } else {
            temp.tv_sec = end.tv_sec - start.tv_sec;
            temp.tv_nsec = end.tv_nsec - start.tv_nsec;
        }
        return temp;
}

int main(int argc, char** argv)
{
    MYFLOAT *A, *B, *C;
    int m, n, k, i, j;
    MYFLOAT alpha, beta;
    struct timespec begin, end;

    if (argc == 1)
    {
        m = 2000, k = 200, n = 1000;
    }
    else if (argc == 4)
    {
        m = atoi(argv[1]);
        k = atoi(argv[2]);
        n = atoi(argv[3]);
    }
    else
    {
    printf( "Usage: %s M K N, the corresponding matrices will be A(M,K) B(K,N)\n", argv[0]);
    return 0;
    }

    // initializing data for matrix multiplication C=A*B for matrix
    alpha = 1.0;
    beta = 0.0;

    A = (MYFLOAT *)malloc( m*k*sizeof(MYFLOAT));
    B = (MYFLOAT *)malloc( k*n*sizeof(MYFLOAT));
    C = (MYFLOAT *)malloc( m*n*sizeof(MYFLOAT));
    if (A == NULL || B == NULL || C == NULL) {
      printf( "\n ERROR: Can't allocate memory for matrices. Aborting... \n\n");
      free(A);
      free(B);
      free(C);
      return 1;
    }

    for (i = 0; i < (m*k); i++) {
        A[i] = (MYFLOAT)(i + 1);
    }

    for (i = 0; i < (k*n); i++) {
        B[i] = (MYFLOAT)(-i - 1);
    }

    for (i = 0; i < (m*n); i++) {
        C[i] = 0.0;
    }

    // computing matrix product using gemm function via CBLAS interface
    clock_gettime(CLOCK_MONOTONIC, &begin);
    GEMMCPU(CblasColMajor, CblasNoTrans, CblasNoTrans, m, n, k, alpha, A, m, B, k, beta, C, m);
    clock_gettime(CLOCK_MONOTONIC, &end);

    struct timespec elapsed_time = diff(begin, end);
    double elapsed = (double)elapsed_time.tv_sec + (double)elapsed_time.tv_nsec / 1000000000.0;

    double gflops = 2.0 * m * n * k;
    gflops = gflops / elapsed * 1000000000;
    printf("%d,%d,%d,%.9f,%.6f\n", m, n, k, elapsed, gflops);


#ifdef PRINT
    printf (" Top left corner of matrix A:\n");
    for (i = 0; i < min(m,6); i++) {
      for (j = 0; j < min(k,6); j++) {
        printf ("%12.0f", A[i+j*m]);
      }
      printf ("\n");
    }

    printf ("\n Top left corner of matrix B:\n");
    for (i = 0; i < min(k,6); i++) {
      for (j = 0; j < min(n,6); j++) {
        printf ("%12.0f", B[i+j*k]);
      }
      printf ("\n");
    }

    printf ("\n Top left corner of matrix C: \n");
    for (i = 0; i < min(m,6); i++) {
      for (j = 0; j < min(n,6); j++) {
        printf ("%12.5G", C[i+j*m]);
      }
      printf ("\n");
    }
#endif

    free(A);
    free(B);
    free(C);

    return 0;
}