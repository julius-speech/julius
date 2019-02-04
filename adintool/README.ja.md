<!-- markdownlint-disable MD041 -->

[English](README.md) / Japanese

# adintool / adintool-gui

マルチインプット・マルチアウトプットの音声波形データ検出・記録・分割・送受信ツール

## Synopsis

```shell
% adintool -in InputDevice -out OutputDevice [options...]
```

GUI版

```shell
% adintool-gui [options...]
```

## Description

`adintool` は音声入力から音声検出をリアルタイムに行い、セグメント化された音声を
様々に出力することができます。

音声波形入力ソース:

- マイクロフォン
- 音声ファイル
- 標準入力
- ネットワークソケット（adinnet）

音声処理：

- 音声区間検出
- 音声区間分割
- 特徴量抽出
- 連続／単発

出力先：音声波形もしくは特徴量ベクトル

- wavファイル
- ネットワークソケット（adinnet）  Julius 等へ
- ネットワークソケット（vecnet） Julius等へ
- 標準出力
- 出力無し

このツールはJuliusのVADモジュールを使用して音声検出を行います。使用するアルゴリ
ズムとパラメータはJuliusと同一であり、Juliusの動きを再現することができます。

保存オーディオファイルの形式は 16bit モノラルの .wav ファイルです。

`adintool-gui` はGUI版の adintool です。すべての機能は `adintool` と同じですが、
サーバへの接続のみは、起動時に自動で行わず、起動後に `c` キーでマニュアル接続し
ます。また、起動時に引数を指定しなかった場合、`adintool-gui` は `-in mic -out
none` を想定して起動します。

### Prerequisites

マイク入力を用いる場合は実行環境に音声録音デバイスが必要です。複数デバイスがある
場合はデフォルトのデバイスが使用されます。環境変数でデバイスを変更することができ
ます（Juliusと同様に）

### Installing

このツールはJuliusのインストール時に同時に同じ場所にインストールされます。
`adintool-gui` をビルドするには SDL v2 ライブラリが必要です。ビルド時に SDL v2
ライブラリが見つからなければ `adintool-gui` はビルドされません。ビルドするには
configure 前に以下を実行して下さい（Ubuntu）：

```shell
% sudo apt-get install libsdl2-dev
```

## Usage

マイク入力を音声区間ごとに "test0001.wav", "test0002.wav" と順次保存する

```shell
% adintool -in mic -out file -filename test
```

起動後マイクへの最初の1発話だけを test.wav へ保存して終了

```shell
% adintool -in mic -out file -oneshot -filename test.wav
```

音声ファイル "speech.wav" をVADで分割し、逐次 Julius に送付

```shell
% echo speech.wav | adintool -in file -out adinnet -server localhost
```

adinnet 経由で送られてきた音声データを受け取りつつ順次ファイルに保存する

```shell
% adintool -in adinnet -out file -nosegment -filename save
```

`adintool-gui` はキーで以下の操作が行えます。

- `Up/Down` キーで検出レベル閾値を上下
- `c` キーでサーバへ接続・切断をトグル
- `Enter` キーで音声区間検出を強制区切り
- `m` キーで入力のミュート・解除トグル

## Options: audio property

### -freq

サンプリングレート（単位：Hz）(Default: 16000)

### -raw

raw (no header) 形式で出力  (Default: .wav 形式で出力)

## Options: speech detection / segmentation

### -nosegment

音声検出を無効にする。入力全体がく切れの無い１つの有効なセグメントとして処理されます。

### -rewind msec

`-in mic` と `-out adinnet` または `-out vecnet` 指定時、 resume 時に指定された
ミリ秒分だけ録音を巻き戻して再開します。resume時に音声冒頭が切れるときに有効で
す。

### -oneshot

One-shot recording: will exit after the end of first speech segment was
detected.  If not specified, `adintool` will perform successive detection.

## Options: feature vector extraction

### -paramtype parameter_type

`-out vecnet` 指定時、抽出する特徴量の形式を "MFCC_E_D_N_Z" のように HTK
parameter の形式で指定します。

### -veclen vector_length

`-out vecnet` 指定時、抽出する特徴量ベクトルの全体の長さを指定します。

## Options: I/O

### -in InputDevice

(必須) 入力音声デバイスを指定

- `mic`: マイク入力を録音
- `file`: ファイル入力（ファイル名は実行後にプロンプトで聞かれる）- `stdin`: 標準入力（raw形式を想定）
- `adinnet`: adinnet サーバになり音声ストリームを adinnet クライアントから受け取る

### -out OutputDevice

(必須) 出力先を指定

- `file`: wavファイルに保存（ファイル名は `-filename` で指定)
- `stdout`: 標準出力（raw形式）
- `adinnet`: adinnet クライアントになり音声ストリームを adinnet サーバへ送信する。送信先サーバ名は `-server` で指定
- `vecnet`: vecnet クライアントになり特徴量ベクトルを vecnet サーバへ送信する。送信先サーバ名は `-server` で指定
- `none`: 出力無し（処理のみ）

### -filename

`-out file` 指定時、保存ファイル名のベースを指定する。 "foobar" と指定したとき、
連続して検出される音声区間は "foobar.0001.wav", "foobar.0002.wav" のように連番保
存される。番号部分は 0001 から開始するが、この初期値は `-startid` で変更可能。
`-oneshot` が指定されているときは、そのまま指定したファイル名へ保存する。

### -startid number

`-out file` と `-filename` 指定時の連番の初期値を変更する。デフォルトは 0

### -server host[,host,...]

`-out adinnet` 指定時、出力先サーバのホスト名を指定する。複数ある場合はコンマ区切りで指定する。

### -port num

`-out adinnet` 指定時、出力先サーバのポート番号を変更する（デフォルト： 5530）

### -inport num

`-in adinnet` 指定時、listenするポート番号を変更する（デフォルト：5530）

## Options: adinnet synchronization

### -autopause

`-out adinnet` 指定時、このオプションを指定すると音声区間検出が終了するたびに自
動的に一時停止して pause 状態へ移行し、クライアントから resume が送られてくるま
で止まるようになる。

### -loosesync

`-out adinnet` でかつ `-server` で複数サーバーが指定されているとき、`adintool`
が pause 状態になったときの挙動を指定する。`adintool` はサーバから resume が来る
まで待つが、デフォルトではすべてのサーバから resume が来るまで待ち、全サーバの
resumeが揃ってから動作を再開する。これに対して `-loosesync` を指定した場合、同期
せず、どれか1つのサーバから resume が来れば即座に動作を開始するようにすることが
できる。

### Other options (-input, -lv, ...)

音声入力部に Julius のライブラリを用いており、Juliusの音声入力オプションがすべて
指定可能です。環境変数による入力デバイスの選択や、レベル閾値の設定、区間前後の無
音区間マージンの長さの変更、Julius用の jconf ファイルの読み込み、等を指定できま
す。詳しくは　Juliusのマニュアルの音声入力オプションの項を見てください。

## Environment Variables

### ALSADEV

ALSA で音声入力デバイス名を指定 (default: "default")

### AUDIODEV

OSS で音声入力デバイス名を指定 (default: "/dev/dsp")

### PORTAUDIO_DEV

PortAudio で音声入力デバイの番号を指定。起動時にデバイスリストが出力されるのでそ
の中から番号を指定する。

### LATENCY_MSEC

マイク入力のレイテンシをミリ秒で指定。小さくすると遅延は少なくなるが動作が不安定
になる。デフォルト値はデバイス・OSによって自動決定される。

## License

本ツールは Julius と同じオープンソースライセンスを保有しています。詳しくはJulius
のライセンスをご覧ください。