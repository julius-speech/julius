HOW TO COMPILE JULIUS ON MSVC
=================================================

This file describes how to compile Julius on Microsoft Visual Studio
2017 and later.  A sample application "SampleApp" and the Julius wrapper class
is also included.  See below to see how to compile and test it.

This version was tested on Microsoft Visual Studio 2017 on Windows 10.

If you are totally new to Julius, please note that an acoustic model
and a language model is needed to run Julius as speech recognizer.
You should also have a jconf configuration file to specify Julius
models and other option values.  If you don't know what to do, learn
from the Julius Web for details.

From ver.4.4:, the PortAudio and zlib library has been also included in 
the source archive, so you can build Julius without any extra source.


1. Build
------------

Just open "Julius.sln" with Visual Studio, choose configuration from
menu either "Release" or "Debug", then build the solution!  You will
get libraries, "julius.exe", "adintool-gui.exe", "SampleApp.exe" and
other tools under "Release" or "Debug" directories, respectively.


2  SampleApp.exe
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


3. The Julius Class
--------------------

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



4.  History
-------------

2020/9/2 (ver.4.6)
        updated for VS2017
        added other tools, including adintool-gui.

2016/8/19 (ver.4.4)

	updated for VS2013.
	included PortAudio and zlib sources.
	added adintool.

2010/12 (ver.4.1.5.1)

	Small fix relating the license issue.
	Fix header.

2009/11 (ver.4.1.3)

	INITIAL RELEASE.
