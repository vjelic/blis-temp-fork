/*

   BLIS
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2023 - 2024, Advanced Micro Devices, Inc. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    - Neither the name(s) of the copyright holder(s) nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include "blis.h"
#include "common/testing_helpers.h"
#include "inc/check_error.h"

/**
 * @brief Computes
 *             y := y + x or y := y + conj(x)
 *        This is a BLIS-specific API, not part of BLAS/CBLAS.
 * @param[in] conjx denotes if x or conj(x) will be used for this operation
 * @param[in] n vector length of x and y
 * @param[in] x pointer which points to the first element of x
 * @param[in] incx increment of x
 * @param[in, out] y pointer which points to the first element of y
 * @param[in] incy increment of y
 */

template<typename T>
static void typed_addv(char conj_x, gtint_t n, T* x, gtint_t incx, T* y, gtint_t incy)
{
    conj_t conjx;
    // Map parameter characters to BLIS constants.
    testinghelpers::char_to_blis_conj( conj_x, &conjx );
    if constexpr (std::is_same<T, float>::value)
        bli_saddv( conjx, n, x, incx, y, incy );
    else if constexpr (std::is_same<T, double>::value)
        bli_daddv( conjx, n, x, incx, y, incy );
    else if constexpr (std::is_same<T, scomplex>::value)
        bli_caddv( conjx, n, x, incx, y, incy );
    else if constexpr (std::is_same<T, dcomplex>::value)
        bli_zaddv( conjx, n, x, incx, y, incy );
    else
        throw std::runtime_error("Error in testsuite/level1/addv.h: Invalid typename in typed_addv().");
}
template<typename T>
static void addv(char conjx, gtint_t n, T* x, gtint_t incx, T* y, gtint_t incy)
{

#ifdef TEST_UPPERCASE_ARGS
    conjx = static_cast<char>(std::toupper(static_cast<unsigned char>(conjx)));
#endif

#ifdef TEST_INPUT_ARGS
    // Create copy of scalar input values so we can check that they are not altered.
    char conjx_cpy = conjx;
    gtint_t n_cpy = n;
    gtint_t incx_cpy = incx;
    gtint_t incy_cpy = incy;

    // Create copy of input arrays so we can check that they are not altered.
    T* x_cpy = nullptr;
    gtint_t size_x = testinghelpers::buff_dim( n, incx );
    if (x && size_x > 0)
    {
        x_cpy = new T[size_x];
        memcpy( x_cpy, x, size_x * sizeof( T ) );
    }
#endif

#ifdef TEST_BLAS
    throw std::runtime_error("Error in testsuite/level1/addv.h: BLAS interface is not available.");
#elif TEST_BLAS_BLIS_IMPL
    throw std::runtime_error("Error in testsuite/level1/addv.h: BLAS_BLIS_IMPL interface is not available.");
#elif TEST_CBLAS
    throw std::runtime_error("Error in testsuite/level1/addv.h: CBLAS interface is not available.");
#elif TEST_BLIS_TYPED
    typed_addv(conjx, n, x, incx, y, incy);
#else
    throw std::runtime_error("Error in testsuite/level1/addv.h: No interfaces are set to be tested.");
#endif

#ifdef TEST_INPUT_ARGS
    //----------------------------------------------------------
    // Check scalar inputs have not been modified.
    //----------------------------------------------------------

    computediff<char>( "conjx", conjx, conjx_cpy );
    computediff<gtint_t>( "n", n, n_cpy );
    computediff<gtint_t>( "incx", incx, incx_cpy );
    computediff<gtint_t>( "incy", incy, incy_cpy );

    //----------------------------------------------------------
    // Bitwise-wise check array inputs have not been modified.
    //----------------------------------------------------------

    if (x && size_x > 0)
    {
        computediff<T>( "x", n, x, x_cpy, incx, true );
        delete[] x_cpy;
    }
#endif
}
