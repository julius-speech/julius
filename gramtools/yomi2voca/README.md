# yomi2voca.pl

Japanese Hiragana-to-phoneme converter for making recognition dictionary.

## Synopsis

```shell
% yomi2voca.pl in.txt > out.txt
```

## Description

`yomi2voca.pl` converts Japanese Hiragana text into phoneme sequence.  The
phoneme set is the one used in the models of Japanese dictation kit.

### Prerequisites

The input file should consists of two fields: the first field will be passed to
output as is, and the second field will be converted.  A line starting with
"`%`" will be skipped from conversion.  The character code must be in UTF-8.

### Installing

This tool will be installed together with Julius.

## Usage

An example file "`sample.txt`" that contains :

```text:sample.txt
東京 とーきょー
京都 きょーと
名古屋 なごや
大阪 おーさか
広島 ひろしま
```

Converting it with `yomi2voca.pl`

```shell
% yomi2voca.pl sample.txt > converted.txt
```

The output file "`converted.txt`" should look like:

```text:converted.txt
東京    t o: ky o:
京都    ky o: t o
名古屋  n a g o y a
大阪    o: s a k a
広島    h i r o sh i m a
```

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
