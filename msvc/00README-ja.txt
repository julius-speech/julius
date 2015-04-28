Juliusのコンパイル方法 / Julius クラス
=======================================

このファイルでは Microsoft Visual C++ 2008 における Julius のコンパイル
方法について解説します．また，GUI版サンプルアプリケーションである
"SampleApp" と Julius のラッパークラスの定義についても解説します．コン
パイルとテストの方法を知りたい方は以下をご覧ください．

サポートする MSVC のバージョンは 2008 のみであり，Professional Edition
および Express Edition でのコンパイルを確認しています．また，Windows
XP, Vista 32bit/64bit で動作確認しています．

Julius を新たに使用する場合，音響モデル，言語モデル，および Julius の設
定を記述した jconf ファイルが必要となります．詳細は以下をご覧ください．


1. 準備
========

1.1 本体
=========

Julius をコンパイルするには "Microsoft DirectX SDK" が必要です．
Microsoft の ウェブサイトから入手してください．

また，Julius は以下の2つのオープンソースのライブラリを使用します．

   - zlib
   - portaudio (V19)

いずれもコンパイル済みの win32 ライブラリとヘッダが "zlib" と
"portaudio" ディレクトリの中に含まれています．これらが正常に動作しない
場合は自身でコンパイルし直し，ヘッダとライブラリを置き換えてください．
また，portaudio をコンパイルした場合は "Release" と"Debug" ディレクトリ
以下の DLL も置き換えてください．

1.2 モデル
===========

Juliusを動かすためには音響モデル，言語モデルの2つのモデルと，Julius の
設定を記述した jconf ファイルが必要となります．モデルの仕様，サポート範
囲，使い方の詳細，入手性などについては Julius のウェブページを参照して
ください．なお，ウェブページでは日本語の標準モデルをまとめたディクテー
ションキットを配布しています．

なお，ディクテーションキットを "SampleApp" で使用する場合，jconf に記述
されている -charconv オプションを削除してから使用してください．


2. コンパイル
==============

"JuliusLib.sln" を MS VC++ で開き，ビルドしてください．"Debug" か
"Release" ディレクトリの中に "julius.exe" と "SampleApp.exe" が生成され
ます．

"zlib" か "portaudio" のリンク時にエラーが起きた場合は自身でコンパイル
し直し，ヘッダとライブラリを置き換えてください．また，portaudio をコン
パイルした場合は "Release" と"Debug" ディレクトリ以下の DLL も置き換え
てください．


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


5.  ソース内の文字コードについて
=================================

Julius のソースコードでは日本語の文字を EUC-JP で記述しています．もしそ
れらを MSVC++ で読みたい場合，UTF-8 へ変換してください．


6.  更新履歴
=============

2010/12 (ver.4.1.5.1)

	ライセンス関係の修正
	リードミーの修正
	ヘッダの修正

2009/11 (ver.4.1.3)

	初版
