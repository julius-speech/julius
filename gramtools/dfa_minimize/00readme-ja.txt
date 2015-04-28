    dfa_minimize

DFA_MINIMIZE(1)                                                DFA_MINIMIZE(1)



名前
           dfa_minimize
          - 有限オートマトン文法を最小化する

概要
       dfa_minimize [-o outfile] {dfafile}

DESCRIPTION
       dfa_minimize は，.dfa ファイルを等価な最小化の .dfa ファイルに変換し，
       標準出力に出力します．オプション -o で出力先を 指定することもできます．

       バージョン 3.5.3 以降の Julius に付属の mkdfa.pl は， このツールを内部
       で自動的に呼び出すので，出力される .dfa は常に最小化 されており，これを
       単体で実行する必要はありません．バージョン 3.5.2 以前の mkdfa.pl で出力
       された .dfa は最小化されていないので， このツールで最小化するとサイズを
       最適化することができます．

OPTIONS
        -o  outfile
           出力ファイル名を指定する．

EXAMPLES
       foo.dfa を最小化して bar.dfa に 保存する．
       別の方法：

SEE ALSO
        mkdfa.pl ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 京都大学 河原研究室

       Copyright (c) 1997-2000 情報処理振興事業協会(IPA)

       Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室

       Copyright (c) 2005-2013 名古屋工業大学 Julius開発チーム

LICENSE
       Julius の使用許諾に準じます．



                                  19/12/2013                   DFA_MINIMIZE(1)
