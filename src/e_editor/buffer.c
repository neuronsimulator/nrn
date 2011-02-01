/* buffer.c: scratch-file buffer routines for the ed line editor. */
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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>

#include "ed.h"


static int current_addr_ = 0;	/* current address in editor buffer */
static int last_addr_ = 0;	/* last address in editor buffer */
static bool isbinary_ = false;	/* if set, buffer contains ASCII NULs */
static bool modified_ = false;	/* if set, buffer modified since last write */
static bool newline_added_ = false; /* if set, newline appended to input file */

static bool seek_write = false;	/* seek before writing */
static FILE * sfp = 0;		/* scratch file pointer */
static long sfpos = 0;		/* scratch file position */
static line_t buffer_head;	/* editor buffer ( linked list of line_t )*/
static line_t yank_buffer_head;


int current_addr( void ) { return current_addr_; }
int inc_current_addr( void )
  { if( ++current_addr_ > last_addr_ ) current_addr_ = last_addr_;
    return current_addr_; }
void set_current_addr( const int addr ) { current_addr_ = addr; }

int last_addr( void ) { return last_addr_; }

bool isbinary( void ) { return isbinary_; }
void set_binary( void ) { isbinary_ = true; }

bool modified( void ) { return modified_; }
void set_modified( const bool m ) { modified_ = m; }

bool newline_added( void ) { return newline_added_; }
void set_newline_added( void ) { newline_added_ = true; }


int inc_addr( int addr )
  { if( ++addr > last_addr_ ) addr = 0; return addr; }

int dec_addr( int addr )
  { if( --addr < 0 ) addr = last_addr_; return addr; }


/* link next and previous nodes */
static void link_nodes( line_t * const prev, line_t * const next )
  { prev->q_forw = next; next->q_back = prev; }


/* insert line node into circular queue after previous */
static void insert_node( line_t * const lp, line_t * const prev )
  {
  link_nodes( lp, prev->q_forw );
  link_nodes( prev, lp );
  }


/* add a line node in the editor buffer after the given line */
static void add_line_node( line_t * const lp, const int addr )
  {
  line_t * const prev = search_line_node( addr );
  insert_node( lp, prev );
  ++last_addr_;
  }


/* return a pointer to a copy of a line node, or to a new node if lp == 0 */
static line_t * dup_line_node( line_t * const lp )
  {
  line_t * const p = (line_t *) malloc( sizeof (line_t) );
  if( !p )
    {
    show_strerror( 0, errno );
    set_error_msg( "Memory exhausted" );
    return 0;
    }
  if( lp ) { p->pos = lp->pos; p->len = lp->len; }
  return p;
  }


/* Insert text from stdin (or from command buffer if global) to after
   line n; stop when either a single period is read or EOF.
   Return false if insertion fails. */
bool append_lines( const char ** const ibufpp, const int addr,
                   const bool isglobal )
  {
  int len;
  undo_t * up = 0;
  current_addr_ = addr;

  while( true )
    {
    if( !isglobal )
      {
      *ibufpp = get_tty_line( &len );
      if( !*ibufpp ) return false;
      if( !len || (*ibufpp)[len-1] != '\n' )
        { clearerr( stdin ); return !len; }
      }
    else
      {
      if( !**ibufpp ) return true;
      for( len = 0; (*ibufpp)[len++] != '\n'; ) ;
      }
    if( len == 2 && **ibufpp == '.' ) { *ibufpp += len; return true; }
    disable_interrupts();
    if( !put_sbuf_line( *ibufpp, current_addr_ ) )
      { enable_interrupts(); return false; }
    if( up ) up->tail = search_line_node( current_addr_ );
    else
      {
      up = push_undo_atom( UADD, current_addr_, current_addr_ );
      if( !up ) { enable_interrupts(); return false; }
      }
    *ibufpp += len;
    modified_ = true;
    enable_interrupts();
    }
  }


static void clear_yank_buffer( void )
  {
  line_t * lp = yank_buffer_head.q_forw;

  disable_interrupts();
  while( lp != &yank_buffer_head )
    {
    line_t * const p = lp->q_forw;
    link_nodes( lp->q_back, lp->q_forw );
    free( lp );
    lp = p;
    }
  enable_interrupts();
  }


/* close scratch file */
bool close_sbuf( void )
  {
  clear_yank_buffer();
  clear_undo_stack();
  if( sfp )
    {
    if( fclose( sfp ) < 0 )
      {
      show_strerror( 0, errno );
      set_error_msg( "Cannot close temp file" );
      return false;
      }
    sfp = 0;
    }
  sfpos = 0;
  seek_write = false;
  return true;
  }


/* copy a range of lines; return false if error */
bool copy_lines( const int first_addr, const int second_addr, const int addr )
  {
  line_t *lp, *np = search_line_node( first_addr );
  undo_t * up = 0;
  int n = second_addr - first_addr + 1;
  int m = 0;

  current_addr_ = addr;
  if( addr >= first_addr && addr < second_addr )
    {
    n = addr - first_addr + 1;
    m = second_addr - addr;
    }
  for( ; n > 0; n = m, m = 0, np = search_line_node( current_addr_ + 1 ) )
    for( ; n-- > 0; np = np->q_forw )
      {
      disable_interrupts();
      lp = dup_line_node( np );
      if( !lp ) { enable_interrupts(); return false; }
      add_line_node( lp, current_addr_++ );
      if( up ) up->tail = lp;
      else
        {
        up = push_undo_atom( UADD, current_addr_, current_addr_ );
        if( !up ) { enable_interrupts(); return false; }
        }
      modified_ = true;
      enable_interrupts();
      }
  return true;
  }


/* delete a range of lines */
bool delete_lines( const int from, const int to, const bool isglobal )
  {
  line_t *n, *p;

  if( !yank_lines( from, to ) ) return false;
  disable_interrupts();
  if( !push_undo_atom( UDEL, from, to ) )
    { enable_interrupts(); return false; }
  n = search_line_node( inc_addr( to ) );
  p = search_line_node( from - 1 );	/* this search_line_node last! */
  if( isglobal ) unset_active_nodes( p->q_forw, n );
  link_nodes( p, n );
  last_addr_ -= to - from + 1;
  current_addr_ = from - 1;
  modified_ = true;
  enable_interrupts();
  return true;
  }


/* return line number of pointer */
int get_line_node_addr( const line_t * const lp )
  {
  const line_t * p = &buffer_head;
  int addr = 0;

  while( p != lp && ( p = p->q_forw ) != &buffer_head ) ++addr;
  if( addr && p == &buffer_head )
    { set_error_msg( "Invalid address" ); return -1; }
  return addr;
  }


/* get a line of text from the scratch file; return pointer to the text */
char * get_sbuf_line( const line_t * const lp )
  {
  static char * buf = 0;
  static int bufsz = 0;
  int len, rd;

  if( lp == &buffer_head ) return 0;
  seek_write = true;			/* force seek on write */
  /* out of position */
  if( sfpos != lp->pos )
    {
    sfpos = lp->pos;
    if( fseek( sfp, sfpos, SEEK_SET ) < 0 )
      {
      show_strerror( 0, errno );
      set_error_msg( "Cannot seek temp file" );
      return 0;
      }
    }
  len = lp->len;
  if( !resize_buffer( &buf, &bufsz, len + 1 ) ) return 0;
  rd = fread( buf, 1, len, sfp );
  if( rd < 0 || rd != len )
    {
    show_strerror( 0, errno );
    set_error_msg( "Cannot read temp file" );
    return 0;
    }
  sfpos += len;		/* update file position */
  buf[len] = 0;
  return buf;
  }


/* open scratch buffer; initialize line queue */
bool init_buffers( void )
  {
  /* Read stdin one character at a time to avoid i/o contention
     with shell escapes invoked by nonterminal input, e.g.,
     ed - <<EOF
     !cat
     hello, world
     EOF */
  setvbuf( stdin, 0, _IONBF, 0 );
  if( !open_sbuf() ) return false;
  link_nodes( &buffer_head, &buffer_head );
  link_nodes( &yank_buffer_head, &yank_buffer_head );
  return true;
  }


/* replace a range of lines with the joined text of those lines */
bool join_lines( const int from, const int to, const bool isglobal )
  {
  static char * buf = 0;
  static int bufsz = 0;
  int size = 0;
  line_t * const ep = search_line_node( inc_addr( to ) );
  line_t * bp = search_line_node( from );

  while( bp != ep )
    {
    const char * const s = get_sbuf_line( bp );
    if( !s || !resize_buffer( &buf, &bufsz, size + bp->len ) ) return false;
    memcpy( buf + size, s, bp->len );
    size += bp->len;
    bp = bp->q_forw;
    }
  if( !resize_buffer( &buf, &bufsz, size + 2 ) ) return false;
  memcpy( buf + size, "\n", 2 );
  if( !delete_lines( from, to, isglobal ) ) return false;
  current_addr_ = from - 1;
  disable_interrupts();
  if( !put_sbuf_line( buf, current_addr_ ) ||
      !push_undo_atom( UADD, current_addr_, current_addr_ ) )
    { enable_interrupts(); return false; }
  modified_ = true;
  enable_interrupts();
  return true;
  }


/* move a range of lines */
bool move_lines( const int first_addr, const int second_addr, const int addr,
                 const bool isglobal )
  {
  line_t *b1, *a1, *b2, *a2;
  int n = inc_addr( second_addr );
  int p = first_addr - 1;

  disable_interrupts();
  if( addr == first_addr - 1 || addr == second_addr )
    {
    a2 = search_line_node( n );
    b2 = search_line_node( p );
    current_addr_ = second_addr;
    }
  else if( !push_undo_atom( UMOV, p, n ) ||
           !push_undo_atom( UMOV, addr, inc_addr( addr ) ) )
    { enable_interrupts(); return false; }
  else
    {
    a1 = search_line_node( n );
    if( addr < first_addr )
      {
      b1 = search_line_node( p );
      b2 = search_line_node( addr );	/* this search_line_node last! */
      }
    else
      {
      b2 = search_line_node( addr );
      b1 = search_line_node( p );	/* this search_line_node last! */
      }
    a2 = b2->q_forw;
    link_nodes( b2, b1->q_forw );
    link_nodes( a1->q_back, a2 );
    link_nodes( b1, a1 );
    current_addr_ = addr + ( ( addr < first_addr ) ?
                           second_addr - first_addr + 1 : 0 );
    }
  if( isglobal ) unset_active_nodes( b2->q_forw, a2 );
  modified_ = true;
  enable_interrupts();
  return true;
  }


/* open scratch file */
bool open_sbuf( void )
  {
  isbinary_ = newline_added_ = false;
  sfp = tmpfile();
  if( !sfp )
    {
    show_strerror( 0, errno );
    set_error_msg( "Cannot open temp file" );
    return false;
    }
  return true;
  }


int path_max( const char * filename )
  {
  long result;
  if( !filename ) filename = "/";
  errno = 0;
  result = pathconf( filename, _PC_PATH_MAX );
  if( result < 0 ) { if( errno ) result = 256; else result = 1024; }
  else if( result < 256 ) result = 256;
  return result;
  }


/* append lines from the yank buffer */
bool put_lines( const int addr )
  {
  undo_t * up = 0;
  line_t *p, *lp = yank_buffer_head.q_forw;

  if( lp == &yank_buffer_head )
    { set_error_msg( "Nothing to put" ); return false; }
  current_addr_ = addr;
  while( lp != &yank_buffer_head )
    {
    disable_interrupts();
    p = dup_line_node( lp );
    if( !p ) { enable_interrupts(); return false; }
    add_line_node( p, current_addr_++ );
    if( up ) up->tail = p;
    else
      {
      up = push_undo_atom( UADD, current_addr_, current_addr_ );
      if( !up ) { enable_interrupts(); return false; }
      }
    modified_ = true;
    lp = lp->q_forw;
    enable_interrupts();
    }
  return true;
  }


/* write a line of text to the scratch file and add a line node to the
   editor buffer; return a pointer to the end of the text */
const char * put_sbuf_line( const char * const s, const int addr )
  {
  line_t * const lp = dup_line_node( 0 );
  const char * const p = (const char *) memchr( s, '\n', INT_MAX - 1 );
  int len, wr;

  if( !lp ) return 0;
  if( !p ) { set_error_msg( "Line too long" ); return 0; }
  len = p - s;
  /* out of position */
  if( seek_write )
    {
    if( fseek( sfp, 0L, SEEK_END ) < 0 )
      {
      show_strerror( 0, errno );
      set_error_msg( "Cannot seek temp file" );
      return 0;
      }
    sfpos = ftell( sfp );
    seek_write = false;
    }
  wr = fwrite( s, 1, len, sfp );	/* assert: interrupts disabled */
  if( wr < 0 || wr != len )
    {
    sfpos = -1;
    show_strerror( 0, errno );
    set_error_msg( "Cannot write temp file" );
    return 0;
    }
  lp->pos = sfpos; lp->len = len;
  add_line_node( lp, addr );
  ++current_addr_;
  sfpos += len;				/* update file position */
  return p + 1;
  }


/* return pointer to a line node in the editor buffer */
line_t * search_line_node( const int addr )
  {
  static line_t * lp = &buffer_head;
  static int o_addr = 0;

  disable_interrupts();
  if( o_addr < addr )
    {
    if( o_addr + last_addr_ >= 2 * addr )
      while( o_addr < addr ) { ++o_addr; lp = lp->q_forw; }
    else
      {
      lp = buffer_head.q_back; o_addr = last_addr_;
      while( o_addr > addr ) { --o_addr; lp = lp->q_back; }
      }
    }
  else if( o_addr <= 2 * addr )
         while( o_addr > addr ) { --o_addr; lp = lp->q_back; }
  else
    { lp = &buffer_head; o_addr = 0;
      while( o_addr < addr ) { ++o_addr; lp = lp->q_forw; } }
  enable_interrupts();
  return lp;
  }


/* copy a range of lines to the cut buffer */
bool yank_lines( const int from, const int to )
  {
  line_t * const ep = search_line_node( inc_addr( to ) );
  line_t * bp = search_line_node( from );
  line_t * lp = &yank_buffer_head;
  line_t * p;

  clear_yank_buffer();
  while( bp != ep )
    {
    disable_interrupts();
    p = dup_line_node( bp );
    if( !p ) { enable_interrupts(); return false; }
    insert_node( p, lp );
    bp = bp->q_forw; lp = p;
    enable_interrupts();
    }
  return true;
  }


static undo_t * ustack = 0;		/* undo stack */
static int usize = 0;			/* ustack size (in bytes) */
static int u_ptr = 0;			/* undo stack pointer */
static int u_current_addr = -1;		/* if < 0, undo disabled */
static int u_last_addr = -1;		/* if < 0, undo disabled */
static bool u_modified = false;


void clear_undo_stack( void )
  {
  while( u_ptr-- )
    if( ustack[u_ptr].type == UDEL )
      {
      line_t * const ep = ustack[u_ptr].tail->q_forw;
      line_t * bp = ustack[u_ptr].head;
      while( bp != ep )
        {
        line_t * const lp = bp->q_forw;
        unmark_line_node( bp );
        free( bp );
        bp = lp;
        }
      }
  u_ptr = 0;
  u_current_addr = current_addr_;
  u_last_addr = last_addr_;
  u_modified = modified_;
  }


void reset_undo_state( void )
  {
  clear_undo_stack();
  u_current_addr = u_last_addr = -1;
  u_modified = false;
  }


/* return pointer to intialized undo node */
undo_t * push_undo_atom( const int type, const int from, const int to )
  {
  disable_interrupts();
  if( !resize_undo_buffer( &ustack, &usize, ( u_ptr + 1 ) * sizeof (undo_t) ) )
    {
    show_strerror( 0, errno );
    set_error_msg( "Memory exhausted" );
    if( ustack )
      {
      clear_undo_stack();
      free( ustack );
      ustack = 0;
      usize = u_ptr = 0;
      u_current_addr = u_last_addr = -1;
      }
    enable_interrupts();
    return 0;
    }
  enable_interrupts();
  ustack[u_ptr].type = type;
  ustack[u_ptr].tail = search_line_node( to );
  ustack[u_ptr].head = search_line_node( from );
  return ustack + u_ptr++;
  }


/* undo last change to the editor buffer */
bool undo( const bool isglobal )
  {
  int n;
  const int o_current_addr = current_addr_;
  const int o_last_addr = last_addr_;
  const bool o_modified = modified_;

  if( u_ptr <= 0 || u_current_addr < 0 || u_last_addr < 0 )
    { set_error_msg( "Nothing to undo" ); return false; }
  search_line_node( 0 );		/* reset cached value */
  disable_interrupts();
  for( n = u_ptr - 1; n >= 0; --n )
    {
    switch( ustack[n].type )
      {
      case UADD: link_nodes( ustack[n].head->q_back, ustack[n].tail->q_forw );
                 break;
      case UDEL: link_nodes( ustack[n].head->q_back, ustack[n].head );
                 link_nodes( ustack[n].tail, ustack[n].tail->q_forw );
                 break;
      case UMOV:
      case VMOV: link_nodes( ustack[n-1].head, ustack[n].head->q_forw );
                 link_nodes( ustack[n].tail->q_back, ustack[n-1].tail );
                 link_nodes( ustack[n].head, ustack[n].tail ); --n;
                 break;
      }
    ustack[n].type ^= 1;
    }
  /* reverse undo stack order */
  for( n = 0; 2 * n < u_ptr - 1; ++n )
    {
    undo_t tmp = ustack[n];
    ustack[n] = ustack[u_ptr-1-n]; ustack[u_ptr-1-n] = tmp;
    }
  if( isglobal ) clear_active_list();
  current_addr_ = u_current_addr; u_current_addr = o_current_addr;
  last_addr_ = u_last_addr; u_last_addr = o_last_addr;
  modified_ = u_modified; u_modified = o_modified;
  enable_interrupts();
  return true;
  }
