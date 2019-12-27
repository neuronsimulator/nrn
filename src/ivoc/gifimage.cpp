#include <../../nrnconf.h>
#if HAVE_IV // to end of file

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <InterViews/raster.h>
#include <InterViews/image.h>
#include "oc2iv.h"

extern "C" {

#define byte unsigned char
#define True 1
#define False 0
#define DEBUG 0
#define Red pinfo_->r
#define Green pinfo_->g
#define Blue pinfo_->b

typedef struct {
	Raster* raster;
	char* comment;
	byte r[256];
	byte g[256];
	byte b[256];
} PICINFO;

PICINFO* pinfo_;

static int LoadGIF(const char* fname, PICINFO*);

} // extern "C"

Image* gif_image(const char* filename) {
	Image* image;
	pinfo_ = new PICINFO;
	if (LoadGIF(filename, pinfo_)) {
		image = new Image(pinfo_->raster);
	}
	if (pinfo_->comment) free(pinfo_->comment);
	delete pinfo_;
	return image;
}

extern "C" {

/*
 * xvgif.c  -  GIF loading code for 'xv'.  Based strongly on...
 *
 * gif2ras.c - Converts from a Compuserve GIF (tm) image to a Sun Raster image.
 *
 * Copyright (c) 1988, 1989 by Patrick J. Naughton
 *
 * Author: Patrick J. Naughton
 * naughton@wind.sun.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 */


#define NEXTBYTE (*dataptr++)
#define NEXTBYTEa (dataptr++) /* avoid 'expression result unused' warnings */
#define EXTENSION     0x21
#define IMAGESEP      0x2c
#define TRAILER       0x3b
#define INTERLACEMASK 0x40
#define COLORMAPMASK  0x80

#define ISTR_WARNING "LoadGif warning:"
#define ISTR_INFO "LoadGif info:"
#define SetISTR(a,f,c,d) fprintf(stderr, f, c, d)
  

static FILE *fp;

static int BitOffset = 0,		/* Bit Offset of next code */
    XC = 0, YC = 0,		/* Output X and Y coords of current pixel */
    Pass = 0,			/* Used by output routine if interlaced pic */
    OutCount = 0,		/* Decompressor output 'stack count' */
    RWidth, RHeight,		/* screen dimensions */
    Width, Height,		/* image dimensions */
    LeftOfs, TopOfs,		/* image offset */
    BitsPerPixel,		/* Bits per pixel, read from GIF header */
    BytesPerScanline,		/* bytes per scanline in output raster */
    ColorMapSize,		/* number of colors */
    Background,			/* background color */
    CodeSize,			/* Code size, read from GIF header */
    InitCodeSize,		/* Starting code size, used during Clear */
    Code,			/* Value returned by ReadCode */
    MaxCode,			/* limiting value for current code size */
    ClearCode,			/* GIF clear code */
    EOFCode,			/* GIF end-of-information code */
    CurCode, OldCode, InCode,	/* Decompressor variables */
    FirstFree,			/* First free code, generated per GIF spec */
    FreeCode,			/* Decompressor,next free slot in hash table */
    FinChar,			/* Decompressor variable */
    BitMask,			/* AND mask for data size */
    ReadMask,			/* Code AND mask for current code size */
    Misc;                       /* miscellaneous bits (interlace, local cmap)*/


static bool Interlace, HasColormap;

static byte *RawGIF;			/* The heap array to hold it, raw */
static byte *GRaster;			/* The raster data stream, unblocked */
static byte *pic8;

    /* The hash table used by the decompressor */
static int Prefix[4096];
static int Suffix[4096];

    /* An output array used by the decompressor */
static int OutCode[4097];

static int   gif89 = 0;
static const char *id87 = "GIF87a";
static const char *id89 = "GIF89a";

static int EGApalette[16][3] = {
  {0,0,0},       {0,0,128},     {0,128,0},     {0,128,128}, 
  {128,0,0},     {128,0,128},   {128,128,0},   {200,200,200},
  {100,100,100}, {100,100,255}, {100,255,100}, {100,255,255},
  {255,100,100}, {255,100,255}, {255,255,100}, {255,255,255} };
  

static int   readImage(PICINFO*);
static int   readCode();
static void  doInterlace(int);
static int   gifError(PICINFO*, const char *);
static void  gifWarning(const char *);

static int   filesize;
static const char *bname;

static byte *dataptr;


/*****************************/
static int LoadGIF(const char* fname, PICINFO* pinfo)
/*****************************/
{
  /* returns '1' if successful */

  register byte  ch, ch1, *origptr;
  register int   i, block;
  int            aspect, gotimage;
  float normaspect;

  /* initialize variables */
  BitOffset = XC = YC = Pass = OutCount = gotimage = 0;
  RawGIF = GRaster = pic8 = 0;
  gif89 = 0;

  pinfo->raster     = NULL;
  pinfo->comment = NULL;

  bname = fname;
  fp = fopen(fname,"rb");
  if (!fp) return ( gifError(pinfo, "can't open file") );


  /* find the size of the file */
  fseek(fp, 0L, 2);
  filesize = ftell(fp);
  fseek(fp, 0L, 0);
  
  /* the +256's are so we can read truncated GIF files without fear of 
     segmentation violation */
  if (!(dataptr = RawGIF = (byte *) calloc((size_t) filesize+256, (size_t) 1)))
    return( gifError(pinfo, "not enough memory to read gif file") );
  
  if (!(GRaster = (byte *) calloc((size_t) filesize+256,(size_t) 1))) 
    return( gifError(pinfo, "not enough memory to read gif file") );
  
  if (fread(dataptr, (size_t) filesize, (size_t) 1, fp) != 1) 
    return( gifError(pinfo, "GIF data read failed") );


  origptr = dataptr;

  if      (strncmp((char *) dataptr, id87, (size_t) 6)==0) gif89 = 0;
  else if (strncmp((char *) dataptr, id89, (size_t) 6)==0) gif89 = 1;
  else    return( gifError(pinfo, "not a GIF file"));
  
  dataptr += 6;
  
  /* Get variables from the GIF screen descriptor */
  
  ch = NEXTBYTE;
  RWidth = ch + 0x100 * NEXTBYTE;	/* screen dimensions... not used. */
  ch = NEXTBYTE;
  RHeight = ch + 0x100 * NEXTBYTE;
  
  ch = NEXTBYTE;
  HasColormap = ((ch & COLORMAPMASK) ? True : False);
  
  BitsPerPixel = (ch & 7) + 1;
  ColorMapSize = 1 << BitsPerPixel;
  BitMask = ColorMapSize - 1;
  
  Background = NEXTBYTE;		/* background color... not used. */
  
  aspect = NEXTBYTE;
  if (aspect) {
    if (!gif89) return(gifError(pinfo,"corrupt GIF file (screen descriptor)"));
    else normaspect = (float) (aspect + 15) / 64.0;   /* gif89 aspect ratio */
    if (DEBUG) fprintf(stderr,"GIF89 aspect = %f\n", normaspect);
  }
  
  
  /* Read in global colormap. */
  
  if (HasColormap)
    for (i=0; i<ColorMapSize; i++) {
      pinfo->r[i] = NEXTBYTE;
      pinfo->g[i] = NEXTBYTE;
      pinfo->b[i] = NEXTBYTE;
    }
  else {  /* no colormap in GIF file */
    /* put std EGA palette (repeated 16 times) into colormap, for lack of
       anything better to do */

    for (i=0; i<256; i++) {
      pinfo->r[i] = EGApalette[i&15][0];
      pinfo->g[i] = EGApalette[i&15][1];
      pinfo->b[i] = EGApalette[i&15][2];
    }
  }

  /* possible things at this point are:
   *   an application extension block
   *   a comment extension block
   *   an (optional) graphic control extension block
   *       followed by either an image
   *	   or a plaintext extension
   */

  while (1) {
    block = NEXTBYTE;

    if (DEBUG) fprintf(stderr,"LoadGIF: ");

    if (block == EXTENSION) {  /* parse extension blocks */
      int i, fn, blocksize, aspnum, aspden;

      /* read extension block */
      fn = NEXTBYTE;

      if (DEBUG) fprintf(stderr,"GIF extension type 0x%02x\n", fn);

      if (fn == 'R') {                  /* GIF87 aspect extension */
	int sbsize;

	blocksize = NEXTBYTE;
	if (blocksize == 2) {
	  aspnum = NEXTBYTE;
	  aspden = NEXTBYTE;
	  if (aspden>0 && aspnum>0) 
	    normaspect = (float) aspnum / (float) aspden;
	  else { normaspect = 1.0;  aspnum = aspden = 1; }

	  if (DEBUG) fprintf(stderr,"GIF87 aspect extension: %d:%d = %f\n\n", 
			     aspnum, aspden,normaspect);
	}
	else {
	  for (i=0; i<blocksize; i++) NEXTBYTEa;
	}

	while ((sbsize=NEXTBYTE)>0) {  /* eat any following data subblocks */
	  for (i=0; i<sbsize; i++) NEXTBYTEa;
	}
      }


      else if (fn == 0xFE) {  /* Comment Extension */
	int   ch, j, sbsize, cmtlen;
	byte *ptr1, *cmt, *cmt1, *sp;

	cmtlen = 0;
	ptr1 = dataptr;      /* remember start of comments */

	/* figure out length of comment */
	do {
	  sbsize = NEXTBYTE;
	  cmtlen += sbsize;
	  for (j=0; j<sbsize; j++) ch = NEXTBYTE;
	} while (sbsize);


	if (cmtlen>0) {   /* build into one un-blocked comment */
	  cmt = (byte *) malloc((size_t) (cmtlen + 1));
	  if (!cmt) gifWarning("couldn't malloc space for comments\n");
	  else {
	    sp = cmt;
	    do {
	      sbsize = (*ptr1++);
	      for (j=0; j<sbsize; j++, sp++, ptr1++) *sp = *ptr1;
	    } while (sbsize);
	    *sp = '\0';

	    if (pinfo->comment) {    /* have to strcat onto old comments */
	      cmt1 = (byte *) malloc(strlen(pinfo->comment) + cmtlen + 2);
	      if (!cmt1) {
		gifWarning("couldn't malloc space for comments\n");
		free(cmt);
	      }
	      else {
		strcpy((char *) cmt1, (char *) pinfo->comment);
		strcat((char *) cmt1, (char *) "\n");
		strcat((char *) cmt1, (char *) cmt);
		free(pinfo->comment);
		free(cmt);
		pinfo->comment = (char *) cmt1;
	      }
	    }
	    else pinfo->comment = (char *) cmt;
	  }  /* if (cmt) */
	}  /* if cmtlen>0 */
      }


      else if (fn == 0x01) {  /* PlainText Extension */
	int j,sbsize,ch;
	int tgLeft, tgTop, tgWidth, tgHeight, cWidth, cHeight, fg, bg;
      
	SetISTR(ISTR_INFO, "%s:  %s\n", bname, 
		"PlainText extension found in GIF file.  Ignored.");

	sbsize   = NEXTBYTE;
	tgLeft   = NEXTBYTE;  tgLeft   += (NEXTBYTE)<<8;
	tgTop    = NEXTBYTE;  tgTop    += (NEXTBYTE)<<8;
	tgWidth  = NEXTBYTE;  tgWidth  += (NEXTBYTE)<<8;
	tgHeight = NEXTBYTE;  tgHeight += (NEXTBYTE)<<8;
	cWidth   = NEXTBYTE;
	cHeight  = NEXTBYTE;
	fg       = NEXTBYTE;
	bg       = NEXTBYTE;
	i=12;
	for ( ; i<sbsize; i++) NEXTBYTEa;   /* read rest of first subblock */
      
	if (DEBUG) fprintf(stderr,
	   "PlainText: tgrid=%d,%d %dx%d  cell=%dx%d  col=%d,%d\n",
	   tgLeft, tgTop, tgWidth, tgHeight, cWidth, cHeight, fg, bg);
	
	/* read (and ignore) data sub-blocks */
	do {
	  j = 0;
	  sbsize = NEXTBYTE;
	  while (j<sbsize) {
	    ch = NEXTBYTE;  j++;
	    if (DEBUG) fprintf(stderr,"%c", ch);
	  }
	} while (sbsize);
	if (DEBUG) fprintf(stderr,"\n\n");
      }


      else if (fn == 0xF9) {  /* Graphic Control Extension */
	int j, sbsize;

	if (DEBUG) fprintf(stderr,"Graphic Control extension\n\n");

	SetISTR(ISTR_INFO, "%s:  %s\n", bname, 
		"Graphic Control Extension in GIF file.  Ignored.");
	
	/* read (and ignore) data sub-blocks */
	do {
	  j = 0; sbsize = NEXTBYTE;
	  while (j<sbsize) { NEXTBYTEa;  j++; }
	} while (sbsize);
      }
      

      else if (fn == 0xFF) {  /* Application Extension */
	int j, sbsize;

	if (DEBUG) fprintf(stderr,"Application extension\n\n");

	/* read (and ignore) data sub-blocks */
	do {
	  j = 0; sbsize = NEXTBYTE;
	  while (j<sbsize) { NEXTBYTEa;  j++; }
	} while (sbsize);
      }
      

      else { /* unknown extension */
	int j, sbsize;

	if (DEBUG) fprintf(stderr,"unknown GIF extension 0x%02x\n\n", fn);

	SetISTR(ISTR_INFO,
		"%s:  Unknown extension 0x%02x in GIF file.  Ignored.\n",
		bname, fn);
	
	/* read (and ignore) data sub-blocks */
	do {
	  j = 0; sbsize = NEXTBYTE;
	  while (j<sbsize) { NEXTBYTEa;  j++; }
	} while (sbsize);
      }
    }


    else if (block == IMAGESEP) {
      if (DEBUG) fprintf(stderr,"imagesep (got=%d)  ",gotimage);
      if (DEBUG) fprintf(stderr,"  at start: offset=%ld\n",dataptr-RawGIF);

      if (gotimage) {   /* just skip over remaining images */
	int i,misc,ch,ch1;

	/* skip image header */
	NEXTBYTEa;  NEXTBYTEa;  /* left position */
	NEXTBYTEa;  NEXTBYTEa;  /* top position */
	NEXTBYTEa;  NEXTBYTEa;  /* width */
	NEXTBYTEa;  NEXTBYTEa;  /* height */
	misc = NEXTBYTE;      /* misc. bits */

	if (misc & 0x80) {    /* image has local colormap.  skip it */
	  for (i=0; i< 1 << ((misc&7)+1);  i++) {
	    NEXTBYTEa;  NEXTBYTEa;  NEXTBYTEa;
	  }
	}

	NEXTBYTEa;       /* minimum code size */

	/* skip image data sub-blocks */
	do {
	  ch = ch1 = NEXTBYTE;
	  while (ch--) NEXTBYTEa;
	  if ((dataptr - RawGIF) > filesize) break;      /* EOF */
	} while(ch1);
      }

      else if (readImage(pinfo)) gotimage = 1;
      if (DEBUG) fprintf(stderr,"  at end:   dataptr=%ld\n",dataptr-RawGIF);
    }


    else if (block == TRAILER) {      /* stop reading blocks */
      if (DEBUG) fprintf(stderr,"trailer");
      break;
    }

    else {      /* unknown block type */
      char str[128];

      if (DEBUG) fprintf(stderr,"block type 0x%02x  ", block);

      /* don't mention bad block if file was trunc'd, as it's all bogus */
      if ((dataptr - origptr) < filesize) {
	sprintf(str, "Unknown block type (0x%02x) at offset %ld",
		block, (dataptr - origptr) - 1);

	if (!gotimage) return gifError(pinfo, str);
	else gifWarning(str);
      }

      break;
    }

    if (DEBUG) fprintf(stderr,"\n");
  }

  free(RawGIF);	 RawGIF = 0;
  free(GRaster);  GRaster = 0;

  if (!gotimage) 
     return( gifError(pinfo, "no image data found in GIF file") );

  return 1;
}


/********************************************/
static int readImage(PICINFO* pinfo)
{
  register byte ch, ch1, *ptr1, *picptr;
  int           i, npixels, maxpixels;

  npixels = maxpixels = 0;

  /* read in values from the image descriptor */
  
  ch = NEXTBYTE;
  LeftOfs = ch + 0x100 * NEXTBYTE;
  ch = NEXTBYTE;
  TopOfs  = ch + 0x100 * NEXTBYTE;
  ch = NEXTBYTE;
  Width   = ch + 0x100 * NEXTBYTE;
  ch = NEXTBYTE;
  Height  = ch + 0x100 * NEXTBYTE;

  Misc = NEXTBYTE;
  Interlace = ((Misc & INTERLACEMASK) ? True : False);

  if (!Interlace) Pass = -1;
  
  if (Misc & 0x80) {
    for (i=0; i< 1 << ((Misc&7)+1); i++) {
      pinfo->r[i] = NEXTBYTE;
      pinfo->g[i] = NEXTBYTE;
      pinfo->b[i] = NEXTBYTE;
    }
  }


  if (!HasColormap && !(Misc&0x80)) {
    /* no global or local colormap */
    SetISTR(ISTR_WARNING, "%s:  %s\n", bname, 
	    "No colormap in this GIF file.  Assuming EGA colors.");
  }
    

  
  /* Start reading the raster data. First we get the intial code size
   * and compute decompressor constant values, based on this code size.
   */
  
  CodeSize = NEXTBYTE;

  ClearCode = (1 << CodeSize);
  EOFCode = ClearCode + 1;
  FreeCode = FirstFree = ClearCode + 2;
  
  /* The GIF spec has it that the code size is the code size used to
   * compute the above values is the code size given in the file, but the
   * code size used in compression/decompression is the code size given in
   * the file plus one. (thus the ++).
   */
  
  CodeSize++;
  InitCodeSize = CodeSize;
  MaxCode = (1 << CodeSize);
  ReadMask = MaxCode - 1;
  


  /* UNBLOCK:
   * Read the raster data.  Here we just transpose it from the GIF array
   * to the Raster array, turning it from a series of blocks into one long
   * data stream, which makes life much easier for readCode().
   */
  
  ptr1 = GRaster;
  do {
    ch = ch1 = NEXTBYTE;
    while (ch--) { *ptr1 = NEXTBYTE; ptr1++; }
    if ((dataptr - RawGIF) > filesize) {
      SetISTR(ISTR_WARNING,"%s:  %s\n", bname,
	      "This GIF file seems to be truncated.  Winging it.");
      break;
    }
  } while(ch1);




  if (DEBUG) {
    fprintf(stderr,"xv: LoadGIF() - picture is %dx%d, %d bits, %sinterlaced\n",
	    Width, Height, BitsPerPixel, Interlace ? "" : "non-");
  }
  

  /* Allocate the 'pic' */
  maxpixels = Width*Height;
//  picptr = pic8 = (byte *) malloc((size_t) maxpixels);
pinfo->raster = new Raster(Width, Height);
  if (!pinfo->raster) return( gifError(pinfo, "couldn't malloc 'pic8'") );

  
  /* Decompress the file, continuing until you see the GIF EOF code.
   * One obvious enhancement is to add checking for corrupt files here.
   */
  
  Code = readCode();
  while (Code != EOFCode) {
    /* Clear code sets everything back to its initial value, then reads the
     * immediately subsequent code as uncompressed data.
     */

    if (Code == ClearCode) {
      CodeSize = InitCodeSize;
      MaxCode = (1 << CodeSize);
      ReadMask = MaxCode - 1;
      FreeCode = FirstFree;
      Code = readCode();
      CurCode = OldCode = Code;
      FinChar = CurCode & BitMask;
	doInterlace(FinChar);
      npixels++;
    }
    else {
      /* If not a clear code, must be data: save same as CurCode and InCode */

      /* if we're at maxcode and didn't get a clear, stop loading */
      if (FreeCode>=4096) { /* printf("freecode blew up\n"); */
			    break; }

      CurCode = InCode = Code;
      
      /* If greater or equal to FreeCode, not in the hash table yet;
       * repeat the last character decoded
       */
      
      if (CurCode >= FreeCode) {
	CurCode = OldCode;
	if (OutCount > 4096) {  /* printf("outcount1 blew up\n"); */ break; }
	OutCode[OutCount++] = FinChar;
      }
      
      /* Unless this code is raw data, pursue the chain pointed to by CurCode
       * through the hash table to its end; each code in the chain puts its
       * associated output code on the output queue.
       */
      
      while (CurCode > BitMask) {
	if (OutCount > 4096) break;   /* corrupt file */
	OutCode[OutCount++] = Suffix[CurCode];
	CurCode = Prefix[CurCode];
      }
      
      if (OutCount > 4096) { /* printf("outcount blew up\n"); */ break; }
      
      /* The last code in the chain is treated as raw data. */
      
      FinChar = CurCode & BitMask;
      OutCode[OutCount++] = FinChar;
      
      /* Now we put the data out to the Output routine.
       * It's been stacked LIFO, so deal with it that way...
       */

      /* safety thing:  prevent exceeding range of 'pic8' */
      if (npixels + OutCount > maxpixels) OutCount = maxpixels-npixels;
	
      npixels += OutCount;
      for (i=OutCount-1; i>=0; i--) doInterlace(OutCode[i]);
      OutCount = 0;

      /* Build the hash table on-the-fly. No table is stored in the file. */
      
      Prefix[FreeCode] = OldCode;
      Suffix[FreeCode] = FinChar;
      OldCode = InCode;
      
      /* Point to the next slot in the table.  If we exceed the current
       * MaxCode value, increment the code size unless it's already 12.  If it
       * is, do nothing: the next code decompressed better be CLEAR
       */
      
      FreeCode++;
      if (FreeCode >= MaxCode) {
	if (CodeSize < 12) {
	  CodeSize++;
	  MaxCode *= 2;
	  ReadMask = (1 << CodeSize) - 1;
	}
      }
    }
    Code = readCode();
    if (npixels >= maxpixels) break;
  }
  
  if (npixels != maxpixels) {
    SetISTR(ISTR_WARNING,"%s:  %s\n", bname,
	    "This GIF file seems to be truncated.  Winging it.");
//    if (!Interlace)  /* clear->EOBuffer */
//      xvbzero((char *) pic8+npixels, (size_t) (maxpixels-npixels));
  }

  fclose(fp);

  /* fill in the PICINFO structure */

#if 0
  pinfo->pic     = pic8;
  pinfo->w       = Width;           
  pinfo->h       = Height;
  pinfo->type    = PIC8;
  pinfo->frmType = F_GIF;
  pinfo->colType = F_FULLCOLOR;

  pinfo->normw = pinfo->w;   pinfo->normh = pinfo->h;

  sprintf(pinfo->fullInfo,
	  "GIF%s, %d bit%s per pixel, %sinterlaced.  (%d bytes)",
 	  (gif89) ? "89" : "87", BitsPerPixel, 
	  (BitsPerPixel==1) ? "" : "s", 
 	  Interlace ? "" : "non-", filesize);

  sprintf(pinfo->shrtInfo, "%dx%d GIF%s.",Width,Height,(gif89) ? "89" : "87");

  /* pinfo.comment gets handled in main LoadGIF() block-reader */
#endif
  return 1;
}



/* Fetch the next code from the raster data stream.  The codes can be
 * any length from 3 to 12 bits, packed into 8-bit bytes, so we have to
 * maintain our location in the Raster array as a BIT Offset.  We compute
 * the byte Offset into the raster array by dividing this by 8, pick up
 * three bytes, compute the bit Offset into our 24-bit chunk, shift to
 * bring the desired code to the bottom, then mask it off and return it. 
 */

static int readCode()
{
  int RawCode, ByteOffset;
  
  ByteOffset = BitOffset / 8;
  RawCode = GRaster[ByteOffset] + (GRaster[ByteOffset + 1] << 8);
  if (CodeSize >= 8)
    RawCode += ( ((int) GRaster[ByteOffset + 2]) << 16);
  RawCode >>= (BitOffset % 8);
  BitOffset += CodeSize;

  return(RawCode & ReadMask);
}


/***************************/
static void doInterlace(int Index)
{
  
  if (YC<Height)
    pinfo_->raster->poke(XC, Height - YC - 1,
	Red[Index]/256., Green[Index]/256., Blue[Index]/256.,
	1.);
  
  /* Update the X-coordinate, and if it overflows, update the Y-coordinate */
  
  if (++XC == Width) {
    
    /* deal with the interlace as described in the GIF
     * spec.  Put the decoded scan line out to the screen if we haven't gone
     * past the bottom of it
     */
    
    XC = 0;
    
    switch (Pass) {
    case 0:
      YC += 8;
      if (YC >= Height) { Pass++; YC = 4; }
      break;
      
    case 1:
      YC += 8;
      if (YC >= Height) { Pass++; YC = 2; }
      break;
      
    case 2:
      YC += 4;
      if (YC >= Height) { Pass++; YC = 1; }
      break;
      
    case 3:
      YC += 2;  break;
      
    default:
	++YC;
      break;
    }
  }
}


      
/*****************************/
static int gifError(PICINFO* pinfo, const char* st)
{
  gifWarning(st);

  if (RawGIF != NULL) free(RawGIF);
  if (GRaster != NULL) free(GRaster);

  if (pinfo->raster) pinfo->raster->unref();
  if (pinfo->comment) free(pinfo->comment);


  pinfo->raster = 0;
  pinfo->comment = (char *) 0;

  return 0;
}


/*****************************/
static void gifWarning(const char* st)
{
//  SetISTR(ISTR_WARNING,"%s:  %s\n", bname, st);
	hoc_warning(bname, st);
}


} //  extern "C"

#endif
