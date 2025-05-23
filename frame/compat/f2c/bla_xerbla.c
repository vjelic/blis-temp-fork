/*

   BLIS
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2014, The University of Texas at Austin
   Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.

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

// Make thread settings local to each thread calling BLIS routines.
// (The definition resides in bli_rntm.c.)
extern BLIS_THREAD_LOCAL rntm_t tl_rntm;

/* xerbla.f -- translated by f2c (version 19991025).
   You must link the resulting object file with the libraries:
	-lf2c -lm   (in that order)
*/

/* Table of constant values */

/* Subroutine */ void xerbla_blis_impl(const bla_character *srname, const bla_integer *info, ftnlen srname_len)
{
/*  -- LAPACK auxiliary routine (preliminary version) -- */
/*     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd., */
/*     Courant Institute, Argonne National Lab, and Rice University */
/*     February 29, 1992 */

/*     .. Scalar Arguments .. */
/*     .. */

/*  Purpose */
/*  ======= */

/*  XERBLA  is an error handler for the LAPACK routines. */
/*  It is called by an LAPACK routine if an input parameter has an */
/*  invalid value.  A message is printed and execution stops. */

/*  Installers may consider modifying the STOP statement in order to */
/*  call system-specific exception-handling facilities. */

/*  Arguments */
/*  ========= */

/*  SRNAME  (input) CHARACTER*6 */
/*          The name of the routine which called XERBLA. */

/*  INFO    (input) INTEGER */
/*          The position of the invalid parameter in the parameter list */
/*          of the calling routine. */
    //int i;

    //for ( i = 0; i < srname_len; ++i )
    //    srname[i] = toupper( srname[i] );

    // Make sure rntm variables are initialized.
    bli_init_once();

    // Store info value in thread-local rntm data structure.
    gint_t info_value = (gint_t) *info;
    bli_rntm_set_info_value_only( info_value, &tl_rntm );

    bool print_on_error = bli_rntm_print_on_error( &tl_rntm );
    if (print_on_error)
    {
        // The check for -10 is specific to xerbla_()'s use-case in ?imatcopy_() APIs.
        // The definition of an info value for memory failure could be abstracted
        // to a higher layer, if needed. This would enable us to reuse xerbla_()
        // with this specific info value, in case of encountering a memory allocation
        // failure.
        if( *info == -10 )
        {
          printf("** On entry to %6s, memory allocation failed\n", srname);
        }
        else
        {
          printf("** On entry to %6s, parameter number %2i had an illegal value\n",
              srname, (int)*info);
        }
    }

    bool stop_on_error = bli_rntm_stop_on_error( &tl_rntm );
    if (stop_on_error)
    {
        bli_abort();
    }

/*     End of XERBLA */

    return;
} /* xerbla_blis_impl */


#ifdef BLIS_ENABLE_BLAS
/* Subroutine */ void PASTEF770(xerbla)(const bla_character *srname, const bla_integer *info, ftnlen srname_len)
{
    xerbla_blis_impl(srname, info, srname_len);
    return;
} /* xerbla */
#endif

