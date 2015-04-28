    mkbingram

MKBINGRAM(1)                                                      MKBINGRAM(1)



NAME
           mkbingram
          - make binary N-gram from ARPA N-gram file

SYNOPSIS
       mkbingram [-nlr forward_ngram.arpa] [-nrl backward_ngram.arpa]
                 [-d old_bingram_file] {output_bingram_file}

DESCRIPTION
       mkbingram is a tool to convert N-gram definition file(s) in ARPA
       standard format to a compact Julius binary format. It will speed up the
       initial loading time of N-gram much faster. It can read gzipped file
       directly.

       From rev.4.0, Julius can deal with forward N-gram, backward N-gram and
       their combinations. So, mkbingram now generates binary N-gram file from
       one of them, or combining them two to produce one binary N-gram.

       When only a forward N-gram is specified, mkbingram generates binary
       N-gram from only the forward N-gram. When using this binary N-gram at
       Julius, it performs the 1st pass with the 2-gram probabilities in the
       N-gram, and run the 2nd pass with the given N-gram fully, with
       converting forward probabilities to backward probabilities by Bayes
       rule.

       When only a backward N-gram is specified, mkbingram generates an binary
       N-gram file that contains only the backward N-gram. The 1st pass will
       use forward 2-gram probabilities that can be computed from the backward
       2-gram using Bayes rule, and the 2nd pass use the given backward N-gram
       fully.

       When both forward and backward N-grams are specified, the 2-gram part
       in the forward N-gram and all backward N-gram will be combined into
       single bingram file. The forward 2-gram will be applied for the 1st
       pass and backward N-gram for the 2nd pass. Note that both N-gram should
       be trained in the same corpus with same parameters (i.e. cut-off
       thresholds), with same vocabulary.

       The character code in binary N-gram can be converted from version 4.2.3
       or later

       The old binary N-gram produced by mkbingram of version 3.x and earlier
       can be used in Julius-4, but you can convert the old version to the new
       version by specifying it as input of current mkbingram by option "-d".

       Please note that binary N-gram file converted by mkbingram of version
       4.0 and later cannot be read by older Julius 3.x.

OPTIONS
        -nlr  forward_ngram.arpa
           Read in a forward (left-to-right) word N-gram file in ARPA standard
           format.

        -nrl  backward_ngram.arpa
           Read in a backward (right-to-left) word N-gram file in ARPA
           standard format.

        -d  old_bingram_file
           Read in a binary N-gram file.

        -swap
           Swap BOS word <s> and EOS word </s> in N-gram.

        -c  from to
           Convert character code in binary N-gram. ("from", "to" are string
           that intend character code)

       output_bingram_file
           binary N-gram file name to output.

EXAMPLES
       Convert a set of forward and backward N-gram in ARPA format into Julius
       binary form:
       Convert a single forward 4-gram in ARPA format into a binary file:
       Convert old binary N-gram file to current format:

SEE ALSO
        julius ( 1 ) ,
        mkbinhmm ( 1 ) ,
        mkbinhmmlist ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 Kawahara Lab., Kyoto University

       Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan

       Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and
       Technology

       Copyright (c) 2005-2013 Julius project team, Nagoya Institute of
       Technology

LICENSE
       The same as Julius.



                                  12/19/2013                      MKBINGRAM(1)
