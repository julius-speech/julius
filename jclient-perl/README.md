# jclient.pl

A minimal Julius client example in perl.

## Synopsis

```shell
% jclient.pl
```

## Description

`jclient.pl` is a sample client script written in perl.  It can:

- connects to Julius running in module mode,
- receives recognition results and other event messages, and
- send control commands to Julius

You can learn by practice how to receive messages from Julius or send control
message to Julius.

### Installing

This tools will be installed together with Julius.

## Usage

Run test on local machine.  First, start Julius with module mode as other process.

```shell
% julius ... -module
```

Then run `jclient.pl`.  It will connect to Julius on localhost.

```shell
% jclient.pl
```

Let's speak to Julius. `jclient.pl` will output all the received messages from
Julius like this (The details of this example may differ by versions and option
settings):

```xml:jclient.pl&nbsp;output
<STARTPROC/>
<INPUT STATUS="LISTEN" TIME="994675053"/>
<INPUT STATUS="STARTREC" TIME="994675055"/>
<STARTRECOG/>
<INPUT STATUS="ENDREC" TIME="994675059"/>
<ENDRECOG/>
<INPUTPARAM FRAMES="382" MSEC="3820"/>
<RECOGOUT>
  <SHYPO RANK="1" SCORE="-6888.637695" GRAM="0">
    <WHYPO WORD="silB" CLASSID="39" PHONE="silB" CM="1.000"/>
    <WHYPO WORD="上着" CLASSID="0" PHONE="u w a g i" CM="1.000"/>
    <WHYPO WORD="を" CLASSID="35" PHONE="o" CM="1.000"/>
    <WHYPO WORD="白" CLASSID="2" PHONE="sh i r o" CM="0.988"/>
    <WHYPO WORD="に" CLASSID="37" PHONE="n i" CM="1.000"/>
    <WHYPO WORD="して" CLASSID="27" PHONE="sh i t e" CM="1.000"/>
    <WHYPO WORD="下さい" CLASSID="28" PHONE="k u d a s a i" CM="1.000"/>
    <WHYPO WORD="silE" CLASSID="40" PHONE="silE" CM="1.000"/>
  </SHYPO>
</RECOGOUT>
.
```

`jclient.pl` sends any text given from stdin to Julius.  For example, sending a
string "PAUSE" (type it and press `Enter` at jclient stdin) will tell Julius to
stop audio input, pause recognition and enter waiting mode.

```shell:jclient.pl&nbsp;output
PAUSE
```

Then sending "RESUME" will restart audio input and resume recognition.

```shell:jclient.pl&nbsp;output
RESUME
```

## Options

Modify the header part of the script to change host or port.

## Related tools

- "[jcontrol](https://github.com/julius-speech/julius/tree/master/jcontrol)" is
  a C version of sample client for Julius.

## License

`jclient.pl` was developed by Ryuichi Nishimura (nisimura@sys.wakayama-u.ac.jp)
and contributed to Julius.
