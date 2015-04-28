    generate

GENERATE(1)                                                        GENERATE(1)



名前
           generate
          - 文法から文をランダム生成する

概要
       generate [-v] [-t] [-n num] [-s spname] {prefix}

DESCRIPTION
       generate は文法に従って文をランダムに生成します．

       実行には .dfa, .dict, .term の各ファイルが必要です． あらかじめ
       mkdfa.pl で生成しておいて下さい．

OPTIONS
        -t
           単語ではなくカテゴリ名で出力する．

        -n  num
           生成する文の数を指定する (default: 10)

        -s  spname
           生成においてスキップすべきショートポーズ単語の名前を指定する．
           (default: "sp")

        -v
           デバッグ出力．

EXAMPLES
       vfr (フィッティングタスク用文法) での実行例：

           % generate vfr
           Reading in dictionary...done
           Reading in DFA grammar...done
           Mapping dict item <-> DFA terminal (category)...done
           Reading in term file (optional)...done
           42 categories, 99 words
           DFA has 135 nodes and 198 arcs
            -----
           silB やめます silE
           silB 終了します silE
           silB シャツ を スーツ と 統一して 下さい silE
           silB スーツ を カッター と 同じ 色 に 統一して 下さい silE
           silB 交換して 下さい silE
           silB これ を 覚えておいて 下さい silE
           silB 覚えておいて 下さい silE
           silB 戻って 下さい silE
           silB スーツ を シャツ と 統一して 下さい silE
           silB 上着 を 橙 に して 下さい silE


SEE ALSO
        mkdfa.pl ( 1 ) ,
        generate-ngram ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 京都大学 河原研究室

       Copyright (c) 1997-2000 情報処理振興事業協会(IPA)

       Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室

       Copyright (c) 2005-2013 名古屋工業大学 Julius開発チーム

LICENSE
       Julius の使用許諾に準じます．



                                  19/12/2013                       GENERATE(1)
