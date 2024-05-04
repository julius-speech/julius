======================================================================
                  Large Vocabulary Continuous Speech
                          Recognition Engine

                                Julius

                                                (Rev 4.6   2020/09/02)
                                                (Rev 4.5   2019/01/02)
                                                (Rev 4.4.2 2016/09/12)
                                                (Rev 4.4   2016/08/30)
                                                (Rev 4.3.1 2014/01/15)
                                                (Rev 4.3   2013/12/25)
                                                (Rev 4.2.3 2013/06/30)
                                                (Rev 4.2.2 2012/08/01)
                                                (Rev 4.2.1 2011/12/25)
                                                (Rev 4.2   2011/05/01)
                                                (Rev 4.1.5 2010/06/04)
                                                (Rev 4.1   2008/10/03)
                                                (Rev 4.0.2 2008/05/27)
                                                (Rev 4.0   2007/12/19)
                                                (Rev 3.5.3 2006/12/29)
                                                (Rev 3.4.2 2004/04/30)
                                                (Rev 2.0   1999/02/20)
                                                (Rev 1.0   1998/02/20)

 Copyright (c) 1991-2020 京都大学 河原研究室
 Copyright (c) 1997-2000 情報処理振興事業協会(IPA)
 Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室
 Copyright (c) 2005-2020 名古屋工業大学 Julius開発チーム
 All rights reserved
======================================================================

Julius について
=================

Julius は，音声認識システムの開発・研究のためのオープンソースの高性能
な汎用大語彙連続音声認識エンジンです．数万語彙の連続音声認識を一般のPC
上でほぼ実時間で実行できます．また，高い汎用性を持ち，発音辞書や言語モ
デル・音響モデルなどの音声認識の各モジュールを組み替えることで，様々な
幅広い用途に応用できます． 動作プラットフォームは Linux, Windows, iOS,
Android, その他の環境です．


What's new in Julius-4.6
==========================

Julius-4.6 はマイナーリリースです。主な変更点は以下のとおりです。

- DNN-HMM 計算での CUDA サポート (Linux + CUDA-8,9,10 でのみ動作確認)
- 1パス文法認識の実装
- DNN-HMM で出力が log10 化されていないモデルのサポート
- 特徴量正規化モードを追加：平均は入力自身、分散は固定値を使うモード
- Visual Studio 2017 でのビルド全面対応 (msvc/Julius.sln)
- 不具合の修正
- 修正BSDライセンスへ移行

全ての変更の詳細は同梱の Release-ja.txt をご覧ください。


UTF-8への移行について
======================

バージョン4.5以前はテキストエンコーディングとして SJISや EUC が混在していましたが、
バージョン4.5から全て UTF-8 に変換されました．

バージョン4.5の時点でのテキストエンコード変換前のコードを
"master-4.5-legacy" ブランチで保存してあります．4.5 リリース以前の
コードから 4.5 までの差分を見る場合はそちらのブランチを checkout して
ください．


Julius-4.6 のファイルの構成
=============================

        00readme-ja.txt		最初に読む文書（このファイル）
        LICENSE.txt		ライセンス条項
        Release-ja.txt		リリースノート/変更履歴
        00readme-DNN.txt	DNN-HMM の使い方説明
        configure		configureスクリプト
        configure.in
        Sample.jconf		jconf 設定ファイルサンプル
        Sample.dnnconf		DNN 設定ファイルのサンプル
        julius/			Julius ソース
        libjulius/		JuliusLib コアエンジンライブラリ ソース
        libsent/		JuliusLib 汎用ライブラリ ソース
        adinrec/		録音ツール adinrec
        adintool/		音声録音/送受信ツール adintool
        generate-ngram/		N-gram文生成ツール
        gramtools/		文法作成ツール群
        jcontrol/		サンプルネットワーククライアント jcontrol
        mkbingram/		バイナリN-gram作成ツール mkbingram
        mkbinhmm/		バイナリHMM作成ツール mkbinhmm
        mkgshmm/		GMS用音響モデル変換ツール mkgshmm
        mkss/			ノイズ平均スペクトル算出ツール mkss
        support/		開発用スクリプト
        jclient-perl/		A simple perl version of module mode client
        plugin/			プラグインソースコードのサンプルと仕様文書
        man/			マニュアル類
        msvc/			Microsoft Visual Studio 2013 用ファイル
        dnntools/		Sample programs for dnn and vecnet client
        binlm2arpa/		バイナリN-gramからARPAへの変換ツール


ライセンスおよび引用
=====================

Juliusのコードは modified BSD License (BSD-3-Clause License) のもとで公開されています．

上記ライセンスによる利用条件のほか、本ソフトウェアを利用して得られた知見に関して発表を行な
う際には、「大語彙連続音声認識エンジン Julius」を利用したことを明記し、可能であれば
適切な参照あるいは引用を示すことを強くお勧めします．このようにしていただくことで、読者が
Julius の情報へ容易にアクセスできるようになるほか、Julius の利用様態の可視化が促進され、
Juliusおよび関連ソフトウェアの今後の開発・拡張につながります．
参照は、Juliusのホームページ（https://julius.osdn.jp) あるいは GitHub のページ
（https://github.com/julius-speech/julius) へリンクしてください．
文献における引用は、下記のJulius に関する論文を引用いただくか、

    A. Lee, T. Kawahara and K. Shikano. "Julius --- An Open Source Real-Time Large Vocabulary
    Recognition Engine".  In Proc. EUROSPEECH, pp.1691--1694, 2001.

    A. Lee and T. Kawahara. "Recent Development of Open-Source Speech Recognition Engine Julius"
    Asia-Pacific Signal and Information Processing Association Annual Summit and Conference
    (APSIPA ASC), 2009.

あるいはこのソフトウェアを直接下記の要領で引用いただくか

    A. Lee and T. Kawahara: Julius v4.5 (2019) https://doi.org/10.5281/zenodo.2530395

あるいは両方をご使用ください．


GitHub への移行について
========================

Julius のコード開発は2016年より GitHub へ移行しました．開発者向けの
最新のソースコード・各種実行キット・開発情報の公開・共有および
フォーラム運営は GitHub にて行っています．

        Julius on GitHub
        https://github.com/julius-speech/julius

ホームページには一般向けのお知らせやキットのリンク、日本語の情報等が
掲載されます．こちらもご活用下さい．

        旧 Julius Web サイト
        http://julius.osdn.jp/


連絡先
===========

Julius 開発に関するご質問・お問い合わせは主に GitHub 上で承っております．

        Julius on GitHub
        https://github.com/julius-speech/julius

以上
