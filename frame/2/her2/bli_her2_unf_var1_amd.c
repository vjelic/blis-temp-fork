/*

   BLIS
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2022 - 2023, Advanced Micro Devices, Inc. All rights reserved.

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

#include "blis.h"

#undef  GENTFUNC
#define GENTFUNC( ctype, ch, varname ) \
\
void PASTEMAC(ch,varname) \
     ( \
       uplo_t  uplo, \
       conj_t  conjx, \
       conj_t  conjy, \
       conj_t  conjh, \
       dim_t   m, \
       ctype*  alpha, \
       ctype*  x, inc_t incx, \
       ctype*  y, inc_t incy, \
       ctype*  c, inc_t rs_c, inc_t cs_c, \
       cntx_t* cntx  \
     ) \
{ \
	const num_t dt = PASTEMAC(ch,type); \
\
	ctype*  x0; \
	ctype*  chi1; \
	ctype*  y0; \
	ctype*  psi1; \
	ctype*  c10t; \
	ctype*  gamma11; \
	ctype   alpha0; \
	ctype   alpha1; \
	ctype   alpha0_chi1; \
	ctype   alpha1_psi1; \
	ctype   alpha0_chi1_psi1; \
	ctype   conjx0_chi1; \
	ctype   conjy1_psi1; \
	ctype   conjy0_psi1; \
	dim_t   i; \
	dim_t   n_behind; \
	inc_t   rs_ct, cs_ct; \
	conj_t  conj0, conj1; \
\
	/* The algorithm will be expressed in terms of the lower triangular case;
	   the upper triangular case is supported by swapping the row and column
	   strides of A and toggling some conj parameters. */ \
	if      ( bli_is_lower( uplo ) ) \
	{ \
		rs_ct = rs_c; \
		cs_ct = cs_c; \
\
		PASTEMAC(ch,copys)( *alpha, alpha0 ); \
		PASTEMAC(ch,copycjs)( conjh, *alpha, alpha1 ); \
	} \
	else /* if ( bli_is_upper( uplo ) ) */ \
	{ \
		rs_ct = cs_c; \
		cs_ct = rs_c; \
\
		/* Toggle conjugation of conjx/conjy, but only if we are being invoked
		   as her2; for syr2, conjx/conjy are unchanged. */ \
		conjx = bli_apply_conj( conjh, conjx ); \
		conjy = bli_apply_conj( conjh, conjy ); \
\
		PASTEMAC(ch,copycjs)( conjh, *alpha, alpha0 ); \
		PASTEMAC(ch,copys)( *alpha, alpha1 ); \
	} \
\
	/* Apply conjh (which carries the conjugation component of the Hermitian
	   transpose, if applicable) to conjx and/or conjy as needed to arrive at
	   the effective conjugation for the vector subproblems. */ \
	conj0 = bli_apply_conj( conjh, conjy ); \
	conj1 = bli_apply_conj( conjh, conjx ); \
\
	PASTECH(ch,axpy2v_ker_ft) kfp_2v; \
\
	/* Query the context for the kernel function pointer. */ \
	kfp_2v = bli_cntx_get_l1f_ker_dt( dt, BLIS_AXPY2V_KER, cntx ); \
\
	for ( i = 0; i < m; ++i ) \
	{ \
		n_behind = i; \
		x0       = x + (0  )*incx; \
		chi1     = x + (i  )*incx; \
		y0       = y + (0  )*incy; \
		psi1     = y + (i  )*incy; \
		c10t     = c + (i  )*rs_ct + (0  )*cs_ct; \
		gamma11  = c + (i  )*rs_ct + (i  )*cs_ct; \
\
		/* Apply conjx and/or conjy to chi1 and/or psi1. */ \
		PASTEMAC(ch,copycjs)( conjx,        *chi1, conjx0_chi1 ); \
		PASTEMAC(ch,copycjs)( conjy,        *psi1, conjy1_psi1 ); \
		PASTEMAC(ch,copycjs)( conj0,        *psi1, conjy0_psi1 ); \
\
		/* Compute scalars for vector subproblems. */ \
		PASTEMAC(ch,scal2s)( alpha0, conjx0_chi1, alpha0_chi1 ); \
		PASTEMAC(ch,scal2s)( alpha1, conjy1_psi1, alpha1_psi1 ); \
\
		/* Compute alpha * chi1 * conj(psi1) after both chi1 and psi1 have
		   already been conjugated, if needed, by conjx and conjy. */ \
		PASTEMAC(ch,scal2s)( alpha0_chi1, conjy0_psi1, alpha0_chi1_psi1 ); \
\
		/* c10t = c10t +      alpha  * chi1 * y0'; */ \
		/* c10t = c10t + conj(alpha) * psi1 * x0'; */ \
		kfp_2v \
		( \
		  conj0, \
		  conj1, \
		  n_behind, \
		  &alpha0_chi1, \
		  &alpha1_psi1, \
		  y0,   incy, \
		  x0,   incx, \
		  c10t, cs_ct, \
		  cntx  \
		); \
\
		/* gamma11 = gamma11 +      alpha  * chi1 * conj(psi1) \
		                     + conj(alpha) * psi1 * conj(chi1); */ \
		PASTEMAC(ch,adds)( alpha0_chi1_psi1, *gamma11 ); \
		PASTEMAC(ch,adds)( alpha0_chi1_psi1, *gamma11 ); \
\
		/* For her2, explicitly set the imaginary component of gamma11 to
           zero. */ \
		if ( bli_is_conj( conjh ) ) \
			PASTEMAC(ch,seti0s)( *gamma11 ); \
	} \
}

/**
 * Following is function declaration
 * that computes her2 for transposed case.
 * It handles triangular part of matrix and
 * remaining computation in optimal way to
 * gain performance improvement.
 * a is triangular matrix, x and y are vectors
 */
void bli_dher2_trans_zen_int_4
     (
       double *a,
       double *x,
       double *y,
       double *alpha,
       dim_t m,
       dim_t lda
     );

void bli_dher2_unf_var1
     (
       uplo_t  uplo,
       conj_t  conjx,
       conj_t  conjy,
       conj_t  conjh,
       dim_t   m,
       double*  alpha,
       double*  x, inc_t incx,
       double*  y, inc_t incy,
       double*  c, inc_t rs_c, inc_t cs_c,
       cntx_t* cntx
     )
{
        const num_t dt = PASTEMAC(d,type);

        double*  x0;
        double*  chi1;
        double*  y0;
        double*  psi1;
        double*  c10t;
        double*  gamma11;
        double   alpha0;
        double   alpha1;
        double   alpha0_chi1;
        double   alpha1_psi1;
        double   alpha0_chi1_psi1;
        double   conjx0_chi1;
        double   conjy1_psi1;
        double   conjy0_psi1;
        dim_t   i;
        dim_t   n_behind;
        inc_t   rs_ct, cs_ct;
        conj_t  conj0, conj1;

        /* The algorithm will be expressed in terms of the lower triangular
	 * case;the upper triangular case is supported by swapping the row
	 * and column strides of A and toggling some conj parameters.
	 */
        if ( bli_is_lower( uplo ) )
        {
                rs_ct = rs_c;
                cs_ct = cs_c;

                PASTEMAC(d,copys)( *alpha, alpha0 );
		PASTEMAC(d,copycjs)( conjh, *alpha, alpha1 );
	}
	else /* if ( bli_is_upper( uplo ) ) */
	{
		rs_ct = cs_c;
		cs_ct = rs_c;

		/* Toggle conjugation of conjx/conjy, but only if we are being
		 * invoked as her2; for syr2, conjx/conjy are unchanged.
		 */
		conjx = bli_apply_conj( conjh, conjx );
		conjy = bli_apply_conj( conjh, conjy );

		PASTEMAC(d,copycjs)( conjh, *alpha, alpha0 );
		PASTEMAC(d,copys)( *alpha, alpha1 );
	}

	/* Apply conjh (which carries the conjugation component of the
	 * Hermitian transpose, if applicable) to conjx and/or conjy as
	 * needed to arrive at the effective conjugation for the vector
	 * subproblems.
	 */
        conj0 = bli_apply_conj( conjh, conjy );
        conj1 = bli_apply_conj( conjh, conjx );

        PASTECH(d,axpy2v_ker_ft) kfp_2v;

        /* Query the context for the kernel function pointer. */
	if ( cntx == NULL ) cntx = bli_gks_query_cntx();
	kfp_2v = bli_cntx_get_l1f_ker_dt( dt, BLIS_AXPY2V_KER, cntx );

	if ( (bli_cpuid_is_avx2fma3_supported() == TRUE)
	   && (incx == 1)
	   && (incy == 1)
	   && (rs_ct == 1))
	{
		for ( i = 0; i < m; )
		{
			n_behind = i;
			x0       = x + (0  )*incx;
			chi1     = x + (i  )*incx;
			y0       = y + (0  )*incy;
			psi1     = y + (i  )*incy;
			c10t     = c + (i  )*rs_ct + (0  )*cs_ct;
			gamma11  = c + (i  )*rs_ct + (i  )*cs_ct;

			if((n_behind >= 3))
			{
				bli_dher2_trans_zen_int_4(c10t, x0, y0,
						         &alpha0,
						         n_behind + 1,
						         cs_ct);
				i+=4;
			}
			else
			{
				/* Apply conjx and/or conjy to chi1
				 * and/or psi1. */
				PASTEMAC(d,copycjs)( conjx, *chi1,
						   conjx0_chi1 );
				PASTEMAC(d,copycjs)( conjy, *psi1,
						   conjy1_psi1 );
				PASTEMAC(d,copycjs)( conj0, *psi1,
						   conjy0_psi1 );

				/* Compute scalars for vector
				 * subproblems. */
				PASTEMAC(d,scal2s)( alpha0,
						  conjx0_chi1,
						  alpha0_chi1 );
				PASTEMAC(d,scal2s)( alpha1,
						  conjy1_psi1,
						  alpha1_psi1 );

				/* Compute alpha * chi1 * conj(psi1)
				 * after both chi1 and psi1 have
				 * already been conjugated, if needed
				 * by conjx and conjy.
				 */
				PASTEMAC(d,scal2s)( alpha0_chi1,
						  conjy0_psi1,
						  alpha0_chi1_psi1 );

				/* c10t = c10t + alpha  * chi1 * y0';*/
				/* c10t = c10t + conj(alpha) * psi1 * x0';*/
				kfp_2v
					(
					 conj0,
					 conj1,
					 n_behind,
					 &alpha0_chi1,
					 &alpha1_psi1,
					 y0,   incy,
					 x0,   incx,
					 c10t, cs_ct,
					 cntx
					);

				/* gamma11 = gamma11 + alpha  * chi1 *conj(psi1)
				 * + conj(alpha) * psi1 * conj(chi1);*/
				PASTEMAC(d,adds)( alpha0_chi1_psi1,
						*gamma11 );
				PASTEMAC(d,adds)( alpha0_chi1_psi1,
						*gamma11 );

				i+=1;
			}
		}
	}
	else
	{
		for ( i = 0; i < m; ++i )
		{
			n_behind = i;
			x0       = x + (0  )*incx;
			chi1     = x + (i  )*incx;
			y0       = y + (0  )*incy;
			psi1     = y + (i  )*incy;
			c10t     = c + (i  )*rs_ct + (0  )*cs_ct;
			gamma11  = c + (i  )*rs_ct + (i  )*cs_ct;

			/* Apply conjx and/or conjy to chi1 and/or psi1. */
			PASTEMAC(d,copycjs)( conjx, *chi1, conjx0_chi1 );
			PASTEMAC(d,copycjs)( conjy, *psi1, conjy1_psi1 );
			PASTEMAC(d,copycjs)( conj0, *psi1, conjy0_psi1 );

			/* Compute scalars for vector subproblems. */
			PASTEMAC(d,scal2s)( alpha0, conjx0_chi1, alpha0_chi1 );
			PASTEMAC(d,scal2s)( alpha1, conjy1_psi1, alpha1_psi1 );

			/* Compute alpha * chi1 * conj(psi1) after both chi1
			 * and psi1 have already been conjugated, if needed,
			 * by conjx and conjy.
			 */
			PASTEMAC(d,scal2s)( alpha0_chi1, conjy0_psi1,
					alpha0_chi1_psi1 );

			/* c10t = c10t +      alpha  * chi1 * y0'; */
			/* c10t = c10t + conj(alpha) * psi1 * x0'; */
			kfp_2v
				(
				 conj0,
				 conj1,
				 n_behind,
				 &alpha0_chi1,
				 &alpha1_psi1,
				 y0,   incy,
				 x0,   incx,
				 c10t, cs_ct,
				 cntx
				);

			/* gamma11 = gamma11 +      alpha  * chi1 * conj(psi1)
			   + conj(alpha) * psi1 * conj(chi1); */
			PASTEMAC(d,adds)( alpha0_chi1_psi1, *gamma11 );
			PASTEMAC(d,adds)( alpha0_chi1_psi1, *gamma11 );

		}
	}
}

GENTFUNC(float, s, her2_unf_var1)
GENTFUNC(scomplex, c, her2_unf_var1)
GENTFUNC(dcomplex, z,her2_unf_var1)


