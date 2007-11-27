/* /local/src/master/nrn/src/memacs/ebind.h,v 1.3 1999/10/15 10:54:02 hines Exp */
/*
ebind.h,v
 * Revision 1.3  1999/10/15  10:54:02  hines
 * NEOSIM simple interface for testing
 *
 * Revision 1.2  1999/10/12  14:21:13  hines
 * bind delgmod to META|CTRL|J since ^M often translated to newline prior
 * to getting to memacs
 *
 * Revision 1.1.1.1  1994/10/12  17:21:23  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.2  91/12/31  09:08:55  hines
 * ^J treated as newline instead of just linefeed. This allows text selections
 * from other windows to be inserted without extra accumulating space before
 * each line.
 * 
 * Revision 1.1  89/07/08  15:37:37  mlh
 * Initial revision
 * 
*/

/*	EBIND:		Initial default key to function bindings for
			MicroEMACS 3.2

			written by Dave G. Conroy
			modified by Steve Wilhite, George Jones
			greatly modified by Daniel Lawrence
*/

/*
 * Command table.
 * This table  is *roughly* in ASCII order, left to right across the
 * characters of the command. This expains the funny location of the
 * control-X commands.
 */
KEYTAB  keytab[NBINDS] = {
#if	V7
	{CTRL|'@',		setmark}, /*Might conflict with function keys*/
#endif
	{CTRL|'A',		gotobol},
	{CTRL|'B',		backchar},
		/* Is this a good idea? */
	{CTRL|'C',		quickexit}, /* write modified buffers */
	{CTRL|'D',		forwdel},
	{CTRL|'E',		gotoeol},
	{CTRL|'F',		forwchar},
	{CTRL|'G',		ctrlg},
	{CTRL|'H',		backdel},
	{CTRL|'I',		tab},
	{CTRL|'J',		newline},
	{CTRL|'K',		killtext},
	{CTRL|'L',		refresh},
	{CTRL|'M',		newline},
	{CTRL|'N',		forwline},
	{CTRL|'O',		openline},
	{CTRL|'P',		backline},
	{CTRL|'Q',		quote},
	{CTRL|'R',		backsearch},
	{CTRL|'S',		forwsearch},
	{CTRL|'T',		twiddle},
	{CTRL|'V',		forwpage},
	{CTRL|'W',		killregion},
	{CTRL|'Y',		yank},
	{CTRL|'Z',		mvdnwind},
	{CTRL|'_',		spawncli},
	{CTLX|CTRL|'B',		listbuffers},
	{CTLX|CTRL|'C',		quit},          /* Hard quit.           */
	{CTLX|CTRL|'F',		filefind},
	{CTLX|CTRL|'I',		insfile},
	{CTLX|CTRL|'L',		lowerregion},
	{CTLX|CTRL|'M',		delmode},
	{CTLX|CTRL|'J',		delmode},
	{CTLX|CTRL|'N',		mvdnwind}, /* Non-std (std: next error)*/
	{CTLX|CTRL|'O',		deblank},
	{CTLX|CTRL|'P',		mvupwind},
	{CTLX|CTRL|'R',		fileread},
	{CTLX|CTRL|'S',		filesave},
	{CTLX|CTRL|'U',		upperregion},
	{CTLX|CTRL|'V',		viewfile},
	{CTLX|CTRL|'W',		filewrite},
	{CTLX|CTRL|'X',		swapmark},
	{CTLX|CTRL|'Z',		shrinkwind},
	{CTLX|'?',		deskey},
	{CTLX|'!',		spawn},         /* Run 1 command.       */
	{CTLX|'=',		showcpos},
	{CTLX|'(',		ctlxlp},
	{CTLX|')',		ctlxrp},
	{CTLX|'^',		enlargewind},
	{CTLX|'1',		onlywind},
	{CTLX|'2',		splitwind},
	{CTLX|'B',		usebuffer},
	{CTLX|'C',		spawncli},      /* Run CLI in subjob.   */
#if	V7 & BSD
	{CTLX|'D',		bktoshell},	/* suspend emacs */
#endif
	{CTLX|'E',		ctlxe},
	{CTLX|'F',		setfillcol},
	{CTLX|'K',		killbuffer},
	{CTLX|'M',		setmode},
	{CTLX|'N',		nextwind},
	{CTLX|'O',		nextwind},
	{CTLX|'P',		prevwind},
	{CTLX|'S',		filesave},
	{CTLX|'X',		nextbuffer},
	{CTLX|'Z',		enlargewind},
	{META|CTRL|'H',		delbword},
	{META|CTRL|'K',		unbindkey},
	{META|CTRL|'L',		reposition},
	{META|CTRL|'M',		delgmode},
	{META|CTRL|'J',		delgmode},
	{META|CTRL|'N',		namebuffer},
	{META|CTRL|'R',		qreplace},
	{META|CTRL|'V',		scrnextdw},
	{META|CTRL|'W',		killpara}, /* Non-std (std: del region to
						end of internal buffer) */
	{META|CTRL|'Z',		scrnextup},
	{META|' ',		setmark},
	{META|'?',		help},
	{META|'!',		reposition},
/*	{META|'.',		setmark},     Non-std (std: end of wind) */
	{META|'>',		gotoeob},
	{META|'<',		gotobob},
	{META|'B',		backword},
	{META|'C',		capword},
	{META|'D',		delfword},
	{META|'F',		forwword},
	{META|'G',		gotoline},
	{META|'J',		fillpara},
	{META|'K',		bindtokey}, /*Non-std (std: kill2endof-sent)*/
	{META|'L',		lowerword},
	{META|'M',		setgmode},  /* Non-std (std: next non-blank)*/
	{META|'N',		gotoeop},
	{META|'P',		gotobop},
	{META|'Q',		qreplace},
	{META|'R',		sreplace},
#if	V7 & BSD
	{META|'S',		bktoshell},
#endif
	{META|'U',		upperword},
	{META|'V',		backpage},
	{META|'W',		copyregion},
	{META|'X',		namedcmd},
	{META|'Z',		mvupwind},
	{META|'[',		gotobop},
	{META|']',		gotoeop},
	{META|0x7F,              delbword},

#if	MSDOS & (HP150 == 0)
	{SPEC|CTRL|'_',		forwhunt},
	{SPEC|CTRL|'S',		backhunt},
	{SPEC|71,		gotobob},
	{SPEC|72,		backline},
	{SPEC|73,		backpage},
	{SPEC|75,		backchar},
	{SPEC|77,		forwchar},
	{SPEC|79,		gotoeob},
	{SPEC|80,		forwline},
	{SPEC|81,		forwpage},
	{SPEC|82,		insspace},
	{SPEC|83,		forwdel},
	{SPEC|115,		backword},
	{SPEC|116,		forwword},
	{SPEC|132,		gotobop},
	{SPEC|118,		gotoeop},
#endif

#if	HP150
	{SPEC|32,		backline},
	{SPEC|33,		forwline},
	{SPEC|35,		backchar},
	{SPEC|34,		forwchar},
	{SPEC|44,		gotobob},
	{SPEC|46,		forwpage},
	{SPEC|47,		backpage},
	{SPEC|82,		nextwind},
	{SPEC|68,		openline},
	{SPEC|69,		killtext},
	{SPEC|65,		forwdel},
	{SPEC|64,		ctlxe},
	{SPEC|67,		refresh},
	{SPEC|66,		reposition},
	{SPEC|83,		help},
	{SPEC|81,		deskey},
#endif

#if	AMIGA
	{SPEC|'?',		help},
	{SPEC|'A',		backline},
	{SPEC|'B',		forwline},
	{SPEC|'C',		forwchar},
	{SPEC|'D',		backchar},
	{SPEC|'T',		backpage},
	{SPEC|'S',		forwpage},
	{SPEC|'a',		backword},
	{SPEC|'`',		forwword},
#endif

	{0x7F,			backdel},
	{0,			NULL}
};

#if RAINBOW

#include "rainbow.h"

/*
 * Mapping table from the LK201 function keys to the internal EMACS character.
 */

int lk_map[][2] = {
	Up_Key,                         CTRL+'P',
	Down_Key,                       CTRL+'N',
	Left_Key,                       CTRL+'B',
	Right_Key,                      CTRL+'F',
	Shift+Left_Key,                 META+'B',
	Shift+Right_Key,                META+'F',
	Control+Left_Key,               CTRL+'A',
	Control+Right_Key,              CTRL+'E',
	Prev_Scr_Key,                   META+'V',
	Next_Scr_Key,                   CTRL+'V',
	Shift+Up_Key,                   META+'<',
	Shift+Down_Key,                 META+'>',
	Cancel_Key,                     CTRL+'G',
	Find_Key,                       CTRL+'S',
	Shift+Find_Key,                 CTRL+'R',
	Insert_Key,                     CTRL+'Y',
	Options_Key,                    CTRL+'D',
	Shift+Options_Key,              META+'D',
	Remove_Key,                     CTRL+'W',
	Shift+Remove_Key,               META+'W',
	Select_Key,                     CTRL+'@',
	Shift+Select_Key,               CTLX+CTRL+'X',
	Interrupt_Key,                  CTRL+'U',
	Keypad_PF2,                     META+'L',
	Keypad_PF3,                     META+'C',
	Keypad_PF4,                     META+'U',
	Shift+Keypad_PF2,               CTLX+CTRL+'L',
	Shift+Keypad_PF4,               CTLX+CTRL+'U',
	Keypad_1,                       CTLX+'1',
	Keypad_2,                       CTLX+'2',
	Do_Key,                         CTLX+'E',
	Keypad_4,                       CTLX+CTRL+'B',
	Keypad_5,                       CTLX+'B',
	Keypad_6,                       CTLX+'K',
	Resume_Key,                     META+'!',
	Control+Next_Scr_Key,           CTLX+'N',
	Control+Prev_Scr_Key,           CTLX+'P',
	Control+Up_Key,                 CTLX+CTRL+'P',
	Control+Down_Key,               CTLX+CTRL+'N',
	Help_Key,                       CTLX+'=',
	Shift+Do_Key,                   CTLX+'(',
	Control+Do_Key,                 CTLX+')',
	Keypad_0,                       CTLX+'Z',
	Shift+Keypad_0,                 CTLX+CTRL+'Z',
	Main_Scr_Key,                   CTRL+'C',
	Keypad_Enter,                   CTLX+'!',
	Exit_Key,                       CTLX+CTRL+'C',
	Shift+Exit_Key,                 CTRL+'Z'
};

#define lk_map_size     (sizeof(lk_map)/2)
#endif

