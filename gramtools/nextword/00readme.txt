    nextword

NEXTWORD(1)                                                        NEXTWORD(1)



NAME
           nextword
          - display next predicted words (in reverse order)

SYNOPSIS
       nextword [-t] [-r] [-s spname] [-v] {prefix}

DESCRIPTION
       Given a partial (part of) sentence from the end, it outputs the next
       words allowed in the specified grammar.

       .dfa, .dict and .term files are needed to execute. They can be
       generated from .grammar and .voca file by mkdfa.pl.

       Please note that the latter part of sentence should be given, since the
       main 2nd pass does a right-to-left parsing.

OPTIONS
        -t
           Input / Output in category name. (default: word)

        -r
           Enter in reverse order

        -s  spname
           the name string of short-pause word to be supressed (default: "sp")

        -v
           Debug output.

EXAMPLES
       Exmple output of a sample grammar "fruit":

           % nextword fruit
           Stat: init_voca: read 36 words
           Reading in term file (optional)...done
           15 categories, 36 words
           DFA has 26 nodes and 42 arcs
           -----
           command completion is disabled
           -----
           wseq > A BANANA </s>
           [wseq: A BANANA </s>]
           [cate: (NUM_1|NUM_1|A|A) FRUIT_SINGULAR NS_E]
           PREDICTED CATEGORIES/WORDS:
                               NS_B (<s> )
                               HAVE (HAVE )
                               WANT (WANT )
                               NS_B (<s> )
                               HAVE (HAVE )
                               WANT (WANT )


SEE ALSO
        mkdfa.pl ( 1 ) ,
        generate ( 1 ) ,
        accept_check ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 Kawahara Lab., Kyoto University

       Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan

       Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and
       Technology

       Copyright (c) 2005-2013 Julius project team, Nagoya Institute of
       Technology

LICENSE
       The same as Julius.



                                  12/19/2013                       NEXTWORD(1)
