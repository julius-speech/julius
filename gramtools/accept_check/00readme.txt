    accept_check

ACCEPT_CHECK(1)                                                ACCEPT_CHECK(1)



NAME
           accept_check
          - Check whether a grammar accept / reject given word sequences

SYNOPSIS
       accept_check [-t] [-s spname] [-v] {prefix}

DESCRIPTION
       accept_check is a tool to check whether a sentence can be accepted or
       rejected on a grammar (prefix.dfa and prefix.dict). The sentence should
       be given from standard input. You can do a batch check by preparing all
       test sentence at each line of a text file, and give it as standard
       input of accept_check.

       This tool needs .dfa, .dict and .term files. You should convert a
       written grammar file to generate them by mkdfa.pl.

       A sentence should be given as space-separated word sequence. It may be
       required to add head / tail silence word like sil, depending on your
       grammar. And should not contain a short-pause word.

       When a word belongs to various category in a grammar, accept_check will
       check all the possible sentence patterns, and accept it if any of those
       is acceptable.

OPTIONS
        -t
           Use category name as input instead of word.

        -s  spname
           Short-pause word name to be skipped. (default: "sp")

        -v
           Debug output.

EXAMPLES
       An output for "date" grammar:

           % echo '<s> NEXT SUNDAY </s>' | accept_check date
           Reading in dictionary...
           143 words...done
           Reading in DFA grammar...done
           Mapping dict item <-> DFA terminal (category)...done
           Reading in term file (optional)...done
           27 categories, 143 words
           DFA has 35 nodes and 71 arcs
           -----
           wseq: <s> NEXT SUNDAY </s>
           cate: NS_B (NEXT|NEXT) (DAYOFWEEK|DAYOFWEEK|DAY|DAY) NS_E
           accepted


SEE ALSO
        mkdfa.pl ( 1 ) ,
        generate ( 1 ) ,
        nextword ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 Kawahara Lab., Kyoto University

       Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan

       Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and
       Technology

       Copyright (c) 2005-2013 Julius project team, Nagoya Institute of
       Technology

LICENSE
       The same as Julius.



                                  12/19/2013                   ACCEPT_CHECK(1)
