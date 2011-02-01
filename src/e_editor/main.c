/*  GNU ed - The GNU line editor.
    Copyright (C) 1993, 1994, 2006, 2007, 2008, 2009, 2010
    Free Software Foundation, Inc.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
    Return values: 0 for a normal exit, 1 for environmental problems
    (file not found, invalid flags, I/O errors, etc), 2 to indicate a
    corrupt or invalid input file, 3 for an internal consistency error
    (eg, bug) which caused ed to panic.
*/
/*
 * CREDITS
 *
 *      This program is based on the editor algorithm described in
 *      Brian W. Kernighan and P. J. Plauger's book "Software Tools
 *      in Pascal," Addison-Wesley, 1981.
 *
 *      The buffering algorithm is attributed to Rodney Ruddock of
 *      the University of Guelph, Guelph, Ontario.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <locale.h>

#include "carg_parser.h"
#include "ed.h"


static const char * invocation_name = 0;
static const char * const Program_name = "GNU Ed";
static const char * const program_name = "ed";
static const char * const program_year = "2010";

static bool restricted_ = false;	/* if set, run in restricted mode */
static bool scripted_ = false;		/* if set, suppress diagnostics */
static bool traditional_ = false;	/* if set, be backwards compatible */


bool restricted( void ) { return restricted_; }
bool scripted( void ) { return scripted_; }
bool traditional( void ) { return traditional_; }


static void show_help( void )
  {
  printf( "%s - The GNU line editor.\n", Program_name );
  printf( "\nUsage: %s [options] [file]\n", invocation_name );
  printf( "\nOptions:\n" );
  printf( "  -h, --help                 display this help and exit\n" );
  printf( "  -V, --version              output version information and exit\n" );
  printf( "  -G, --traditional          run in compatibility mode\n" );
  printf( "  -l, --loose-exit-status    exit with 0 status even if a command fails\n" );
  printf( "  -p, --prompt=STRING        use STRING as an interactive prompt\n" );
  printf( "  -r, --restricted           run in restricted mode\n" );
  printf( "  -s, --quiet, --silent      suppress diagnostics\n" );
  printf( "  -v, --verbose              be verbose\n" );
  printf( "Start edit by reading in `file' if given.\n" );
  printf( "If `file' begins with a `!', read output of shell command.\n" );
  printf( "\nReport bugs to <bug-ed@gnu.org>.\n" );
  printf( "Ed home page: http://www.gnu.org/software/ed/ed.html\n" );
  printf( "General help using GNU software: http://www.gnu.org/gethelp\n" );
  }


static void show_version( void )
  {
  printf( "%s %s\n", Program_name, PROGVERSION );
  printf( "Copyright (C) %s Free Software Foundation, Inc.\n", program_year );
  printf( "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n" );
  printf( "This is free software: you are free to change and redistribute it.\n" );
  printf( "There is NO WARRANTY, to the extent permitted by law.\n" );
  }


void show_strerror( const char * const filename, const int errcode )
  {
  if( !scripted_ )
    {
    if( filename && filename[0] != 0 )
      fprintf( stderr, "%s: ", filename );
    fprintf( stderr, "%s\n", strerror( errcode ) );
    }
  }


static void show_error( const char * const msg, const int errcode, const bool help )
  {
  if( msg && msg[0] )
    {
    fprintf( stderr, "%s: %s", program_name, msg );
    if( errcode > 0 ) fprintf( stderr, ": %s", strerror( errcode ) );
    fprintf( stderr, "\n" );
    }
  if( help && invocation_name && invocation_name[0] )
    fprintf( stderr, "Try `%s --help' for more information.\n", invocation_name );
  }


/* return true if file descriptor is a regular file */
bool is_regular_file( const int fd )
  {
  struct stat st;
  return ( fstat( fd, &st ) < 0 || S_ISREG( st.st_mode ) );
  }


bool may_access_filename( const char * const name )
  {
  if( restricted_ &&
      ( *name == '!' || !strcmp( name, ".." ) || strchr( name, '/' ) ) )
    {
    set_error_msg( "Shell access restricted" );
    return false;
    }
  return true;
  }


int main( const int argc, const char * const argv[] )
  {
  int argind;
  bool loose = false;
  const struct ap_Option options[] =
    {
    { 'G', "traditional",       ap_no  },
    { 'h', "help",              ap_no  },
    { 'l', "loose-exit-status", ap_no  },
    { 'p', "prompt",            ap_yes },
    { 'r', "restricted",        ap_no  },
    { 's', "quiet",             ap_no  },
    { 's', "silent",            ap_no  },
    { 'v', "verbose",           ap_no  },
    { 'V', "version",           ap_no  },
    {  0 ,  0,                  ap_no } };

  struct Arg_parser parser;

  if( !ap_init( &parser, argc, argv, options, 0 ) )
    { show_error( "Memory exhausted.", 0, false ); return 1; }
  if( ap_error( &parser ) )				/* bad option */
    { show_error( ap_error( &parser ), 0, true ); return 1; }
  invocation_name = argv[0];

  for( argind = 0; argind < ap_arguments( &parser ); ++argind )
    {
    const int code = ap_code( &parser, argind );
    const char * const arg = ap_argument( &parser, argind );
    if( !code ) break;					/* no more options */
    switch( code )
      {
      case 'G': traditional_ = true; break;	/* backward compatibility */
      case 'h': show_help(); return 0;
      case 'l': loose = true; break;
      case 'p': set_prompt( arg ); break;
      case 'r': restricted_ = true; break;
      case 's': scripted_ = true; break;
      case 'v': set_verbose(); break;
      case 'V': show_version(); return 0;
      default : show_error( "internal error: uncaught option.", 0, false );
                return 3;
      }
    }
  setlocale( LC_ALL, "" );
  if( !init_buffers() ) return 1;

  while( argind < ap_arguments( &parser ) )
    {
    const char * const arg = ap_argument( &parser, argind );
    if( !strcmp( arg, "-" ) ) { scripted_ = true; ++argind; continue; }
    if( may_access_filename( arg ) )
      {
      if( read_file( arg, 0 ) < 0 && is_regular_file( 0 ) )
        return 2;
      else if( arg[0] != '!' ) set_def_filename( arg );
      }
    else
      {
      fputs( "?\n", stderr );
      if( arg[0] ) set_error_msg( "Invalid filename" );
      if( is_regular_file( 0 ) ) return 2;
      }
    break;
    }
  ap_free( &parser );

  return main_loop( loose );
  }
