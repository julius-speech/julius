    mkdfa.pl

MKDFA.PL(1)                                                        MKDFA.PL(1)



NAME
           mkdfa.pl
          - grammar compiler

SYNOPSIS
       mkdfa.pl [options...] {prefix}

DESCRIPTION
       mkdfa.pl compiles the Julian format grammar (.grammar and .voca) to
       Julian native formats (.dfa and .dict). In addition, ".term" will be
       also generated that stores correspondence of category ID used in the
       output files to the source category name.

       prefix should be the common file name prefix of ".grammar" and "voca"
       file. From prefix.grammar and prefix.voca file, prefix.dfa, prefix.dict
       and prefix.term will be output.

OPTIONS
        -n
           Not process dictionary. You can only convert .grammar file to .dfa
           file without .voca file.

ENVIRONMENT VARIABLES
        TMP or TEMP
           Set directory to store temporal file. If not specified, one of them
           on the following list will be used: /tmp, /var/tmp, /WINDOWS/Temp,
           /WINNT/Temp.

EXAMPLES
       Convert a grammar foo.grammar and foo.voca to foo.dfa, foo.voca and
       foo.term.

SEE ALSO
        julius ( 1 ) ,
        generate ( 1 ) ,
        nextword ( 1 ) ,
        accept_check ( 1 ) ,
        dfa_minimize ( 1 )

DIAGNOSTICS
       mkdfa.pl invokes mkfa and dfa_minimize internally. They should be
       placed at the same directory as mkdfa.pl.

COPYRIGHT
       Copyright (c) 1991-2013 Kawahara Lab., Kyoto University

       Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan

       Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and
       Technology

       Copyright (c) 2005-2013 Julius project team, Nagoya Institute of
       Technology

LICENSE
       The same as Julius.



                                  12/19/2013                       MKDFA.PL(1)
