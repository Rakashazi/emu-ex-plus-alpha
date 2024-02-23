/** \file   boolexpr.h
 * \brief   Boolean expression evaluation - header
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/* Copyright (C) 2023  Bas Wassink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef VICE_HOTKEYS_BOOLEXPR_H
#define VICE_HOTKEYS_BOOLEXPR_H

#include <stdbool.h>

/* Token IDs
 *
 * The values of this enum must match the array indexes in token_info[].
 */
enum {
    BEXPR_INVALID = -1, /**< invalid token */
    BEXPR_FALSE,        /**< FALSE constant */
    BEXPR_TRUE,         /**< TRUE constant */
    BEXPR_LPAREN,       /**< left parenthesis '(' */
    BEXPR_RPAREN,       /**< right parenthesis ')' */
    BEXPR_NOT,          /**< logical NOT operator '&&' */
    BEXPR_AND,          /**< logical AND operator '!'*/
    BEXPR_OR            /**< logical OR operator '||' */
};

/* Error codes */
enum {
    BEXPR_ERR_OK = 0,           /**< OK (no error) */
    BEXPR_ERR_FATAL,            /**< fatal error, should not normally happen */
    BEXPR_ERR_EXPECTED_TOKEN,   /**< parser expected a token */
    BEXPR_ERR_INVALID_TOKEN,    /**< parser didn't recognize token */
    BEXPR_ERR_EXPECTED_LPAREN,  /**< expected left parenthesis */
    BEXPR_ERR_EXPECTED_RPAREN,  /**< expected right parenthesis */
    BEXPR_ERR_UNMATCHED_PARENS, /**< unmatched parenthesis */
    BEXPR_ERR_EMPTY_EXPRESSION, /**< empty expression */
    BEXPR_ERR_MISSING_OPERAND,  /**< missing operand for operator */

    BEXPR_ERROR_COUNT
};


extern int  bexpr_errno;
const char *bexpr_strerror(int errnum);

void bexpr_init (void);
void bexpr_reset(void);
void bexpr_free (void);
void bexpr_print(void);

int  bexpr_token_parse(const char *text, const char **endptr);
bool bexpr_token_add  (int token);
bool bexpr_tokenize   (const char *text);
bool bexpr_evaluate   (bool *result);

#endif
