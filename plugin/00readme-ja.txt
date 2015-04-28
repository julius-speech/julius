サンプルプラグイン


◆ プラグインについて

Julius-4.1 よりプラグインが使えるようになりました．
このディレクトリには，プラグインのサンプルソースが含まれています．

プラグインの仕組みや概要，作り方など全般的な事項については，
Juliusbook をご覧ください．
サンプルソースには，関数の仕様がコメントとして書かれています．


◆ ファイル構成

    00readme.txt		このファイル
    plugin_defs.h		プラグイン用定義ヘッダ
    adin_oss.c			音声入力プラグインのサンプル：OSSマイク入力
    audio_postprocess.c		音声後処理プラグインのテンプレート
    fvin.c			特徴量入力プラグインのテンプレート
    feature_postprocess.c	特徴量後処理プラグインのテンプレート
    calcmix.c			ガウス分布集合計算プラグインのサンプル
    Makefile			Linux 用 Makefile


◆ プラグインの仕様とコンパイルについて

プラグインファイルの拡張子は .jpi です．実態は，共有オブジェクトファイ
ルです．Linux や cygwin ですと，以下のようにしてコンパイルできます．

    % gcc -shared -o result.jpi result.c

cygwin でコンパイルしたプラグインを，cygwin 環境無しでも動作させるには，
-mno-cygwin をつけます．

    % gcc -shared -mno-cygwin -o result.jpi result.c

Mac OS X (darwin) では以下のようにコンパイルします．

    % gcc -bundle -flat_namespace -undefined suppress -o result.jpi result.c


◆ Julius にプラグインを読み込ませる方法

Julius のオプション "-plugindir dirname" を使います．dirname にはプラ
グインを置いてあるディレクトリを指定してください．
指定したディレクトリ内にある全ての .jpi ファイルが読み込まれます．

なお，オプションを拡張するプラグインも存在するので，"-plugindir" は
設定のできるだけ最初のほうで指定した方がよいでしょう．


◆ 動作テストその１（result.jpi）

result.c は，認識結果の文字列を受け取って出力する簡単なプラグインです．
Julius をコンパイル後，以下のようにして試してみましょう．

	% cd plugin (このディレクトリ)
	% make result.jpi
	% cd ..
	% ./julius/julius ... -plugindir plugin

なお，Mac OS X では以下のように Makefile.darwin をお使い下さい．

        % make -f Makefile.darwin result.jpi


◆ 動作テストその２（オーディオ入力プラグイン）

adin_oss.c は，OSS API を使った入力の拡張を行うプラグインです．
Julius 本体からは "-input myadin" で選択できます．
Julius をコンパイル後，以下のようにして試してみましょう．

	% cd plugin (このディレクトリ)
	% make adin_oss.jpi
	% cd ..
	% ./julius/julius -plugindir plugin -input myadin

また，音声入力プラグインは adintool や adinrec からも呼び出せます．

	% ./adinrec/adinrec -plugindir plugin -input myadin


