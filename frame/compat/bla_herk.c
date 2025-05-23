/*

   BLIS
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2014, The University of Texas at Austin
   Copyright (C) 2019 - 2024, Advanced Micro Devices, Inc. All rights reserved.

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


//
// Define BLAS-to-BLIS interfaces.
//

#if defined(BLIS_KERNELS_ZEN4)

    #define HERK_BLIS_IMPL(ch, blasname) \
        PASTEF77S(ch,blasname) ( uploc, transa, m, k, alpha, a, lda, beta, c, ldc ); \
        arch_t id = bli_arch_query_id(); \
        if (id == BLIS_ARCH_ZEN5 || id == BLIS_ARCH_ZEN4) \
        { \
            bli_zero_zmm(); \
        } \

#else

    #define HERK_BLIS_IMPL(ch, blasname) \
        PASTEF77S(ch,blasname) ( uploc, transa, m, k, alpha, a, lda, beta, c, ldc ); \

#endif

#ifdef BLIS_BLAS3_CALLS_TAPI

#undef  GENTFUNCCO
#define GENTFUNCCO( ftype, ftype_r, ch, chr, blasname, blisname ) \
\
void PASTEF77S(ch,blasname) \
     ( \
       const f77_char* uploc, \
       const f77_char* transa, \
       const f77_int*  m, \
       const f77_int*  k, \
       const ftype_r*  alpha, \
       const ftype*    a, const f77_int* lda, \
       const ftype_r*  beta, \
             ftype*    c, const f77_int* ldc  \
     ) \
{ \
	uplo_t  blis_uploc; \
	trans_t blis_transa; \
	dim_t   m0, k0; \
	inc_t   rs_a, cs_a; \
	inc_t   rs_c, cs_c; \
\
	AOCL_DTL_TRACE_ENTRY(AOCL_DTL_LEVEL_TRACE_1) \
	AOCL_DTL_LOG_HERK_INPUTS(AOCL_DTL_LEVEL_TRACE_1, *MKSTR(ch), *uploc, *transa, *m, *k, (void*)alpha, *lda, (void*)beta, *ldc);\
	/* Initialize BLIS. */ \
	bli_init_auto(); \
\
	/* Perform BLAS parameter checking. */ \
	PASTEBLACHK(blasname) \
	( \
	  MKSTR(ch), \
	  MKSTR(blasname), \
	  uploc, \
	  transa, \
	  m, \
	  k, \
	  lda, \
	  ldc  \
	); \
\
	/* Map BLAS chars to their corresponding BLIS enumerated type value. */ \
	bli_param_map_netlib_to_blis_uplo( *uploc, &blis_uploc ); \
	bli_param_map_netlib_to_blis_trans( *transa, &blis_transa ); \
\
	/* Typecast BLAS integers to BLIS integers. */ \
	bli_convert_blas_dim1( *m, m0 ); \
	bli_convert_blas_dim1( *k, k0 ); \
\
	/* We emulate the BLAS early return behavior with the following
	   conditional, which returns if one of the following is true:
	   - matrix C is empty
	   - the rank-k product is empty (either because alpha is zero or k
	     is zero) AND matrix C is not scaled. */ \
	if ( m0 == 0 || \
	     ( ( PASTEMAC(chr,eq0)( *alpha ) || k0 == 0 ) \
	       && PASTEMAC(chr,eq1)( *beta ) \
         ) \
	   ) \
	{ \
		AOCL_DTL_TRACE_EXIT(AOCL_DTL_LEVEL_TRACE_1) \
		/* Finalize BLIS. */ \
		bli_finalize_auto(); \
\
		return; \
	} \
\
	/* Set the row and column strides of the matrix operands. */ \
	rs_a = 1; \
	cs_a = *lda; \
	rs_c = 1; \
	cs_c = *ldc; \
\
	/* If alpha is zero, scale C by beta and return early */ \
	if( PASTEMAC(chr,eq0)( *alpha ) ) \
	{ \
		ftype beta_complex; \
		beta_complex.real = *beta; \
		beta_complex.imag = 0.0; \
		PASTEMAC2(ch,scalm,_ex)( BLIS_NO_CONJUGATE, \
								  0, \
								  BLIS_NONUNIT_DIAG, \
								  blis_uploc, \
								  m0, \
								  m0, \
								  (ftype*) &beta_complex, \
								  (ftype*) c, rs_c, cs_c, \
								  NULL, NULL \
								); \
		/* The Hermitian rank-k product was computed as Re(alpha)*A*A', even for the
		   diagonal elements. Mathematically, the imaginary components of
		   diagonal elements of a Hermitian rank-k product should always be
		   zero. However, in practice, they sometimes accumulate meaningless
		   non-zero values. To prevent this, we explicitly set those values
		   to zero before returning.
		*/ \
		PASTEMAC2(ch,setid,_ex)( 0, m0, m0, (void*)alpha, c, rs_c, cs_c, NULL, NULL ); \
		AOCL_DTL_TRACE_EXIT(AOCL_DTL_LEVEL_TRACE_1) \
		/* Finalize BLIS. */ \
		bli_finalize_auto(); \
		return; \
	} \
\
	/* Call BLIS interface. */ \
	PASTEMAC2(ch,blisname,BLIS_TAPI_EX_SUF) \
	( \
	  blis_uploc, \
	  blis_transa, \
	  m0, \
	  k0, \
	  (ftype_r*)alpha, \
	  (ftype*)a, rs_a, cs_a, \
	  (ftype_r*)beta, \
	  (ftype*)c, rs_c, cs_c, \
	  NULL, \
	  NULL  \
	); \
\
	AOCL_DTL_TRACE_EXIT(AOCL_DTL_LEVEL_TRACE_1) \
	/* Finalize BLIS. */ \
	bli_finalize_auto(); \
} \
IF_BLIS_ENABLE_BLAS(\
void PASTEF77(ch,blasname) \
     ( \
       const f77_char* uploc, \
       const f77_char* transa, \
       const f77_int*  m, \
       const f77_int*  k, \
       const ftype_r*  alpha, \
       const ftype*    a, const f77_int* lda, \
       const ftype_r*  beta, \
             ftype*    c, const f77_int* ldc  \
     ) \
{ \
	HERK_BLIS_IMPL(ch,blasname) \
} \
)

#else

#undef  GENTFUNCCO
#define GENTFUNCCO( ftype, ftype_r, ch, chr, blasname, blisname ) \
\
void PASTEF77S(ch,blasname) \
     ( \
       const f77_char* uploc, \
       const f77_char* transa, \
       const f77_int*  m, \
       const f77_int*  k, \
       const ftype_r*  alpha, \
       const ftype*    a, const f77_int* lda, \
       const ftype_r*  beta, \
             ftype*    c, const f77_int* ldc  \
     ) \
{ \
	uplo_t  blis_uploc; \
	trans_t blis_transa; \
	dim_t   m0, k0; \
\
	AOCL_DTL_TRACE_ENTRY(AOCL_DTL_LEVEL_TRACE_1) \
	AOCL_DTL_LOG_HERK_INPUTS(AOCL_DTL_LEVEL_TRACE_1, *MKSTR(ch), *MKSTR(uploc), *transa, *m, *k, (void*)alpha, *lda, (void*)beta, *ldc);\
	/* Initialize BLIS. */ \
	bli_init_auto(); \
\
	/* Perform BLAS parameter checking. */ \
	PASTEBLACHK(blasname) \
	( \
	  MKSTR(ch), \
	  MKSTR(blasname), \
	  uploc, \
	  transa, \
	  m, \
	  k, \
	  lda, \
	  ldc  \
	); \
\
	/* Map BLAS chars to their corresponding BLIS enumerated type value. */ \
	bli_param_map_netlib_to_blis_uplo( *uploc, &blis_uploc ); \
	bli_param_map_netlib_to_blis_trans( *transa, &blis_transa ); \
\
	/* Typecast BLAS integers to BLIS integers. */ \
	bli_convert_blas_dim1( *m, m0 ); \
	bli_convert_blas_dim1( *k, k0 ); \
\
	/* We emulate the BLAS early return behavior with the following
	   conditional, which returns if one of the following is true:
	   - matrix C is empty
	   - the rank-k product is empty (either because alpha is zero or k
	     is zero) AND matrix C is not scaled. */ \
	if ( m0 == 0 || \
	     ( ( PASTEMAC(chr,eq0)( *alpha ) || k0 == 0 ) \
	       && PASTEMAC(chr,eq1)( *beta ) \
         ) \
	   ) \
	{ \
		AOCL_DTL_TRACE_EXIT(AOCL_DTL_LEVEL_TRACE_1) \
		/* Finalize BLIS. */ \
		bli_finalize_auto(); \
\
		return; \
	} \
\
	/* Set the row and column strides of the matrix operands. */ \
	const inc_t rs_a = 1; \
	const inc_t cs_a = *lda; \
	const inc_t rs_c = 1; \
	const inc_t cs_c = *ldc; \
\
	const num_t   dt_r   = PASTEMAC(chr,type); \
	const num_t   dt     = PASTEMAC(ch,type); \
\
	/* If alpha is zero, scale C by beta and return early */ \
	if( PASTEMAC(chr,eq0)( *alpha ) ) \
	{ \
		ftype beta_complex; \
		beta_complex.real = *beta; \
		beta_complex.imag = 0.0; \
		PASTEMAC2(ch,scalm,_ex)( BLIS_NO_CONJUGATE, \
								  0, \
								  BLIS_NONUNIT_DIAG, \
								  blis_uploc, \
								  m0, \
								  m0, \
								  (ftype*) &beta_complex, \
								  (ftype*) c, rs_c, cs_c, \
								  NULL, NULL \
								); \
		/* The Hermitian rank-k product was computed as Re(alpha)*A*A', even for the
		   diagonal elements. Mathematically, the imaginary components of
		   diagonal elements of a Hermitian rank-k product should always be
		   zero. However, in practice, they sometimes accumulate meaningless
		   non-zero values. To prevent this, we explicitly set those values
		   to zero before returning.
		*/ \
		PASTEMAC2(ch,setid,_ex)( 0, m0, m0, (void*)alpha, c, rs_c, cs_c, NULL, NULL ); \
		AOCL_DTL_TRACE_EXIT(AOCL_DTL_LEVEL_TRACE_1) \
		/* Finalize BLIS. */ \
		bli_finalize_auto(); \
		return; \
	} \
\
	const struc_t strucc = BLIS_HERMITIAN; \
\
	obj_t       alphao = BLIS_OBJECT_INITIALIZER_1X1; \
	obj_t       ao     = BLIS_OBJECT_INITIALIZER; \
	obj_t       betao  = BLIS_OBJECT_INITIALIZER_1X1; \
	obj_t       co     = BLIS_OBJECT_INITIALIZER; \
\
	dim_t       m0_a, n0_a; \
\
	bli_set_dims_with_trans( blis_transa, m0, k0, &m0_a, &n0_a ); \
\
	bli_obj_init_finish_1x1( dt_r, (ftype_r*)alpha, &alphao ); \
	bli_obj_init_finish_1x1( dt_r, (ftype_r*)beta,  &betao  ); \
\
	bli_obj_init_finish( dt, m0_a, n0_a, (ftype*)a, rs_a, cs_a, &ao ); \
	bli_obj_init_finish( dt, m0,   m0,   (ftype*)c, rs_c, cs_c, &co ); \
\
	bli_obj_set_uplo( blis_uploc, &co ); \
	bli_obj_set_conjtrans( blis_transa, &ao ); \
\
	bli_obj_set_struc( strucc, &co ); \
\
	PASTEMAC(blisname,BLIS_OAPI_EX_SUF) \
	( \
	  &alphao, \
	  &ao, \
	  &betao, \
	  &co, \
	  NULL, \
	  NULL  \
	); \
\
	AOCL_DTL_TRACE_EXIT(AOCL_DTL_LEVEL_TRACE_1) \
	/* Finalize BLIS. */ \
	bli_finalize_auto(); \
} \
IF_BLIS_ENABLE_BLAS(\
void PASTEF77(ch,blasname) \
     ( \
       const f77_char* uploc, \
       const f77_char* transa, \
       const f77_int*  m, \
       const f77_int*  k, \
       const ftype_r*  alpha, \
       const ftype*    a, const f77_int* lda, \
       const ftype_r*  beta, \
             ftype*    c, const f77_int* ldc  \
     ) \
{ \
	HERK_BLIS_IMPL(ch,blasname) \
} \
)

#endif

INSERT_GENTFUNCCO_BLAS( herk, herk )

