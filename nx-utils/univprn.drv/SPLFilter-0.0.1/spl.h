/*****************************************************************************
 * Terms of Use
 * Copyright © Christoph Lindemann. All rights reserved.
 *
 * Permission is granted to copy, distribute and/or modify this document under
 * the terms of the GNU Free Documentation License, Version 1.2 or any later 
 * version published by the Free Software Foundation; with no Invariant 
 * Sections, no Front-Cover Texts, and no Back-Cover Texts. A copy of the 
 * license is included in the section entitled "GNU Free Documentation 
 * License".
 *----------------------------------------------------------------------------
 * History:
 *  24-03-2004  Initial Document
 *     Christoph Lindemann, christoph@lindemann.nu
 *  
 *  24-03-2004  Release to public
 *     Christoph Lindemann, christoph@lindemann.nu
 *  
 *  10-06-2004  Added SPL_SMR_PADDING
 *     Thanks to Fabian Franz
 *  
 *  11-06-2004  CORRECTED FAULTY INFORMATIONS
 *     Christoph Lindemann, christoph@lindemann.nu
 *  
 *  14-06-2004  Added some text explaining the format.
 *     Christoph Lindemann, christoph@lindemann.nu
 *  
 *  19-05-2005  Corrected typos in SMREXT definition
 *     Thanks to Peter Wasser
 *  
 *  02-11-2005  Updated End-Of-Page records 0x0D and 0x0E
 *     Thanks to Krzys
 *
 *  15-06-2006  Updated EMF Spool Metafile record types
 *     Christoph Lindemann, christoph@lindemann.nu
 *
 *  16-06-2006  Added information about PRESTARTPAGE record
 *     Christoph Lindemann, christoph@lindemann.nu
 *
 *****************************************************************************/
 
//Spool Metafile constants
#define SPLMETA_SIGNATURE 0x00010000 //Version 1.0 
 
// Spool Metafile record types
// Please note remarks in the corresponding struct definitions
#define SRT_PAGE_EMF1         0x00000001 /*  1 Enhanced Meta File (EMF) NT4   */                                         
#define SRT_FONT1             0x00000002 /*  2 Font Data                      */
#define SRT_DEVMODE           0x00000003 /*  3 DevMode                        */
#define SRT_FONT2             0x00000004 /*  4 Font Data                      */
#define SRT_PRESTARTPAGE      0x00000005 /*  5 PRESTARTPAGE                   */
#define SRT_FONT_MM           0x00000006 /*  6 Font Data (Multiple Master)    */
#define SRT_FONT_SUB1         0x00000007 /*  7 Font Data (SubsetFont 1)       */
#define SRT_FONT_SUB2         0x00000008 /*  8 Font Data (SubsetFont 2)       */
#define SRT_RESERVED_9        0x00000009 /*  9                                */
#define SRT_RESERVED_A        0x0000000A /* 10                                */ 
#define SRT_RESERVED_B        0x0000000B /* 11                                */
#define SRT_PAGE_EMF2         0x0000000C /* 12 Enhanced Meta File (EMF) Win2k */ 
#define SRT_EXT_EOPAGE_RASTER 0x0000000D /* 13 Ext EndOfPage raster bitmap    */
#define SRT_EXT_EOPAGE_VECTOR 0x0000000E /* 14 Ext EndOfPage GDI image data   */
#define SRT_EXT_FONT1         0x0000000F /* 15 Ext Font (SRT_FONT1)           */
#define SRT_EXT_FONT2         0x00000010 /* 16 Ext Font (SRT_FONT2)           */
#define SRT_EXT_FONT_MM       0x00000011 /* 17 Ext Font (SRT_FONT_MM)         */
#define SRT_EXT_FONT_SUB1     0x00000012 /* 18 Ext Font (SRT_FONT_SUB1)       */
#define SRT_EXT_FONT_SUB2     0x00000013 /* 19 Ext Font (SRT_FONT_SUB2)       */
#define SRT_EXT_PS_JOB_DATA   0x00000014 /* 20 PS_JOB_DATA escape data        */
#define SRT_EXT_FONT_EMBED    0x00000015 /* 21 Font (Embeded/Ghost?)          */
 
/*****************************************************************************
 * SPL_HEADER
 *----------------------------------------------------------------------------
 * SPL file header for EMFSPL files
 *****************************************************************************/
typedef struct tagSPLHEADER { 
    DWORD SIGNATURE; 
    DWORD nSize;            // record size INCLUDING header 
    DWORD offDocumentName;  // offset of Job Title from start 
    DWORD offPort;          // offset of portname from start
    //BYTE HeaderData[1]; 
	//LPCTSTR lpszDocName; 
    //LPCTSTR lpszOutput; 
} SPL_HEADER, *PSPL_HEADER;
 
/*****************************************************************************
 * SMR - Base record
 *----------------------------------------------------------------------------
 * Base record type for the Spool Metafile.
 *****************************************************************************/
typedef struct tagSMR{ 
    DWORD iType; // Spool metafile record type 
    DWORD nSize; // length of the following data 
                 // NOT INCLUDING this header 
} SMR, *PSMR;
 
/*****************************************************************************
 * SMREXT - Extended record
 *----------------------------------------------------------------------------
 * Contains neg. distance to start of Data
 *****************************************************************************/
typedef struct tagSMREXT{ 
    SMR smr; 
    DWORD DistanceLow; 
    DWORD DistanceHigh;
} SMREXT, *PSMREXT;
 
/*****************************************************************************
 * SMRPRESTARTPAGE - PRESTARTPAGE
 *----------------------------------------------------------------------------
 * Written before pagedata is written to spoolfile
 * Used as a temporary "end of file" indicating following data is not
 * fully spooled yet
 *****************************************************************************/
typedef struct tagSMRPRESTARTPAGE{ 
    SMR smr; 
    DWORD Unknown1; 
    DWORD Unknown2; //0xFFFFFFFF
} SMRPRESTARTPAGE, *PSMRPRESTARTPAGE;
 
/*****************************************************************************
 * SMR_PAGE - EMF/Page data
 *----------------------------------------------------------------------------
 * EMF/Page data
 *****************************************************************************/
typedef struct tagSMRPAGE{ 
    SMR smr;   // if smr.nSize == 0, this indicates EndOfFile
    BYTE EMFData[1];
} SMR_PAGE, *PSMR_PAGE;
 
/*****************************************************************************
 * SMR_DEVMODE - DEVMODE data
 *----------------------------------------------------------------------------
 * DEVMODE data
 *****************************************************************************/
typedef struct tagSMRDEVMODE{ 
    SMR smr; 
    BYTE DEVMODEData[1];
} SMR_DEVMODE, *PSMR_DEVMODE;
 
/*****************************************************************************
 * SMR_FONT - FONT data
 *****************************************************************************/
typedef struct tagSMRFONT{ 
    SMR smr; 
    BYTE FONTData[1];
} SMR_FONT, *PSMR_FONT;
 
/*****************************************************************************
 * SMR_EXTFONT - Extended Font Data
 *----------------------------------------------------------------------------
 * Contains neg. distance to start of
 * Font Data
 * Font data is typically embedded as
 * GDICOMMENT in the prev EMF data
 *****************************************************************************/
typedef struct tagEXTFONT{ 
    SMREXT smrext;
} SMR_EXTFONT, *PSMR_EXTFONT;
 
/*****************************************************************************
 * SMR_EOPAGE - End of Page
 *----------------------------------------------------------------------------
 * Contains neg. distance to
 * start of page record
 *****************************************************************************/
typedef struct tagSMREOPAGE{ 
    SMREXT smrext; 
} SMR_EOPAGE, *PSMR_EOPAGE;