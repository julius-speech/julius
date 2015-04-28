    mkbinhmmlist

MKBINHMMLIST(1)                                                MKBINHMMLIST(1)



名前
           mkbinhmmlist
          - HMMList ファイルをバイナリ形式に変換

概要
       mkbinhmmlist {hmmdefs_file} {HMMList_file} {output_binhmmlist_file}

DESCRIPTION
       mkbinhmmlist は，主にトライフォンとともに使用される HMMList ファイルを
       バイナリ形式に変換します．通常のテキスト形式の代わりにこれを使うことで
       Juliusの起動を高速化することができます．

       変換には，HMMList ファイルのほかに，一緒に使う音響モデル定義ファイル
       hmmdefs_file が必要です（HTK ASCII形式 / Juliusバイナリ形式のどちらも
       可）．

       Julius で使用する際には，通常のテキスト形式と同じく "-hlist" オプション
       で指定します． テキスト形式かバイナリ形式かの判定は Julius 側で自動的に
       行われます．

       バージョン 4.2より音響モデルから抽出した pseudo phone 情報も書きだす よ
       うになりました．これによりJuliusの起動を高速化することができます．た だ
       し作成した音響モデルとセットで使用する必要があります．また，このバー
       ジョン以降の mkbinhmmlist で作成した binhmmlist ファイルは以前のバー
       ジョ ンでは使用できません．

       mkbinhmmlist は gzip 圧縮されたファイルをそのまま読み込めます．

OPTIONS
       hmmdefs_file
           音響モデル定義ファイル．HTK ASCII 形式，あるいはJulius バイナ リ形
           式．

       HMMList_file
           変換対象の HMMList ファイル．

       output_binhmmlist_file
           出力先となるJulius用バイナリ形式HMMListファイル．すでに ある場合は
           上書きされる．

EXAMPLES
       HMMList ファイル logicalTriをバイナリ形式に変換して logicalTri.bin に保
       存する：

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



                                  19/12/2013                   MKBINHMMLIST(1)
