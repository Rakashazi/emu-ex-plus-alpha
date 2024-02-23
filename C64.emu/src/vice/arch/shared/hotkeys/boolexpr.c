/** \file   boolexpr.c
 * \brief   Boolean expression evaluation
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * This code was originally developed at https://github.com/Compyx/boolexpr/
 * The project page at github has a test driver which can be used to debug this
 * code in a test driven way, should that be required.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

#include "lib.h"

#include "boolexpr.h"


/* Enable debugging messages */
/* #define DEBUG_BEXPR */

/** \def bexpr_debug
 *
 * Print debugging message on stdout if \c DEBUG_BEXPR is defined. Use just like
 * printf(), for example: `bexpr_debug("value = %d\n", 42)` will print
 * "[debug] \<funcname\>(): value = 42".
 */

#ifdef DEBUG_BEXPR
#define bexpr_debug(...) \
    printf("[debug] %s(): ", __func__); \
    printf(__VA_ARGS__)
#else
#define bexpr_debug(...)
#endif


/* {{{ Types, enums, macros */
/** \brief  Array length helper
 *
 * Determine size of \a arr in number of elements.
 */
#define ARRAY_LEN(arr)  (sizeof arr / sizeof arr[0])

/** \brief  Set error code (and for now print error code and message on stderr
 *
 * \param[in]   errnum  error code
 */
#define SET_ERROR(errnum) \
    bexpr_errno = errnum; \
    fprintf(stderr, "%s(): error %d: %s\n", __func__, errnum, bexpr_strerror(errnum));

/* Associativity of operators */
enum {
    BEXPR_LTR,  /**< left-to-right associativity */
    BEXPR_RTL,  /**< right-to-left associativity */
};

/* Arity of operators */
enum {
    BEXPR_UNARY  = 1,   /**< unary operator */
    BEXPR_BINARY = 2    /**< binary operator */
};

/** \brief  Token object
 */
typedef struct token_s {
    const char *text;   /**< text */
    int         id;     /**< ID */
    int         arity;  /**< operator arity */
    int         assoc;  /**< operator associativity */
    int         prec;   /**< operator precedence */
} token_t;

/** \brief  Token list object
 */
typedef struct token_list_s {
    const token_t **tokens; /**< array of token info pointers */
    size_t          size;   /**< size of \c tokens */
    int             index;  /**< index in \c tokens, -1 means list is empty */
} token_list_t;

/** \brief  Maximum lenght of a token's text */
#define MAX_TOKEN_LEN   5

/** \brief  Initializer for a token list */
#define TLIST_INIT { .tokens = NULL, .size = 0, .index = -1 }

/** \brief  Initial number of available tokens in a token list */
#define TLIST_INITIAL_SIZE  32u
/* }}} */

/* {{{ Static constant data */
/** \brief  List of valid tokens
 *
 * Contains both operators and operands.
 *
 * The \c id members here must match their array index.
 */
static const token_t token_info[] = {
    { "false",  BEXPR_FALSE,    0,                  0,              0 },
    { "true",   BEXPR_TRUE,     0,                  0,              0 },
    { "(",      BEXPR_LPAREN,   0,                  BEXPR_LTR,      4 },
    { ")",      BEXPR_RPAREN,   0,                  BEXPR_LTR,      4 },
    { "!",      BEXPR_NOT,      BEXPR_UNARY,        BEXPR_RTL,      3 },
    { "&&",     BEXPR_AND,      BEXPR_BINARY,       BEXPR_LTR,      2 },
    { "||",     BEXPR_OR,       BEXPR_BINARY,       BEXPR_LTR,      1 }
};

/** \brief  Valid characters in a token's text */
static const int token_chars[] = {
    '(', ')', '!', '&', '|', '0', '1', 'a', 'e', 'f', 'l', 'r', 's', 't', 'u'
};

/** \brief  Error messages */
static const char *error_messages[] = {
    "OK",
    "fatal error",
    "expected token",
    "invalid token",
    "expected left parenthesis",
    "expected right parenthesis",
    "unmatched parentheses",
    "expression is empty",
    "missing operand"
};
/* }}} */

/** \brief  Copy of text fed to tokenizer with bexpr_parse()
 */
static char *infix_text = NULL;

/** \brief  Tokenized infix expression
 *
 * Input for the infix to postfix conversion.
 */
static token_list_t infix_tokens = TLIST_INIT;

/** \brief  Token stack
 *
 * Stack used for operators during postfix to infix conversion, and for
 * operands during postfix expression evaluation.
 */
static token_list_t stack = TLIST_INIT;

/** \brief  Token queue
 *
 * Queue used for output during postfix to infix conversion, and as input
 * during postfix expression evaluation.
 */
static token_list_t queue = TLIST_INIT;

/** \brief  Error code */
int bexpr_errno = 0;


/** \brief  Get error message for error number
 *
 * \param[in]   errnum  error number
 *
 * \return  error message
 */
const char *bexpr_strerror(int errnum)
{
    if (errnum < 0 || errnum >= (int)ARRAY_LEN(error_messages)) {
        return "unknown error";
    } else {
        return error_messages[errnum];
    }
}


/* {{{ Memory management: reimplementation of VICE's lib_foo() functions */
#if 0
static void lib_free(void *ptr);

/** \brief  Allocate memory
 *
 * Allocate \a size bytes on the heap, call \c exit(1) when no memory is
 * available.
 *
 * \param[in]   size    number of bytes to allocate
 *
 * \return  pointer to allocated memory
 */
static void *lib_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "fatal: failed to allocate %zu bytes, exiting.\n", size);
        exit(1);
    }
    return ptr;
}

/** \brief  Reallocate (resize) memory
 *
 * Like \c realloc(3), but call exit on error.
 *
 * \param[in]   ptr     memory to reallocate
 * \param[in]   size    new size for \a ptr
 *
 * \return  pointer to (re)allocated memory
 *
 * \note    Like \c realloc(3), using \c NULL for \a ptr is equivalent to
 *          calling lib_malloc(\a size)
 * \note    Like \c realloc(3), using \c 0 for \a size is equivalent to calling
 *          lib_free(\a ptr)
 */
static void *lib_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {
        ptr = lib_malloc(size);
    } else if (size == 0) {
        lib_free(ptr);
        ptr = NULL;
    } else {
        void *tmp = realloc(ptr, size);
        if (tmp == NULL) {
            /* realloc(3) can return NULL when reallocating to a smaller size,
             * in which case we should return the original pointer, but there's
             * no way to determine this.
             */
            fprintf(stderr, "fatal: failed to reallocate %zu bytes, exiting.\n", size);
            free(ptr);
            exit(1);
        }
        ptr = tmp;
    }
    return ptr;
}

/** \brief  Free memory
 *
 * \param[in]   ptr memory to free
 */
static void lib_free(void *ptr)
{
    free(ptr);
}
#endif
/* }}} */

/* {{{ Token handling */
/** \brief  Skip whitespace in string
 *
 * \param[in]   s   string
 *
 * \return  pointer to first non-whitepace character (can be the terminating
 *          nul character if \a s consists of only whitespace)
 */
static const char *skip_whitespace(const char *s)
{
    while (*s != '\0' && isspace((unsigned char)*s)) {
        s++;
    }
    return s;
}

/** \brief  Determine if a character is a valid token text character
 *
 * \param[in]   ch  character to check
 *
 * \return  \c true if valid
 */
static bool is_token_char(int ch)
{
    for (size_t i = 0; i < ARRAY_LEN(token_chars); i++) {
        if (token_chars[i] == ch) {
            return true;
        }
    }
    return false;
}

/** \brief  Determine if token ID is valid
 *
 * \param[in]   id  token ID
 *
 * \return  \c true if valid
 */
static bool is_valid_token_id(int id)
{
    return id >= 0 && id < (int)ARRAY_LEN(token_info);
}

#if 0
static bool is_operator(int token)
{
    return (is_valid_token(token)) &&
           ((token != BEXPR_FALSE) && (token != BEXPR_TRUE));
}
#endif

/** \brief  Determine if a token is an operand
 *
 * \param[in]   id  token ID
 *
 * \return  \c true if token is an operand
 */
static bool is_operand(int id)
{
    return (is_valid_token_id(id)) &&
           ((id == BEXPR_FALSE) || (id == BEXPR_TRUE));
}

/** \brief  Parse text for a valid token
 *
 * Parse \a text looking for a valid token text, and if found return the ID.
 * A pointer to the first non-valid character in \a text is stored in \a endptr
 * if \a endptr isn't \c NULL.
 *
 * \param[in]   text    text to parse
 * \param[out]  endptr  location in \a text of first non-token character
 *
 * \return  token ID or \c BEXPR_INVALID on error
 *
 * \throw   BEXPR_ERR_EXPECTED_TOKEN
 * \throw   BEXPR_ERR_INVALID_TOKEN
 */
int bexpr_token_parse(const char *text, const char **endptr)
{
    const char *pos;
    size_t      tlen;

    pos = text = skip_whitespace(text);
    while (*pos != '\0' && is_token_char(*pos)) {
        pos++;
    }
    if (pos - text == 0) {
        if (endptr != NULL) {
            *endptr = NULL;
        }
        SET_ERROR(BEXPR_ERR_EXPECTED_TOKEN);
        return BEXPR_INVALID;
    }

    tlen = (size_t)(pos - text);
    if (tlen > MAX_TOKEN_LEN) {
        tlen = MAX_TOKEN_LEN;
    }

    /* look up token, reducing size (greedy matching) each time */
    while (tlen >= 1) {
        for (size_t i = 0; i < ARRAY_LEN(token_info); i++) {
            if (strncmp(token_info[i].text, text, tlen) == 0) {
                if (endptr != NULL) {
                    *endptr = text + tlen;
                }
                return token_info[i].id;
            }
        }
        tlen--;
    }
    SET_ERROR(BEXPR_ERR_INVALID_TOKEN);
    return BEXPR_INVALID;
}

/** \brief  Get pointer to element in token info array
 *
 * \param[in]   id  token ID
 *
 * \return  token info array element or \c NULL when not found
 */
static const token_t *token_get(int id)
{
    if (is_valid_token_id(id)) {
        return &token_info[id];
    }
    return NULL;
}

/** \brief  Get bool value from token
 *
 * \param[in]   token   token
 *
 * \return  \c true if \a token is \c BEXPR_TRUE
 */
static bool token_to_bool(const token_t *token)
{
    return (bool)(token->id == BEXPR_TRUE);
}

/** \brief  Get token for boolean value
 *
 * \param[in]   value   boolean value
 *
 * \return  pointer to token representing \a value
 */
static const token_t *token_from_bool(bool value)
{
    return token_get(value ? BEXPR_TRUE : BEXPR_FALSE);
}
/* }}} */


/* Dynamic token list, implements list, stack, and queue operations as required
 * by the rest of the code.
 */

/** \brief  Initialize token list for use
 *
 * Initialize \a list by allocating space for \c TLIST_INITIAL_SIZE tokens and
 * marking the \a list empty.
 *
 * \param[in]   list    token list
 */
static void token_list_init(token_list_t *list)
{
    list->size   = TLIST_INITIAL_SIZE;
    list->index  = -1;
    list->tokens = lib_malloc(sizeof *(list->tokens) * list->size);
}

/** \brief  Reset token list for reuse
 *
 * Reset \a list for reuse by marking it empty, without freeing the token array.
 * To properly free a token list, use token_list_free().
 *
 * \param[in]   list    token list
 */
static void token_list_reset(token_list_t *list)
{
    list->index = -1;
}

/** \brief  Resize token array if required
 *
 * Double the size of the token array if it's full.
 *
 * \param[in]   list    token list
 */
static void token_list_resize_maybe(token_list_t *list)
{
    if ((size_t)(list->index + 1) == list->size) {
        list->size  *= 2;
        list->tokens = lib_realloc(list->tokens, sizeof *(list->tokens) * list->size);
    }
}

/** \brief  Free memory used by token list
 *
 * Free memory used by the \a list's token array.
 */
static void token_list_free(token_list_t *list)
{
    lib_free(list->tokens);
}

/** \brief  Get length of token list
 *
 * Get number of tokens in \a list.
 *
 * \param[in]   list    token list
 *
 * \return  length of \a list
 */
static int token_list_length(const token_list_t *list)
{
    return list->index + 1;
}

/** \brief  Determine if token list is empty
 *
 * \param[in]   list    token list
 *
 * \return  \c true if \a list is empty
 */
static bool token_list_is_empty(const token_list_t *list)
{
    return (bool)(list->index < 0);
}

#ifdef DEBUG_BOOLEXPR
/** \brief  Print token list on stdout
 *
 * Print the text of each token in \a list separated by commas. Does not print
 * a newline so the output can be used "inline" with other output.
 *
 * \param[in]   list    token list
 */
static void token_list_print(const token_list_t *list)
{
    int index;
    int length = token_list_length(list);

    putchar('[');
    for (index = 0; index < length; index++) {
        printf("%s", list->tokens[index]->text);
        if (index < length - 1) {
            printf(", ");
        }
    }
    putchar(']');
}
#endif

/** \brief  Push token onto end of list
 *
 * Push \a token onto end of \a list, treating \a list like a stack.
 *
 * \param[in]   list    token list
 * \param[in]   token   token
 */
static void token_list_push(token_list_t *list, const token_t *token)
{
    token_list_resize_maybe(list);
    list->tokens[++list->index] = token;
}

/** \brief  Push token by its ID onto end of list
 *
 * Push token onto end of \a list, treating \a list like a stack.
 *
 * \param[in]   list    token list
 * \param[in]   id      token ID
 *
 * \return  \c false if \a id is invalid
 */
static bool token_list_push_id(token_list_t *list, int id)
{
    const token_t *token = token_get(id);
    if (token != NULL) {
        token_list_push(list, token);
        return true;
    }
    return false;
}

/** \brief  Enqueue token onto? token list
 *
 * Enqueue \a token onto \a list, treating \a list as a queue.
 *
 * \param[in]   list    token list
 * \param[in]   token   token
 */
#define token_list_enqueue(list, token) token_list_push(list, token)

/** \brief  Peek at the end of the list
 *
 * Get token at the tail of \a list without removing it.
 *
 * \param[in]   list    toke list
 *
 * \return  token or \c NULL if \a list is empty
 */
static const token_t *token_list_peek(const token_list_t *list)
{
    if (list->index >= 0) {
        return list->tokens[list->index];
    } else {
        return NULL;
    }
}

/** \brief  Pull token from end of list
 *
 * Get item from end of \a list and remove it, treating \a list as a stack.
 *
 * \param[in]   list    token list
 *
 * \return  token or \c NULL if \a list is empty
 */
static const token_t *token_list_pull(token_list_t *list)
{
    if (list->index >= 0) {
        return list->tokens[list->index--];
    }
    return NULL;
}

/** \brief  Get token at index without removing it
 *
 * \param[in]   list    token list
 * \param[in]   index   index in \a list
 *
 * \return  token or \c NULL if \a index is out of bounds
 */
static const token_t* token_list_token_at(const token_list_t *list, int index)
{
    /* list->index is the index of the last item */
    if (index < 0 || index > list->index) {
        return NULL;
    }
    return list->tokens[index];
}


/** \brief  Initialize expression for use
 *
 * Allocate memory for tokenizer and evaluator.
 */
void bexpr_init(void)
{
    token_list_init(&infix_tokens);
    token_list_init(&stack);
    token_list_init(&queue);
    infix_text  = NULL;
    bexpr_errno = 0;
}


/** \brief  Reset tokenizer and evaluator for new expression
 *
 * Reset internal data structures for new expression, without freeing and then
 * allocating resources again.
 *
 * If using this module multiple times during the lifetime of a program (which
 * is likely), do not call bexpr_free() followed by bexpr_init() when having to
 * handle another expression, but call this function instead.
 */
void bexpr_reset(void)
{
    lib_free(infix_text);
    infix_text  = NULL;
    token_list_reset(&infix_tokens);
    token_list_reset(&stack);
    token_list_reset(&queue);
    bexpr_errno = 0;
}


/** \brief  Print tokenized expression on stdout */
void bexpr_print(void)
{
    int index;
    int length = token_list_length(&infix_tokens);

    for (index = 0; index < length; index++) {
        const token_t *token = token_list_token_at(&infix_tokens, index);

        printf("'%s'", token->text);
        if (index < length) {
            printf(", ");
        }
    }
    putchar('\n');
}


/** \brief  Free memory used by expression
 *
 * Free all memory used by the expression, including the operator stack and the
 * output queue.
 */
void bexpr_free(void)
{
    lib_free(infix_text);
    token_list_free(&infix_tokens);
    token_list_free(&stack);
    token_list_free(&queue);
}


/** \brief  Add token to expression
 *
 * \param[in]   id  token ID
 *
 * \return  \c false if token \a id is invalid
 */
bool bexpr_token_add(int id)
{
    if (token_list_push_id(&infix_tokens, id)) {
        return true;
    } else {
        SET_ERROR(BEXPR_ERR_INVALID_TOKEN);
        return false;
    }
}


/** \brief  Generate expression from a string
 *
 * Parse \a text and tokenize into an expression. There is no syntax checking
 * performed, only splitting the \a text into tokens for the evaluator.
 *
 * \param[in]   text    string to tokenize
 *
 * \return  \c true on success
 */
bool bexpr_tokenize(const char *text)
{
    size_t len;

    text = skip_whitespace(text);
    len  = strlen(text);

    /* make copy of input text */
    infix_text = lib_malloc(len + 1u);
    memcpy(infix_text, text, len + 1u);

    while (*text != '\0') {
        const char *endptr;
        int         token;

        text  = skip_whitespace(text);
        token = bexpr_token_parse(text, &endptr);
        if (token == BEXPR_INVALID) {
            /* error code already set */
            return false;
        }
        bexpr_token_add(token);
        text = endptr;
    }
    return true;
}


/** \brief  Convert infix expression to postfix expression
 *
 * Use the Shunting yard algorithm to convert an infix expression to postfix
 * ("reverse polish notation").
 *
 * \return  \c true on succces
 */
static bool infix_to_postfix(void)
{
    const token_t *oper1 = NULL;
    const token_t *oper2 = NULL;

    /* reset stack for use as operand stack */
    token_list_reset(&stack);
    /* reset output queue */
    token_list_reset(&queue);

    /* iterate infix expression, generating a postfix expression */
    for (int i = 0; i < token_list_length(&infix_tokens); i++) {
        oper1 = token_list_token_at(&infix_tokens, i);
#if 0
        printf("\n%s(): stack: ", __func__);
        token_list_print(&stack);
        putchar('\n');
        printf("%s(): queue: ", __func__);
        token_list_print(&queue);
        putchar('\n');
        printf("%s(): token: '%s':\n",  __func__, oper1->text);
#endif
        if (is_operand(oper1->id)) {
            /* operands are added unconditionally to the output queue */
            token_list_enqueue(&queue, oper1);
        } else {
            /* handle operators */
            if (oper1->id == BEXPR_LPAREN) {
                /* left parenthesis: onto the operator stack */
                token_list_push(&stack, oper1);
            } else if (oper1->id == BEXPR_RPAREN) {
                /* right parenthesis: while there's an operator on the stack
                 * and it's not a left parenthesis: pull from stack and add to
                 * the output queue */

                while (!token_list_is_empty(&stack)) {
                    oper1 = token_list_pull(&stack);
                    if (oper1->id == BEXPR_LPAREN) {
                        break;
                    }
                    token_list_enqueue(&queue, oper1);
                }
                /* sanity check: must have a left parenthesis otherwise we
                 * have mismatched parenthesis */
                if (oper1->id != BEXPR_LPAREN) {
                    SET_ERROR(BEXPR_ERR_EXPECTED_LPAREN);
                    return false;
                }

            } else {
                /* handle operator until a left parenthesis is on top of the
                 * operator stack */
                while (!token_list_is_empty(&stack)) {

                    /* check for left parenthesis */
                    oper2 = token_list_peek(&stack);
                    if (oper2->id == BEXPR_LPAREN) {
                        break;
                    }

                    if ((oper2->prec > oper1->prec) ||
                            ((oper2->prec == oper1->prec) && oper1->assoc == BEXPR_LTR)) {
                        oper2 = token_list_pull(&stack);
                        token_list_enqueue(&queue, oper2);
                    } else {
                        break;
                    }
                }
                token_list_push(&stack, oper1);
            }
        }
    }
#if 0
    printf("%s(): operator stack = ", __func__);
    token_list_print(&stack);
    putchar('\n');
#endif
    while (!token_list_is_empty(&stack)) {
        oper1 = token_list_pull(&stack);
#if 0
        printf("%s(): pulled operator (%s,%d)\n",
               __func__, oper1->text, oper1->id);
#endif
        if (oper1->id == BEXPR_LPAREN) {
            /* unexpected left parenthesis */
            SET_ERROR(BEXPR_ERR_UNMATCHED_PARENS);
            return false;
        }
        token_list_enqueue(&queue, oper1);
    }

    bexpr_debug("output queue = ");
#ifdef DEBUG_BEXPR
    token_list_print(&queue);
    putchar('\n');
#endif
    return true;
}

/** \brief  Evaluate postfix expression
 *
 * Evaluate postfix expression in \c queue.
 *
 * \param[out]  result  result of expression
 *
 * \return  \c true on success
 */
static bool eval_postfix(bool *result)
{
    int  index;
    int  length;
    const token_t *token;

    /* reset stack for use as operand stack */
    token_list_reset(&stack);

    /* iterate queue containing postfix expression and try to evaluate it */
    length = token_list_length(&queue);
    for (index = 0; index < length; index++) {
        token = token_list_token_at(&queue, index);

        bexpr_debug("token %d = %s\n", index, token->text);
        if (is_operand(token->id)) {
            bexpr_debug("operand, pusing onto the stack: ");

            token_list_push(&stack, token);
#ifdef DEBUG_BEXPR
            token_list_print(&stack);
            putchar('\n');
#endif
        } else {
            /* operator, pull argument(s) from stack */
            const token_t *arg1;
            const token_t *arg2 = NULL;

            bool b1 = false;
            bool b2 = false;
            bool res;

            arg1 = token_list_pull(&stack);
            if (arg1 == NULL) {
                SET_ERROR(BEXPR_ERR_MISSING_OPERAND);
                return false;
            }
            b1 = token_to_bool(arg1);

            if (token->arity == BEXPR_BINARY) {
                /* binary operator, pull another argument */
                arg2 = token_list_pull(&stack);
                if (arg2 == NULL) {
                    SET_ERROR(BEXPR_ERR_MISSING_OPERAND);
                    return false;
                }
                b2 = token_to_bool(arg2);
            }

            switch (token->id) {
                case BEXPR_NOT:
                    res = !b1;
                    break;
                case BEXPR_AND:
                    res = b1 && b2;
                    break;
                case BEXPR_OR:
                    res = b1 || b2;
                    break;
                default:
                    SET_ERROR(BEXPR_ERR_INVALID_TOKEN);
                    return false;
            }
            token_list_push(&stack, token_from_bool(res));
        }
    }

    /* final result should be on the stack */
    token = token_list_pull(&stack);
    if (token == NULL) {
        SET_ERROR(BEXPR_ERR_MISSING_OPERAND);   /* illegal expression/syntax error? */
        return false;
    }

    *result = token_to_bool(token);
    return true;
}


/** \brief  Evaluate boolean expression
 *
 * Evaluate boolean expression, either obtained by bexpr_parse() or by adding
 * tokens with bexpr_add_token().
 *
 * \param[out]  result  result of evaluation
 *
 * \return  \c true on succes
 */
bool bexpr_evaluate(bool *result)
{
    *result = false;
    bexpr_errno = 0;

    if (token_list_length(&infix_tokens) <= 0) {
        SET_ERROR(BEXPR_ERR_EMPTY_EXPRESSION);
        return false;
    }

    /* convert infix expression to postfix expression */
    if (!infix_to_postfix()) {
        /* error code already set */
        return false;
    }

    /* try to evaluate the postfix expression in the queue */
    if (!eval_postfix(result)) {
        /* error code already set */
        return false;
    }

    return true;
}
