    generate‐ngram

GENERATE-NGRAM(1)                                            GENERATE-NGRAM(1)



名前
           generate-ngram
          - N-gram に従って文をランダム生成する

概要
       generate-ngram [options...] {binary_ngram}

DESCRIPTION
       generate-ngram は，与えられた N-gram 確率に従って文をランダム生成する
       ツールです．binary_ngram には， バイナリ形式の N-gram ファイルを指定し
       ます．

OPTIONS
        -n  num
           生成する文数を指定する（デフォルト：10）

        -N
           使用する N-gram の長さを制限する（デフォルト：与えられたモデルで定
           義されている最大値，3-gram なら 3）．

        -bos
           文開始記号を指定する（デフォルト：<s>）

        -eos
           文終了記号を指定する（デフォルト：</s>）

        -ignore
           出力してほしくない単語を指定する（デフォルト：<UNK>）

        -v
           冗長な出力を行う．

        -debug
           デバッグ用出力を行う．

SEE ALSO
        julius ( 1 ) ,
        mkbingram ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 京都大学 河原研究室

       Copyright (c) 1997-2000 情報処理振興事業協会(IPA)

       Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室

       Copyright (c) 2005-2013 名古屋工業大学 Julius開発チーム

LICENSE
       Julius の使用許諾に準じます．



                                  19/12/2013                 GENERATE-NGRAM(1)
