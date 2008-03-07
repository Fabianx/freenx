SPLFilter - A universal printer driver for CUPS -> Windows
----------------------------------------------------------

SPLFilter is now in a very early stage and is for testing only!

And it needs a commercial evaluation .DLL to function at all. 
(fpdfview.dll)

Apply *.diff patches to wine (0.9.29) and do:

Have a printer called 'FF' with datatype 'raw', 
which will revert to share/wine/generic.ppd. Or use 'FF' with some other .ppd, 
which supports multiple Resolutions up to the dpi value you want to use.

(This is needed as reference for GDI drawing functions)

Usage: wine SPLFilter.exe

Input file is testdoc.pdf (Use symlink)
Output file is: test.spl (Use symlink)

Then you can do:

kprinter test.spl -> CUPS printer with "RAW" smb windows printer.

Hopefully the printer will print the file.

cu

Fabian, January 2007

PS: Wine does not yet embed fonts in EMF, so you might want to try to set resolution a bit higher.
