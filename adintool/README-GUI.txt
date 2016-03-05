About adintool-gui

2016/03/05 

----------------
This directory contains two tools:

  - "adintool"     --- the legacy audio input frontend for Julius
  - "adintool-gui" --- GUI version of adintool

In addition to "adintool", "adintool-gui" displays real-time input
waveform with trigger information on screen.  You can directly monitor
the input waveform and how VAD works, and you can also change the
trigger level threshold (-lv) on the fly by up/down key.

----------------
- Compile

To compile "adintool-gui", SDL2 library (Version > 2.0) is required.
On Debian based system, you can install it by:

   % sudo apt-get install libsdl2-dev

----------------
- Test

Start "adintool-gui" with no argument to monitor input.  It will
display the real-time input waveform, trigger threshold (horizontal
yellow line) and show how its VAD is running.  You can adjust the
threshold by up and down arrow key.

----------------
- Usage

You can give the same option of "adintool" to "adintool-gui".  The
differences are:

 - With no argument, "adintool-gui" runs in monitor mode, as desribed
   above.  Actually, the monitor mode is equivalent to giving option
   "-in mic -out none", output nothing.

 - When "-out adinnet" or "-out vecnet" is specified, it DOES NOT
   CONNECT TO THE SERVER AT STARTUP!  You can connect/disconnect to
   the server by pressing 'c' key.  Status "Connect/Disconnect" is
   shown as filled/unfilled box on the top-right corner.

 - Pressing 'Enter' key immediately sends segmentation request to
   server, causing Julius to stop recognition and start again.  It is
   useful when the speaker is too loud and input can not be segmented
   by VAD.

-------------------------------
Keys for adintool-gui:

- 'ESC'	   	exit
- 'UP'/'DOWN'	trigger threshold up/down
- 'c'		output start/stop | server connect/disconnect
- 'm'		mute/unmute
- 'Enter'	force audio segmentation

-------------------------------
Upper-right red box indicator:

  not displayed  = capturing audio but not output
  filled square  = capturing audio and outputing the triggered samples
  line square    = pausing by server request, capturing audio but not output
