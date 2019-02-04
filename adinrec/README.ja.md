<!-- markdownlint-disable MD041 -->

[English](README.md) / Japanese

# adinrec

１発話の音声入力データをファイルに記録する

## Synopsis

```shell
% adinrec [options...] file.wav
```

## Description

`adinrec` はオーディオ入力から音声発話を検出し、ファイルに保存して終了します。

このツールはJuliusのVADモジュールを使用して音声検出を行います。使用するアルゴリ
ズムとパラメータはJuliusと同一であり、Juliusの動きを再現することができます。

保存オーディオファイルの形式は 16bit モノラルの .wav ファイルです。指定された
ファイル名が既にある場合は上書きします。ファイル名が "-" のときは no header
(raw) 形式で標準出力へ出力されます。

### Prerequisites

マイク入力を用いる場合は実行環境に音声録音デバイスが必要です。複数デバイスがある
場合はデフォルトのデバイスが使用されます。環境変数でデバイスを変更することができ
ます（Juliusと同様に）

### Installing

このツールはJuliusのインストール時に同時に同じ場所にインストールされます。

## Usage

16kHz, 16bit モノラルでファイルに記録：

```shell
% adinrec test.wav
```

48kHz で記録

```shell
% adinrec -freq 48000 test.wav
```

adinnet から音声ストリームを受信しながら音声検出と保存を行う。また libfvad ベー
スのVADモジュールを使用する。

```shell
% adinrec -input adinnet -fvad 3 test.wav
```

## Options

### -freq

サンプリングレート（単位：Hz）(Default: 16000)

### -raw

raw (no header) 形式で出力  (Default: .wav 形式で出力)

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