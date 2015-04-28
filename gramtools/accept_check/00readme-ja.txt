    accept_check

ACCEPT_CHECK(1)                                                ACCEPT_CHECK(1)



名前
           accept_check
          - 文法における単語列の受理/非受理チェック

概要
       accept_check [-t] [-s spname] [-v] {prefix}

DESCRIPTION
       accept_check は，文法で文の受理・非受理を判定するツールです．文は標準
       入力から与えます．受理すべき文を一行ずつテキストファイルにまとめて書い
       ておき，それをaccept_check の標準入力に与えることで，その文法
       (prefix.dfa および prefix.dict) において目的の文が受理されるかどうかを
       バッチ的にチェックできます．

       実行には .dfa, .dict, .term の各ファイルが必要です． あらかじめ
       mkdfa.pl で生成しておいて下さい．

       対象とする文は，文法の語彙単位(.vocaの第1フィールド)で空白で区切って与
       えます．最初と最後には多くの場合 silB, silE が必要であることに気をつけ
       て下さい．また， ショートポーズ単語は文に含めないでください．

       同一表記の単語が複数ある場合，accept_check はその可能な解釈の全ての組
       み合わせについて調べ，どれか１つのパターンでも受理可能であれば受理，す
       べてのパターンで受理不可能であれば受理不可能とします．

OPTIONS
        -t
           単語ではなくカテゴリ名で入力・出力する．

        -s  spname
           スキップすべきショートポーズ単語の名前を指定する． (default: "sp")

        -v
           デバッグ出力．

EXAMPLES
       vfr (フィッティングタスク用文法) での実行例：

           % accept_check vfr
           Reading in dictionary...done
           Reading in DFA grammar...done
           Mapping dict item <-> DFA terminal (category)...done
           Reading in term file (optional)...done
           42 categories, 99 words
           DFA has 135 nodes and 198 arcs
           -----
           please input word sequence>silB 白 に して 下さい silE
           wseq: silB 白 に して 下さい silE
           cate: NS_B COLOR_N (NI|NI_AT) SURU_V KUDASAI_V NS_E
           accepted
           please input word sequence>


SEE ALSO
        mkdfa.pl ( 1 ) ,
        generate ( 1 ) ,
        nextword ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 京都大学 河原研究室

       Copyright (c) 1997-2000 情報処理振興事業協会(IPA)

       Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室

       Copyright (c) 2005-2013 名古屋工業大学 Julius開発チーム

LICENSE
       Julius の使用許諾に準じます．



                                  19/12/2013                   ACCEPT_CHECK(1)
