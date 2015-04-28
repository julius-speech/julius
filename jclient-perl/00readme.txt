    jclient.pl

JCLIENT.PL(1)                                                    JCLIENT.PL(1)



NAME
           jclient.pl
          - sample client for module mode (perl version)

SYNOPSIS
       jclient.pl

DESCRIPTION
       This is yet another sample client written in perl. It will connect to
       Julius running in module mode, receive recognition results from Julius,
       and cna send commands to control Julius.

       This is a tiny program with only 57 lines. You can use it for free.

EXAMPLES
       Invoke Julius with module mode by specifying "-module" option:
       Then, at other terminal or other host, invoke jclient.pl like below.
       The default hostname is "localhost", and port number is 10500. You can
       change them by editing the top part of the script.
       It will then receive the outputs of Julius and output the raw message
       to standard out. Also, by inputting a raw module command to the
       standard input of jclient.pl, it will be sent to Julius. See manuals
       for the specification of module mode.

SEE ALSO
        julius ( 1 ) ,
        jcontrol ( 1 )

COPYRIGHT
       "jclient.pl" has been developed by Dr. Ryuichi Nisimura
       (nisimura@sys.wakayama-u.ac.jp). Use at your own risk.

       If you have any feedback, comment or request, please contact the E-mail
       address above, or look at the Web page below.

       http://w3voice.jp/



                                  12/19/2013                     JCLIENT.PL(1)
