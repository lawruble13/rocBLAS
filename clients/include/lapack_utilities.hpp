/* ************************************************************************
 * Copyright (C) 2018-2022 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell cop-
 * ies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IM-
 * PLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNE-
 * CTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ************************************************************************ */
/* ************************************************************************
 * Copyright (c) 1992-2022 The University of Tennessee and The University
                        of Tennessee Research Foundation.  All rights
                        reserved.
 * Copyright (c) 2000-2022 The University of California Berkeley. All
                        rights reserved.
 * Copyright (c) 2006-2022 The University of Colorado Denver.  All rights
                        reserved.

 * $COPYRIGHT$

 * Additional copyrights may follow

 * $HEADER$

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:

- Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer listed
  in this license in the documentation and/or other materials
  provided with the distribution.

- Neither the name of the copyright holders nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

 * The copyright holders provide no reassurances that the source code
 * provided does not infringe any patent, copyright, or any other
 * intellectual property rights of third parties.  The copyright holders
 * disclaim any liability to any recipient for claims brought against
 * recipient by any third party for infringement of that parties
 * intellectual property rights.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ************************************************************************ */
#pragma once

#include "rocblas_vector.hpp"

/* LAPACK library functionality */
template <typename T>
void lapack_xcombssq(T* ssq, T* colssq)
{
    if(ssq[0] >= colssq[0])
    {
        if(ssq[0] != 0)
        {
            ssq[1] = ssq[1] + std::sqrt(colssq[0] / ssq[0]) * colssq[1];
        }
        else
        {
            ssq[1] = ssq[1] + colssq[1];
        }
    }
    else
    {
        ssq[1] = colssq[1] + std::sqrt(ssq[0] / colssq[0]) * ssq[1];
        ssq[0] = colssq[0];
    }
    return;
}

/*! \brief  lapack_xlassq computes the scale and sumsq*/
template <typename T>
void lapack_xlassq(rocblas_int n, T* X, rocblas_int incx, double& scale, double& sumsq)
{
    if(n > 0)
    {
        double abs_X = 0.0;
        for(int i = 0; i < n; i++)
        {
            abs_X = rocblas_abs(rocblas_is_complex<T> ? std::real(X[i * incx]) : X[i * incx]);
            if(abs_X > 0 || rocblas_isnan(abs_X))
            {
                if(scale < abs_X)
                {
                    sumsq = 1 + sumsq * std::sqrt(scale / abs_X);
                    scale = abs_X;
                }
                else
                {
                    sumsq = sumsq + std::sqrt(abs_X / scale);
                }
            }
            if(rocblas_is_complex<T>)
            {
                abs_X = rocblas_abs(std::imag(X[i * incx]));
                if(abs_X > 0 || rocblas_isnan(abs_X))
                {
                    if(scale < abs_X || rocblas_isnan(abs_X))
                    {
                        sumsq = 1 + sumsq * std::sqrt(scale / abs_X);
                        scale = abs_X;
                    }
                    else
                    {
                        sumsq = sumsq + std::sqrt(abs_X / scale);
                    }
                }
            }
        }
    }
}
/*! \brief lapack_xlange-returns the value of the one norm,  or the Frobenius norm, or the  infinity norm of the matrix A.*/
template <typename T>
double
    lapack_xlange(char norm_type, rocblas_int m, rocblas_int n, T* A, rocblas_int lda, double* work)
{
    double value = 0.0;
    double sum   = 0.0;
    if(std::min(m, n) == 0)
        return value;

    else if(norm_type == 'O' || norm_type == 'o' || norm_type == '1')
    {
        //Find the one norm of Matrix A.
        for(int j = 0; j < n; j++)
        {
            sum = 0.0;
            for(int i = 0; i < m; i++)
                sum = sum + rocblas_abs(A[i + j * lda]);

            if(value < sum || rocblas_isnan(sum))
                value = sum;
        }
    }
    else if(norm_type == 'I' || norm_type == 'i')
    {
        //Find the infinity norm of Matrix A.
        for(int j = 0; j < n; j++)
            for(int i = 0; i < m; i++)
            {
                work[i] = work[i] + rocblas_abs(A[i + j * lda]);
            }
        for(int i = 0; i < m; i++)
            if(value < work[i] || rocblas_isnan(work[i]))
                value = work[i];
    }
    else if(norm_type == 'F' || norm_type == 'f')
    {
        //Find the Frobenius norm of Matrix A.
        //SSQ(1) is scale
        //SSQ(2) is sum-of-squares
        //For better accuracy, sum each column separately.
        host_vector<double> ssq(2);
        host_vector<double> colssq(2);
        ssq[0] = 0.0;
        ssq[1] = 1.0;
        for(int j = 0; j < n; j++)
        {
            colssq[0] = 0.0;
            colssq[1] = 1.0;
            lapack_xlassq(m, A + j * lda, 1, colssq[0], colssq[1]);
            lapack_xcombssq(ssq.data(), colssq.data());
        }
        value = ssq[0] * std::sqrt(ssq[1]);
    }
    return value;
}

/*! \brief lapack_xlansy-returns the value of the one norm,  or the Frobenius norm, or the  infinity norm of the matrix A.*/
template <bool HERM, typename T>
double lapack_xlansy(char norm_type, char uplo, rocblas_int n, T* A, rocblas_int lda, double* work)
{
    double value = 0.0;
    double sum   = 0.0;
    double abs_A = 0.0;
    if(n == 0)
        return value;

    else if(norm_type == 'O' || norm_type == 'o' || norm_type == 'I' || norm_type == 'i'
            || norm_type == '1')
    {
        //Since A is symmetric the infinity norm and the one norm is the same.
        if(uplo == 'U')
        {
            for(int j = 0; j < n; j++)
            {
                sum = 0.0;
                for(int i = 0; i < j; i++)
                {
                    abs_A   = rocblas_abs(A[i + j * lda]);
                    sum     = sum + abs_A;
                    work[i] = work[i] + abs_A;
                }
                work[j] = sum + rocblas_abs(HERM ? std::real(A[j + j * lda]) : A[j + j * lda]);
            }
            for(int i = 0; i < n; i++)
            {
                sum = work[i];
                if(value < sum || rocblas_isnan(sum))
                    value = sum;
            }
        }
        else
        {
            for(int i = 0; i < n; i++)
                work[i] = 0.0;
            for(int j = 0; j < n; j++)
            {
                sum = work[j] + rocblas_abs(HERM ? std::real(A[j + j * lda]) : A[j + j * lda]);
                for(int i = j + 1; j < n; j++)
                {
                    abs_A   = rocblas_abs(A[i + j * lda]);
                    sum     = sum + abs_A;
                    work[i] = work[i] + abs_A;
                }
                if(value < sum || rocblas_isnan(sum))
                    value = sum;
            }
        }
    }
    else if(norm_type == 'F' || norm_type == 'f')
    {
        //Find the Frobenius norm of Matrix A.
        //ssq[1] is scale
        //ssq[2] is sum-of-squares
        //For better accuracy, sum each column separately.
        host_vector<double> ssq(2);
        host_vector<double> colssq(2);
        ssq[0] = 0.0;
        ssq[1] = 1.0;

        if(uplo == 'U')
        {
            for(int j = 1; j < n; j++)
            {
                colssq[0] = 0.0;
                colssq[1] = 1.0;
                lapack_xlassq(j - 1, A + j * lda, 1, colssq[0], colssq[1]);
                lapack_xcombssq(ssq.data(), colssq.data());
            }
        }
        else
        {
            for(int j = 0; j < n - 1; j++)
            {
                colssq[0] = 0.0;
                colssq[1] = 1.0;
                lapack_xlassq(n - j, A + ((j + 1) + j * lda), 1, colssq[0], colssq[1]);
                lapack_xcombssq(ssq.data(), colssq.data());
            }
        }
        ssq[1] = 2 * ssq[1];

        //sum of diagonal
        if(HERM)
        {
            for(int i = 0; i < n; i++)
            {
                if(std::real(A[i + i * lda]) != 0)
                {
                    abs_A = rocblas_abs(std::real(A[i + i * lda]));
                    if(ssq[0] < abs_A)
                    {
                        ssq[1] = 1.0 + ssq[1] * std::sqrt(ssq[0] / abs_A);
                        ssq[0] = abs_A;
                    }
                    else
                    {
                        ssq[1] = ssq[1] + std::sqrt(abs_A / ssq[0]);
                    }
                }
            }
        }
        else
        {
            colssq[0] = 0.0;
            colssq[1] = 1.0;
            lapack_xlassq(n, A, lda + 1, colssq[0], colssq[1]);
            lapack_xcombssq(ssq.data(), colssq.data());
        }
        value = ssq[0] * std::sqrt(ssq[1]);
    }
    return value;
}

template <typename T, typename U, typename V>
void lapack_xrot(const int n, T* cx, const int incx, T* cy, const int incy, const U c, const V s)
{
    T stemp = T(0.0);
    if(n <= 0)
        return;
    if(incx == 1 && incy == 1)
    {
        for(int i = 0; i < n; i++)
        {
            stemp = c * cx[i] + s * cy[i];
            cy[i] = c * cy[i] - (rocblas_is_complex<V> ? conjugate(s) : s) * cx[i];
            cx[i] = stemp;
        }
    }
    else
    {
        if(incx < 0)
            cx -= (n - 1) * incx;
        if(incy < 0)
            cy -= (n - 1) * incy;
        for(int i = 0; i < n; i++)
        {
            stemp = c * cx[i * incx] + s * cy[i * incy];
            cy[i * incy]
                = c * cy[i * incy] - (rocblas_is_complex<V> ? conjugate(s) : s) * cx[i * incx];
            cx[i * incx] = stemp;
        }
    }
}

template <typename T, typename U>
void lapack_xrotg(T& ca, T& cb, U& c, T& s)
{
    T      alpha = T(0.0);
    double norm  = 0.0;
    if(rocblas_abs(ca) == 0)
    {
        c  = 0.0;
        s  = T(1.0);
        ca = cb;
    }
    else
    {
        const rocblas_complex_num<real_t<T>> scale = {rocblas_abs(ca) + rocblas_abs(cb), 0};
        const double norm = std::real(scale) * std::sqrt(rocblas_abs(ca / scale))
                            + std::sqrt(rocblas_abs(cb / scale));
        alpha = ca / rocblas_abs(ca);
        c     = rocblas_abs(ca) / norm;
        s     = alpha * conjugate(cb) / norm;
        ca    = alpha * norm;
    }
}

// cblas_xsyr doesn't have complex support so implementation below for float/double complex
template <typename T>
void lapack_xsyr(
    rocblas_fill uplo, rocblas_int n, T alpha, T* xa, rocblas_int incx, T* A, rocblas_int lda)
{
    if(n <= 0 || alpha == 0)
        return;

    T* x = (incx < 0) ? xa - ptrdiff_t(incx) * (n - 1) : xa;

    if(uplo == rocblas_fill_upper)
    {
        for(int j = 0; j < n; ++j)
        {
            T tmp = alpha * x[j * incx];
            for(int i = 0; i <= j; ++i)
            {
                A[i + j * lda] = A[i + j * lda] + x[i * incx] * tmp;
            }
        }
    }
    else
    {
        for(int j = 0; j < n; ++j)
        {
            T tmp = alpha * x[j * incx];
            for(int i = j; i < n; ++i)
            {
                A[i + j * lda] = A[i + j * lda] + x[i * incx] * tmp;
            }
        }
    }
}

// cblas_xsymv doesn't have complex support so implementation below for float/double complex
template <typename T>
void lapack_xsymv(rocblas_fill uplo,
                  rocblas_int  n,
                  T            alpha,
                  T*           A,
                  rocblas_int  lda,
                  T*           xa,
                  rocblas_int  incx,
                  T            beta,
                  T*           ya,
                  rocblas_int  incy)
{
    if(n <= 0)
        return;

    T* x = (incx < 0) ? xa - ptrdiff_t(incx) * (n - 1) : xa;
    T* y = (incy < 0) ? ya - ptrdiff_t(incy) * (n - 1) : ya;

    if(beta != 1)
    {
        for(int j = 0; j < n; j++)
        {
            y[j * incy] = beta == T(0) ? T(0) : y[j * incy] * beta;
        }
    }

    if(alpha == 0)
        return;

    T temp1 = T(0);
    T temp2 = T(0);
    if(uplo == rocblas_fill_upper)
    {
        for(int j = 0; j < n; j++)
        {
            temp1 = alpha * x[j * incx];
            temp2 = T(0);
            for(int i = 0; i <= j - 1; i++)
            {
                y[i * incy] = y[i * incy] + temp1 * A[i + j * lda];
                temp2       = temp2 + A[i + j * lda] * x[i * incx];
            }
            y[j * incy] = y[j * incy] + temp1 * A[j + j * lda] + alpha * temp2;
        }
    }
    else
    {
        for(int j = 0; j < n; j++)
        {
            temp1       = alpha * x[j * incx];
            temp2       = T(0);
            y[j * incy] = y[j * incy] + temp1 * A[j + j * lda];
            for(int i = j + 1; i < n; i++)
            {
                y[i * incy] = y[i * incy] + temp1 * A[i + j * lda];
                temp2       = temp2 + A[i + j * lda] * x[i * incx];
            }
            y[j * incy] = y[j * incy] + alpha * temp2;
        }
    }
}

// cblas_xspr doesn't have complex support so implementation below for float/double complex
template <typename T>
void lapack_xspr(rocblas_fill uplo, rocblas_int n, T alpha, T* xa, rocblas_int incx, T* A)
{
    if(n <= 0 || alpha == 0)
        return;

    T*          x  = (incx < 0) ? xa - ptrdiff_t(incx) * (n - 1) : xa;
    rocblas_int kk = 0, k = 0;
    T           tmpx = T(0);
    if(uplo == rocblas_fill_upper)
    {
        for(int j = 0; j < n; ++j)
        {
            if(x[j * incx] != 0)
            {
                tmpx = alpha * x[j * incx];
                k    = kk;
                for(int i = 0; i < j; ++i)
                {
                    A[k] = A[k] + x[i * incx] * tmpx;
                    k    = k + 1;
                }
                A[kk + j] += x[j * incx] * tmpx;
            }
            kk += j + 1;
        }
    }
    else
    {
        for(int j = 0; j < n; ++j)
        {
            if(x[j * incx] != 0)
            {
                tmpx = alpha * x[j * incx];
                A[kk] += x[j * incx] * tmpx;
                k = kk + 1;
                for(int i = j + 1; i < n; ++i)
                {
                    A[k] = A[k] + x[i * incx] * tmpx;
                    k    = k + 1;
                }
            }
            kk += (n - 1) - j + 1;
        }
    }
}

// cblas_xsyr2 doesn't have complex support so implementation below for float/double complex
template <typename T>
inline void lapack_xsyr2(rocblas_fill uplo,
                         rocblas_int  n,
                         T            alpha,
                         T*           xa,
                         rocblas_int  incx,
                         T*           ya,
                         rocblas_int  incy,
                         T*           A,
                         rocblas_int  lda)
{
    if(n <= 0 || alpha == 0)
        return;

    T* x = (incx < 0) ? xa - ptrdiff_t(incx) * (n - 1) : xa;
    T* y = (incy < 0) ? ya - ptrdiff_t(incy) * (n - 1) : ya;

    if(uplo == rocblas_fill_upper)
    {
        for(int j = 0; j < n; ++j)
        {
            T tmpx = alpha * x[j * incx];
            T tmpy = alpha * y[j * incy];
            for(int i = 0; i <= j; ++i)
            {
                A[i + j * lda] = A[i + j * lda] + x[i * incx] * tmpy + y[i * incy] * tmpx;
            }
        }
    }
    else
    {
        for(int j = 0; j < n; ++j)
        {
            T tmpx = alpha * x[j * incx];
            T tmpy = alpha * y[j * incy];
            for(int i = j; i < n; ++i)
            {
                A[i + j * lda] = A[i + j * lda] + x[i * incx] * tmpy + y[i * incy] * tmpx;
            }
        }
    }
}

// cblas doesn't have potrf2 implementation for now so using the lapack potrf2 implementation below
template <typename T>
void lapack_xpotrf2(rocblas_fill uplo, rocblas_int n, T* A, rocblas_int lda, rocblas_int info)
{
    rocblas_int iinfo = 0;

    if(n < 0)
        info = -2;

    if(n == 0)
        return;

    if(n == 1)
    {
        //Test for non-positive-definiteness
        double A_real = std::real(A[0]);
        if(A_real <= 0 || rocblas_isnan(A_real))
        {
            info = 1;
            return;
        }
        //Factor
        A[0] = std::sqrt(A_real);
    }
    else
    {
        rocblas_int N1 = n / 2;
        rocblas_int N2 = n - N1;

        //Factor A11
        lapack_xpotrf2(uplo, N1, A, lda, iinfo);
        if(iinfo != 0)
        {
            info = iinfo;
            return;
        }
        //Compute the Cholesky factorization A = U**H*U
        if(uplo == rocblas_fill_upper)
        {
            //Update and scale A12
            cblas_trsm(rocblas_side_left,
                       rocblas_fill_upper,
                       rocblas_is_complex<T> ? rocblas_operation_conjugate_transpose
                                             : rocblas_operation_transpose,
                       rocblas_diagonal_non_unit,
                       N1,
                       N2,
                       T(1.0),
                       A,
                       lda,
                       A + ((N1 * lda)),
                       lda);

            //Update and factor A22
            if(rocblas_is_complex<T>)
            {
                cblas_herk(uplo,
                           rocblas_operation_conjugate_transpose,
                           N2,
                           N1,
                           real_t<T>(-1.0),
                           A + (N1 * lda),
                           lda,
                           real_t<T>(1.0),
                           A + (N1 + N1 * lda),
                           lda);
            }
            else
            {
                cblas_syrk(uplo,
                           rocblas_operation_transpose,
                           N2,
                           N1,
                           T(-1.0),
                           A + (N1 * lda),
                           lda,
                           T(1.0),
                           A + (N1 + N1 * lda),
                           lda);
            }
            lapack_xpotrf2(uplo, N2, A + (N1 + N1 * lda), lda, iinfo);
            if(iinfo != 0)
            {
                info = iinfo + N1;
                return;
            }
        }
        else
        {
            //Update and scale A21
            cblas_trsm(rocblas_side_right,
                       rocblas_fill_lower,
                       rocblas_is_complex<T> ? rocblas_operation_conjugate_transpose
                                             : rocblas_operation_transpose,
                       rocblas_diagonal_non_unit,
                       N2,
                       N1,
                       T(1.0),
                       A,
                       lda,
                       A + N1,
                       lda);

            //Update and factor A22
            if(rocblas_is_complex<T>)
            {
                cblas_herk(uplo,
                           rocblas_operation_none,
                           N2,
                           N1,
                           real_t<T>(-1.0),
                           A + N1,
                           lda,
                           real_t<T>(1.0),
                           A + (N1 + N1 * lda),
                           lda);
            }
            else
            {
                cblas_syrk(uplo,
                           rocblas_operation_none,
                           N2,
                           N1,
                           T(-1.0),
                           A + N1,
                           lda,
                           T(1.0),
                           A + (N1 + N1 * lda),
                           lda);
            }
            lapack_xpotrf2(uplo, N2, A + (N1 + N1 * lda), lda, iinfo);

            if(iinfo != 0)
            {
                info = iinfo + N1;
                return;
            }
        }
    }
}

// cblas doesn't have potrf implementation for now so using the lapack potrf implementation below
template <typename T>
void lapack_xpotrf(rocblas_fill uplo, rocblas_int n, T* A, rocblas_int lda, rocblas_int info)
{
    info = 0;

    if(n < 0)
        info = -2;

    if(n == 0)
        return;

    rocblas_int NB = 64;
    if(NB <= 1 || NB >= n)
    {
        lapack_xpotrf2(uplo, n, A, lda, info);
    }

    if(uplo == rocblas_fill_upper)
    {
        //Compute the Cholesky factorization A = U**H *U.
        for(int j = 0; j < n; j += NB)
        {
            //Update and factorize the current diagonal block and test for non-positive-definiteness.
            rocblas_int jb = std::min(NB, n - j);
            if(rocblas_is_complex<T>)
            {
                cblas_herk(uplo,
                           rocblas_operation_conjugate_transpose,
                           jb,
                           j,
                           real_t<T>(-1.0),
                           A + (j * lda),
                           lda,
                           real_t<T>(1.0),
                           A + (j + j * lda),
                           lda);
            }
            else
            {
                cblas_syrk(uplo,
                           rocblas_operation_transpose,
                           jb,
                           j,
                           T(-1.0),
                           A + (j * lda),
                           lda,
                           T(1.0),
                           A + (j + j * lda),
                           lda);
            }

            lapack_xpotrf2(uplo, jb, A + (j + j * lda), lda, info);
            if(info == 0)
            {
                info = info + j - 1;
                return;
            }
            if(j + jb <= n)
            {
                cblas_gemm(rocblas_is_complex<T> ? rocblas_operation_conjugate_transpose
                                                 : rocblas_operation_transpose,
                           rocblas_operation_none,
                           jb,
                           n - j - jb,
                           j,
                           T(-1.0),
                           A + (j * lda),
                           lda,
                           A + ((j + jb) * lda),
                           lda,
                           T(1.0),
                           A + (j + (j + jb) * lda),
                           lda);

                cblas_trsm(rocblas_side_left,
                           uplo,
                           rocblas_is_complex<T> ? rocblas_operation_conjugate_transpose
                                                 : rocblas_operation_transpose,
                           rocblas_diagonal_non_unit,
                           jb,
                           n - j - jb,
                           T(1.0),
                           A + (j + j * lda),
                           lda,
                           A + (j + (j + jb) * lda),
                           lda);
            }
        }
    }
    else
    {
        //Compute the Cholesky factorization A = U**H *U.
        for(int j = 0; j < n; j += NB)
        {
            //Update and factorize the current diagonal block and test for non-positive-definiteness.
            rocblas_int jb = std::min(NB, n - j);
            if(rocblas_is_complex<T>)
            {
                cblas_herk(uplo,
                           rocblas_operation_none,
                           jb,
                           j,
                           real_t<T>(-1.0),
                           A + j,
                           lda,
                           real_t<T>(1.0),
                           A + (j + j * lda),
                           lda);
            }
            else
            {
                cblas_syrk(uplo,
                           rocblas_operation_none,
                           jb,
                           j,
                           T(-1.0),
                           A + j,
                           lda,
                           T(1.0),
                           A + (j + j * lda),
                           lda);
            }

            lapack_xpotrf2(uplo, jb, A + (j + j * lda), lda, info);
            if(info == 0)
            {
                info = info + j - 1;
                return;
            }
            if(j + jb <= n)
            {
                cblas_gemm(rocblas_operation_none,
                           rocblas_is_complex<T> ? rocblas_operation_conjugate_transpose
                                                 : rocblas_operation_transpose,
                           n - j - jb,
                           jb,
                           j,
                           T(-1.0),
                           A + (j + jb),
                           lda,
                           A + j,
                           lda,
                           T(1.0),
                           A + ((j + jb) + j * lda),
                           lda);

                cblas_trsm(rocblas_side_right,
                           uplo,
                           rocblas_is_complex<T> ? rocblas_operation_conjugate_transpose
                                                 : rocblas_operation_transpose,
                           rocblas_diagonal_non_unit,
                           n - j - jb,
                           jb,
                           T(1.0),
                           A + (j + j * lda),
                           lda,
                           A + ((j + jb) + j * lda),
                           lda);
            }
        }
    }
}

template <typename T, typename U>
void cblas_scal(rocblas_int n, T alpha, U x, rocblas_int incx);

// cblas doesn't have trti2 implementation for now so using the lapack trti2 implementation below
template <typename T>
void lapack_xtrti2(char uplo, char diag, rocblas_int n, T* A, rocblas_int lda)
{
    if(n < 0)
        return;

    T AJJ = T(0);

    if(uplo == 'U')
    {
        for(int j = 0; j < n; j++)
        {
            if(diag == 'N')
            {
                A[j + j * lda] = T(1.0) / A[j + j * lda];
                AJJ            = -A[j + j * lda];
            }
            else
            {
                AJJ = T(-1.0);
            }
            //Compute elements 0:j-1 of j-th column.
            cblas_trmv(rocblas_fill_upper,
                       rocblas_operation_none,
                       char2rocblas_diagonal(diag),
                       j,
                       A,
                       lda,
                       A + (j * lda),
                       1);
            cblas_scal(j, AJJ, A + (j * lda), 1);
        }
    }
    else
    {
        for(int j = n - 1; j >= 0; j--)
        {
            if(diag == 'N')
            {
                A[j + j * lda] = T(1.0) / A[j + j * lda];
                AJJ            = -A[j + j * lda];
            }
            else
            {
                AJJ = T(-1.0);
            }
            if(j < n - 1)
            {
                //Compute elements 0:j-1 of j-th column.
                cblas_trmv(rocblas_fill_lower,
                           rocblas_operation_none,
                           char2rocblas_diagonal(diag),
                           n - j - 1,
                           A + ((j + 1) + (j + 1) * lda),
                           lda,
                           A + ((j + 1) + j * lda),
                           1);
                cblas_scal(n - j - 1, AJJ, A + ((j + 1) + j * lda), 1);
            }
        }
    }
}

// cblas doesn't have trtri implementation for now so using the lapack trtri implementation below
template <typename T>
void lapack_xtrtri(char uplo, char diag, rocblas_int n, T* A, rocblas_int lda)
{
    if(n <= 0)
        return;

    rocblas_int NB = 64;
    rocblas_int JB = 0;
    if(NB <= 1 || NB >= n)
    {
        lapack_xtrti2(uplo, diag, n, A, lda);
    }
    else
    {
        if(uplo == 'U')
        {
            // Compute inverse of upper triangular matrix
            for(int j = 0; j < n; j += NB)
            {
                JB = std::min(NB, n - j);

                // Compute rows 0:j-1 of current block column
                cblas_trmm(rocblas_side_left,
                           rocblas_fill_upper,
                           rocblas_operation_none,
                           char2rocblas_diagonal(diag),
                           j,
                           JB,
                           T(1.0),
                           A,
                           lda,
                           A + (j * lda),
                           lda);
                cblas_trsm(rocblas_side_right,
                           rocblas_fill_upper,
                           rocblas_operation_none,
                           char2rocblas_diagonal(diag),
                           j,
                           JB,
                           T(-1.0),
                           A + (j + j * lda),
                           lda,
                           A + j * lda,
                           lda);
                lapack_xtrti2(uplo, diag, JB, A + (j + j * lda), lda);
            }
        }
        else
        {
            rocblas_int NN = ((n - 1) / NB) * NB;
            for(int j = NN; j >= 0; j -= NB)
            {
                JB = std::min(NB, n - j);
                if(j + JB <= n)
                {
                    cblas_trmm(rocblas_side_left,
                               rocblas_fill_lower,
                               rocblas_operation_none,
                               char2rocblas_diagonal(diag),
                               n - j - JB,
                               JB,
                               T(1.0),
                               A + ((j + JB) + (j + JB) * lda),
                               lda,
                               A + ((j + JB) + j * lda),
                               lda);
                    cblas_trsm(rocblas_side_right,
                               rocblas_fill_lower,
                               rocblas_operation_none,
                               char2rocblas_diagonal(diag),
                               n - j - JB,
                               JB,
                               T(-1.0),
                               A + (j + j * lda),
                               lda,
                               A + ((j + JB) + j * lda),
                               lda);
                }
                lapack_xtrti2(uplo, diag, JB, A + (j + j * lda), lda);
            }
        }
    }
}
