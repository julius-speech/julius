    jclient.pl

JCLIENT.PL(1)                                                    JCLIENT.PL(1)



名前
           jclient.pl
          - perl 版サンプルクライアント

概要
       jclient.pl

DESCRIPTION
       Julius に付属のサンプルクライアント "jcontrol" の Perl 版です． モ
       ジュール（サーバ）モードで動く Julius から認識結果を受け取ったり，
       Julius を制御したりできます．

       わずか 57 行の簡単なプログラムです．アプリケーションへ Julius を組み込
       む際の参考になれば幸いです．ご自由にご利用ください。

EXAMPLES
       上記のようにして Julius をモジュールモードで起動した後，jclient.pl を
       起動します．接続するホストのデフォルトは localhost, ポート番号は 10500
       です．変えたい場合はスクリプトの冒頭を書き換えてください．
       音声入力を行えば，イベント内容や結果が jclient.pl 側に送信され， 標準出
       力に出力されます．また，jclient.pl に対してコマンドを入力する （最後に
       Enter を押す）と，Julius にコマンドが送信され，Julius が制御されます．
       コマンドは，仕様書にあるモジュールコマンドを生のまま記述します．

SEE ALSO
        julius ( 1 ) ,
        jcontrol ( 1 )

COPYRIGHT
       jclient.pl は 西村竜一 さん (nisimura@sys.wakayama-u.ac.jp) によって作
       成されました．本プログラムのご利用に関しては，作者は一切の保証をしませ
       ん．各自の責任のもとでご利用ください．

       感想、御意見、御要望などのフィードバックは歓迎いたしますので， 上記メー
       ルアドレス，または下記ホームページへ御連絡ください．

       http://w3voice.jp/



                                  19/12/2013                     JCLIENT.PL(1)
