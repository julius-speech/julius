    mkbinhmm

MKBINHMM(1)                                                        MKBINHMM(1)



名前
           mkbinhmm
          - バイナリ HMM 変換

概要
       mkbinhmm [-htkconf HTKConfigFile] {hmmdefs_file} {binhmm_file}

DESCRIPTION
       mkbinhmm は，HTKのアスキー形式のHMM定義ファイルを，Julius用のバイナ リ
       形式へ変換します．これを使うことで Juliusの起動を高速化することができま
       す．

       この音響モデルの特徴抽出条件を出力ファイルのヘッダに埋め込むことができ
       ます．埋め込むには，学習時に特徴量抽出に用いた HTK Config ファイルを
       "-htkconf" で指定します．ヘッダに抽出条件を埋め込むことで， 認識時に自
       動的に必要な特徴抽出パラメータがセットされるので，便利です．

       入力として，HTKアスキー形式のほかに，既に変換済みのJulius用バイナリHMM
       を与えることもできます．-htkconf と併用すれば， 既存のバイナリHMMに特徴
       量抽出条件パラメータを埋め込むことができます．

       mkbinhmm は gzip 圧縮されたHMM定義ファイルをそのまま読み込めます．

OPTIONS
        -htkconf  HTKConfigFile
           学習時に特徴量抽出に使用したHTK Configファイルを指定する．指定さ れ
           た場合，その中の設定値が出力ファイルのヘッダに埋め込まれる． 入力に
           既にヘッダがある場合上書きされる．

       hmmdefs_file
           変換元の音響モデル定義ファイル (MMF)．HTK ASCII 形式，あるいは
           Julius バイナリ形式．

       hmmdefs_file
           Julius用バイナリ形式ファイルの出力先．

EXAMPLES
       HTK ASCII 形式の HMM 定義をバイナリ形式に変換する：
       HTKの設定ファイル Config の内容をヘッダに書き込んで出力：
       古いバイナリ形式ファイルにヘッダ情報だけ追加する：

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



                                  19/12/2013                       MKBINHMM(1)
