HOW TO COMPILE JULIUS ON MSVC / THE JULIUS CLASS
=================================================

This file describes how to compile Julius on Microsoft Visual C++
2008.  A sample application "SampleApp" and the Julius wrapper class
is also included.  See below to see how to compile and test it.

The MSVC support has been developed and tested for MS Visual C++ 2008
on both Professional and Express Edition, on Windows Vista
32bit/64bit and XP.

If you are totally new to Julius, please note that an acoustic model
and a language model is needed to run Julius as speech recognizer.
You should also have a jconf configuration file to specify Julius
models and other option values.  If you don't know what to do, learn
from the Julius Web for details.


1. Preparation
===============

"Microsoft DirectX SDK" is required to compile Julius.
You can get it from the Microsoft Web site.

Also, Julius uses these two open-source libraries:

   - zlib
   - portaudio (V19)

The pre-compiled win32 libraries and header files are already included
under the "zlib" and "portaudio" directory.  If they don't work on
your environment, compile them by yourself and replace the headers and
libraries under each directory.  Also, please place the compiled
portaudio DLL to both "Release" and "Debug" directories.


2. Compile
===========

Open "JuliusLib.sln" with MS VC++, and build it!  You will get
libraries, "julius.exe" and "SampleApp.exe" under "Debug" or "Release"
directory.

If you got an error when linking "zlib" or "portaudio", compile them
by yourself and replace the headers and libraries under each
directory.  Also, please place the compiled portaudio DLL to both
"Release" and "Debug" directories.


3. Test
========

3.1  julius.exe
-----------------

"julius.exe" is a console application, which runs as the same as the
distributed win32 version of Julius.  You can run it from command
prompt with a working jconf file, just the same way as the
pre-compiled win32 version:

    % julius.exe -C xxx.jconf

3.2  SampleApp.exe
-------------------

"SampleApp.exe" is a sample GUI version of Julius which uses a simple
Julius wrapper class and JuliusLib libraries.

At SampleApp main window, open the jconf file you want to run.  Then
the Julius engine will start inside as a separate thread, and will
send messages to the main window at each speech event (trigger,
recognition result, etc.).

If you have some trouble displaying the results, try modifying the
locale setting at line 98 of SampleApp.cpp to match your language
model and re-compile.

The Julius enging output will be stored in a text file "juliuslog.txt".
Please check it if you encounter engine error.


4. The Julius Class
====================

A simple class definition "Julius.cpp" and "Julius.h" is used in
SampleApp.  They defines a wrapper class named "cJulius" that utilizes
JuliusLib functions in Windows messaging style.  You can use it in
your application like this:

-----------------------------------------------------------------
#include "Julius.h"

cJulius julius;

....

// Windows Procedure callback
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch( message ) {
	case WM_CREATE:
	    // start Julius when the main window is created
	    julius.initialize( "fast.jconf" );
	    julius.startProcess( hWnd );
	    break;
	case WM_JULIUS:
            // Julius events
	    switch( LOWORD( wParam ) ) {
		case JEVENT_AUDIO_READY: ...
		case JEVENT_RECOG_BEGIN: ...
		case JEVENT_RESULT_FINAL:....
	    }
	.....
    }
    ...
}
-----------------------------------------------------------------

See SampleApp.cpp and Julius.cpp for details.


5.  About the character codes in the sources
=============================================

The source code of Julius contains Japanese characters at EUC-JP encoding.
If you want to read them in MSVC++, convert them to UTF-8.


6.  History
==============

2010/12 (ver.4.1.5.1)

	Small fix relating the license issue.
	Fix header.

2009/11 (ver.4.1.3)

	INITIAL RELEASE.
