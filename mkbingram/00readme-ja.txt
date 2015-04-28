    mkbingram

MKBINGRAM(1)                                                      MKBINGRAM(1)



名前
           mkbingram
          - バイナリ N-gram 変換

概要
       mkbingram [-nlr forward_ngram.arpa] [-nrl backward_ngram.arpa]
                 [-d old_bingram_file] {output_bingram_file}

DESCRIPTION
       mkbingram は，ARPA形式の N-gram 定義ファイルをJulius用のバイナリN-gram
       ファイルに変換するツールです．あらかじめ変換しておくことで，Juliusの起
       動を大幅に高速化できます．

       Julius-4より，N-gram は前向き，後ろ向き，あるいは両方を指定できるよう
       になりました．mkbingram でも，どちらか一方だけでバイナリN-gramを作成す
       るこ とができます．また，両方を指定した場合は，それら2つのN-gramは一つ
       のバ イナリN-gramに結合されます．

       前向きN-gramのみが指定されたとき，mkbingram は 前向きN-gramだけからバ
       イナリN-gramを生成します．このバイナリN-gramを使うとき，Julius はその
       中の 2-gram を使って第1パスを行い，第2 パ スではその前向き確率から後向
       きの確率を，ベイズ則に従って算出しながら認識を行います．

       後向きN-gramのみが指定されたとき，mkbingramは後ろ向きN-gramだけからバ
       イナリN-gramを生成します．このバイナリN-gramを使うとき，Julius はその
       中の後向き 2-gram からベイズ則に従って算出しながら第1パスの認識を行い，
       第2パスでは後向き N-gramを使った認識を行います．

       両方が指定されたときは，前向きN-gram中の2-gramと後向きN-gramが統合され
       たバイナリN-gramが生成されます．Juliusではその前向き2-gramで第1パスを
       行い，後向きN-gramで第2パスを行います．なお両 N-gram は同一のコーパス
       から同 一の条件（カットオフ値，バックオフ計算方法等）で学習されてあり，
       同一の語彙を持っている必要があります．

       なお，mkbingram は gzip 圧縮された ARPA ファイルもそのまま読み込めま
       す．

       また，バージョン 4.2.3よりバイナリN-gram内の文字コードの変換が可 能にな
       りました．

       バージョン 3.x 以前で作成したバイナリN-gramは，そのまま 4.0 でも読めま
       す．mkbingram に -d で与えることで，古いバイナリ形式 を新しいバイナリ形
       式に変換することもできます．なお，4.0 以降の mkbingram で作成したバイナ
       リN-gramファイルは3.x 以前のバージョンでは 使えませんのでご注意くださ
       い．

OPTIONS
        -nlr  forward_ngram.arpa
           前向き（left-to-right）のARPA形式 N-gram ファイルを読み込む

        -nrl  backward_ngram.arpa
           後ろ向き（right-to-left）のARPA形式 N-gram ファイルを読み込む

        -d  old_bingram_file
           バイナリN-gramを読み込む（古いバイナリ形式の変換用）

        -swap
           文頭記号 <s> と文末記号 </s> を入れ替える．

        -c  from to
           バイナリN-gram内の文字コードを変換する．（from, toは文字コードを表
           す文字列）

       output_bingram_file
           出力先のバイナリN-gramファイル名

EXAMPLES
       ARPA形式の N-gram をバイナリ形式に変換する（前向き+後ろ向き）：
       ARPA形式の前向き 4-gram をバイナリ形式に変換する（前向きのみ）：
       古いバイナリN-gramファイルを現在の形式に変換する：

SEE ALSO
        julius ( 1 ) ,
        mkbinhmm ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 京都大学 河原研究室

       Copyright (c) 1997-2000 情報処理振興事業協会(IPA)

       Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室

       Copyright (c) 2005-2013 名古屋工業大学 Julius開発チーム

LICENSE
       Julius の使用許諾に準じます．



                                  19/12/2013                      MKBINGRAM(1)
