# jcontrol

A sample module client for Julius written in C.

## Synopsis

```shell
% jcontrol HostName [PortNum]
```

## Description

`jcontrol` is a CUI command to demonstrate server-client communication with
Julius. It connects to Julius running in module mode, then send commands and
receive messages.

### Installing

This tools will be installed together with Julius.

## Usage

```shell
% jcontrol localhost
connecting to localhost:10500...done

(read command from stdin)

(output Julius messages to stdout)
```

## Options

### `HostName`

Host name where Julius is running in module mode.

### `PortNum`

Port number (default: 10500)

## Command strings

`jcontrol` can issue command based on **command strings**.  It reads a command
string per line from standard input, the given commands are interpreted, and
then `jcontrol` sends required message to Julius.

Here is the list of all command strings.

### Engine control commands

#### `pause`

Stop Julius and enter into paused status. In paused status, Julius will not run
recognition even if speech input occurs. When this command is issued while
recognition is running, Julius will stop after the recognition has been
finished.

#### `terminate`

Same as `pause`, but discard the current speech input when received
command in the middle of recognition process.

#### `resume`

Restart Julius that has been paused or terminated.

#### `inputparam arg`

Configure how to deal with current speech input, in case grammar has been
changed while recognition is running. `arg` should be one of `TERMINATE`,
`PAUSE` or `WAIT`

#### `version`

Request Julius to return a version description string.

#### `status`

Request Julius to return the system status (active / sleep)

### Grammar handling commands

#### `graminfo`

Request the current recognition process to return information about current
grammars (name etc.).

#### `changegram prefix`

Send the specified grammar to Julius, and request it to switch the whole grammar
to the grammar.  The grammar can be specified by `prefix` which means
`prefix.dfa` and `prefix.dict`.  Valid only when the current recognition process
is grammar-mode or isolated-word-mode.  On isolated-word-mode, the argument
should be the full name of the dictionary file `foobar.dict`, not the prefix.

#### `addgram prefix`

Send the specified grammar to Julius, and request it to add the grammar as a new
grammar.  The grammar files can be specified by `prefix`, which means
`prefix.dfa` and `prefix.dict`.  On isolated-word-mode, the argument should be
the full name of the dictionary file `foobar.dict`, not the prefix.

#### `deletegram gramlist`

Request Julius to delete an existing grammar. `gramlist` should contains
comma-separated list of grammars, each one is either the file prefix name or
number. (The number can be determined from the message sent from Julius at each
time grammar information has changed)

#### `deactivategram gramlist`

Request Julius to temporary de-activate existing grammars. The specified grammar
will still be kept inside Julius, but will not be applied for recognition.
`gramlist` should contains comma-separated list of grammars, each one is either
the file prefix name or number. (The number can be determined from the message
sent from Julius at each time grammar information has changed)

#### `activategram gramlist`

Request Julius to (re)activate the grammars currently being deactivated.
`gramlist` should contains comma-separated list of grammars, each one is either
the file prefix name or number. (The number can be determined from the message
sent from Julius at each time grammar information has changed)

#### `addword grammar_name_or_id dictfile`

Send the `dictfile` to Julius, and Request it to append the words defined in the
dictfile to the grammar.

#### `syncgram`

Request Julius to force updating grammar status now.  By default Julius updates
the internal grammar structure when recognition is idle.  This command will
ensure grammar updates at that time.

### Process management commands

Julius supports multi-model recognition and multi decoding.  When multi decoding
is set up on the Julius server by "-SR" option and has several **recognition
process** running concurrently, the commands from the module client will be
applied to a single "current process".  Below are commands which controls the
current process and multi-decoding issues.

#### `listprocess`

Request Julius to return information about existing recognition processes.

#### `currentprocess procname`

Request Julius to Switch the current process to the process specified by the name.

#### `shiftprocess`

Request Julius to rotate the current process.  At each call the current process
will be changed to the next one.

#### `addprocess jconffile`

Request Julius to load the jconf file as a new recognition process. `jconffile`
should be a jconf file that contains only one set of LM options and one SR
definition. Note that the file path should be local to the server: the argument
is just a path, and the jconf file itself will not be sent to Julius.  When
succeeded in loading, the name of the newly created LM and SR processes will be
the jconffile.

#### `delprocess procname`

Request Julius to delete the specified recognition process from the engine.

#### `deactivateprocess procname`

Request Julius to temporary stop the specified recognition process. The stopped
process will not be executed for the input until activated again.

#### `activateprocess procname`

Request Julius to activate the temporarily stopped process.

## Related tools

- "[jclient.pl](https://github.com/julius-speech/julius/tree/master/jclient-perl)"
  is a perl version of sample client for Julius.

## License

This tool is licensed under the same license with Julius.  See the license term
of Julius for details.
