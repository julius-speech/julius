    adintool

ADINTOOL(1)                                                        ADINTOOL(1)



名前
           adintool
          - 音声波形データの記録・分割・送信・受信ツール

概要
       adintool {-in inputdev} {-out outputdev} [options...]

DESCRIPTION
       adintool は，音声波形データ中の音声区間の検出および記録を連続的に行 う
       ツールです．入力音声に対して零交差数と振幅レベルに基づく音声区間検 出を
       逐次行い，音声区間部分を連続出力します．

       adintool は adinrec の高機能版です．音声データの入力元として，マイク 入
       力・ 音声波形ファイル・標準入力・ネットワーク入力(adinnet サーバー モー
       ド)が選択できます．Julius の -input オプションも 使用可能で，プラグイン
       入力も選択できます．

       出力先として，音声波形ファイル・標準出力・ネットワーク出力(adinnet ク
       ライアントモード)が選択できます．特にネットワーク出力（adinnet クライ
       アントモード）では， julius へネットワーク経由で音声を送信して音声認識
       させることができます．

       入力音声は音声区間ごとに自動分割され，逐次出力されます．音声区間の切 り
       出しには adinrec と同じ，一定時間内の零交差数とパワー（振幅レベル） の
       しきい値を用います．音声区間開始と同時に音声出力が開始されます．出 力と
       してファイル出力を選んだ場合は，連番ファイル名で検出された区間ごと に保
       存します．

       サンプリング周波数は任意に設定可能です．録音形式は 16bit, 1 channel
       で，書き出されるファイル形式は Microsoft WAV 形式です． 既に同じ名前の
       ファイルが存在する場合は上書きされます．

OPTIONS
       Julius の全てのオプションが指定可能である．指定されたもののうち， 音声
       入力に関係するオプションのみ扱われる．以下に，adintool の オプショ
       ン，および有効な Julius オプションを解説する．

   adintool specific options
        -freq  Hz
           音声のサンプリング周波数 (Hz) を指定する．(default: 16,000)

        -in  inputdev
           音声を読み込む入力デバイスを指定する．"mic" でマイク入力， "file"
           でファイル入力, "stdin" で標準入力から音声を読み込む． ファイル入力
           の場合，ファイル名は起動後に出てくるプロンプトに対 して指定する．ま
           た，"adinnet" で adintool は adinnet サーバー となり，adinnet クラ
           イアントから音声データを tcp/ip 経由で 受け取る．ポート番号は 5530
           である（"-inport" で変更可能）．

           入力デバイスは，そのほか Julius の "-input" オプションでも指定可能
           である．その場合，プラグインからの入力も可能である．

        -out  outputdev
           音声を出力するデバイスを指定する．"file" でファイル出力， stdout で
           標準出力へ出力する．ファイルの場合，出力ファイル名は オプション
           "-filename" で与える．出力ファイル 形式は 16bit WAV 形式である． ま
           た，"adinnet" で adintool は adinnet クライアント となり，adinnet
           サーバへ取り込んだ音声データを tcp/ip 経由で 送信できる．送信先ホス
           トは "-server" で指定する． ポート番号は 5530 である（"-port" で変
           更可能）． "vecnet" では，音声入力から抽出した特徴量ベクトルをサー
           バへ送信できる．

        -inport  num
           入力が adinnet の場合 (-in adinnet)，接続を受けるポート番号 を指定
           する．指定しない場合のデフォルトは 5530 である．

        -server  [host] [,host...]
           出力が adinnet の場合 (-out adinnet)，送信先のサーバ名を指定する．
           複数ある場合は，カンマで区切って指定する．

        -port  [num] [,num...]
           出力が adinnet の場合 (-out adinnet)，送信先の各サーバのポート番号
           を指定する．指定しない場合のデフォルトは 5530 である． -server で複
           数のサーバを指定している場合， 全てについて明示的にポート番号を指定
           する必要がある．

        -filename  file
           ファイル出力 (-out file) 時，出力ファイル名を 与える．デフォルトで
           は，検出された音声区間検出ごとに， "file.0000.wav" ,
           "file.0001.wav" ... のように区間ごとに連番で 記録される．番号の初期
           値は 0 である（-startidで 変更可能）．なお，オプション -oneshot 指
           定時は 最初の区間だけが "file" の名前で保存される．

        -startid  number
           ファイル出力時，記録を開始する連番番号の初期値を指定する．（ デフォ
           ルト：0）

        -oneshot
           最初の音声区間が終了したら終了する．

        -nosegment
           入力音声の音声区間検出（無音による区切りと無音区間のスキップ）を 行
           わない．

        -raw
           RAWファイル形式で出力する．

        -autopause
           出力が adinnet の場合（-out adinnet），音声区間が終了するたび に入
           力停止・動作停止状態に移行する．出力先の adinnet サーバか ら動作再
           開信号がくると音声入力を再開する．

        -loosesync
           出力が adinnet （-out adinnet）で複数の出力先サーバへ出力している
           場合，動作停止状態から動作再開信号によって動作を再開する
           際，adintool は すべてのサーバから動作再開信号を受けるまで動作を再
           開しない． このオプションを指定すると，少なくとも１つのサーバから再
           開信号 がくれば動作を再開するようになる．

        -rewind  msec
           入力がマイクのとき，停止状態から動作を再開するとき，停止中から 持続
           して音声入力中だった場合，指定されたミリ秒分だけさかのぼって 録音を
           開始する．

        -paramtype  parameter_type
           出力が vecnet の時（-out vecnet），抽出する特 徴量の型をHTK形式で指
           定する（例:"MFCC_E_D_N_Z"）．

        -veclen  vector_length
           出力が vecnet の時（-out vecnet），出力するベ クトル長（次元数）を
           指定する．

   Concerning Julius options
        -input  {mic|rawfile|adinnet|stdin|netaudio|esd|alsa|oss}
           音声入力ソースを選択する．"-in" の代わりにこちらを使うことも できる
           （最後に指定したほうが優先される）．esd やプラグイン入力が 指定可能
           である．

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

EXAMPLES
       マイクからの音声入力を，発話ごとに "data.0000.wav" から順に記録する：
       巨大な収録音声ファイル "foobar.raw" を，音声区間ごとに
       "foobar.1500.wav" "foobar.1501.wav" ... に分割する：
       ネットワーク経由で音声ファイルを転送する(区間検出なし)：
       マイクからの入力音声を Julius へ送信して認識：

SEE ALSO
        julius ( 1 ) ,
        adinrec ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 京都大学 河原研究室

       Copyright (c) 1997-2000 情報処理振興事業協会(IPA)

       Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室

       Copyright (c) 2005-2013 名古屋工業大学 Julius開発チーム

LICENSE
       Julius の使用許諾に準じます．



                                  19/12/2013                       ADINTOOL(1)
