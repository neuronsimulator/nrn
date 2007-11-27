/* /local/src/master/nrn/src/memacs/efunc.h,v 1.1.1.1 1994/10/12 17:21:23 hines Exp */
/*
efunc.h,v
 * Revision 1.1.1.1  1994/10/12  17:21:23  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.1  89/07/08  15:37:45  mlh
 * Initial revision
 * 
*/

/*	EFUNC.H:	MicroEMACS function declarations and names

		This file list all the C code functions used by MicroEMACS
	and the names to use to bind keys to them. To add functions,
	declare it here in both the extern function list and the name
	binding table.

	Update History:

	Daniel Lawrence
		29-jan-86
		- separeted out external declarations to a separate .h file
		- built original name to function binding table
		30-jan-86
		- added command declarations for Fill Paragraph command
		11-feb-86
		- added declaration for help and describe key commands
		13-feb-86
		- added declarations for view-file command
		15-feb-86
		- added declataitons for insert file command
		17-feb-86
		- added declarations for scroll next window up/down
		20-feb-86
		- expanded binding table to make room for new bindings
		24-feb-86
		- added declarations for bind-to-key and unbind-key
		  execute named command and describe bindings
		26-feb-86
		- added declarations for execute command
		- changed name of "visit-file" to "find-file"
		- added declaration for execute-buffer command
		27-feb-86
		- added declarations for execute-file command
		3-apr-86
		- added declarations for next-buffer command
		5-apr-86
		- added declarations for kill-paragraph command
		8-apr-86
		- added declarations for add/delete global mode
		8-apr-86
		- added declarations for insert space command
*/

/*	External function declarations		*/

extern  int     ctrlg();                /* Abort out of things          */
extern  int     quit();                 /* Quit                         */
extern  int     ctlxlp();               /* Begin macro                  */
extern  int     ctlxrp();               /* End macro                    */
extern  int     ctlxe();                /* Execute macro                */
extern  int     fileread();             /* Get a file, read only        */
extern  int     filefind();		/* Get a file, read write       */
extern  int     filewrite();            /* Write a file                 */
extern  int     filesave();             /* Save current file            */
extern  int     filename();             /* Adjust file name             */
extern  int     getccol();              /* Get current column           */
extern  int     gotobol();              /* Move to start of line        */
extern  int     forwchar();             /* Move forward by characters   */
extern  int     gotoeol();              /* Move to end of line          */
extern  int     backchar();             /* Move backward by characters  */
extern  int     forwline();             /* Move forward by lines        */
extern  int     backline();             /* Move backward by lines       */
extern  int     forwpage();             /* Move forward by pages        */
extern  int     backpage();             /* Move backward by pages       */
extern  int     gotobob();              /* Move to start of buffer      */
extern  int     gotoeob();              /* Move to end of buffer        */
extern  int     setfillcol();           /* Set fill column.             */
extern  int     setmark();              /* Set mark                     */
extern  int     swapmark();             /* Swap "." and mark            */
extern  int     forwsearch();           /* Search forward               */
extern  int     backsearch();           /* Search backwards             */
extern	int	sreplace();		/* search and replace		*/
extern	int	qreplace();		/* search and replace w/query	*/
extern  int     showcpos();             /* Show the cursor position     */
extern  int     nextwind();             /* Move to the next window      */
extern  int     prevwind();             /* Move to the previous window  */
extern  int     onlywind();             /* Make current window only one */
extern  int     splitwind();            /* Split current window         */
extern  int     mvdnwind();             /* Move window down             */
extern  int     mvupwind();             /* Move window up               */
extern  int     enlargewind();          /* Enlarge display window.      */
extern  int     shrinkwind();           /* Shrink window.               */
extern  int     listbuffers();          /* Display list of buffers      */
extern  int     usebuffer();            /* Switch a window to a buffer  */
extern  int     killbuffer();           /* Make a buffer go away.       */
extern  int     reposition();           /* Reposition window            */
extern  int     refresh();              /* Refresh the screen           */
extern  int     twiddle();              /* Twiddle characters           */
extern  int     tab();                  /* Insert tab                   */
extern  int     newline();              /* Insert CR-LF                 */
extern  int     indent();               /* Insert CR-LF, then indent    */
extern  int     openline();             /* Open up a blank line         */
extern  int     deblank();              /* Delete blank lines           */
extern  int     quote();                /* Insert literal               */
extern  int     backword();             /* Backup by words              */
extern  int     forwword();             /* Advance by words             */
extern  int     forwdel();              /* Forward delete               */
extern  int     backdel();              /* Backward delete              */
extern  int     killtext();             /* Kill forward                 */
extern  int     yank();                 /* Yank back from killbuffer.   */
extern  int     upperword();            /* Upper case word.             */
extern  int     lowerword();            /* Lower case word.             */
extern  int     upperregion();          /* Upper case region.           */
extern  int     lowerregion();          /* Lower case region.           */
extern  int     capword();              /* Initial capitalize word.     */
extern  int     delfword();             /* Delete forward word.         */
extern  int     delbword();             /* Delete backward word.        */
extern  int     killregion();           /* Kill region.                 */
extern  int     copyregion();           /* Copy region to kill buffer.  */
extern  int     spawncli();             /* Run CLI in a subjob.         */
extern  int     spawn();                /* Run a command in a subjob.   */
extern  int     quickexit();            /* low keystroke style exit.    */
extern	int	setmode();		/* set an editor mode		*/
extern	int	delmode();		/* delete a mode		*/
extern	int	gotoline();		/* go to a numbered line	*/
extern	int	namebuffer();		/* rename the current buffer	*/
extern	int	gotobop();		/* go to begining/paragraph	*/
extern	int	gotoeop();		/* go to end/paragraph		*/
extern	int	fillpara();		/* fill current paragraph	*/
extern	int	help();			/* get the help file here	*/
extern	int	deskey();		/* describe a key's binding	*/
extern	int	viewfile();		/* find a file in view mode	*/
extern	int	insfile();		/* insert a file		*/
extern	int	scrnextup();		/* scroll next window back	*/
extern	int	scrnextdw();		/* scroll next window down	*/
extern	int	bindtokey();		/* bind a function to a key	*/
extern	int	unbindkey();		/* unbind a key's function	*/
extern	int	namedcmd();		/* execute named command	*/
extern	int	desbind();		/* describe bindings		*/
extern	int	execcmd();		/* execute a command line	*/
extern	int	execbuf();		/* exec commands from a buffer	*/
extern	int	execfile();		/* exec commands from a file	*/
extern	int	nextbuffer();		/* switch to the next buffer	*/
extern	int	killpara();		/* kill the current paragraph	*/
extern	int	setgmode();		/* set a global mode		*/
extern	int	delgmode();		/* delete a global mode		*/
extern	int	insspace();		/* insert a space forword	*/
extern	int	forwhunt();		/* hunt forward for next match	*/
extern	int	backhunt();		/* hunt backwards for next match*/

#if	V7 & BSD
extern	int	bktoshell();		/* suspend emacs to parent shell*/
extern	int	rtfrmshell();		/* return from a suspended state*/
#endif

/*	Name to function binding table

		This table gives the names of all the bindable functions
	end their C function address. These are used for the bind-to-key
	function.
*/

NBIND	names[] = {
	{"add-mode",			setmode},
	{"add-global-mode",		setgmode},
	{"backward-character",		backchar},
	{"begin-macro",			ctlxlp},
	{"begining-of-file",		gotobob},
	{"begining-of-line",		gotobol},
	{"bind-to-key",			bindtokey},
	{"buffer-position",		showcpos},
	{"case-region-lower",		lowerregion},
	{"case-region-upper",		upperregion},
	{"case-word-capitalize",	capword},
	{"case-word-lower",		lowerword},
	{"case-word-upper",		upperword},
	{"change-file-name",		filename},
	{"clear-and-redraw",		refresh},
	{"copy-region",			copyregion},
	{"delete-blank-lines",		deblank},
	{"delete-buffer",		killbuffer},
	{"delete-mode",			delmode},
	{"delete-global-mode",		delgmode},
	{"delete-next-character",	forwdel},
	{"delete-next-word",		delfword},
	{"delete-other-windows",	onlywind},
	{"delete-previous-character",	backdel},
	{"delete-previous-word",	delbword},
	{"describe-bindings",		desbind},
	{"describe-key",		deskey},
	{"end-macro",			ctlxrp},
	{"end-of-file",			gotoeob},
	{"end-of-line",			gotoeol},
	{"exchange-point-and-mark",	swapmark},
	{"execute-buffer",		execbuf},
	{"execute-command-line",	execcmd},
	{"execute-file",		execfile},
	{"execute-macro",		ctlxe},
	{"execute-named-command",	namedcmd},
	{"exit-emacs",			quit},
	{"fill-paragraph",		fillpara},
	{"find-file",			filefind},
	{"forward-character",		forwchar},
	{"goto-line",			gotoline},
	{"grow-window",			enlargewind},
	{"handle-tab",			tab},
	{"hunt-forward",		forwhunt},
	{"hunt-backward",		backhunt},
	{"help",			help},
	{"i-shell",			spawncli},
	{"insert-file",			insfile},
	{"insert-space",		insspace},
	{"kill-paragraph",		killpara},
	{"kill-region",			killregion},
	{"kill-to-end-of-line",		killtext},
	{"list-buffers",		listbuffers},
	{"move-window-down",		mvdnwind},
	{"move-window-up",		mvupwind},
	{"name-buffer",			namebuffer},
	{"newline",			newline},
	{"newline-and-indent",		indent},
	{"next-buffer",			nextbuffer},
	{"next-line",			forwline},
	{"next-page",			forwpage},
	{"next-paragraph",		gotoeop},
	{"next-window",			nextwind},
	{"next-word",			forwword},
	{"open-line",			openline},
	{"previous-line",		backline},
	{"previous-page",		backpage},
	{"previous-paragraph",		gotobop},
	{"previous-window",		prevwind},
	{"previous-word",		backword},
	{"query-replace-string",	qreplace},
	{"quick-exit",			quickexit},
	{"quote-character",		quote},
	{"read-file",			fileread},
	{"redraw-display",		reposition},
	{"replace-string",		sreplace},
	{"save-file",			filesave},
	{"scroll-next-up",		scrnextup},
	{"scroll-next-down",		scrnextdw},
	{"search-forward",		forwsearch},
	{"search-reverse",		backsearch},
	{"select-buffer",		usebuffer},
	{"set-fill-column",		setfillcol},
	{"set-mark",			setmark},
	{"shell-command",		spawn},
	{"shrink-window",		shrinkwind},
	{"split-current-window",	splitwind},
#if	V7 & BSD
	{"suspend-emacs",		bktoshell},
#endif
	{"transpose-characters",	twiddle},
	{"unbind-key",			unbindkey},
	{"view-file",			viewfile},
	{"write-file",			filewrite},
	{"yank",			yank},

	{"",			NULL}
};
