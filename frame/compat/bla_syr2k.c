/*

   BLIS
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2014, The University of Texas at Austin.
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

    #define SYR2K_BLIS_IMPL(ch, blasname) \
        PASTEF77S(ch,blasname) ( uploc, transa, m, k, alpha, a, lda, b, ldb, beta, c, ldc ); \
        arch_t id = bli_arch_query_id(); \
        if (id == BLIS_ARCH_ZEN5 || id == BLIS_ARCH_ZEN4) \
        { \
            bli_zero_zmm(); \
        } \

#else

    #define SYR2K_BLIS_IMPL(ch, blasname) \
        PASTEF77S(ch,blasname) ( uploc, transa, m, k, alpha, a, lda, b, ldb, beta, c, ldc ); \

#endif

#ifdef BLIS_BLAS3_CALLS_TAPI

#undef  GENTFUNC
#define GENTFUNC( ftype, ch, blasname, blisname ) \
\
void PASTEF77S(ch,blasname) \
     ( \
       const f77_char* uploc, \
       const f77_char* transa, \
       const f77_int*  m, \
       const f77_int*  k, \
       const ftype*    alpha, \
       const ftype*    a, const f77_int* lda, \
       const ftype*    b, const f77_int* ldb, \
       const ftype*    beta, \
             ftype*    c, const f77_int* ldc  \
     ) \
{ \
	AOCL_DTL_TRACE_ENTRY(AOCL_DTL_LEVEL_TRACE_1) \
	AOCL_DTL_LOG_SYR2K_INPUTS(AOCL_DTL_LEVEL_TRACE_1, *MKSTR(ch), *uploc, *transa, *m, *k, (void*)alpha, *lda, *ldb, (void*)beta, *ldc ) \
	uplo_t  blis_uploc; \
	trans_t blis_transa; \
	dim_t   m0, k0; \
	inc_t   rs_a, cs_a; \
	inc_t   rs_b, cs_b; \
	inc_t   rs_c, cs_c; \
\
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
	  ldb, \
	  ldc  \
	); \
\
	/* Quick return if possible. */ \
	if ( *m == 0 || (( PASTEMAC(ch,eq0)( *alpha ) || *k == 0) \
	   && PASTEMAC(ch,eq1)( *beta ) )) \
	{ \
	  AOCL_DTL_TRACE_EXIT(AOCL_DTL_LEVEL_TRACE_1); \
	  /* Finalize BLIS. */ \
	  bli_finalize_auto(); \
	  return; \
	} \
\
	/* Map BLAS chars to their corresponding BLIS enumerated type value. */ \
	bli_param_map_netlib_to_blis_uplo( *uploc, &blis_uploc ); \
	bli_param_map_netlib_to_blis_trans( *transa, &blis_transa ); \
\
	/* The real domain ssyr2k and dsyr2k in netlib BLAS treat a trans value
	   of 'C' (conjugate-transpose) as 'T' (transpose only). So, we have
	   to go out of our way a little to support this behavior. */ \
	if ( bli_is_real( PASTEMAC(ch,type) ) && \
	     bli_is_conjtrans( blis_transa ) ) \
	{ \
		blis_transa = BLIS_TRANSPOSE; \
	} \
\
	/* Typecast BLAS integers to BLIS integers. */ \
	bli_convert_blas_dim1( *m, m0 ); \
	bli_convert_blas_dim1( *k, k0 ); \
\
	/* Set the row and column strides of the matrix operands. */ \
	rs_a = 1; \
	cs_a = *lda; \
	rs_b = 1; \
	cs_b = *ldb; \
	rs_c = 1; \
	cs_c = *ldc; \
\
	/* If alpha is zero, scale C by beta and return early */ \
	if( PASTEMAC(ch,eq0)( *alpha ) ) \
	{ \
		PASTEMAC2(ch,scalm,_ex)( BLIS_NO_CONJUGATE, \
								  0, \
								  BLIS_NONUNIT_DIAG, \
								  blis_uploc, \
								  m0, \
								  m0, \
								  (ftype*) beta, \
								  (ftype*) c, rs_c, cs_c, \
								  NULL, \
								  NULL \
								); \
		/* Finalize BLIS. */ \
		AOCL_DTL_TRACE_EXIT(AOCL_DTL_LEVEL_TRACE_1) \
		bli_finalize_auto(); \
		return; \
	} \
\
	/* Call BLIS interface. */ \
	PASTEMAC2(ch,blisname,BLIS_TAPI_EX_SUF) \
	( \
	  blis_uploc, \
	  blis_transa, \
	  blis_transa, \
	  m0, \
	  k0, \
	  (ftype*)alpha, \
	  (ftype*)a, rs_a, cs_a, \
	  (ftype*)b, rs_b, cs_b, \
	  (ftype*)beta, \
	  (ftype*)c, rs_c, cs_c, \
	  NULL, \
	  NULL  \
	); \
\
	/* Finalize BLIS. */ \
	AOCL_DTL_TRACE_EXIT(AOCL_DTL_LEVEL_TRACE_1) \
	bli_finalize_auto(); \
} \
IF_BLIS_ENABLE_BLAS(\
void PASTEF77(ch,blasname) \
     ( \
       const f77_char* uploc, \
       const f77_char* transa, \
       const f77_int*  m, \
       const f77_int*  k, \
       const ftype*    alpha, \
       const ftype*    a, const f77_int* lda, \
       const ftype*    b, const f77_int* ldb, \
       const ftype*    beta, \
             ftype*    c, const f77_int* ldc  \
     ) \
{ \
	SYR2K_BLIS_IMPL(ch,blasname) \
} \
)

#else

#undef  GENTFUNC
#define GENTFUNC( ftype, ch, blasname, blisname ) \
\
void PASTEF77S(ch,blasname) \
     ( \
       const f77_char* uploc, \
       const f77_char* transa, \
       const f77_int*  m, \
       const f77_int*  k, \
       const ftype*    alpha, \
       const ftype*    a, const f77_int* lda, \
       const ftype*    b, const f77_int* ldb, \
       const ftype*    beta, \
             ftype*    c, const f77_int* ldc  \
     ) \
{ \
	AOCL_DTL_TRACE_ENTRY(AOCL_DTL_LEVEL_TRACE_1) \
	AOCL_DTL_LOG_SYR2K_INPUTS(AOCL_DTL_LEVEL_TRACE_1, *MKSTR(ch), *uploc,\
			 *transa, *m, *k, (void*)alpha, *lda, *ldb, (void*)beta, *ldc ) \
	uplo_t  blis_uploc; \
	trans_t blis_transa; \
	dim_t   m0, k0; \
\
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
	  ldb, \
	  ldc  \
	); \
\
	/* Quick return if possible. */ \
	if ( *m == 0 || (( PASTEMAC(ch,eq0)( *alpha ) || *k == 0) \
	   && PASTEMAC(ch,eq1)( *beta ) )) \
	{ \
	  AOCL_DTL_TRACE_EXIT(AOCL_DTL_LEVEL_TRACE_1); \
	  /* Finalize BLIS. */ \
	  bli_finalize_auto(); \
	  return; \
	} \
\
	/* Map BLAS chars to their corresponding BLIS enumerated type value. */ \
	bli_param_map_netlib_to_blis_uplo( *uploc, &blis_uploc ); \
	bli_param_map_netlib_to_blis_trans( *transa, &blis_transa ); \
\
	/* The real domain ssyr2k and dsyr2k in netlib BLAS treat a trans value
	   of 'C' (conjugate-transpose) as 'T' (transpose only). So, we have
	   to go out of our way a little to support this behavior. */ \
	if ( bli_is_real( PASTEMAC(ch,type) ) && \
	     bli_is_conjtrans( blis_transa ) ) \
	{ \
		blis_transa = BLIS_TRANSPOSE; \
	} \
\
	/* Typecast BLAS integers to BLIS integers. */ \
	bli_convert_blas_dim1( *m, m0 ); \
	bli_convert_blas_dim1( *k, k0 ); \
\
	/* Set the row and column strides of the matrix operands. */ \
	const inc_t rs_a = 1; \
	const inc_t cs_a = *lda; \
	const inc_t rs_b = 1; \
	const inc_t cs_b = *ldb; \
	const inc_t rs_c = 1; \
	const inc_t cs_c = *ldc; \
\
	/* If alpha is zero, scale C by beta and return early */ \
	if( PASTEMAC(ch,eq0)( *alpha ) ) \
	{ \
		PASTEMAC2(ch,scalm,_ex)( BLIS_NO_CONJUGATE, \
								  0, \
								  BLIS_NONUNIT_DIAG, \
								  blis_uploc, \
								  m0, \
								  m0, \
								  (ftype*) beta, \
								  (ftype*) c, rs_c, cs_c, \
								  NULL, \
								  NULL \
								); \
		/* Finalize BLIS. */ \
		AOCL_DTL_TRACE_EXIT(AOCL_DTL_LEVEL_TRACE_1) \
		bli_finalize_auto(); \
		return; \
	} \
\
	const num_t   dt     = PASTEMAC(ch,type); \
\
	const trans_t transb = blis_transa; \
	const struc_t strucc = BLIS_SYMMETRIC; \
\
	obj_t       alphao = BLIS_OBJECT_INITIALIZER_1X1; \
	obj_t       ao     = BLIS_OBJECT_INITIALIZER; \
	obj_t       bo     = BLIS_OBJECT_INITIALIZER; \
	obj_t       betao  = BLIS_OBJECT_INITIALIZER_1X1; \
	obj_t       co     = BLIS_OBJECT_INITIALIZER; \
\
	dim_t       m0_a, n0_a; \
	dim_t       m0_b, n0_b; \
\
	bli_set_dims_with_trans( blis_transa, m0, k0, &m0_a, &n0_a ); \
	bli_set_dims_with_trans( transb,      m0, k0, &m0_b, &n0_b ); \
\
	bli_obj_init_finish_1x1( dt, (ftype*)alpha, &alphao ); \
	bli_obj_init_finish_1x1( dt, (ftype*)beta,  &betao  ); \
\
	bli_obj_init_finish( dt, m0_a, n0_a, (ftype*)a, rs_a, cs_a, &ao ); \
	bli_obj_init_finish( dt, m0_b, n0_b, (ftype*)b, rs_b, cs_b, &bo ); \
	bli_obj_init_finish( dt, m0,   m0,   (ftype*)c, rs_c, cs_c, &co ); \
\
	bli_obj_set_uplo( blis_uploc, &co ); \
	bli_obj_set_conjtrans( blis_transa, &ao ); \
	bli_obj_set_conjtrans( transb, &bo ); \
\
	bli_obj_set_struc( strucc, &co ); \
\
	PASTEMAC(blisname,BLIS_OAPI_EX_SUF) \
	( \
	  &alphao, \
	  &ao, \
	  &bo, \
	  &betao, \
	  &co, \
	  NULL, \
	  NULL  \
	); \
\
	/* Finalize BLIS. */ \
	AOCL_DTL_TRACE_EXIT(AOCL_DTL_LEVEL_TRACE_1) \
	bli_finalize_auto(); \
} \
IF_BLIS_ENABLE_BLAS(\
void PASTEF77(ch,blasname) \
     ( \
       const f77_char* uploc, \
       const f77_char* transa, \
       const f77_int*  m, \
       const f77_int*  k, \
       const ftype*    alpha, \
       const ftype*    a, const f77_int* lda, \
       const ftype*    b, const f77_int* ldb, \
       const ftype*    beta, \
             ftype*    c, const f77_int* ldc  \
     ) \
{ \
	SYR2K_BLIS_IMPL(ch,blasname) \
} \
)

#endif

INSERT_GENTFUNC_BLAS( syr2k, syr2k )

