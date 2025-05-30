/*

   BLIS
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2021 - 2025, Advanced Micro Devices, Inc. All rights reserved.

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

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "blis.h"
#include "blis_int_type.h"


// Benchmark application to process aocl logs generated by BLIS library.
#ifndef DT
#define DT BLIS_DOUBLE
#endif


#define AOCL_MATRIX_INITIALISATION

//#define BLIS_ENABLE_CBLAS

/* For BLIS since logs are collected at BLAS interfaces
 * we disable cblas interfaces for this benchmark application
 */

/* #ifdef BLIS_ENABLE_CBLAS */
/* #define CBLAS */
/* #endif */

int main( int argc, char** argv )
{
    obj_t a, x, y;
    obj_t y_save;
    obj_t alpha, beta;
    dim_t  p_inc = 0; // to keep track of number of inputs
    num_t dt;
    //    ind_t    ind;
    char     dt_ch;
    int   r, n_repeats;
    trans_t  transa;

    double   dtime;
    double   dtime_save;
    double   gflops;

    FILE* fin  = NULL;
    FILE* fout = NULL;

    n_repeats = N_REPEAT;  // This macro will get from Makefile.

    dt = DT;

    if (argc < 3)
      {
        printf("Usage: ./test_gemv_XX.x input.csv output.csv\n");
        exit(1);
      }
    fin = fopen(argv[1], "r");
    if (fin == NULL)
      {
        printf("Error opening the file %s\n", argv[1]);
        exit(1);
      }
    fout = fopen(argv[2], "w");
    if (fout == NULL)
      {
        printf("Error opening output file %s\n", argv[2]);
        exit(1);
      }

    fprintf(fout, "Func Dt transa m n alphaR alphaI lda incx betaR betaI incy gflops\n");

    char transA;
    dim_t m;
    dim_t n;
    double alpha_r, beta_r, alpha_i, beta_i;
    inc_t lda;
    inc_t incx;
    inc_t incy;
    char tmp[256]; // to store function name, line no present in logs.


    // {S,D,C,Z} {transa m n alpha lda, incx, beta, incy}
    while (fscanf(fin, "%s %c %c " INT_FS INT_FS " %lf %lf " INT_FS INT_FS " %lf %lf " INT_FS "\n",
      tmp, &dt_ch, &transA, &m, &n,  &alpha_r, &alpha_i, &lda,\
                  &incx, &beta_r, &beta_i, &incy) == 12)
      {
        if (dt_ch == 'D' || dt_ch == 'd') dt = BLIS_DOUBLE;
        else if (dt_ch == 'Z' || dt_ch == 'z') dt = BLIS_DCOMPLEX;
        else if (dt_ch == 'S' || dt_ch == 's') dt = BLIS_FLOAT;
        else if (dt_ch == 'C' || dt_ch == 'c') dt = BLIS_SCOMPLEX;
        else
          {
            printf("Invalid data type %c\n", dt_ch);
            continue;
          }

        if      ( transA == 'n' || transA == 'N') transa = BLIS_NO_TRANSPOSE;
        else if ( transA == 't' || transA == 'T') transa = BLIS_TRANSPOSE;
        else if ( transA == 'c' || transA == 'C') transa = BLIS_CONJ_TRANSPOSE;
        else
          {
            printf("Invalid option for transA \n");
            continue;
          }


        bli_obj_create( dt, 1, 1, 0, 0, &alpha);
        bli_obj_create( dt, 1, 1, 0, 0, &beta );

        // Crate objects with required sizes and strides.


        // The lenght of the vector(x) must match the width of
        // the matrix, based on transpose we need to choose
        // correct dimension for the x and y vectors
        //        n
        //    [ a b c]  [x0]   [y0]
        // m  [ d e f]  [x1]   [y1]
        //              [x2]
        //         A     x      y
        //
        // In above example sizes needs to be changed as below
        //
        // Input    No Transpose  With Transpose
        //
        //   A         mxn        mxn *
        //   x         nx1        mx1
        //   y         mx1        nx1
        //
        // * Actual transpose operation on matrix A happens
        // inside the gemv API implementation however, we
        // need to ensure that x and y has correct size to
        // perform the operation on transposed matrix.

        bli_obj_create(dt, m, n, 1, lda, &a);
        if (transa == BLIS_NO_TRANSPOSE)
          {
			// For non transpose operation
			// Vector x has n entries and vector y as m entries
			//
			// However, the actual buffer size needs to consider
			// the strids as well
			//
			// buffer size for x = (1+(n-1)*abs(incx))
			// buffer size for y = (1+(m-1)*abs(incy))
			//
			// We achieve this by passing incx and incy as row-strides
			// to bli_obj_create function.
            bli_obj_create(dt, n, 1, incx, 1, &x);
            bli_obj_create(dt, m, 1, incy, 1, &y);
            bli_obj_create(dt, m, 1, incy, 1, &y_save);
          }
        else
          {
			// For transpose operation
			// Vector x has m entries and vector y as n entries
			//
			// However, the actual buffer size needs to consider
			// the strids as well
			//
			// buffer size for x = (1+(m-1)*abs(incx))
			// buffer size for y = (1+(n-1)*abs(incy))
			//
			// We achieve this by passing incx and incy as row-strides
			// to bli_obj_create function.
            bli_obj_create(dt, m, 1, incx, 1, &x);
            bli_obj_create(dt, n, 1, incy, 1, &y);
            bli_obj_create(dt, n, 1, incy, 1, &y_save);
          }
		
#ifdef AOCL_MATRIX_INITIALISATION
        bli_randm( &a );
        bli_randm( &x );
        bli_randm( &y );
#endif

        bli_obj_set_conjtrans( transa, &a);

        bli_setsc( alpha_r, alpha_i, &alpha );
        bli_setsc( beta_r, beta_i, &beta );

        bli_copym( &y, &y_save );

        dtime_save = DBL_MAX;

        for ( r = 0; r < n_repeats; ++r )
          {
            bli_copym( &y_save, &y );

#ifdef PRINT
            bli_printm( "a", &a, "%4.1f", "" );
            bli_printm( "x", &x, "%4.1f", "" );
            bli_printm( "y", &y, "%4.1f", "" );
#endif
            dtime = bli_clock();

#ifdef BLIS
            bli_gemv( &alpha,
                      &a,
                      &x,
                      &beta,
                      &y );

#else // BLIS Interface

#ifdef CBLAS // set transpose options
            enum CBLAS_ORDER cblas_order;
            enum CBLAS_TRANSPOSE cblas_transa;

            if ( bli_obj_row_stride( &a ) == 1 )
                    cblas_order = CblasColMajor;
            else
                    cblas_order = CblasRowMajor;

            cblas_transa = CblasNoTrans;
#else
            f77_char f77_transa;
            bli_param_map_blis_to_netlib_trans( transa, &f77_transa );
#endif

            // Set data type independent inputs for BLAS and
            // CBLAS API's

            f77_int  mm     = bli_obj_length( &a );
            f77_int  nn     = bli_obj_width( &a );
            f77_int  blas_lda    = bli_obj_col_stride( &a );
            f77_int  blas_incx   = incx;
            f77_int  blas_incy   = incy;

            if ( bli_is_float( dt ) ){
                float*  alphap = bli_obj_buffer( &alpha );
                float*  ap     = bli_obj_buffer( &a );
                float*  xp     = bli_obj_buffer( &x );
                float*  betap  = bli_obj_buffer( &beta );
                float*  yp     = bli_obj_buffer( &y );
#ifdef CBLAS
                cblas_sgemv( cblas_order,
                            cblas_transa,
                            mm,
                            nn,
                            *alphap,
                            ap, blas_lda,
                            xp, blas_incx,
                            *betap,
                            yp, blas_incy );
#else // cblas sgemv
                sgemv_( &f77_transa,
                        &mm,
                        &nn,
                        alphap,
                        ap, &blas_lda,
                        xp, &blas_incx,
                        betap,
                        yp, &blas_incy );
#endif // cblas sgemv
            }
            else if ( bli_is_double( dt ) )
            {

                double*  alphap = bli_obj_buffer( &alpha );
                double*  ap     = bli_obj_buffer( &a );
                double*  xp     = bli_obj_buffer( &x );
                double*  betap  = bli_obj_buffer( &beta );
                double*  yp     = bli_obj_buffer( &y );
#ifdef CBLAS
                cblas_dgemv( cblas_order,
                            cblas_transa,
                            mm,
                            nn,
                            *alphap,
                            ap, blas_lda,
                            xp, blas_incx,
                            *betap,
                            yp, blas_incy );
#else // cblas dgemv
                dgemv_( &f77_transa,
                        &mm,
                        &nn,
                        alphap,
                        ap, &blas_lda,
                        xp, &blas_incx,
                        betap,
                        yp, &blas_incy );
#endif // cblas dgemv
            }
            else if ( bli_is_scomplex( dt ) )
            {
                scomplex*  alphap = bli_obj_buffer( &alpha );
                scomplex*  ap     = bli_obj_buffer( &a );
                scomplex*  xp     = bli_obj_buffer( &x );
                scomplex*  betap  = bli_obj_buffer( &beta );
                scomplex*  yp     = bli_obj_buffer( &y );
#ifdef CBLAS
                cblas_cgemv( cblas_order,
                            cblas_transa,
                            mm,
                            nn,
                            alphap,
                            ap, blas_lda,
                            xp, blas_incx,
                            betap,
                            yp, blas_incy );
#else // cblas cgemv
                cgemv_( &f77_transa,
                        &mm,
                        &nn,
                        alphap,
                        ap, &blas_lda,
                        xp, &blas_incx,
                        betap,
                        yp, &blas_incy );
#endif // cblas cgemv
            }
            else if ( bli_is_dcomplex( dt ) )
            {
                dcomplex*  alphap = bli_obj_buffer( &alpha );
                dcomplex*  ap     = bli_obj_buffer( &a );
                dcomplex*  xp     = bli_obj_buffer( &x );
                dcomplex*  betap  = bli_obj_buffer( &beta );
                dcomplex*  yp     = bli_obj_buffer( &y );
#ifdef CBLAS
                cblas_zgemv( cblas_order,
                            cblas_transa,
                            mm,
                            nn,
                            alphap,
                            ap, blas_lda,
                            xp, blas_incx,
                            betap,
                            yp, blas_incy );
#else // cblas zgemv
                zgemv_( &f77_transa,
                        &mm,
                        &nn,
                        alphap,
                        ap, &blas_lda,
                        xp, &blas_incx,
                        betap,
                        yp, &blas_incy );

#endif  // cblas zgemv
            }

#endif // BLIS Interface

#ifdef PRINT
            bli_printm( "y after", &y, "%4.1f", "" );
            exit(1);
#endif

            dtime_save = bli_clock_min_diff( dtime_save, dtime );
          }

        gflops = ( 2.0 * m  * n ) / ( dtime_save * 1.0e9 );

        if ( bli_is_complex( dt ) ) gflops *= 4.0;

        printf( "data_gemv_%s", BLAS );

        p_inc++;
        printf("( %2lu, 1:4 ) = [ %4lu %4lu %7.2f ];\n",
               (unsigned long)(p_inc),
               (unsigned long)m,
               (unsigned long)n,
                gflops);

        fprintf (fout, "%s %c %c" INT_FS INT_FS " %lf %lf"  INT_FS INT_FS  " %lf %lf " INT_FS " %6.3f\n",
                   tmp, dt_ch, transA, m, n,  alpha_r, alpha_i, lda,\
                    incx, beta_r, beta_i, incy, gflops);

        fflush(fout);

        bli_obj_free( &alpha );
        bli_obj_free( &beta );

        bli_obj_free( &a );
        bli_obj_free( &x );
        bli_obj_free( &y );
        bli_obj_free( &y_save );
      }

    //bli_finalize();
    fclose(fin);
    fclose(fout);

    return 0;
}
