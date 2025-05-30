/*

   BLIS
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.

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

#define bla_gemm_get_size_check( dt_str, op_str, identifier, m, n, k ) \
{ \
	f77_int info = 0; \
	f77_int A_identifier, B_identifier; \
\
	A_identifier  = PASTE_LSAME( identifier, "A", (ftnlen)1, (ftnlen)1 ); \
	B_identifier  = PASTE_LSAME( identifier, "B", (ftnlen)1, (ftnlen)1 ); \
\
	if      ( !A_identifier && !B_identifier ) \
		info = 1; \
	else if ( *m < 0 ) \
		info = 2; \
	else if ( *n < 0 ) \
		info = 3; \
	else if ( *k < 0 ) \
		info = 4; \
\
	if ( info != 0 ) \
	{ \
		char func_str[ BLIS_MAX_BLAS_FUNC_STR_LENGTH ]; \
\
		sprintf( func_str, "%s%-5s", dt_str, op_str ); \
\
		bli_string_mkupper( func_str ); \
\
		PASTE_XERBLA( func_str, &info, (ftnlen)6 ); \
\
		return 0; \
	} \
}

#define bla_gemm_pack_check( dt_str, op_str, identifier, trans, m, n, k, pld ) \
{ \
	f77_int info = 0; \
	f77_int A_identifier, B_identifier; \
	f77_int no_trans_param, conj_param, trans_param; \
	f77_int nrow; \
\
	A_identifier  = PASTE_LSAME( identifier, "A", (ftnlen)1, (ftnlen)1 ); \
	B_identifier  = PASTE_LSAME( identifier, "B", (ftnlen)1, (ftnlen)1 ); \
\
	no_trans_param  = PASTE_LSAME( trans, "N", (ftnlen)1, (ftnlen)1 ); \
	conj_param      = PASTE_LSAME( trans, "C", (ftnlen)1, (ftnlen)1 ); \
	trans_param     = PASTE_LSAME( trans, "T", (ftnlen)1, (ftnlen)1 ); \
\
	if ( A_identifier ) \
	{ \
		if ( no_trans_param ) { nrow = *m; } \
		else                  { nrow = *k; } \
	} \
	else if ( B_identifier ) \
	{ \
		if ( no_trans_param ) { nrow = *k; } \
		else                  { nrow = *n; } \
	} \
\
	if      ( !A_identifier && !B_identifier ) \
		info = 1; \
	else if ( !no_trans_param && !conj_param && !trans_param ) \
		info = 2; \
	else if ( *m < 0 ) \
		info = 3; \
	else if ( *n < 0 ) \
		info = 4; \
	else if ( *k < 0 ) \
		info = 5; \
	else if ( *pld < bli_max( 1, nrow ) ) \
		info = 6; \
\
	if ( info != 0 ) \
	{ \
		char func_str[ BLIS_MAX_BLAS_FUNC_STR_LENGTH ]; \
\
		sprintf( func_str, "%s%-5s", dt_str, op_str ); \
\
		bli_string_mkupper( func_str ); \
\
		PASTE_XERBLA( func_str, &info, (ftnlen)6 ); \
\
		return; \
	} \
}

#define bla_gemm_compute_check( dt_str, op_str, transa, transb, m, n, k, lda, ldb, rs_c, cs_c ) \
{ \
	f77_int info = 0; \
	f77_int nota,  notb; \
	f77_int conja, conjb; \
	f77_int ta,    tb; \
	f77_int packa, packb; \
	f77_int nrowa, nrowb; \
\
	nota  = PASTE_LSAME( transa, "N", (ftnlen)1, (ftnlen)1 ); \
	notb  = PASTE_LSAME( transb, "N", (ftnlen)1, (ftnlen)1 ); \
	conja = PASTE_LSAME( transa, "C", (ftnlen)1, (ftnlen)1 ); \
	conjb = PASTE_LSAME( transb, "C", (ftnlen)1, (ftnlen)1 ); \
	ta    = PASTE_LSAME( transa, "T", (ftnlen)1, (ftnlen)1 ); \
	tb    = PASTE_LSAME( transb, "T", (ftnlen)1, (ftnlen)1 ); \
	packa = PASTE_LSAME( transa, "P", (ftnlen)1, (ftnlen)1 ); \
	packb = PASTE_LSAME( transb, "P", (ftnlen)1, (ftnlen)1 ); \
\
	if ( nota || packa ) { nrowa = *m; } \
	else        { nrowa = *k; } \
	if ( notb || packb ) { nrowb = *k; } \
	else        { nrowb = *n; } \
\
	if      ( !nota && !conja && !ta && !packa ) \
		info = 1; \
	else if ( !notb && !conjb && !tb && !packb ) \
		info = 2; \
	else if ( *m < 0 ) \
		info = 3; \
	else if ( *n < 0 ) \
		info = 4; \
	else if ( *k < 0 ) \
		info = 5; \
	else if ( !packa && *lda < bli_max( 1, nrowa ) ) /* lda is ignored when A is packed. */ \
		info = 7; \
	else if ( !packb && *ldb < bli_max( 1, nrowb ) ) /* ldb is ignored when B is packed. */ \
		info = 9; \
	else if ( *rs_c < 1 || *cs_c < 1 ) \
		info = 12; \
	else if ( /* Skip check for validity of strides when m==1 or n==1. */ \
	         ( *m != 1 && *n != 1 ) && \
	         ( ( *rs_c == 1 && *cs_c < bli_max( 1, *m ) ) || \
	           ( *cs_c == 1 && *rs_c < bli_max( 1, *n ) ) ) ) \
		info = 12; \
\
	if ( info != 0 ) \
	{ \
		char func_str[ BLIS_MAX_BLAS_FUNC_STR_LENGTH ]; \
\
		sprintf( func_str, "%s%-5s", dt_str, op_str ); \
\
		bli_string_mkupper( func_str ); \
\
		PASTE_XERBLA( func_str, &info, (ftnlen)6 ); \
\
		return; \
	} \
}
