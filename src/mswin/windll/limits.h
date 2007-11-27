/* limits.h */
/* Produced by enquire version 4.3, CWI, Amsterdam */

   /* Number of bits in a storage unit */
#define CHAR_BIT 8
   /* Maximum char */
#define CHAR_MAX 127
   /* Minimum char */
#define CHAR_MIN (-128)
   /* Maximum signed char */
#define SCHAR_MAX 127
   /* Minimum signed char */
#define SCHAR_MIN (-128)
   /* Maximum unsigned char (minimum is always 0) */
#define UCHAR_MAX 255
   /* Maximum short */
#define SHRT_MAX 32767
   /* Minimum short */
#define SHRT_MIN (-32768)
   /* Maximum int */
#define INT_MAX 2147483647
   /* Minimum int */
#define INT_MIN (-2147483647-1)
   /* Maximum long */
#define LONG_MAX 2147483647L
   /* Minimum long */
#define LONG_MIN (-2147483647L-1L)
   /* Maximum unsigned short (minimum is always 0) */
#define USHRT_MAX 65535
   /* Maximum unsigned int (minimum is always 0) */
#define UINT_MAX 4294967295U
   /* Maximum unsigned long (minimum is always 0) */
#define ULONG_MAX 4294967295UL

/* All of these constants can be obtained from sysconf() instead */
#define	ARG_MAX			127	/* Max length of arg to exec() */
#define	CHILD_MAX		1	/* Max processes per user */
#define	CLK_TCK			100	/* Number of clock ticks per second */
#define	NGROUPS_MAX		0	/* Max group IDs per process */
#define	OPEN_MAX		50	/* Max open files per process */
#define	PASS_MAX		8	/* Max bytes in password */
#define	_POSIX_STREAM_MAX	8	/* Max open stdio FILEs */
#define	TZNAME_MAX		50	/* Max length of timezone name */
/* Not sure why the following should be here, but I guess they are... */
#define	BC_BASE_MAX		99	/* Largest ibase and obase for bc */
#define	BC_DIM_MAX		2048	/* Max array elements for bc */
#define	BC_SCALE_MAX		99	/* Max scale value for bc */
#define	COLL_ELEM_MAX		4	/* Max bytes in collation element */
#define	EXPR_NEST_MAX		32	/* Max nesting of (...) for expr */
#define	LINE_MAX		2048	/* Max length in bytes of input line */
#define	PASTE_FILES_MAX		12	/* Max file operands for paste */
#define	RE_DUP_MAX		255	/* Max regular expressions permitted */
#define	SED_PATTERN_MAX		20480	/* Max size in bytes of sed pattern */
#define	SENDTO_MAX		90000	/* Max bytes of message for sendto */
#define	SORT_LINE_MAX		20480	/* Max bytes of input line for sort */
/* We use a linked list, so no limit to atexit() funcs */
#define ATEXIT_MAX		INT_MAX	/* Max atexit() funcs */

/* All of these constants can be obtained from pathconf() instead */
#define	LINK_MAX		1	/* Max links to a single file */
#define	MAX_CANON		127	/* Max bytes in TTY canonical input */
#define	MAX_INPUT		127	/* Max bytes in TTY input queue */
#define	NAME_MAX		12	/* Max bytes in a filename */
#define	PATH_MAX		80	/* Max bytes in a pathname */
#define	PIPE_BUF		0	/* Max bytes for atomic pipe writes */ 
