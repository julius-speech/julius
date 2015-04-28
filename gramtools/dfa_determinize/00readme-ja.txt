    dfa_determinize

DFA_DETERMINIZE(1)                                          DFA_DETERMINIZE(1)



名前
           dfa_determinize
          - 有限オートマトン文法を決定化する

概要
       dfa_determinize [-o outfile] {dfafile}

DESCRIPTION
       dfa_determinize は，.dfa ファイルを等価な決定性 .dfa ファイルに変換し，
       標準出力に出力します．オプション -o で出力先を 指定することもできます．

       mkdfa.pl が生成するDFAは常に決定化されており， 通常，mkdfa.pl で作成さ
       れた .dfa ファイルに対して このツールを使う必要はありません．

OPTIONS
        -o  outfile
           出力ファイル名を指定する．

EXAMPLES
       foo.dfa を決定化して bar.dfa に 保存する．
       別の方法：

SEE ALSO
        mkdfa.pl ( 1 ) ,
        dfa_minimize ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 京都大学 河原研究室

       Copyright (c) 1997-2000 情報処理振興事業協会(IPA)

       Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室

       Copyright (c) 2005-2013 名古屋工業大学 Julius開発チーム

LICENSE
       Julius の使用許諾に準じます．



                                  19/12/2013                DFA_DETERMINIZE(1)
