    nextword

NEXTWORD(1)                                                        NEXTWORD(1)



名前
           nextword
          - DFA 文法で（逆向きに）次単語を予測するツール

概要
       nextword [-t] [-r] [-s spname] [-v] {prefix}

DESCRIPTION
       nextword は，mkdfa.pl によって変換された DFA 文法 上で，与えられた部分
       文に対して接続しうる次単語の集合を出力します．

       実行には .dfa, .dict, .term の各ファイルが必要です． あらかじめ
       mkdfa.pl で生成しておいて下さい．

       ！注意！ mkdfa.pl で出力される文法は，元の 文法と異なり，文の後ろから前
       に向かう逆向きの文法となっています． これは，Julius の第2パスで後ろ向き
       の探索を行うためです． このため，nextword で与える部分文も逆向きとなり
       ます．

OPTIONS
        -t
           単語ではなくカテゴリ名で入力・出力する．

        -r
           単語を逆順に入力する．

        -s  spname
           スキップすべきショートポーズ単語の名前を指定する． (default: "sp")

        -v
           デバッグ出力．

EXAMPLES
       vfr (フィッティングタスク用文法) での実行例：

           % nextword vfr
           Reading in dictionary...done
           Reading in DFA grammar...done
           Mapping dict item <-> DFA terminal (category)...done
           Reading in term file (optional)...done
           42 categories, 99 words
           DFA has 135 nodes and 198 arcs
           -----
           wseq > に して 下さい silE
           [wseq: に して 下さい silE]
           [cate: (NI|NI_AT) SURU_V KUDASAI_V NS_E]
           PREDICTED CATEGORIES/WORDS:
                       KEIDOU_A (派手 地味 )
                       BANGOU_N (番 )
                         HUKU_N (服 服装 服装 )
                      PATTERN_N (チェック 縦縞 横縞 ...)
                         GARA_N (柄 )
                        KANZI_N (感じ )
                          IRO_N (色 )
                        COLOR_N (赤 橙 黄 ...)
           wseq >


SEE ALSO
        mkdfa.pl ( 1 ) ,
        generate ( 1 ) ,
        accept_check ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 京都大学 河原研究室

       Copyright (c) 1997-2000 情報処理振興事業協会(IPA)

       Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室

       Copyright (c) 2005-2013 名古屋工業大学 Julius開発チーム

LICENSE
       Julius の使用許諾に準じます．



                                  19/12/2013                       NEXTWORD(1)
