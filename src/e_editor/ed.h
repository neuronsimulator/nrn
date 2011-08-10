/*  Global declarations for the ed editor.  */
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

#ifndef __cplusplus
enum Bool { false = 0, true = 1 };
typedef enum Bool bool;
#endif

enum Gflags
  {
  GLB = 0x01,			/* global command */
  GLS = 0x02,			/* list after command */
  GNP = 0x04,			/* enumerate after command */
  GPR = 0x08,			/* print after command */
  GSG = 0x10			/* global substitute */
  };


typedef struct line		/* Line node */
  {
  struct line * q_forw;
  struct line * q_back;
  long pos;			/* position of text in scratch buffer */
  int len;			/* length of line */
  }
line_t;


typedef struct
  {
  enum { UADD = 0, UDEL = 1, UMOV = 2, VMOV = 3 } type;
  line_t * head;			/* head of list */
  line_t * tail;			/* tail of list */
  }
undo_t;

#ifndef max
#define max( a,b ) ( (( a ) > ( b )) ? ( a ) : ( b ) )
#endif
#ifndef min
#define min( a,b ) ( (( a ) < ( b )) ? ( a ) : ( b ) )
#endif


/* defined in buffer.c */
bool append_lines( const char ** const ibufpp, const int addr,
                   const bool isglobal );
bool close_sbuf( void );
bool copy_lines( const int first_addr, const int second_addr, const int addr );
int current_addr( void );
int dec_addr( int addr );
bool delete_lines( const int from, const int to, const bool isglobal );
int get_line_node_addr( const line_t * const lp );
char * get_sbuf_line( const line_t * const lp );
int inc_addr( int addr );
int inc_current_addr( void );
bool init_buffers( void );
bool isbinary( void );
bool join_lines( const int from, const int to, const bool isglobal );
int last_addr( void );
bool modified( void );
bool move_lines( const int first_addr, const int second_addr, const int addr,
                 const bool isglobal );
bool newline_added( void );
bool open_sbuf( void );
int path_max( const char * filename );
bool put_lines( const int addr );
const char * put_sbuf_line( const char * const s, const int addr );
line_t * search_line_node( const int addr );
void set_binary( void );
void set_current_addr( const int addr );
void set_modified( const bool m );
void set_newline_added( void );
bool yank_lines( const int from, const int to );
void clear_undo_stack( void );
undo_t * push_undo_atom( const int type, const int from, const int to );
void reset_undo_state( void );
bool undo( const bool isglobal );

/* defined in global.c */
void clear_active_list( void );
const line_t * next_active_node( void );
bool set_active_node( const line_t * const lp );
void unset_active_nodes( const line_t * bp, const line_t * const ep );

/* defined in io.c */
bool display_lines( int from, const int to, const int gflags );
bool get_extended_line( const char ** const ibufpp, int * const lenp,
                        const bool strip_escaped_newlines );
const char * get_tty_line( int * const lenp );
int read_file( const char * const filename, const int addr );
int write_file( const char * const filename, const char * const mode,
                const int from, const int to );

/* defined in main.c */
bool is_regular_file( const int fd );
bool may_access_filename( const char * const name );
bool restricted( void );
bool scripted( void );
void show_strerror( const char * const filename, const int errcode );
bool traditional( void );

/* defined in main_loop.c */
int main_loop( const bool loose );
void set_def_filename( const char * const s );
void set_error_msg( const char * msg );
void set_prompt( const char * const s );
void set_verbose( void );
void unmark_line_node( const line_t * const lp );

/* defined in regex.c */
bool build_active_list( const char ** const ibufpp, const int first_addr,
                        const int second_addr, const bool match );
bool extract_subst_tail( const char ** const ibufpp, int * const gflagsp,
                         int * const snump, const bool isglobal );
int next_matching_node_addr( const char ** const ibufpp, const bool forward );
bool new_compiled_pattern( const char ** const ibufpp );
bool prev_pattern( void );
bool search_and_replace( const int first_addr, const int second_addr,
                         const int gflags, const int snum, const bool isglobal );

/* defined in signal.c */
void disable_interrupts( void );
void enable_interrupts( void );
bool parse_int( int * const i, const char * const str, const char ** const tail );
bool resize_buffer( char ** const buf, int * const size, const int min_size );
bool resize_line_buffer( const line_t *** const buf, int * const size,
                         const int min_size );
bool resize_undo_buffer( undo_t ** const buf, int * const size,
                         const int min_size );
void set_signals( void );
void set_window_lines( const int lines );
const char * strip_escapes( const char * p );
int window_columns( void );
int window_lines( void );
