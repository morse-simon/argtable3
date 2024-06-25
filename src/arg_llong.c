/*******************************************************************************
 * arg_llong: Implements the long long int command-line option
 *
 * This file is part of the argtable3 library.
 *
 * Copyright (C) 1998-2001,2003-2011,2013 Stewart Heitmann
 * <sheitmann@users.sourceforge.net>
 * All rights reserved.
 * Copyright 2024 Morse Micro
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of STEWART HEITMANN nor the  names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL STEWART HEITMANN BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGEg
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#include "argtable3.h"

#ifndef ARG_AMALGAMATION
#include "argtable3_private.h"
#endif

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>

static void arg_llong_resetfn(struct arg_llong* parent) {
    ARG_TRACE(("%s:resetfn(%p)\n", __FILE__, parent));
    parent->count = 0;
}

static int arg_llong_scanfn(struct arg_llong* parent, const char* argval) {
    int errorcode = 0;

    if (parent->count == parent->hdr.maxcount) {
        /* maximum number of arguments exceeded */
        errorcode = ARG_ERR_MAXCOUNT;
    } else if (!argval) {
        /* a valid argument with no argument value was given. */
        /* This happens when an optional argument value was invoked. */
        /* leave parent arguiment value unaltered but still count the argument. */
        parent->count++;
    } else {
        long long int val;
        const char* end;

        /* attempt to extract hex integer (eg: +0x123) from argval into val conversion */
        val = strtoll0X(argval, &end, 'X', 16);
        if (end == argval) {
            /* hex failed, attempt octal conversion (eg +0o123) */
            val = strtoll0X(argval, &end, 'O', 8);
            if (end == argval) {
                /* octal failed, attempt binary conversion (eg +0B101) */
                val = strtoll0X(argval, &end, 'B', 2);
                if (end == argval) {
                    /* binary failed, attempt decimal conversion with no prefix (eg 1234) */
                    val = strtoll(argval, (char**)&end, 10);
                    if (end == argval) {
                        /* all supported number formats failed */
                        return ARG_ERR_BADINT;
                    }
                }
            }
        }

        /* Check that strtoll consumed entire buffer (1.234 would leave end pointing to ".234") */
        if (*end != '\0')
        { 
            return ARG_ERR_BADINT;
        }

        /* Safety check for integer overflow. WARNING: this check    */
        /* achieves nothing on machines where size(int)==size(long). */
        if (val > LLONG_MAX || val < LLONG_MIN)
            errorcode = ARG_ERR_OVERFLOW;

        /* if success then store result in parent->ival[] array */
        if (errorcode == 0)
            parent->ival[parent->count++] = (long long int)val;
    }

    /* printf("%s:scanfn(%p,%p) returns %d\n",__FILE__,parent,argval,errorcode); */
    return errorcode;
}

static int arg_llong_checkfn(struct arg_llong* parent) {
    int errorcode = (parent->count < parent->hdr.mincount) ? ARG_ERR_MINCOUNT : 0;
    /*printf("%s:checkfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
    return errorcode;
}

static void arg_llong_errorfn(struct arg_llong* parent, arg_dstr_t ds, int errorcode, const char* argval, const char* progname) {
    const char* shortopts = parent->hdr.shortopts;
    const char* longopts = parent->hdr.longopts;
    const char* datatype = parent->hdr.datatype;

    /* make argval NULL safe */
    argval = argval ? argval : "";

    arg_dstr_catf(ds, "%s: ", progname);
    switch (errorcode) {
        case ARG_ERR_MINCOUNT:
            arg_dstr_cat(ds, "missing option ");
            arg_print_option_ds(ds, shortopts, longopts, datatype, "\n");
            break;

        case ARG_ERR_MAXCOUNT:
            arg_dstr_cat(ds, "excess option ");
            arg_print_option_ds(ds, shortopts, longopts, argval, "\n");
            break;

        case ARG_ERR_BADINT:
            arg_dstr_catf(ds, "invalid argument \"%s\" to option ", argval);
            arg_print_option_ds(ds, shortopts, longopts, datatype, "\n");
            break;

        case ARG_ERR_OVERFLOW:
            arg_dstr_cat(ds, "integer overflow at option ");
            arg_print_option_ds(ds, shortopts, longopts, datatype, " ");
            arg_dstr_catf(ds, "(%s is too large)\n", argval);
            break;
    }
}

struct arg_llong* arg_llong0(const char* shortopts, const char* longopts, const char* datatype, const char* glossary) {
    return arg_llongn(shortopts, longopts, datatype, 0, 1, glossary);
}

struct arg_llong* arg_llong1(const char* shortopts, const char* longopts, const char* datatype, const char* glossary) {
    return arg_llongn(shortopts, longopts, datatype, 1, 1, glossary);
}

struct arg_llong* arg_llongn(const char* shortopts, const char* longopts, const char* datatype, int mincount, int maxcount, const char* glossary) {
    size_t nbytes;
    struct arg_llong* result;

    /* foolproof things by ensuring maxcount is not less than mincount */
    maxcount = (maxcount < mincount) ? mincount : maxcount;

    nbytes = sizeof(struct arg_llong)    /* storage for struct arg_int */
             + (size_t)maxcount * sizeof(long long int); /* storage for ival[maxcount] array */

    result = (struct arg_llong*)xmalloc(nbytes);

    /* init the arg_hdr struct */
    result->hdr.flag = ARG_HASVALUE;
    result->hdr.shortopts = shortopts;
    result->hdr.longopts = longopts;
    result->hdr.datatype = datatype ? datatype : "<int>";
    result->hdr.glossary = glossary;
    result->hdr.mincount = mincount;
    result->hdr.maxcount = maxcount;
    result->hdr.parent = result;
    result->hdr.resetfn = (arg_resetfn*)arg_llong_resetfn;
    result->hdr.scanfn = (arg_scanfn*)arg_llong_scanfn;
    result->hdr.checkfn = (arg_checkfn*)arg_llong_checkfn;
    result->hdr.errorfn = (arg_errorfn*)arg_llong_errorfn;

    /* store the ival[maxcount] array immediately after the arg_int struct */
    result->ival = (long long int*)(result + 1);
    result->count = 0;

    ARG_TRACE(("arg_llongn() returns %p\n", result));
    return result;
}
