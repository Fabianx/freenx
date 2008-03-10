// FPDFVIEW.H - Header file for FPDFVIEW component
// Copyright (c) 2004-2005 Foxit Software Company, All Right Reserved.

// Revision: 1.4
// Date: 2005-12-15

#ifndef _FPDFVIEW_H_
#define _FPDFVIEW_H_

// Data types
typedef void* FPDF_DOCUMENT;
typedef void* FPDF_PAGE;
typedef void* FPDF_BITMAP;

// String types
// FPDFSDK may use three types of strings: byte string, wide string (UTF-16LE encoded), and platform dependant string
typedef const char* FPDF_BYTESTRING;

typedef const unsigned short* FPDF_WIDESTRING;		// Foxit PDF SDK always use UTF-16LE encoding wide string,
													// each character use 2 bytes (except surrogation), with low byte first.

// For Windows programmers: for most case it's OK to treat FPDF_WIDESTRING as Windows unicode string,
//		 however, special care needs to be taken if you expect to process Unicode larger than 0xffff.
// For Linux/Unix programmers: most compiler/library environment uses 4 bytes for a Unicode character,
//		you have to convert between FPDF_WIDESTRING and system wide string by yourself.

#ifdef _WIN32_WCE
typedef const unsigned short* FPDF_STRING;
#else
typedef const char* FPDF_STRING;
#endif

#ifdef _WIN32
// On Windows system, functions are exported in a DLL
#define DLLEXPORT __declspec( dllexport )
#define STDCALL 
//__stdcall
#else
#define DLLEXPORT
#define STDCALL
#endif

// Exported Functions
#ifdef __cplusplus
extern "C" {
#endif

// Function: FPDF_UnlockDLL
//			Unlock the DLL using license key info received from Foxit
// Parameters: 
//			license_id	-	A string received from Foxit identifying the SDK license
//			unlock_code	-	A string received from Foxit for unlocking the DLL
// Return value:
//			None
// Comments:
//			For SDK evaluators, this function call is not required, then all
//			rendered pages will come with an evaluation mark.
//			For purchased SDK customers, this should be the first function
//			to call before any other functions to be called.
//
DLLEXPORT void STDCALL FPDF_UnlockDLL(const char* license_id, const char* unlock_code);

// Function: FPDF_LoadDocument
//			Open and load a PDF document.
// Parameters: 
//			file_path	-	Path to the PDF file (including extension)
//			password	-	A string used as the password for PDF file. 
//							If no password needed, empty or NULL can be used.
// Return value:
//			A handle to the loaded document. If failed, NULL is returned.
// Comments:
//			Loaded document can be closed by FPDF_CloseDocument.
//			If this function fails, you can use FPDF_GetLastError() to retrieve
//			the reason why it fails.
//
DLLEXPORT FPDF_DOCUMENT	STDCALL FPDF_LoadDocument(FPDF_STRING file_path, 
												  FPDF_BYTESTRING password);

// Function: FPDF_LoadMemDocument
//			Open and load a PDF document from memory.
// Parameters: 
//			data_buf	-	Pointer to a buffer containing the PDF document
//			size		-	Number of bytes in the PDF document
//			password	-	A string used as the password for PDF file. 
//							If no password needed, empty or NULL can be used.
// Return value:
//			A handle to the loaded document. If failed, NULL is returned.
// Comments:
//			The memory buffer must remain valid when the document is open.
//			Loaded document can be closed by FPDF_CloseDocument.
//			If this function fails, you can use FPDF_GetLastError() to retrieve
//			the reason why it fails.
//
DLLEXPORT FPDF_DOCUMENT	STDCALL FPDF_LoadMemDocument(const void* data_buf, 
											int size, FPDF_BYTESTRING password);

#define FPDF_ERR_SUCCESS		0		// no error
#define FPDF_ERR_UNKNOWN		1		// unknown error
#define FPDF_ERR_FILE			2		// file not found or could not be opened
#define FPDF_ERR_FORMAT			3		// file not in PDF format or corrupted
#define FPDF_ERR_PASSWORD		4		// password required or incorrect password
#define FPDF_ERR_SECURITY		5		// unsupported security scheme
#define FPDF_ERR_PAGE			6		// page not found or content error

// Function: FPDF_GetLastError
//			Get last error code when an SDK function failed
// Parameters: 
//			None
// Return value:
//			A 32-bit integer indicating error codes (defined above).
// Comments:
//			If the previous SDK call succeeded, the return value of this function
//			is not defined.
//
DLLEXPORT unsigned long	STDCALL FPDF_GetLastError();

// Function: FPDF_GetDocPermission
//			Get file permission flags of the document.
// Parameters: 
//			document	-	Handle to document. Returned by FPDF_LoadDocument function.
// Return value:
//			A 32-bit integer indicating permission flags. Please refer to PDF Reference for
//			detailed description. If the document is not protected, 0xffffffff will be returned.
//
DLLEXPORT unsigned long	STDCALL FPDF_GetDocPermissions(FPDF_DOCUMENT document);

// Function: FPDF_GetPageCount
//			Get total number of pages in a document.
// Parameters: 
//			document	-	Handle to document. Returned by FPDF_LoadDocument function.
// Return value:
//			total number of pages in the document.
//
DLLEXPORT int STDCALL FPDF_GetPageCount(FPDF_DOCUMENT document);

// Function: FPDF_LoadPage
//			Load a page inside a document.
// Parameters: 
//			document	-	Handle to document. Returned by FPDF_LoadDocument function.
//			page_index	-	Index number of the page. 0 for the first page.
// Return value:
//			A handle to the loaded page. If failed, NULL is returned.
// Comments:
//			Loaded page can be rendered to devices using FPDF_RenderPage function.
//			Loaded page can be closed by FPDF_CloseDocument.
//
DLLEXPORT FPDF_PAGE	STDCALL FPDF_LoadPage(FPDF_DOCUMENT document, int page_index);

// Function: FPDF_GetPageWidth
//			Get page width
// Parameters:
//			page		-	Handle to the page. Returned by FPDF_LoadPage function.
// Return value:
//			Page width (excluding non-displayable area) measured in points.
//			One point is 1/72 inch (around 0.3528 mm)
//
DLLEXPORT double STDCALL FPDF_GetPageWidth(FPDF_PAGE page);

// Function: FPDF_GetPageHeight
//			Get page height
// Parameters:
//			page		-	Handle to the page. Returned by FPDF_LoadPage function.
// Return value:
//			Page height (excluding non-displayable area) measured in points.
//			One point is 1/72 inch (around 0.3528 mm)
//
DLLEXPORT double STDCALL FPDF_GetPageHeight(FPDF_PAGE page);

// Page rendering flags. They can be combined with bit OR
#define FPDF_ANNOT			0x01		// Set if annotations are to be rendered
#define FPDF_LCD_TEXT		0x02		// Set if using text rendering optimized for LCD display
#define FPDF_NO_GDIPLUS		0x04		// Set if you don't want to use GDI+ (for fast rendering with poorer graphic quality)
										// Applicable to desktop Windows systems only.
#define FPDF_DEBUG_INFO		0x80		// Set if you want to get some debug info. 
										// Please discuss with Foxit first if you need to collect debug info.

#ifdef _WIN32
// Function: FPDF_RenderPage
//			Render contents in a page to a device (screen, bitmap, or printer)
//			This function is only supported on Windows system
// Parameters: 
//			dc			-	Handle to device context.
//			page		-	Handle to the page. Returned by FPDF_LoadPage function.
//			start_x		-	Left pixel position of the display area in the device coordination
//			start_y		-	Top pixel position of the display area in the device coordination
//			size_x		-	Horizontal size (in pixels) for displaying the page
//			size_y		-	Vertical size (in pixels) for displaying the page
//			rotate		-	Page orientation: 0 (normal), 1 (rotated 90 degrees clockwise),
//								2 (rotated 180 degrees), 3 (rotated 90 degrees counter-clockwise).
//			flags		-	0 for normal display, or combination of flags defined above
// Return value:
//			None.
//
DLLEXPORT void STDCALL FPDF_RenderPage(HDC dc, FPDF_PAGE page, int start_x, int start_y, int size_x, int size_y,
						int rotate, int flags);
#endif

// Function: FPDF_RenderPageBitmap
//			Render contents in a page to a device independant bitmap
// Parameters: 
//			bitmap		-	Handle to the device independant bitmap (as the output buffer).
//							Bitmap handle can be created by FPDFBitmap_Create function.
//			page		-	Handle to the page. Returned by FPDF_LoadPage function.
//			start_x		-	Left pixel position of the display area in the device coordination
//			start_y		-	Top pixel position of the display area in the device coordination
//			size_x		-	Horizontal size (in pixels) for displaying the page
//			size_y		-	Vertical size (in pixels) for displaying the page
//			rotate		-	Page orientation: 0 (normal), 1 (rotated 90 degrees clockwise),
//								2 (rotated 180 degrees), 3 (rotated 90 degrees counter-clockwise).
//			flags		-	0 for normal display, or combination of flags defined above
// Return value:
//			None.
//
DLLEXPORT void STDCALL FPDF_RenderPageBitmap(FPDF_BITMAP bitmap, FPDF_PAGE page, int start_x, int start_y, 
						int size_x, int size_y, int rotate, int flags);

// Function: FPDF_LoadDocument
//			Close a loaded PDF page.
// Parameters: 
//			page		-	Handle to the loaded page
// Return value:
//			None.
//
DLLEXPORT void STDCALL FPDF_ClosePage(FPDF_PAGE page);

// Function: FPDF_CloseDocument
//			Close a loaded PDF document.
// Parameters: 
//			document	-	Handle to the loaded document
// Return value:
//			None.
//
DLLEXPORT void STDCALL FPDF_CloseDocument(FPDF_DOCUMENT document);

// Function: FPDF_DeviceToPage
//			Convert the screen coordinations of a point to page coordinations.
// Parameters:
//			page		-	Handle to the page. Returned by FPDF_LoadPage function.
//			start_x		-	Left pixel position of the display area in the device coordination
//			start_y		-	Top pixel position of the display area in the device coordination
//			size_x		-	Horizontal size (in pixels) for displaying the page
//			size_y		-	Vertical size (in pixels) for displaying the page
//			rotate		-	Page orientation: 0 (normal), 1 (rotated 90 degrees clockwise),
//								2 (rotated 180 degrees), 3 (rotated 90 degrees counter-clockwise).
//			device_x	-	X value in device coordination, for the point to be converted
//			device_y	-	Y value in device coordination, for the point to be converted
//			page_x		-	Pointer to a double value receiving the converted X value in page coordination
//			page_y		-	Pointer to a double value receiving the converted Y value in page coordination
// Return value:
//			None.
// Comments:
//			The page coordination system has its origin at left-bottom corner of the page, with X axis goes along
//			the bottom side to the right, and Y axis goes along the left side upward. NOTE: this coordination system 
//			can be altered when you zoom, scroll, or rotate a page, however, a point on the page should always have 
//			the same coordination values in the page coordination system. 
//
//			The device coordination system is device dependant. For screen device, its origin is at left-top
//			corner of the window. However this origin can be altered by Windows coordination transformation
//			utilities. You must make sure the start_x, start_y, size_x, size_y and rotate parameters have exactly
//			same values as you used in FPDF_RenderPage() function call.
//
DLLEXPORT void STDCALL FPDF_DeviceToPage(FPDF_PAGE page, int start_x, int start_y, int size_x, int size_y,
						int rotate, int device_x, int device_y, double* page_x, double* page_y);

// Function: FPDF_PageToDevice
//			Convert the screen coordinations of a point to page coordinations.
// Parameters:
//			page		-	Handle to the page. Returned by FPDF_LoadPage function.
//			start_x		-	Left pixel position of the display area in the device coordination
//			start_y		-	Top pixel position of the display area in the device coordination
//			size_x		-	Horizontal size (in pixels) for displaying the page
//			size_y		-	Vertical size (in pixels) for displaying the page
//			rotate		-	Page orientation: 0 (normal), 1 (rotated 90 degrees clockwise),
//								2 (rotated 180 degrees), 3 (rotated 90 degrees counter-clockwise).
//			page_x		-	X value in page coordination, for the point to be converted
//			page_y		-	Y value in page coordination, for the point to be converted
//			device_x	-	Point to an integer value receiving the result X value in device coordination.
//			device_y	-	Point to an integer value receiving the result Y value in device coordination.
// Return value:
//			None
// Comments:
//			See comments of FPDF_DeviceToPage() function.
//
DLLEXPORT void STDCALL FPDF_PageToDevice(FPDF_PAGE page, int start_x, int start_y, int size_x, int size_y,
						int rotate, double page_x, double page_y, int* device_x, int* device_y);

// Fucntion: FPDFBitmap_Create
//			Create a Foxit Device Independant Bitmap (FXDIB)
// Parameters:
//			width		-	Number of pixels in a horizontal line of the bitmap. Must be greater than 0.
//			height		-	Number of pixels in a vertical line of the bitmap. Must be greater than 0.
//			alpha		-	A flag indicating whether alpha channel is used. Non-zero for using alpha, zero for not using.
// Return value:
//			The created bitmap handle, or NULL if parameter error or out of memory.
// Comments:
//			An FXDIB always use 4 byte per pixel. The first byte of a pixel is always double word aligned.
//			Each pixel contains red (R), green (G), blue (B) and optionally alpha (A) values.
//			The byte order is BGRx (the last byte unused if no alpha channel) or BGRA.
//			
//			The pixels in a horizontal line (also called scan line) are stored side by side, with left most
//			pixel stored first (with lower memory address). Each scan line uses width*4 bytes.
//
//			Scan lines are stored one after another, with top most scan line stored first. There is no gap
//			between adjacent scan lines.
//
//			This function allocates enough memory for holding all pixels in the bitmap, but it doesn't 
//			initialize the buffer. Applications can use FPDFBitmap_FillRect to fill the bitmap using any color.
DLLEXPORT FPDF_BITMAP STDCALL FPDFBitmap_Create(int width, int height, int alpha);

// Fucntion: FPDFBitmap_FillRect
//			Fill a rectangle area in an FXDIB
// Parameters:
//			bitmap		-	The handle to the bitmap. Returned by FPDFBitmap_Create function.
//			left		-	The left side position. Starting from 0 at the left-most pixel.
//			top			-	The top side position. Starting from 0 at the top-most scan line.
//			width		-	Number of pixels to be filled in each scan line.
//			height		-	Number of scan lines to be filled.
//			red			-	A number from 0 to 255, identifying the red intensity
//			green		-	A number from 0 to 255, identifying the green intensity
//			blue		-	A number from 0 to 255, identifying the blue intensity
//			alpha		-	(Only if the alpha channeled is used when bitmap created) A number from 0 to 255,
//							identifying the alpha value.
// Return value:
//			None
// Comments:
//			This function set the color and (optionally) alpha value in specified region of the bitmap.
//			NOTE: If alpha channel is used, this function does NOT composite the background with the source color,
//			instead the background will be replaced by the source color and alpha.
//			If alpha channel is not used, the "alpha" parameter is ignored.
DLLEXPORT void STDCALL FPDFBitmap_FillRect(FPDF_BITMAP bitmap, int left, int top, int width, int height, 
									int red, int green, int blue, int alpha);

// Function: FPDFBitmap_GetBuffer
//			Get data buffer of an FXDIB
// Parameters:
//			bitmap		-	Handle to the bitmap. Returned by FPDFBitmap_Create function.
// Return value:
//			The pointer to the first byte of the bitmap buffer.
// Comments:
//			Applications can use this function to get the bitmap buffer pointer, then manipulate any color
//			and/or alpha values for any pixels in the bitmap.
DLLEXPORT void* STDCALL FPDFBitmap_GetBuffer(FPDF_BITMAP bitmap);

// Function: FPDFBitmap_GetWidth
//			Get width of an FXDIB
// Parameters:
//			bitmap		-	Handle to the bitmap. Returned by FPDFBitmap_Create function.
// Return value:
//			The number of pixels in a horizontal line of the bitmap.
DLLEXPORT int STDCALL FPDFBitmap_GetWidth(FPDF_BITMAP bitmap);

// Function: FPDFBitmap_GetHeight
//			Get height of an FXDIB
// Parameters:
//			bitmap		-	Handle to the bitmap. Returned by FPDFBitmap_Create function.
// Return value:
//			The number of pixels in a vertical line of the bitmap.
DLLEXPORT int STDCALL FPDFBitmap_GetHeight(FPDF_BITMAP bitmap);

// Function: FPDFBitmap_Destroy
//			Destroy an FXDIB and release all related buffers
// Parameters:
//			bitmap		-	Handle to the bitmap. Returned by FPDFBitmap_Create function.
// Return value:
//			None.
DLLEXPORT void STDCALL FPDFBitmap_Destroy(FPDF_BITMAP bitmap);

// Funcion: FPDF_SetModuleFolder
//			Set the folder path for module files (like the FPDFCJK.BIN)
// Parameters;
//			module_name	-	Name of the module. Currently please use NULL (0) only
//			folder_name	-	Name of the folder. For example: "C:\\program files\\FPDFVIEW"
// Return value:
//			None.
DLLEXPORT void STDCALL FPDF_SetModulePath(FPDF_STRING module_name, FPDF_STRING folder_name);

#ifdef __cplusplus
};
#endif

#endif // _FPDFVIEW_H_
