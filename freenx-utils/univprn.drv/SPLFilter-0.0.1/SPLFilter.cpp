// SPLFilter.cpp : Generiert aus PDF Eingangsdaten, SPL/EMF Ausgangsdaten
//
// Copyright (c) 2007 by Fabian Franz.
//
// License: GPL, v2

#include <windows.h>
#include "stdafx.h"
#include "fpdfview.h"
#include "spl.h"

// Debug?
//#undef DEBUG
#define DEBUG 1

// Also print to some printer?
#undef PRINT
//#define PRINT 1

// Change and embed DEV MODE structures?
//#undef EXT_PRINT
#define EXT_PRINT 1

// Show a print dialog?
#undef EXT_PRNDLG
//#define EXT_PRNDLG 1

// Show printer options dialog?
#undef EXT_PRNOPTS
//#define EXT_PRNOPTS 1

/* Everything for the main loop is global, because due to a dll-imports bug we have massive stack corruption */

// Variables

FILE *out = NULL;
char *fbuf = NULL;
char buf[256];
DWORD bufSize = 0;
RECT   rect;
HDC hMeta;
HDC hDC;
HBRUSH brush;
FPDF_DOCUMENT pdf_doc;
FPDF_PAGE pdf_page;
HENHMETAFILE efile;
int logpixelsx, logpixelsy, size_x, size_y;
int i;

// DEVMODE

HANDLE hPrinter;
DEVMODE* lpDevMode = NULL;

// ErrorExit out function

void ErrorExit(LPTSTR lpszFunction) 
{ 
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;

    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    fwprintf(stderr, TEXT("%s failed with error %d: %s"), lpszFunction, dw, lpMsgBuf); 
    
    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
	if (out)
		fclose(out);
    ExitProcess(dw); 
}

// Enumeration function for the enh meta file records.

int CALLBACK enum_it(
  HDC hDC,                      // handle to DC
  HANDLETABLE *lpHTable,        // metafile handle table
  CONST ENHMETARECORD *lpEMFR,  // metafile record
  int nObj,                     // count of objects
  LPARAM lpData                 // optional data
)
{
	// Okay, we need to send a StartPage() first

	if (lpEMFR->iType == EMR_HEADER)
	{
		PENHMETAHEADER header=(PENHMETAHEADER)lpEMFR;

		bufSize = header->nBytes;
		
		// Write "Start of Page" record
		SMR smr;
		
		smr.iType=SRT_PAGE_EMF2;
		smr.nSize=bufSize;

		fwrite(&smr, sizeof(smr), 1, out);

#ifdef DEBUG
		fprintf(stderr, "header->nBytes = %d\n", header->nBytes);
#endif
	}

	// Write EMF records
	if (lpEMFR->iType == EMR_EXTTEXTOUTW)
	{
		EMREXTTEXTOUTW* emr=(PEMREXTTEXTOUTW)lpEMFR;

		if (emr != NULL && emr->emrtext.nChars > 1)
		{
			wchar_t buf[16384];

			if (emr == NULL || emr->emrtext.nChars > 4095)
				fprintf(stderr, "Error: EMR_EXTTEXTOUTW: Too long\n");
			else
			{
				memcpy(buf, (char*)emr + emr->emrtext.offString, emr->emrtext.nChars * sizeof(WCHAR));
				buf[emr->emrtext.nChars]='\0';
				buf[emr->emrtext.nChars+1]='\0';
#ifdef DEBUG
				fwprintf(stderr, L"EMR_EXTTEXTOUTW (%d): %s\n", wcslen(buf), buf);
#endif
			}
		}
	}

#ifdef DEBUG
	//fprintf(stderr, "Got %d with %u bytes\n", lpEMFR->iType, lpEMFR->nSize);
#endif
	
	// TODO: Implement embedding of fonts
	fwrite(lpEMFR, 1, lpEMFR->nSize, out);
	
	return 1;
}

int _tmain(int argc, _TCHAR* argv[])
{
	// Before we can do anything, we need to unlock the DLL
	// NOTE: If you are evaluating FOXIT READER SDK, you don’t need unlock the DLL,
	// then evaluation marks will be shown with all rendered pages.
	FPDF_UnlockDLL("license_id", "unlock_code");

	// first, load the document (no password specified)
	pdf_doc = FPDF_LoadDocument("testdoc.pdf", NULL);

	// error handling
	if (pdf_doc == NULL) 
	{
		fprintf(stderr, "ERROR - doc\n");
		exit(1);
	}

	// Open the out file
	
	fopen_s(&out, "test.spl", "wb");

	// Send the StartDoc header
	
	wchar_t* dname = L"desktop.ini - Editor";
	wchar_t* prn = L"c:\\output.prn";
	
	{

	if (wcsnlen(dname, 255) == 255)
		ErrorExit(L"dname too long");
	if (wcsnlen(prn, 255) == 255)
		ErrorExit(L"prn too long");

	SPL_HEADER spl;

	spl.SIGNATURE=SPLMETA_SIGNATURE;
	spl.nSize=(DWORD)sizeof(spl)+wcslen(dname)*2+wcslen(prn)*2+4; // +4, because \0\0 is after dname and prn
	spl.offDocumentName=(DWORD)sizeof(spl);
	spl.offPort=(DWORD)sizeof(spl)+wcslen(dname)*2+2; // +2 because \0\0 is after dname
	
	fwrite(&spl, sizeof(spl), 1, out);
	
	fwrite(dname, wcslen(dname)*2, 1, out);
	fwrite("\0\0", 2, 1, out);

	fwrite(prn, wcslen(prn)*2, 1, out);
	fwrite("\0\0", 2, 1, out);
	}

	// Load the first page and calculate the bbox
	// based on the printer margins

	pdf_page = FPDF_LoadPage(pdf_doc, 0);
	if (pdf_page == NULL)
		ErrorExit(L"FPDF_LoadPage");

	double page_width, page_height;
    
	page_width = FPDF_GetPageWidth(pdf_page);
    page_height = FPDF_GetPageHeight(pdf_page);

#ifdef EXT_PRINT
#ifdef EXT_PRNDLG
	PRINTDLG pd;
	HWND hwnd = NULL;

	// Initialize PRINTDLG
	ZeroMemory(&pd, sizeof(pd));
	pd.lStructSize = sizeof(pd);
	pd.hwndOwner   = hwnd;
	pd.hDevMode    = NULL;     // Don't forget to free or store hDevMode
	pd.hDevNames   = NULL;     // Don't forget to free or store hDevNames
	pd.Flags       = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC; 
	pd.nCopies     = 1;
	pd.nFromPage   = 0xFFFF; 
	pd.nToPage     = 0xFFFF; 
	pd.nMinPage    = 1; 
	pd.nMaxPage    = 0xFFFF; 
	

	if (PrintDlg(&pd) != TRUE)
		ErrorExit(L"PrintDialog\n");

	hDC = pd.hDC;
#else
	wchar_t* printer=L"FF";
	if (!OpenPrinter(printer, &hPrinter, NULL))
		ErrorExit(L"OpenPrinter\n");
    DWORD dwNeeded = DocumentProperties(NULL, hPrinter, printer, NULL, NULL, 0);
    lpDevMode = (LPDEVMODE)malloc(dwNeeded);
	
	DocumentProperties(NULL, hPrinter, printer, lpDevMode, NULL, DM_OUT_BUFFER);
	/* Try to set a higher print quality */
	lpDevMode->dmPrintQuality=1200;
	lpDevMode->dmFields|=DM_PRINTQUALITY;
	DocumentProperties(NULL, hPrinter, printer, lpDevMode, lpDevMode, DM_IN_BUFFER | DM_OUT_BUFFER);
#ifdef EXT_PRNOPTS
	DocumentProperties(NULL, hPrinter, printer, lpDevMode, lpDevMode, DM_IN_BUFFER | DM_PROMPT | DM_OUT_BUFFER);
#endif
	hDC = CreateDC(L"WINEPS.DRV", printer, NULL, lpDevMode);

	ClosePrinter(&hPrinter);
#endif
#else
	hDC = CreateDC(L"WINEPS.DRV", L"FF", NULL, lpDevMode);
#endif

	
#ifdef PRINT
	DOCINFO doc_info;

	doc_info.cbSize=sizeof(DOCINFO)+12;
	doc_info.lpszDocName=dname;
	doc_info.lpszOutput=prn;
	doc_info.lpszDatatype=NULL;
	doc_info.fwType=0;

	// Start a printer job
	StartDoc(hDC, &doc_info);
#endif
	
	// get number of pixels per inch (horizontally and vertically)
	logpixelsx = GetDeviceCaps(hDC, LOGPIXELSX);
	logpixelsy = GetDeviceCaps(hDC, LOGPIXELSY);
	
	// convert points into pixels
	size_x = (int)page_width / 72 * logpixelsx;
	size_y = (int)page_height / 72 * logpixelsy;

	DWORD p_width =GetDeviceCaps(hDC, HORZSIZE)*100;
	DWORD p_height=GetDeviceCaps(hDC, VERTSIZE)*100;

	SetRect( &rect, 0, 0, p_width, p_height );

#ifdef DEBUG
	//fprintf(stderr, "x=%u, y=%u, pw=%u, ph=%u,sx=%u,sy=%u,lpx=%u,lpy=%u,size_x=%u,size_y=%u\n",x,y,page_width,page_height,sx,sy,logpixelsx,logpixelsy,size_x,size_y);
#endif

	// now load the pages one after another

	for (i=0; i < FPDF_GetPageCount(pdf_doc); i++)
	{
#ifdef DEBUG
		fprintf(stderr, "Load page %d/%d\n", i, FPDF_GetPageCount(pdf_doc));
#endif

		// Load the next page

		pdf_page = FPDF_LoadPage(pdf_doc, i);
		
		if (pdf_page == NULL)
			ErrorExit(L"FPDF_LoadPage");
	
		fbuf=NULL;
#ifdef DEBUG
		sprintf_s(buf, 255, "test-%d.emf", i);
		//fbuf=buf;
#endif

		// Create a metafile to render to

		hMeta = CreateEnhMetaFileA(hDC, 
	          fbuf, 
	          &rect, "SPLFilter.exe\0Created by Fabian\0\0");

		if (hMeta == NULL)
			ErrorExit(L"CreateEnhMetaFileA");

	 	// Call FPDF_RenderPage function to render the whole page
		FPDF_RenderPage(hMeta, pdf_page, 0, 0, size_x, size_y, 0, 0);

#ifdef PRINT
		// Start a new printing page
		StartPage(hDC);
		FPDF_RenderPage(hDC, pdf_page, 0, 0, size_x, size_y, 0, 0);
		EndPage(hDC);
#endif

		// Close PDF page

		FPDF_ClosePage(pdf_page);

		efile=CloseEnhMetaFile(hMeta);

		if (efile == NULL)
			ErrorExit(L"CloseEnhMetaFile");

		// Write EMF data - via enumeration, because we want to embed fonts later
	    EnumEnhMetaFile(hDC, efile, enum_it, NULL, &rect);

		// Write EndPage() record
		{
		SMR_EOPAGE smr_eopage;
		
		smr_eopage.smrext.smr.iType=SRT_EXT_EOPAGE_VECTOR;
		smr_eopage.smrext.smr.nSize=sizeof(smr_eopage)-sizeof(smr_eopage.smrext.smr);

		/* FIXME: Need to calcualte low and high correctly */
		smr_eopage.smrext.DistanceLow=bufSize+smr_eopage.smrext.smr.nSize;
		smr_eopage.smrext.DistanceHigh=0;

		fwrite(&smr_eopage, sizeof(smr_eopage), 1, out);
		}
		
		DeleteEnhMetaFile(efile);
	}
#ifdef PRINT
	EndDoc(hDC);
#endif

	fclose(out);
	out=NULL;
	DeleteDC(hDC);
	FPDF_CloseDocument(pdf_doc);

#ifdef DEBUG
	fprintf(stderr, "SPLFilter: Conversion successful!\n");
#endif
	exit(0);
}

