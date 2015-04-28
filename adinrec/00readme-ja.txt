    adinrec

ADINREC(1)                                                          ADINREC(1)



名前
           adinrec
          - １発話の音声入力データをファイルに記録する

概要
       adinrec [options...] {filename}

DESCRIPTION
       adinrec は，音声区間を一定時間内の零交差数とパワー（振幅レベル）のしき
       い値に基づいて切り出し，ファイルに記録する．デフォルトでは標準デバイス
       を用いてマイク入力から録音するが，-input オプションで デバイスを選択可
       能である．またプラグイン入力も選択できる．

       サンプリング周波数は任意に設定可能である．録音形式は 16bit, 1 channel
       であり，書き出されるファイル形式は Microsoft WAV 形式である． 既に同じ
       名前のファイルが存在する場合は上書きされる．

       ファイル名に "-" を指定すると取り込んだ音声データを標準出力へ出 力す
       る．この場合データ形式は RAW 形式になる．

OPTIONS
       Julius の全てのオプションが指定可能である．指定されたもののうち， 音声
       入力に関係するオプションのみ扱われる．以下に，adinrec 独自の オプション
       と関係する Julius オプションに分けて解説する．

   adinrec specific options
        -freq  Hz
           音声のサンプリング周波数 (Hz) を指定する．(default: 16,000)

        -raw
           RAWファイル形式で出力する．

   Concerning Julius options
        -input  {mic|rawfile|adinnet|stdin|netaudio|esd|alsa|oss}
           音声入力ソースを選択する．音声波形ファイルの場合は fileあるい
           はrawfileを指 定する．起動後にプロンプトが表れるので，それに対して
           ファイ ル名を入力する．adinnet では， adintool などのクライアントプ
           ロセスから音声 データをネットワーク経由で受け取ることができる．
           netaudio はDatLinkのサーバから， stdinは標準入力から音声入力を行
           う． esdは，音声デバイスの共有手段として多くの Linuxのデスクトップ
           環境で利用されている EsounD daemon から入力する．

        -lv  thres
           振幅レベルのしきい値．値は 0 から 32767 の範囲で指定する．
           (default: 2000)

        -zc  thres
           零交差数のしきい値．値は１秒あたりの交差数で指定する． (default:
           60)

        -headmargin  msec
           音声区間開始部のマージン．単位はミリ秒． (default: 300)

        -tailmargin  msec
           音声区間終了部のマージン．単位はミリ秒． (default: 400)

        -zmean
           入力音声ストリームに対して直流成分除去を行う．全ての音声処理の の前
           段として処理される．

        -smpFreq  Hz
           音声のサンプリング周波数 (Hz) を指定する．(default: 16,000)

        -48
           48kHzで入力を行い，16kHzにダウンサンプリングする． これは 16kHz の
           モデルを使用しているときのみ有効である． ダウンダンプリングの内部機
           能は sptk から 移植された． (Rev. 4.0)

        -NA  devicename
           DatLink サーバのデバイス名 (-input netaudio).

        -adport  port_number

           -input adinnet 使用時，接続を受け付ける adinnet のボート番号を指定
           する．(default: 5530)

        -nostrip
           音声取り込み時，デバイスやファイルによっては，音声波形中に振幅 が
           "0" となるフレームが存在することがある．Julius は通常，音声 入力に
           含まれるそのようなフレームを除去する．この零サンプル除去が うまく動
           かない場合，このオプションを指定することで自動消去を 無効化すること
           ができる．

        -C  jconffile
           jconf設定ファイルを読み込む．ファイルの内容がこの場所に展開される．

        -plugindir  dirlist
           プラグインを読み込むディレクトリを指定する．複数の場合は コロンで区
           切って並べて指定する．

ENVIRONMENT VARIABLES
        ALSADEV
           (マイク入力で alsa デバイス使用時) 録音デバイス名を指定する． 指定
           がない場合は "default"．

        AUDIODEV
           (マイク入力で oss デバイス使用時) 録音デバイス名を指定する． 指定が
           ない場合は "/dev/dsp"．

        PORTAUDIO_DEV
           (portaudio V19 使用時) 録音デバイス名を指定する． 具体的な指定方法
           は adinrec の初期化時にログに出力されるので参照のこと．

        LATENCY_MSEC
           Linux (alsa/oss) および Windows で，マイク入力時の遅延時間をミ リ秒
           単位で指定する．短い値を設定することで入力遅延を小さくでき る
           が，CPU の負荷が大きくなり，また環境によってはプロセスやOSの 挙動が
           不安定になることがある．最適な値はOS やデバイスに大きく 依存す
           る．デフォルト値は動作環境に依存する．

SEE ALSO
        julius ( 1 ) ,
        adintool ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 京都大学 河原研究室

       Copyright (c) 1997-2000 情報処理振興事業協会(IPA)

       Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室

       Copyright (c) 2005-2013 名古屋工業大学 Julius開発チーム

LICENSE
       Julius の使用許諾に準じます．



                                  19/12/2013                        ADINREC(1)
