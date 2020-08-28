Juliusのコンパイル方法
=======================

このファイルでは Microsoft Visual Studio 2017 以降における Julius のコン
パイル方法について解説します．また，GUI版サンプルアプリケーションであ
る"SampleApp" と Julius のラッパークラスの定義についても解説します．コ
ンパイルとテストの方法を知りたい方は以下をご覧ください．

本バージョンは Microsoft Visual Studio 2017 で Windows10 にて動作確認
しています．

Julius を新たに使用する場合，音響モデル，言語モデル，および Julius の設
定を記述した jconf ファイルが必要となります．詳細は以下をご覧ください．


1. 準備
========

Juliusを動かすためには音響モデル，言語モデルの2つのモデルと，Julius の
設定を記述した jconf ファイルが必要となります．モデルの仕様，サポート範
囲，使い方の詳細，入手性などについては Julius のウェブページを参照して
ください．なお，ウェブページでは日本語の標準モデルをまとめたディクテー
ションキットを配布しています．

なお，ディクテーションキットを "SampleApp" で使用する場合，jconf に記述
されている -charconv オプションを削除してから使用してください．


2. ビルド
==============

VisualStudio 2017以降で "Julius.sln" をで開き，ビルドしてください．
"Debug" あるいは "Release" ディレクトリの中に "julius.exe", "adintool-gui.exe",
"SampleApp.exe" などのツールが生成されます．


3. テスト
==========

3.1  julius.exe
-----------------

"julius.exe" は Win32 のコンソールアプリケーションです．コマンドプロン
プトで jconf ファイルを指定することで実行することができます．

    % julius.exe -C xxx.jconf

3.2  SampleApp.exe
-------------------

"SampleApp.exe" はシンプルな Julius ラッパークラスと JuliusLib ライブラ
リを使用する Julius のGUI版サンプルアプリケーションです．

使用するには，SampleApp を起動後，メニューから使用したい jconf ファイル
を開き，同じくメニューからエンジンの実行を指定します．Julius は子スレッ
ドとして動作し，音声入力開始や認識結果出力などの各イベントをメインメッ
セージに描画します．

結果の表示に問題がある場合，SampleApp.cpp の98行にあるロケールの設定を，
使用する言語モデルに合わせて変更してコンパイルしなおしてください．

Julius の出力は "juliuslog.txt" に保存されます．もし Julius にエラーが
起きた場合，このファイルをチェックしてください．

なお，SampleApp でディクテーションキットを使用する場合，jconf ファイル
に記述されている -charconv オプションを削除してから使用してください．


4. Julius クラス
=================

SampleApp ではシンプルなクラス定義である "Julius.cpp" と "Julius.h" を
使用しています．これらは Windows のメッセージ形式で JuliusLib の機能を
利用するための "cJulius" というラッパークラスを定義しています．これは以
下のようにアプリケーションで利用することができます．

-----------------------------------------------------------------
#include "Julius.h"

cJulius julius;

....

// Windows Procedure callback
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch( message ) {
	case WM_CREATE:
	    // start Julius when the main window is created
	    julius.initialize( "fast.jconf" );
	    julius.startProcess( hWnd );
	    break;
	case WM_JULIUS:
            // Julius events
	    switch( LOWORD( wParam ) ) {
		case JEVENT_AUDIO_READY: ...
		case JEVENT_RECOG_BEGIN: ...
		case JEVENT_RESULT_FINAL:....
	    }
	.....
    }
    ...
}
-----------------------------------------------------------------

詳細はSampleApp.cppとJulius.cppをご覧ください．


5.  更新履歴
=============
2020/9/2 (ver.4.6)
        VS2017 に合わせて更新
        adintool-gui など他のツールを追加

2016/8/19 (ver.4.4)

	VisualStudio 2013 に合わせて変更．
	PortAudio と zlib のソースを同梱
	adintool を追加

2010/12 (ver.4.1.5.1)

	ライセンス関係の修正
	リードミーの修正
	ヘッダの修正

2009/11 (ver.4.1.3)

	初版
