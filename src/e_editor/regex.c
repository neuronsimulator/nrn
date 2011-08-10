/* regex.c: regular expression interface routines for the ed line editor. */
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

#include <stddef.h>
#include <errno.h>
#include <sys/types.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ed.h"


static regex_t * global_pat = 0;
static bool patlock = false;	/* if set, pattern not freed by get_compiled_pattern */

static char * stbuf = 0;	/* substitution template buffer */
static int stbufsz = 0;		/* substitution template buffer size */
static int stlen = 0;		/* substitution template length */

static char * rbuf = 0;		/* replace_matching_text buffer */
static int rbufsz = 0;		/* replace_matching_text buffer size */


bool prev_pattern( void ) { return global_pat != 0; }


/* translate characters in a string */
static void translit_text( char * p, int len, const char from, const char to )
  {
  while( --len > 0 )
    {
    if( *p == from ) *p = to;
    ++p;
    }
  }


/* overwrite newlines with ASCII NULs */
static void newline_to_nul( char * const s, const int len )
  { translit_text( s, len, '\n', '\0' ); }

/* overwrite ASCII NULs with newlines */
static void nul_to_newline( char * const s, const int len )
  { translit_text( s, len, '\0', '\n' ); }


/* expand a POSIX character class */
static const char * parse_char_class( const char * p )
  {
  char c, d;

  if( *p == '^' ) ++p;
  if( *p == ']' ) ++p;
  for( ; *p != ']' && *p != '\n'; ++p )
    if( *p == '[' && ( ( d = p[1] ) == '.' || d == ':' || d == '=' ) )
      for( ++p, c = *++p; *p != ']' || c != d; ++p )
        if( ( c = *p ) == '\n' )
          return 0;
  return ( ( *p == ']' ) ? p : 0 );
  }


/* copy a pattern string from the command buffer; return pointer to the copy */
static char * extract_pattern( const char ** const ibufpp, const char delimiter )
  {
  static char * buf = 0;
  static int bufsz = 0;
  const char * nd = *ibufpp;
  int len;

  while( *nd != delimiter && *nd != '\n' )
    {
    if( *nd == '[' )
      {
      nd = parse_char_class( ++nd );
      if( !nd ) { set_error_msg( "Unbalanced brackets ([])" ); return 0; }
      }
    else if( *nd == '\\' && *++nd == '\n' )
      { set_error_msg( "Trailing backslash (\\)" ); return 0; }
    ++nd;
    }
  len = nd - *ibufpp;
  if( !resize_buffer( &buf, &bufsz, len + 1 ) ) return 0;
  memcpy( buf, *ibufpp, len );
  buf[len] = 0;
  *ibufpp = nd;
  if( isbinary() ) nul_to_newline( buf, len );
  return buf;
  }


/* return pointer to compiled pattern from command buffer */
static regex_t * get_compiled_pattern( const char ** const ibufpp )
  {
  static regex_t * exp = 0;
  const char * exps;
  const char delimiter = **ibufpp;
  int n;

  if( delimiter == ' ' )
    { set_error_msg( "Invalid pattern delimiter" ); return 0; }
  if( delimiter == '\n' || *++*ibufpp == '\n' || **ibufpp == delimiter )
    {
    if( !exp ) set_error_msg( "No previous pattern" );
    return exp;
    }
  exps = extract_pattern( ibufpp, delimiter );
  if( !exps ) return 0;
  /* buffer alloc'd && not reserved */
  if( exp && !patlock ) regfree( exp );
  else
    {
    exp = (regex_t *) malloc( sizeof (regex_t) );
    if( !exp )
      {
      show_strerror( 0, errno );
      set_error_msg( "Memory exhausted" );
      return 0;
      }
    }
  patlock = false;
  n = regcomp( exp, exps, 0 );
  if( n )
    {
    char buf[80];
    regerror( n, exp, buf, sizeof buf );
    set_error_msg( buf );
    free( exp );
    exp = 0;
    }
  return exp;
  }


/* add line matching a pattern to the global-active list */
bool build_active_list( const char ** const ibufpp, const int first_addr,
                        const int second_addr, const bool match )
  {
  const regex_t * pat;
  const line_t * lp;
  int addr;
  const char delimiter = **ibufpp;

  if( delimiter == ' ' || delimiter == '\n' )
    { set_error_msg( "Invalid pattern delimiter" ); return false; }
  pat = get_compiled_pattern( ibufpp );
  if( !pat ) return false;
  if( **ibufpp == delimiter ) ++*ibufpp;
  clear_active_list();
  lp = search_line_node( first_addr );
  for( addr = first_addr; addr <= second_addr; ++addr, lp = lp->q_forw )
    {
    char * const s = get_sbuf_line( lp );
    if( !s ) return false;
    if( isbinary() ) nul_to_newline( s, lp->len );
    if( !regexec( pat, s, 0, 0, 0 ) == match && !set_active_node( lp ) )
      return false;
    }
  return true;
  }


/* return pointer to copy of substitution template in the command buffer */
static char * extract_subst_template( const char ** const ibufpp,
                                      const bool isglobal )
  {
  int i = 0, n = 0;
  char c;
  const char delimiter = **ibufpp;

  ++*ibufpp;
  if( **ibufpp == '%' && (*ibufpp)[1] == delimiter )
    {
    ++*ibufpp;
    if( !stbuf ) set_error_msg( "No previous substitution" );
    return stbuf;
    }
  while( **ibufpp != delimiter )
    {
    if( !resize_buffer( &stbuf, &stbufsz, i + 2 ) ) return 0;
    c = stbuf[i++] = *(*ibufpp)++;
    if( c == '\n' && **ibufpp == 0 ) { --i, --*ibufpp; break; }
    if( c == '\\' && ( stbuf[i++] = *(*ibufpp)++ ) == '\n' && !isglobal )
      {
      while( ( *ibufpp = get_tty_line( &n ) ) &&
             ( n == 0 || ( n > 0 && (*ibufpp)[n-1] != '\n' ) ) )
        clearerr( stdin );
      if( !*ibufpp ) return 0;
      }
    }
  if( !resize_buffer( &stbuf, &stbufsz, i + 1 ) ) return 0;
  stbuf[stlen = i] = 0;
  return stbuf;
  }


/* extract substitution tail from the command buffer */
bool extract_subst_tail( const char ** const ibufpp, int * const gflagsp,
                         int * const snump, const bool isglobal )
  {
  const char delimiter = **ibufpp;

  *gflagsp = *snump = 0;
  if( delimiter == '\n' ) { stlen = 0; *gflagsp = GPR; return true; }
  if( !extract_subst_template( ibufpp, isglobal ) ) return false;
  if( **ibufpp == '\n' ) { *gflagsp = GPR; return true; }
  if( **ibufpp == delimiter ) ++*ibufpp;
  if( **ibufpp >= '1' && **ibufpp <= '9' )
    return parse_int( snump, *ibufpp, ibufpp );
  if( **ibufpp == 'g' ) { ++*ibufpp; *gflagsp = GSG; }
  return true;
  }


/* return the address of the next line matching a pattern in a given
   direction. wrap around begin/end of editor buffer if necessary */
int next_matching_node_addr( const char ** const ibufpp, const bool forward )
  {
  const regex_t * const pat = get_compiled_pattern( ibufpp );
  int addr = current_addr();

  if( !pat ) return -1;
  do {
    addr = ( forward ? inc_addr( addr ) : dec_addr( addr ) );
    if( addr )
      {
      const line_t * const lp = search_line_node( addr );
      char * const s = get_sbuf_line( lp );
      if( !s ) return -1;
      if( isbinary() ) nul_to_newline( s, lp->len );
      if( !regexec( pat, s, 0, 0, 0 ) ) return addr;
      }
    }
  while( addr != current_addr() );
  set_error_msg( "No match" );
  return -1;
  }


bool new_compiled_pattern( const char ** const ibufpp )
  {
  regex_t * tpat;

  disable_interrupts();
  tpat = get_compiled_pattern( ibufpp );
  if( tpat && tpat != global_pat )
    {
    if( global_pat ) { regfree( global_pat ); free( global_pat ); }
    global_pat = tpat;
    patlock = true;		/* reserve pattern */
    }
  enable_interrupts();
  return ( tpat ? true : false );
  }


/* modify text according to a substitution template; return offset to
   end of modified text */
static int apply_subst_template( const char * const boln,
                                 const regmatch_t * const rm, int offset,
                                 const int re_nsub )
  {
  const char * sub = stbuf;

  for( ; sub - stbuf < stlen; ++sub )
    {
    int n;
    if( *sub == '&' )
      {
      int j = rm[0].rm_so; int k = rm[0].rm_eo;
      if( !resize_buffer( &rbuf, &rbufsz, offset + k - j ) ) return -1;
      while( j < k ) rbuf[offset++] = boln[j++];
      }
    else if( *sub == '\\' && *++sub >= '1' && *sub <= '9' &&
             ( n = *sub - '0' ) <= re_nsub )
      {
      int j = rm[n].rm_so; int k = rm[n].rm_eo;
      if( !resize_buffer( &rbuf, &rbufsz, offset + k - j ) ) return -1;
      while( j < k ) rbuf[offset++] = boln[j++];
      }
    else
      {
      if( !resize_buffer( &rbuf, &rbufsz, offset + 1 ) ) return -1;
      rbuf[offset++] = *sub;
      }
    }
  if( !resize_buffer( &rbuf, &rbufsz, offset + 1 ) ) return -1;
  rbuf[offset] = 0;
  return offset;
  }


/* replace text matched by a pattern according to a substitution
   template; return length of the modified text */
static int replace_matching_text( const line_t * const lp, const int gflags,
                                  const int snum )
  {
  enum { se_max = 30 };	/* max subexpressions in a regular expression */
  regmatch_t rm[se_max];
  char * txt = get_sbuf_line( lp );
  const char * eot;
  int i = 0, offset = 0;
  bool changed = false;

  if( !txt ) return -1;
  if( isbinary() ) nul_to_newline( txt, lp->len );
  eot = txt + lp->len;
  if( !regexec( global_pat, txt, se_max, rm, 0 ) )
    {
    int matchno = 0;
    do {
      if( !snum || snum == ++matchno )
        {
        changed = true; i = rm[0].rm_so;
        if( !resize_buffer( &rbuf, &rbufsz, offset + i ) ) return -1;
        if( isbinary() ) newline_to_nul( txt, rm[0].rm_eo );
        memcpy( rbuf + offset, txt, i ); offset += i;
        offset = apply_subst_template( txt, rm, offset, global_pat->re_nsub );
        if( offset < 0 ) return -1;
        }
      else
        {
        i = rm[0].rm_eo;
        if( !resize_buffer( &rbuf, &rbufsz, offset + i ) ) return -1;
        if( isbinary() ) newline_to_nul( txt, i );
        memcpy( rbuf + offset, txt, i ); offset += i;
        }
      txt += rm[0].rm_eo;
      }
    while( *txt && ( !changed || ( ( gflags & GSG ) && rm[0].rm_eo ) ) &&
           !regexec( global_pat, txt, se_max, rm, REG_NOTBOL ) );
    i = eot - txt;
    if( !resize_buffer( &rbuf, &rbufsz, offset + i + 2 ) ) return -1;
    if( i > 0 && !rm[0].rm_eo && ( gflags & GSG ) )
      { set_error_msg( "Infinite substitution loop" ); return -1; }
    if( isbinary() ) newline_to_nul( txt, i );
    memcpy( rbuf + offset, txt, i );
    memcpy( rbuf + offset + i, "\n", 2 );
    }
  return ( changed ? offset + i + 1 : 0 );
  }


/* for each line in a range, change text matching a pattern according to
   a substitution template; return false if error */
bool search_and_replace( const int first_addr, const int second_addr,
                         const int gflags, const int snum, const bool isglobal )
  {
  int lc;
  bool match_found = false;

  set_current_addr( first_addr - 1 );
  for( lc = 0; lc <= second_addr - first_addr; ++lc )
    {
    const line_t * const lp = search_line_node( inc_current_addr() );
    int len = replace_matching_text( lp, gflags, snum );
    if( len < 0 ) return false;
    if( len )
      {
      const char * txt = rbuf;
      const char * const eot = rbuf + len;
      undo_t * up = 0;
      disable_interrupts();
      if( !delete_lines( current_addr(), current_addr(), isglobal ) )
        { enable_interrupts(); return false; }
      do {
        txt = put_sbuf_line( txt, current_addr() );
        if( !txt ) { enable_interrupts(); return false; }
        if( up ) up->tail = search_line_node( current_addr() );
        else
          {
          up = push_undo_atom( UADD, current_addr(), current_addr() );
          if( !up ) { enable_interrupts(); return false; }
          }
        }
      while( txt != eot );
      enable_interrupts();
      match_found = true;
      }
    }
  if( !match_found && !( gflags & GLB ) )
    { set_error_msg( "No match" ); return false; }
  return true;
  }
