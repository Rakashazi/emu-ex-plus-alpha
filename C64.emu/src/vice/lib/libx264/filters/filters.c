/*****************************************************************************
 * filters.c: common filter functions
 *****************************************************************************
 * Copyright (C) 2010-2014 x264 project
 *
 * Authors: Diogo Franco <diogomfranco@gmail.com>
 *          Steven Walters <kemuri9@gmail.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/

#include "filters.h"

#if !defined(IDE_COMPILE) || (defined(IDE_COMPILE) && (_MSC_VER >= 1400))
#define RETURN_IF_ERROR( cond, ... ) RETURN_IF_ERR( cond, "options", NULL, __VA_ARGS__ )
#endif

char **x264_split_string( char *string, char *sep, int limit )
{
    int sep_count = 0;
    char *tmp;
    char **split;
    int i;
    char *str;
    char *esc;
    char *tok;
	char *nexttok;

	if( !string )
        return NULL;
    tmp = string;
    while( ( tmp = ( tmp = strstr( tmp, sep ) ) ? tmp + strlen( sep ) : 0 ) )
        ++sep_count;
    if( sep_count == 0 )
    {
        char **ret;

		if( string[0] == '\0' )
            return calloc( 1, sizeof( char* ) );
        ret = calloc( 2, sizeof( char* ) );
        ret[0] = strdup( string );
        return ret;
    }

    split = calloc( ( limit > 0 ? limit : sep_count ) + 2, sizeof(char*) );
    i = 0;
    str = strdup( string );
    assert( str );
    esc = NULL;
    tok = str;
	nexttok = str;
    do
    {
        nexttok = strstr( nexttok, sep );
        if( nexttok )
            *nexttok++ = '\0';
        if( ( limit > 0 && i >= limit ) ||
            ( i > 0 && ( ( esc = strrchr( split[i-1], '\\' ) ) ? esc[1] == '\0' : 0 ) ) ) // Allow escaping
        {
            int j = i-1;
            if( esc )
                esc[0] = '\0';
            split[j] = realloc( split[j], strlen( split[j] ) + strlen( sep ) + strlen( tok ) + 1 );
            assert( split[j] );
            strcat( split[j], sep );
            strcat( split[j], tok );
            esc = NULL;
        }
        else
        {
            split[i++] = strdup( tok );
            assert( split[i-1] );
        }
        tok = nexttok;
    } while ( tok );
    free( str );
    assert( !split[i] );

    return split;
}

void x264_free_string_array( char **array )
{
	int i;

	if( !array )
        return;
    for( i = 0; array[i] != NULL; i++ )
        free( array[i] );
    free( array );
}

char **x264_split_options( const char *opt_str, const char *options[] )
{
    char *opt_str_dup;
    char **split;
    int split_count;
    int options_count;
    char **opts;
    char **arg;
    int opt;
	int found_named;
	int invalid;
	int i;

	if( !opt_str )
        return NULL;
    opt_str_dup = strdup( opt_str );
    split = x264_split_string( opt_str_dup, ",", 0 );
    free( opt_str_dup );
    split_count = 0;
    while( split[split_count] != NULL )
        ++split_count;

    options_count = 0;
    while( options[options_count] != NULL )
        ++options_count;

    opts = calloc( split_count * 2 + 2, sizeof( char * ) );
    arg = NULL;
    opt = 0;
	found_named = 0;
	invalid = 0;
    for( i = 0; split[i] != NULL; i++, invalid = 0 )
    {
        arg = x264_split_string( split[i], "=", 2 );
        if( arg == NULL )
        {
            if( found_named )
                invalid = 1;
            else
#if !defined(IDE_COMPILE) || (defined(IDE_COMPILE) && (_MSC_VER >= 1400))
				RETURN_IF_ERROR( i > options_count || options[i] == NULL, "Too many options given\n" )
#else
            if( i > options_count || options[i] == NULL ) {
				x264_cli_log( "options", 0, "Too many options given\n" );
				return NULL;
			}
#endif
		else
            {
                opts[opt++] = strdup( options[i] );
                opts[opt++] = strdup( "" );
            }
        }
        else if( arg[0] == NULL || arg[1] == NULL )
        {
            if( found_named )
                invalid = 1;
            else
#if !defined(IDE_COMPILE) || (defined(IDE_COMPILE) && (_MSC_VER >= 1400))
				RETURN_IF_ERROR( i > options_count || options[i] == NULL, "Too many options given\n" )
#else
            if( i > options_count || options[i] == NULL ) {
				x264_cli_log( "options", 0, "Too many options given\n" );
				return NULL;
			}
#endif
            else
            {
                opts[opt++] = strdup( options[i] );
                if( arg[0] )
                    opts[opt++] = strdup( arg[0] );
                else
                    opts[opt++] = strdup( "" );
            }
        }
        else
        {
            int j = 0;
            found_named = 1;
            while( options[j] != NULL && strcmp( arg[0], options[j] ) )
                ++j;
#if !defined(IDE_COMPILE) || (defined(IDE_COMPILE) && (_MSC_VER >= 1400))
            RETURN_IF_ERROR( options[j] == NULL, "Invalid option '%s'\n", arg[0] )
#else
            if( options[j] == NULL ) {
				x264_cli_log( "options", 0, "Invalid option '%s'\n", arg[0] );
				return NULL;
			}
#endif
            else
            {
                opts[opt++] = strdup( arg[0] );
                opts[opt++] = strdup( arg[1] );
            }
        }
#if !defined(IDE_COMPILE) || (defined(IDE_COMPILE) && (_MSC_VER >= 1400))
        RETURN_IF_ERROR( invalid, "Ordered option given after named\n" )
#else
            if( invalid ) {
				x264_cli_log( "options", 0, "Ordered option given after named\n" );
				return NULL;
			}
#endif
        x264_free_string_array( arg );
    }
    x264_free_string_array( split );
    return opts;
}

char *x264_get_option( const char *name, char **split_options )
{
    int last_i;
	int i;

	if( !split_options )
        return NULL;
    last_i = -1;
    for( i = 0; split_options[i] != NULL; i += 2 )
        if( !strcmp( split_options[i], name ) )
            last_i = i;
    if( last_i >= 0 )
        return split_options[last_i+1][0] ? split_options[last_i+1] : NULL;
    return NULL;
}

int x264_otob( char *str, int def )
{
   int ret = def;
   if( str )
       ret = !strcasecmp( str, "true" ) ||
             !strcmp( str, "1" ) ||
             !strcasecmp( str, "yes" );
   return ret;
}

double x264_otof( char *str, double def )
{
   double ret = def;
   if( str )
   {
       char *end;
       ret = strtod( str, &end );
       if( end == str || *end != '\0' )
           ret = def;
   }
   return ret;
}

int x264_otoi( char *str, int def )
{
    int ret = def;
    if( str )
    {
        char *end;
        ret = strtol( str, &end, 0 );
        if( end == str || *end != '\0' )
            ret = def;
    }
    return ret;
}

char *x264_otos( char *str, char *def )
{
    return str ? str : def;
}
