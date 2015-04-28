    jcontrol

JCONTROL(1)                                                        JCONTROL(1)



名前
           jcontrol
          - Juliusモジュールモード用のサンプルクライアント

概要
       jcontrol {hostname} [portnum]

DESCRIPTION
       jcontrol は，モジュールモードで動作している julius に接続し，APIを介し
       てコントロールする簡単なコンソールプログラムです．Julius への一時停止
       や再開などのコマンドの送信，および Julius からの認識結果や音声イベント
       の メッセージ受信を行うことができます．

       起動後，jcontrol は，指定ホスト上において「モジュールモード」で動作中
       のJulius に対し，接続を試みます．接続確立後，jcontrol はユーザーからの
       コマンド入力およびメッセージ受信待ち状態となります．

       jcontrol は ユーザーが入力したコマンドを解釈し，対応するAPIコマンドを
       Julius へ送信します．また，Julius から認識結果や入力トリガ情報 など の
       メッセージが送信されてきたときは，その内容を標準出力へ書き出します．

       モジュールモードの仕様については，関連文書をご覧下さい．

OPTIONS
        hostname
           接続先のホスト名

        portnum
           ポート番号（デフォルト：10500）

COMMANDS
       jcontrol は標準入力から1行ずつコマンド文字列を受け取る． コマンドの一覧
       は以下の通り．

   動作制御
       pause
           Juliusの認識動作を中断させ，一時停止状態に移行させる．一時停止状 態
           にあるJuliusは，たとえ音声入力があっても認識処理を行わない． ある区
           間の音声認識処理の途中でこのコマンドを受け取った場合， Julius はそ
           の認識処理が終了した後，一時停止状態に移行する．

       terminate

           pauseと同じく，Juliusの認識動作を中断させ， 一時停止状態に移行させ
           る．ある区間の音声認識処理の途中でこのコ マンドを受け取った場合，そ
           の入力を破棄して即座に一時停止状態に 移行する．

       resume
           Julius を一時停止状態から通常状態へ移行させ，認識を再開させる．

       inputparam arg
           文法切り替え時に音声入力であった場合の入力中音声の扱いを指定．
           "TERMINATE", "PAUSE", "WAIT"のうちいずれかを指定．

       version
           Julius にバージョン文字列を返させる．

       status
           Julius からシステムの状態 (active / sleep) を報告させる．

   文法・単語認識関連
       graminfo
           カレントプロセスが保持している文法の一覧をクライアントへ出力させ
           る．

       changegram prefix
           カレントプロセスの認識文法を "prefix.dfa" と "prefix.dict" に入れ替
           える．カレントプロ セス内の文法は全て消去され，指定された文法に置き
           換わる．

           カレントプロセスが孤立単語認識の場合， "prefix" の変わりに辞書ファ
           イルのみを "filename.dict" の形で指定する．

       addgram prefix
           認識文法として "prefix.dfa" と "prefix.dict" をカレントプロセスに追
           加する．

           カレントプロセスが孤立単語認識の場合， "prefix" の変わりに辞書ファ
           イルのみを "filename.dict" の形で指定する．

       deletegram gramlist
           カレントプロセスから指定された文法を削除する．文法の指定は，文 法名
           （追加時の prefix）か，あるいは Julius から送られる GRAMINFO内にあ
           る文法 ID で指定する．複数の文法を削除したい場合は，文法名もしく
           はIDをカ ンマで区切って複数指定する（IDと文法名が混在してもよい）．

       deactivategram gramlist
           カレントプロセスの指定された文法を一時的に無効にする．無効にされた
           文法は，エンジン内に保持されたまま，認識処理からは一時的に除外され
           る． 無効化された文法は activategram で再び有効化できる．

           文法の指定は，文法名（追加時の prefix）か，あるいはJulius から送ら
           れる GRAMINFO内にある文法 ID で指定する．複 数の文法を指定したい場
           合は，文法名もしくはIDをカンマで区切って 複数指定する（IDと文法名が
           混在してもよい）．

       activategram gramlist
           カレントプロセスで無効化されている文法を有効化する． 文法の指定
           は，文法名（追加時の prefix）か，あるいはJulius から送ら れる
           GRAMINFO内にある文法 ID で指定する．複 数の文法を指定したい場合
           は，文法名もしくはIDをカンマで区切って 複数指定する（IDと文法名が混
           在してもよい）．

       addword grammar_name_or_id dictfile
           dictfile の中身を，カレントプロセスの指定された文法に追加する．

       syncgram
           addgram や deletegram などによる文法の更新を即時に行う． 同期確認用
           である．

   プロセス関連のコマンド
       Julius-4 では複数モデルの同時認識が行える．この場合， 認識プロセス
       ("-SR" で指定された認識処理インスタンス) ごとにモジュールクライアントか
       ら操作を行うことができる．

       クライアントからはどれか一つのプロセスが「カレントプロセス」として 割り
       当てられる．文法関連の命令はカレントプロセスに対して行われる．

       listprocess
           Julius に現在エンジンにある認識プロセスの一覧を送信させる．

       currentprocess procname
           カレントプロセスを指定された名前のプロセスに切り替える．

       shiftprocess
           カレントプロセスを循環切り替えする．呼ばれるたびにその次のプロセス
           に カレントプロセスが切り替わる．

       addprocess jconffile
           エンジンに認識プロセスを新たに追加する．与える jconffile は，通常の
           ものと違い， ただ一種類の LM 設定を含むものである必要がある．ま
           た，実際に送られる のはパス名のみであり，ファイル読み込みはJulius側
           で行われるため， ファイルパスは Julius から見える場所を指定する必要
           が有る．

           追加された LM および認識プロセスは，jconffile の名前が プロセス名と
           なる．

       delprocess procname
           指定された名前の認識プロセスをエンジンから削除する．

       deactivateprocess procname
           指定された名前の認識プロセスを，一時的に無効化する．無効化され たプ
           ロセスは次回以降の入力に対して認識処理からスキップされる． 無効化さ
           れたプロセスは activateprocess で 再び有効化できる．

       activateprocess procname
           指定された名前の認識プロセスを有効化する．

EXAMPLES
       Julius からのメッセージは "> " を行の先頭につけてそのまま標準出力に出力
       されます．以下は実行例です．
       上記のようにして Julius をモジュールモードで起動した後， jcontrol をそ
       のホスト名を指定して起動します．
       音声入力を行えば，イベント内容や結果が jcontrol 側に送信されます．
       jcontrol に対してコマンドを入力する（最後に Enter を押す）と， Julius
       にコマンドが送信され，Julius が制御されます．

       詳しいプロトコルについては，関連文書を参照してください．

SEE ALSO
        julius ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 京都大学 河原研究室

       Copyright (c) 1997-2000 情報処理振興事業協会(IPA)

       Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室

       Copyright (c) 2005-2013 名古屋工業大学 Julius開発チーム

LICENSE
       Julius の使用許諾に準じます．



                                  19/12/2013                       JCONTROL(1)
