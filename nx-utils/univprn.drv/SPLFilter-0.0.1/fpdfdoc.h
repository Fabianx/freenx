// FPDFDOC.H - Header file for FPDFDOC module - a part of Foxit Reader SDK
// Copyright (c) 2005 Foxit Software Company, All Right Reserved.

// Revision: 0.1
// Date: 2005-10-19

#ifndef _FPDFDOC_H_
#define _FPDFDOC_H_

#ifndef _FPDFVIEW_H_
#include "fpdfview.h"
#endif

// Data types
typedef void* FPDF_BOOKMARK;
typedef void* FPDF_DEST;
typedef void* FPDF_ACTION;
typedef void* FPDF_LINK;

typedef unsigned short FPDF_WCHAR;

// Exported Functions
#ifdef __cplusplus
extern "C" {
#endif

// Function: FPDFBookmark_GetFirstChild
//			Get first child of a bookmark item, or first top level bookmark item
// Parameters: 
//			document	-	Handle to the document. Returned by FPDF_LoadDocument or FPDF_LoadMemDocument
//			bookmark	-	Handle to the current bookmark. Can be NULL if you want to get the first top level item
// Return value:
//			Handle to the first child or top level bookmark item. NULL if no child or top level bookmark found.
//
__declspec(dllimport)  FPDF_BOOKMARK STDCALL FPDFBookmark_GetFirstChild(FPDF_DOCUMENT document, FPDF_BOOKMARK bookmark);

// Function: FPDFBookmark_GetNextSibling
//			Get next bookmark item on the same level
// Parameters: 
//			document	-	Handle to the document. Returned by FPDF_LoadDocument or FPDF_LoadMemDocument
//			bookmark	-	Handle to the current bookmark. Cannot be NULL.
// Return value:
//			Handle to the next bookmark item on the same level. NULL if this is the last bookmark on this level.
//
__declspec(dllimport)  FPDF_BOOKMARK STDCALL FPDFBookmark_GetNextSibling(FPDF_DOCUMENT document, FPDF_BOOKMARK bookmark);

// Function: FPDFBookmark_Find
//			Find a bookmark in the document, using the bookmark title
// Parameters: 
//			document	-	Handle to the document. Returned by FPDF_LoadDocument or FPDF_LoadMemDocument
//			title		-	The Unicode string (UTF-16LE encoded) for the bookmark title. Can't be NULL.
// Return value:
//			Handle to the found bookmark item. NULL if the title can't be found.
// Comments:
//			It always returns the first found bookmark if more than one bookmarks have the same title.
//
__declspec(dllimport)  FPDF_BOOKMARK STDCALL FPDFBookmark_Find(FPDF_DOCUMENT document, const FPDF_WCHAR* title);

// Function: FPDFBookmark_GetColorRef
//			Get designated color of a bookmark item
// Parameters:
//			bookmark	-	Handle to the bookmark
// Return value:
//			A COLORREF value (0x00ggbbrr) for the bookmark item.
//
__declspec(dllimport)  unsigned long STDCALL FPDFBookmark_GetColorRef(FPDF_BOOKMARK bookmark);

// Bookmark font styles
#define PDFBOOKMARK_NORMAL			0
#define PDFBOOKMARK_ITALIC			1
#define PDFBOOKMARK_BOLD			2
#define PDFBOOKMARK_BOLDITALIC		3

// Function: FPDFBookmark_GetFontStyle
//			Get designated font style for a bookmark item
// Parameters:
//			bookmark	-	Handle to the bookmark
// Return value:
//			A number indicating the font style, as defined above
//
__declspec(dllimport)  unsigned long STDCALL FPDFBookmark_GetFontStyle(FPDF_BOOKMARK bookmark);

// Function: FPDFBookmark_GetTitle
//			Get title of a bookmark
// Parameters:
//			bookmark	-	Handle to the bookmark
//			buffer		-	A buffer for output the title. Can be NULL.
//			buflen		-	The length of the buffer, number of bytes. Can be 0.
// Return value:
//			Number of bytes the title consumes, including trailing zeroes.
// Comments:
//			The title is always output in UTF-16LE encoding, which means the buffer can be regarded as
//			an array of WORD (on Intel and compatible CPUs), each WORD represent the Unicode of a character.
//			The string is followed by two bytes of zero indicating end of the string.
//			It's up to the application to convert the Unicode string to local encoded string, if necessary.
//
//			The return value always indicated number of bytes required for the buffer, even when there is
//			no buffer specified, or the buffer size is less then required. In this case, the buffer will not
//			be modified.
//
__declspec(dllimport)  unsigned long STDCALL FPDFBookmark_GetTitle(FPDF_BOOKMARK bookmark, void* buffer, unsigned long buflen);

// Function: FPDFBookmark_GetAction
//			Get the action associated with a bookmark item
// Parameters:
//			bookmark	-	Handle to the bookmark
// Return value:
//			Handle to the action data. NULL if no action is associated with this bookmark. In this case, the 
//			application should try FPDFBookmark_GetDest
//
__declspec(dllimport)  FPDF_ACTION STDCALL FPDFBookmark_GetAction(FPDF_BOOKMARK bookmark);

// Function: FPDFBookmark_GetDest
//			Get the destination associated with a bookmark item
// Parameters:
//			document	-	Handle to the document
//			bookmark	-	Handle to the bookmark
// Return value:
//			Handle to the destination data. NULL if no destination is associated with this bookmark
//
__declspec(dllimport)  FPDF_DEST STDCALL FPDFBookmark_GetDest(FPDF_DOCUMENT document, FPDF_BOOKMARK bookmark);

#define PDFACTION_UNSUPPORTED		0		// Unsupported action type
#define PDFACTION_GOTO				1		// Go to a destination within current document
#define PDFACTION_REMOTEGOTO		2		// Go to a destination within another document
#define PDFACTION_URI				3		// Universal Resource Identifier, including web pages and 
											// other Internet based resources
#define PDFACTION_LAUNCH			4		// Launch an application or open a file

// Function: FPDFAction_GetType
//			Get type of an action
// Parameters:
//			action		-	Handle to the action
// Return value:
//			A type number as defined above.
//
__declspec(dllimport)  unsigned long STDCALL FPDFAction_GetType(FPDF_ACTION action);

// Function: FPDFAction_GetFilePath
//			Get file path of an remote goto action
// Parameters:
//			action		-	Handle to the action. Must be a RMEOTEGOTO or LAUNCH action
//			buffer		-	A buffer for output the title. Can be NULL.
//			buflen		-	The length of the buffer, number of bytes. Can be 0.
// Return value:
//			Number of bytes the file path consumes, including trailing zero.
// Comments:
//			The file path is output in local encoding.
// 
//			The return value always indicated number of bytes required for the buffer, even when there is
//			no buffer specified, or the buffer size is less then required. In this case, the buffer will not
//			be modified.
//
__declspec(dllimport)  unsigned long STDCALL FPDFAction_GetFilePath(FPDF_ACTION action, void* buffer, unsigned long buflen);

// Function: FPDFAction_GetDest
//			Get destination of an action
// Parameters:
//			document	-	Handle to the document.
//			action		-	Handle to the action. It must be a GOTO or REMOTEGOTO action.
// Return value:
//			Handle to the destination data.
// Comments:
//			In case of remote goto action, the application should first use FPDFAction_GetFilePath to
//			get file path, then load that particular document, and use its document handle to call this
//			function.
//
__declspec(dllimport)  FPDF_DEST STDCALL FPDFAction_GetDest(FPDF_DOCUMENT document, FPDF_ACTION action);

// Function: FPDFAction_GetURIPath
//			Get URI path of a URI action
// Parameters:
//			document	-	Handle to the document.
//			action		-	Handle to the action. Must be a URI action
//			buffer		-	A buffer for output the title. Can be NULL.
//			buflen		-	The length of the buffer, number of bytes. Can be 0.
// Return value:
//			Number of bytes the URI path consumes, including trailing zero.
// Comments:
//			The URI path is always encoded in 7-bit ASCII.
// 
//			The return value always indicated number of bytes required for the buffer, even when there is
//			no buffer specified, or the buffer size is less then required. In this case, the buffer will not
//			be modified.
//
__declspec(dllimport)  unsigned long STDCALL FPDFAction_GetURIPath(FPDF_DOCUMENT document, FPDF_ACTION action,
													  void* buffer, unsigned long buflen);

// Function: FPDFDest_GetPageIndex
//			Get page index of a destination
// Parameters:
//			document	-	Handle to the document
//			dest		-	Handle to the destination
// Return value:
//			The page index. Starting from 0 for the first page.
//
__declspec(dllimport)  unsigned long STDCALL FPDFDest_GetPageIndex(FPDF_DOCUMENT document, FPDF_DEST dest);

// Zoom modes
#define PDFZOOM_XYZ				1			// zoom level with specified offset
#define PDFZOOM_FITPAGE			2			// fit both the width and height of the page (whichever smaller)
#define PDFZOOM_FITHORZ			3			// fit the page width
#define PDFZOOM_FITVERT			4			// fit the page height
#define PDFZOOM_FITRECT			5			// fit a specific rectangle area within the window

// Function: FPDFDest_GetZoomMode
//			Get the designated zoom mode of a destination
// Parameters:
//			dest		-	Handle to the destination
// Return value:
//			The zoom mode as defined above.
//
__declspec(dllimport)  unsigned long STDCALL FPDFDest_GetZoomMode(FPDF_DEST dest);

// Function: FPDFDest_GetZoomParam
//			Get zoom parameters
// Parameters:
//			dest		-	Handle to the destination
//			param		-	Index of the parameter, starting with zero (see comments below)
// Return value:
//			A float number for the zoom parameter.
// Comments:
//			Different zoom mode has different parameters. Here is a list:
//			XYZ				three parameters: x, y position in the page and the zoom ratio (0 for not specified);
//			FITPAGE			no parameters;
//			FITHORZ			one parameter: the top margin of the page
//			FITVERT			one parameter: the left margin of the page
//			FITRECT			four parameters: the left, top, right, bottom margin of the fitting rectangle.
//							Use 0-3 as parameter index for them, respectively.
//
__declspec(dllimport)  double STDCALL FPDFDest_GetZoomParam(FPDF_DEST dest, int param);

// Function: FPDFLink_GetLinkAtPoint
//			Find a link at specified point on a document page
// Parameters:
//			page		-	Handle to the document page
//			x			-	The x coordination of the point, specified in page coordination system.
//			y			-	The y coordination of the point, specified in page coordination system.
// Return value:
//			Handle to the link. NULL if no link at that point found.
// Comments:
//			The point coordinations are specified in page coordination system. You can convert coordinations 
//			from screen system to page system using FPDF_DeviceToPage functions.
//
__declspec(dllimport)  FPDF_LINK STDCALL FPDFLink_GetLinkAtPoint(FPDF_PAGE page, double x, double y);

// Function: FPDFLink_GetDest
//			Get destination info of a link
// Parameters:
//			document	-	Handle to the document
//			link		-	Handle to the link. Returned by FPDFLink_GetLinkAtPoint
// Return value:
//			Handle to the destination. NULL if there is no destination associated with the link, in this case
//			the application should try FPDFLink_GetAction
//
__declspec(dllimport)  FPDF_DEST STDCALL FPDFLink_GetDest(FPDF_DOCUMENT document, FPDF_LINK link);

// Function: FPDFLink_GetAction
//			Get action info of a link
// Parameters:
//			link		-	Handle to the link.
// Return value:
//			Handle to the action. NULL if there is no action associated with the link.
//
__declspec(dllimport)  FPDF_ACTION STDCALL FPDFLink_GetAction(FPDF_LINK link);

// Function: FPDF_GetThumbnail
//			Get stored thumbnail image of a page
// Parameters:
//			page		-	Handle to a page.
// Return value:
//			Handle to a Windows bitmap storing the thumbnail. NULL if no thumbnail is stored for this page.
// Comments:
//			If no thumbnail is stored for a page, the application can render the page into a small bitmap for thumbnail.
//			The application should free the image with DeleteObject WIN32 function, when it's done with the thumbnail.
//
//			NOTE: This function is not supported on Windows CE.
//
__declspec(dllimport)  HBITMAP STDCALL FPDF_GetPageThumbnail(FPDF_PAGE page);

#ifdef __cplusplus
};
#endif

#endif	// _FPDFDOC_H_