#!/usr/bin/perl
#
# "<SID> number" is inserted just at the head of "stateinfo" definition
#
# More detail:
#  case 1: direct definition
#             <STATE> short HERE stateinfo(not_macro_ref) ...
#  case 2: macro definition
#             ~s macro HERE stateinfo...
#
$mode = 0;
$id = 0;
while(<>) {
    $op = 0;
    $modstr = "";
    foreach $tok (split(/[ \r\n]+/)) {
	if ($mode == 0) {
	    if ($tok =~ /\~s/i) {
		$mode = 10;
	    }
	    if ($tok =~ /\<STATE\>/i) {
		$mode = 1;
	    }
	} elsif ($mode == 1) {
	    $mode = 2;
	} elsif ($mode == 2) {
	    if (! ($tok =~ /\~s/i)) {
		$tok = "<SID> " . $id . " " . $tok;
		$id++;
		$op = 1;
	    }
	    $mode = 0;
	} elsif ($mode == 10) {
	    $mode = 11;
	} elsif ($mode == 11) {
	    $tok = "<SID> " . $id . " " . $tok;
	    $id++;
	    $op = 1;
	    $mode = 0;
	}
	$modstr .= " " . $tok;
    }
    if ($op != 0) {
	# output the modified version of this line
	print $modstr . "\n";
    } else {
	# output the original to minimize space diff
	print $_;
    }
}

# end of probram
