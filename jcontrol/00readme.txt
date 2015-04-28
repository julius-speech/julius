    jcontrol

JCONTROL(1)                                                        JCONTROL(1)



NAME
           jcontrol
          - a sample module client written in C

SYNOPSIS
       jcontrol {hostname} [portnum]

DESCRIPTION
       jcontrol is a simple console program to control julius running on other
       host via network API. It can send command to Julius, and receive
       messages from Julius.

       When invoked, jcontrol tries to connect to Julius running in "module
       mode" on specified hostname. After connection established, jcontrol
       waits for user commands from standard input.

       When user types a command to jcontrol, it will be interpreted and cor-
       responding API command will be sent to Julius. When a message is
       received from Julius, its content will be output to standard output.

       For the details about the API, see the related documents.

OPTIONS
        hostname
           Host name where Julius is runnning in module mode.

        portnum
           port number (default: 10500)

COMMANDS
       jcontrol interprets commands from standard input. Below is a list of
       all commands.

   Engine control
       pause
           Stop Julius and enter into paused status. In paused status, Julius
           will not run recognition even if speech input occurs. When this
           command is issued while recognition is running, Julius will stop
           after the recognition has been finished.

       terminate
           Same as pause, but discard the current speech input when received
           command in the middle of recognition process.

       resume
           Restart Julius that has been paused or terminated.

       inputparam arg
           Tell Julius how to deal with speech input in case grammar is
           changed just when recognition is running. Specify one: "TERMINATE",
           "PAUSE" or "WAIT".

       version
           Tell Julius to send version description string.

       status
           Tell Julius to send the system status (active / sleep)

   Grammar handling
       graminfo
           Tell the current process to send the list of current grammars to
           client.

       changegram prefix
           Send a new grammar "prefix.dfa" and "prefix.dict", and tell julius
           to use it as a new grammar. All the current grammars used in the
           current process of Julius will be deleted and replaced to the
           specifed grammar.

           On isolated word recognition, the dictionary alone should be
           specified as "filename.dict" instead of prefix.

       addgram prefix
           Send a new grammar "prefix.dfa" and "prefix.dict" and add it to the
           current grammar.

           On isolated word recognition, the dictionary alone should be
           specified as "filename.dict" instead of prefix.

       deletegram gramlist
           Tell Julius to delete existing grammar. The grammar can be
           specified by either prefix name or number ID. The number ID can be
           determined from the message sent from Julius at each time grammar
           information has changed. When want to delete more than one grammar,
           specify all of them as comma-sparated.

       deactivategram gramlist
           Tell Julius to de-activate a specified grammar. The specified
           grammar will still be kept but will not be used for recognition.

           The target grammar can be specified by either prefix name or number
           ID. The number ID can be determined from the message sent from
           Julius at each time grammar information has changed. When want to
           delete more than one grammar, specify all of them as
           comma-sparated.

       activategram gramlist
           Tell Julius to activate previously de-activated grammar. The target
           grammar can be specified by either prefix name or number ID. The
           number ID can be determined from the message sent from Julius at
           each time grammar information has changed. When want to delete more
           than one grammar, specify all of them as comma-sparated.

       addword grammar_name_or_id dictfile
           Add the recognition word entries in the specified dictfile to the
           specified grammar on current process.

       syncgram
           Force synchronize grammar status, like unix command "sync".

   Process management
       Julius-4 supports multi-model recognition nad multi decoding. In this
       case it is possible to control each recognition process, as defined by
       "-SR" option, from module client.

       In multi decoding mode, the module client holds "current process", and
       the process commands and grammar related commands will be issued toward
       the current process.

       listprocess
           Tell Julius to send the list of existing recognition process.

       currentprocess procname
           Switch the current process to the process specified by the name.

       shiftprocess
           Rotate the current process. At each call the current process will
           be changed to the next one.

       addprocess jconffile
           Tell Julisu to load a new recognition process into engine. The
           argument jconffile should be a jconf file that contains only one
           set of LM options and one SR definition. Note that the file should
           be visible on the running Julius, since jcontrol only send the path
           name and Julius actually read the jconf file.

           The new LM and SR process will have the name of the jconffile.

       delprocess procname
           Delete the specified recognition process from the engine.

       deactivateprocess procname
           Tell Julius to temporary stop the specified recognition process.
           The stopped process will not be executed for the input until
           activated again.

       activateprocess procname
           Tell Julius to activate the temporarily stopped process.

EXAMPLES
       The dump messages from Julius are output to tty with prefix ">"
       appended to each line. Julius can be started in module mode like this:
       jcontrolcan be launched with the host name:
       It will then receive the outputs of Julius and output the raw message
       to standard out. Also, by inputting the commands above to the standard
       input of jcontrol, it will be sent to Julius. See manuals for the
       specification of module mode.

SEE ALSO
        julius ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 Kawahara Lab., Kyoto University

       Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan

       Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and
       Technology

       Copyright (c) 2005-2013 Julius project team, Nagoya Institute of
       Technology

LICENSE
       The same as Julius.



                                  12/19/2013                       JCONTROL(1)
