/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void transpose_32x32(int M, int N, int A[N][M], int B[M][N]);
void transpose_64x64(int M, int N, int A[N][M], int B[M][N]);
void transpose_61x67(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    if (M == 32 && N == 32) {
        transpose_32x32(M, N, A, B);
    }
    if (M == 64 && N == 64) {
        transpose_64x64(M, N, A, B);
    }
    if (M == 61 && N == 67) {
        transpose_61x67(M, N, A, B);
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

void transpose_32x32(int M, int N, int A[N][M], int B[M][N]) {
    // s = 5, E = 1, b = 5
    // 总变量：4 个循环变量 + 8 个临时变量 = 12 个变量
    int v0, v1, v2, v3, v4, v5, v6, v7;

    // 先把 A 复制 B，再转置 B，避免因为 A 的下一次读驱逐 B 的同一行，导致 B
    // 的下一次写 MISS 8*8 分块 总 MISS：16(块数)*[8(读)+8(写)] = 256 显示
    // MISS =  260，但是通过添加 trans()
    // 的代码并清空缓存，然后对比测试差异，可知实际只有 256 个 MISS 故猜测那
    // 4 个多的 MISS 可能是别的函数调用所致，也可通过观察 trace.f0
    // 发现确实开头多了 1 个 S 和 3 个 L
    for (int i = 0; i < N; i += 8) {      // 当前行
        for (int j = 0; j < M; j += 8) {  // 当前列
            // 首先将 A[i][j]~A[i+7][j+7] 复制到 B[j][i]~B[j+7][i+7]
            for (int k = 0; k < 8; ++k) {
                v0 = A[i + k][j];
                v1 = A[i + k][j + 1];
                v2 = A[i + k][j + 2];
                v3 = A[i + k][j + 3];
                v4 = A[i + k][j + 4];
                v5 = A[i + k][j + 5];
                v6 = A[i + k][j + 6];
                v7 = A[i + k][j + 7];
                B[j + k][i] = v0;
                B[j + k][i + 1] = v1;
                B[j + k][i + 2] = v2;
                B[j + k][i + 3] = v3;
                B[j + k][i + 4] = v4;
                B[j + k][i + 5] = v5;
                B[j + k][i + 6] = v6;
                B[j + k][i + 7] = v7;
            }
            // 转置 B
            for (int k = 0; k < 8; ++k) {
                // 对角线不用交换
                for (int l = 0; l < k; ++l) {
                    v0 = B[j + k][i + l];
                    B[j + k][i + l] = B[j + l][i + k];
                    B[j + l][i + k] = v0;
                }
            }
        }
    }
}

void transpose_64x64(int M, int N, int A[N][M], int B[M][N]) {
    // loop indices
    int i, j, ii, jj;
    // temporary variables
    int a0, a1, a2, a3, a4, a5, a6, a7;
    // main loop: ii, jj for each block of size 8x8

    for (jj = 0; jj < N; jj += 8) {
        // process diagonal blocks first

        // ii: j-index of target block (block-shifting)
        // more specifically, use the upper half of [jj, ii] to transpose [jj,
        // jj] block the target block is the one that will be used immediately
        // after the diagonal processing
        if (jj == 0)
            ii = 8;
        else
            ii = 0;

        // move the lower 4x8 blocks from A to B, with block-shifting to the
        // target block
        for (i = jj; i < jj + 4; ++i) {
            a0 = A[i + 4][jj + 0];
            a1 = A[i + 4][jj + 1];
            a2 = A[i + 4][jj + 2];
            a3 = A[i + 4][jj + 3];
            a4 = A[i + 4][jj + 4];
            a5 = A[i + 4][jj + 5];
            a6 = A[i + 4][jj + 6];
            a7 = A[i + 4][jj + 7];

            B[i][ii + 0] = a0;
            B[i][ii + 1] = a1;
            B[i][ii + 2] = a2;
            B[i][ii + 3] = a3;
            B[i][ii + 4] = a4;
            B[i][ii + 5] = a5;
            B[i][ii + 6] = a6;
            B[i][ii + 7] = a7;
        }

        // taking transpose of lower-left and lower-right 4x4 within themselves
        // respectively
        for (i = 0; i < 4; ++i) {
            for (j = i + 1; j < 4; ++j) {
                a0 = B[jj + i][ii + j];
                B[jj + i][ii + j] = B[jj + j][ii + i];
                B[jj + j][ii + i] = a0;

                a0 = B[jj + i][ii + j + 4];
                B[jj + i][ii + j + 4] = B[jj + j][ii + i + 4];
                B[jj + j][ii + i + 4] = a0;
            }
        }

        // moving the upper 4x8 blocks from A to B
        for (i = jj; i < jj + 4; ++i) {
            a0 = A[i][jj + 0];
            a1 = A[i][jj + 1];
            a2 = A[i][jj + 2];
            a3 = A[i][jj + 3];
            a4 = A[i][jj + 4];
            a5 = A[i][jj + 5];
            a6 = A[i][jj + 6];
            a7 = A[i][jj + 7];

            B[i][jj + 0] = a0;
            B[i][jj + 1] = a1;
            B[i][jj + 2] = a2;
            B[i][jj + 3] = a3;
            B[i][jj + 4] = a4;
            B[i][jj + 5] = a5;
            B[i][jj + 6] = a6;
            B[i][jj + 7] = a7;
        }

        // taking transpose of upper-left and upper-right 4x4 within themselves
        // respectively
        for (i = jj; i < jj + 4; ++i) {
            for (j = i + 1; j < jj + 4; ++j) {
                a0 = B[i][j];
                B[i][j] = B[j][i];
                B[j][i] = a0;

                a0 = B[i][j + 4];
                B[i][j + 4] = B[j][i + 4];
                B[j][i + 4] = a0;
            }
        }

        // swaping the lower-left and upper-right
        for (i = 0; i < 4; ++i) {
            a0 = B[jj + i][jj + 4];
            a1 = B[jj + i][jj + 5];
            a2 = B[jj + i][jj + 6];
            a3 = B[jj + i][jj + 7];

            B[jj + i][jj + 4] = B[jj + i][ii + 0];
            B[jj + i][jj + 5] = B[jj + i][ii + 1];
            B[jj + i][jj + 6] = B[jj + i][ii + 2];
            B[jj + i][jj + 7] = B[jj + i][ii + 3];

            B[jj + i][ii + 0] = a0;
            B[jj + i][ii + 1] = a1;
            B[jj + i][ii + 2] = a2;
            B[jj + i][ii + 3] = a3;
        }

        // filling the original lower 4x8 from the block-shifting block
        for (i = 0; i < 4; ++i) {
            B[jj + i + 4][jj + 0] = B[jj + i][ii + 0];
            B[jj + i + 4][jj + 1] = B[jj + i][ii + 1];
            B[jj + i + 4][jj + 2] = B[jj + i][ii + 2];
            B[jj + i + 4][jj + 3] = B[jj + i][ii + 3];
            B[jj + i + 4][jj + 4] = B[jj + i][ii + 4];
            B[jj + i + 4][jj + 5] = B[jj + i][ii + 5];
            B[jj + i + 4][jj + 6] = B[jj + i][ii + 6];
            B[jj + i + 4][jj + 7] = B[jj + i][ii + 7];
        }

        // processing off-diagonal blocks
        for (ii = 0; ii < M; ii += 8) {
            if (ii == jj) {
                // skip diagonal blocks
                continue;
            } else {
                // taking transpose of upper-left 4x4 and upper-right 4x4 within
                // themselves respectively
                for (i = ii; i < ii + 4; ++i) {
                    a0 = A[i][jj + 0];
                    a1 = A[i][jj + 1];
                    a2 = A[i][jj + 2];
                    a3 = A[i][jj + 3];
                    a4 = A[i][jj + 4];
                    a5 = A[i][jj + 5];
                    a6 = A[i][jj + 6];
                    a7 = A[i][jj + 7];

                    B[jj + 0][i] = a0;
                    B[jj + 1][i] = a1;
                    B[jj + 2][i] = a2;
                    B[jj + 3][i] = a3;
                    B[jj + 0][i + 4] = a4;
                    B[jj + 1][i + 4] = a5;
                    B[jj + 2][i + 4] = a6;
                    B[jj + 3][i + 4] = a7;
                }

                // taking transpose of lower-left 4x4 and store to upper-right
                // 4x4, and move upper-right 4x4 to lower-left 4x4
                for (j = jj; j < jj + 4; ++j) {
                    a0 = A[ii + 4][j];
                    a1 = A[ii + 5][j];
                    a2 = A[ii + 6][j];
                    a3 = A[ii + 7][j];

                    a4 = B[j][ii + 4];
                    a5 = B[j][ii + 5];
                    a6 = B[j][ii + 6];
                    a7 = B[j][ii + 7];

                    B[j][ii + 4] = a0;
                    B[j][ii + 5] = a1;
                    B[j][ii + 6] = a2;
                    B[j][ii + 7] = a3;

                    B[j + 4][ii + 0] = a4;
                    B[j + 4][ii + 1] = a5;
                    B[j + 4][ii + 2] = a6;
                    B[j + 4][ii + 3] = a7;
                }

                // taking transpose of lower-right 4x4
                for (i = ii + 4; i < ii + 8; ++i) {
                    a0 = A[i][jj + 4];
                    a1 = A[i][jj + 5];
                    a2 = A[i][jj + 6];
                    a3 = A[i][jj + 7];

                    B[jj + 4][i] = a0;
                    B[jj + 5][i] = a1;
                    B[jj + 6][i] = a2;
                    B[jj + 7][i] = a3;
                }
            }
        }
    }
}

void transpose_61x67(int M, int N, int A[N][M], int B[M][N]) {
    int i, j;
    for (i = 0; i < N; i += 17) {
        for (j = 0; j < M; j += 17) {
            for (int x = i; x < N && x < i + 17; ++x)
                for (int y = j; y < M && y < j + 17; ++y)
                    B[y][x] = A[x][y];
        }
    }
}

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    // registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
