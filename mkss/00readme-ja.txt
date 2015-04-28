    mkss

MKSS(1)                                                                MKSS(1)



名前
           mkss
          - スペクトルサブトラクション用のノイズスペクトル計算

概要
       mkss [options...] {filename}

DESCRIPTION
       mkss は，スペクトルサブトラクション用のノイズスペクトル計算ツールです．
       指定時間分の音声のない雑音音声をマイク入力から録音し， その短時間スペク
       トラムの平均を ファイルに出力します．出力されたファイルは，Julius でス
       ペクトル サブトラクションのためのノイズスペクトルファイル（オプション
       "-ssload"）として使用できます．

       録音は起動と同時に開始します．サンプリング条件は16bit signed short (big
       endian), monoral で固定です．既に同じ名前のファイルが存在する場合 は上
       書きします．また，ファイル名に "-" を指定するこ とで標準出力へ出力でき
       ます．

OPTIONS
        -freq  Hz
           音声のサンプリング周波数 (Hz) を指定する．(default: 16,000)

        -len  msec
           録音する時間長をミリ秒単位で指定する（default: 3000）

        -fsize  sample_num
           窓サイズをサンプル数で指定 (default: 400)．

        -fshift  sample_num
           フレームシフト幅をサンプル数で指定 (default: 160)．

SEE ALSO
        julius ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 京都大学 河原研究室

       Copyright (c) 1997-2000 情報処理振興事業協会(IPA)

       Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室

       Copyright (c) 2005-2013 名古屋工業大学 Julius開発チーム

LICENSE
       Julius の使用許諾に準じます．



                                  19/12/2013                           MKSS(1)
