/* /local/src/master/nrn/src/memacs/redef.h,v 1.2 1994/10/25 17:21:31 hines Exp */
/*
redef.h,v
 * Revision 1.2  1994/10/25  17:21:31  hines
 * bsearch and atoi conflict with names in djg libraries
 *
 * Revision 1.1.1.1  1994/10/12  17:21:23  hines
 * NEURON 3.0 distribution
 *
 * Revision 1.1  89/07/08  15:37:56  mlh
 * Initial revision
 * 
*/

/*Publics by module*/

#define exit emacs_exit

/*ANSI size = 250*/
#define ansibeep emacs_ansibeep
#define ansieeol emacs_ansieeol
#define ansieeop emacs_ansieeop
#define ansimove emacs_ansimove
#define ansiopen emacs_ansiopen
#define ansiparm emacs_ansiparm
#define ansirev emacs_ansirev
#define term emacs_term

/*BASIC size = 1743*/
#define backchar emacs_backchar
#define backline emacs_backline
#define backpage emacs_backpage
#define forwchar emacs_forwchar
#define forwline emacs_forwline
#define forwpage emacs_forwpage
#define getgoal emacs_getgoal
#define gotobob emacs_gotobob
#define gotobol emacs_gotobol
#define gotobop emacs_gotobop
#define gotoeob emacs_gotoeob
#define gotoeol emacs_gotoeol
#define gotoeop emacs_gotoeop
#define gotoline emacs_gotoline
#define setmark emacs_setmark
#define swapmark emacs_swapmark

/*BIND size = 3039*/
#define bindtokey emacs_bindtokey
#define cmdstr emacs_cmdstr
#define desbind emacs_desbind
#define deskey emacs_deskey
#define dobuf emacs_dobuf
#define docmd emacs_docmd
#define dofile emacs_dofile
#define execbuf emacs_execbuf
#define execcmd emacs_execcmd
#define execfile emacs_execfile
#define fncmatch emacs_fncmatch
#define getckey emacs_getckey
#define gettok emacs_gettok
#define help emacs_help
#define namedcmd emacs_namedcmd
#define nxtarg emacs_nxtarg
#define pathname emacs_pathname
#define startup emacs_startup
#define unbindkey emacs_unbindkey


/*BUFFER size = 2274*/
#define addline emacs_addline
#define anycb emacs_anycb
#define bclear emacs_bclear
#define bfind emacs_bfind
/*#define itoa emacs_itoa*/
#define killbuffer emacs_killbuffer
#define listbuffers emacs_listbuffers
#define makelist emacs_makelist
#define namebuffer emacs_namebuffer
#define nextbuffer emacs_nextbuffer
#define swbuffer emacs_swbuffer
#define usebuffer emacs_usebuffer
#define zotbuf emacs_zotbuf


/*DISPLAY size = 4830*/
#define getname emacs_getname
#define kbdtext emacs_kbdtext
#define lbound emacs_lbound
#define mlerase emacs_mlerase
#define mlputi emacs_mlputi
#define mlputli emacs_mlputli
#define mlputs emacs_mlputs
#define mlreply emacs_mlreply
#define mlreplyt emacs_mlreplyt
#define mlwrite emacs_mlwrite
#define mlyesno emacs_mlyesno
#define modeline emacs_modeline
#define movecursor emacs_movecursor
#define pscreen emacs_pscreen
#define ttcol emacs_ttcol
#define ttrow emacs_ttrow
#define update emacs_update
#define updateline emacs_updateline
#define updext emacs_updext
#define upmode emacs_upmode
#define vscreen emacs_vscreen
#define vtcol emacs_vtcol
#define vteeol emacs_vteeol
#define vtinit emacs_vtinit
#define vtmove emacs_vtmove
#define vtputc emacs_vtputc
#define vtpute emacs_vtpute
#define vtrow emacs_vtrow
#define vttidy emacs_vttidy


/*FILE size = 2602*/
#define filefind emacs_filefind
#define filename emacs_filename
#define fileread emacs_fileread
#define filesave emacs_filesave
#define filewrite emacs_filewrite
#define getfile emacs_getfile
#define ifile emacs_ifile
#define insfile emacs_insfile
#define makename emacs_makename
#define readin emacs_readin
#define viewfile emacs_viewfile
#define writeout emacs_writeout

/*FILEIO size = 464*/
#define ffclose emacs_ffclose
#define ffgetline emacs_ffgetline
#define ffp emacs_ffp
#define ffputline emacs_ffputline
#define ffropen emacs_ffropen
#define ffwopen emacs_ffwopen

/*LINE size = 2250*/
#define insspace emacs_insspace
#define kbufp emacs_kbufp
#define kdelete emacs_kdelete
#define kinsert emacs_kinsert
#define kremove emacs_kremove
#define ksize emacs_ksize
#define kused emacs_kused
#define lalloc emacs_lalloc
#define lchange emacs_lchange
#define ldelete emacs_ldelete
#define ldelnewline emacs_ldelnewline
#define lfree emacs_lfree
#define linsert emacs_linsert
#define lnewline emacs_lnewline

/*LOCK size = 1*/
#define lckhello emacs_lckhello


/*MAIN1 size = 5711*/
#define bheadp emacs_bheadp
#define blistp emacs_blistp
#define clexec emacs_clexec
#define ctlxe emacs_ctlxe
#define ctlxlp emacs_ctlxlp
#define ctlxrp emacs_ctlxrp
#define ctrlg emacs_ctrlg
#define curbp emacs_curbp
#define curcol emacs_curcol
#define curgoal emacs_curgoal
#define currow emacs_currow
#define curwp emacs_curwp
#define edinit emacs_edinit
#define eolexist emacs_eolexist
#define execute emacs_execute
#define fillcol emacs_fillcol
#define getctl emacs_getctl
#define getkey emacs_getkey
#define gmode emacs_gmode
#define kbdm emacs_kbdm
#define kbdmip emacs_kbdmip
#define kbdmop emacs_kbdmop
#define keytab emacs_keytab
#define lastflag emacs_lastflag
#define modecode emacs_modecode
#define modename emacs_modename
#define mpresf emacs_mpresf
#define names emacs_names
#define pat emacs_pat
#define quickexit emacs_quickexit
#define quit emacs_quit
#define rdonly emacs_rdonly
#define revexist emacs_revexist
#define rpat emacs_rpat
#define sarg emacs_sarg
#define sgarbf emacs_sgarbf
#define tabsize emacs_tabsize
#define thisflag emacs_thisflag
#define wheadp emacs_wheadp

/*RANDOM size = 2549*/
#define adjustmode emacs_adjustmode
#define backdel emacs_backdel
#define cinsert emacs_cinsert
#define deblank emacs_deblank
#define delgmode emacs_delgmode
#define delmode emacs_delmode
#define forwdel emacs_forwdel
#define getccol emacs_getccol
#define indent emacs_indent
#define insbrace emacs_insbrace
#define inspound emacs_inspound
#define killtext emacs_killtext
#define newline emacs_newline
#define openline emacs_openline
#define quote emacs_quote
#define setfillcol emacs_setfillcol
#define setgmode emacs_setgmode
#define setmode emacs_setmode
#define showcpos emacs_showcpos
#define tab emacs_tab
#define twiddle emacs_twiddle
#define yank emacs_yank

/*REGION size = 926*/
#define copyregion emacs_copyregion
#define getregion emacs_getregion
#define killregion emacs_killregion
#define lowerregion emacs_lowerregion
#define upperregion emacs_upperregion


/*SEARCH size = 2288*/
#define backhunt emacs_backhunt
#define backsearch emacs_backsearch
/*#define bsearch emacs_bsearch*/
#define eq emacs_eq
#define expandp emacs_expandp
#define forscan emacs_forscan
#define forwhunt emacs_forwhunt
#define forwsearch emacs_forwsearch
#define qreplace emacs_qreplace
#define readpattern emacs_readpattern
#define replaces emacs_replaces
#define sreplace emacs_sreplace

/*SPAWN size = 162*/
#define spawn emacs_spawn
#define spawncli emacs_spawncli

/*TCAP size = 1*/
#define hello emacs_hello


/*TERMIO size = 172*/
#define nxtchar emacs_nxtchar
#define rg emacs_rg
#define ttclose emacs_ttclose
#define ttflush emacs_ttflush
#define ttgetc emacs_ttgetc
#define ttopen emacs_ttopen
#define ttputc emacs_ttputc
#define typahead emacs_typahead

/*WINDOW size = 1803*/
#define enlargewind emacs_enlargewind
#define mvdnwind emacs_mvdnwind
#define mvupwind emacs_mvupwind
#define nextwind emacs_nextwind
#define onlywind emacs_onlywind
#define prevwind emacs_prevwind
#define refresh emacs_refresh
#define reposition emacs_reposition
#define scrnextdw emacs_scrnextdw
#define scrnextup emacs_scrnextup
#define shrinkwind emacs_shrinkwind
#define splitwind emacs_splitwind
#define wpopup emacs_wpopup


/*WORD size = 2008*/
#define backword emacs_backword
#define capword emacs_capword
#define delbword emacs_delbword
#define delfword emacs_delfword
#define fillpara emacs_fillpara
#define forwword emacs_forwword
#define inword emacs_inword
#define killpara emacs_killpara
#define lowerword emacs_lowerword
#define upperword emacs_upperword
#define wrapword emacs_wrapword

