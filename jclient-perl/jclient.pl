#! /usr/bin/perl
use strict;
use IO::Socket;
use IO::Select;

my $host = "localhost";
my $port = 10500;

print STDERR "$host($port) に接続します\n";

# Socketを生成して接続
my $socket;
while(!$socket){
    $socket = IO::Socket::INET->new(PeerAddr => $host,
                                    PeerPort => $port,
                                    Proto    => 'tcp',
                                    );
    if (!$socket){
        printf STDERR "$host($port) の接続に失敗しました\n";
        printf STDERR "再接続を試みます\n";
        sleep 10;
    }
}

print STDERR "$host($port) に接続しました\n";

# バッファリングをしない
$| = 1;
my($old) = select($socket); $| = 1; select($old);

# Selecterを生成
my $selecter = IO::Select->new;
$selecter->add($socket);
$selecter->add(\*STDIN);

# 入力待ち
while(1){
    my ($active_socks) = IO::Select->select($selecter, undef, undef, undef);

    foreach my $sock (@{$active_socks}){
        # Juliusからの出力を表示
        if ($sock == $socket){
            while(<$socket>){
                print;
                last if(/^\./);
            }
	    # 標準入力をJuliusに送信
        }else{
            my $input = <STDIN>;
            # 小文字を大文字に変換
            $input =~ tr/a-z/A-Z/d;

            print $socket $input;
        }
    }
}       

