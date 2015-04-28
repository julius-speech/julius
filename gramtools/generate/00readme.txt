    generate

GENERATE(1)                                                        GENERATE(1)



NAME
           generate
          - random sentence generator from a grammar

SYNOPSIS
       generate [-v] [-t] [-n num] [-s spname] {prefix}

DESCRIPTION
       This small program randomly generates sentences that are acceptable by
       the given grammar.

       .dfa, .dict and .term files are needed to execute. They can be
       generated from .grammar and .voca file by mkdfa.pl.

OPTIONS
        -t
           Output in word's category name.

        -n  num
           Set number of sentences to be generated (default: 10)

        -s  spname
           the name string of short-pause word to be supressed (default: "sp")

        -v
           Debug output mode.

EXAMPLES
       Exmple output of a sample grammar "fruit":

           % generate fruit
           Stat: init_voca: read 36 words
           Reading in term file (optional)...done
           15 categories, 36 words
           DFA has 26 nodes and 42 arcs
           -----
            <s> I WANT ONE APPLE </s>
            <s> I WANT TEN PEARS </s>
            <s> CAN I HAVE A PINEAPPLE </s>
            <s> I WANT ONE PEAR </s>
            <s> COULD I HAVE A BANANA </s>
            <s> I WANT ONE APPLE PLEASE </s>
            <s> I WANT NINE APPLES </s>
            <s> NINE APPLES </s>
            <s> I WANT ONE PINEAPPLE </s>
            <s> I WANT A PEAR </s>


SEE ALSO
        mkdfa.pl ( 1 ) ,
        generate-ngram ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 Kawahara Lab., Kyoto University

       Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan

       Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and
       Technology

       Copyright (c) 2005-2013 Julius project team, Nagoya Institute of
       Technology

LICENSE
       The same as Julius.



                                  12/19/2013                       GENERATE(1)
