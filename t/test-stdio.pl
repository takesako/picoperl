sub ok{die"NG $_[1]\n"unless$_[0];print"ok $_[1]\n"}

# 通常のopen()(sysopenではない)がfopen系(vfs経由)で動くことを確認する。
# このビルド(useperlio=undef)では通常のopen()はPerlIO_open=fopen()に
# 直結しており、POSIXの生fd(open/close/read/write/lseek)やfdopen()を
# 経由しない(sysopenだけが例外で対象外。TODO.md「Phase 5」参照)。

open(my $fh, '>', 'made.txt') or die "open w: $!";
print $fh "hello vfs\n";
close($fh);

open(my $rh, '<', 'made.txt') or die "open r: $!";
my $line = <$rh>;
close($rh);
ok($line eq "hello vfs\n", "open/print/close then open/readline round-trips via vfs");

# ホストの実ファイルシステムには存在しないこと(vfs=ramfsにしか無い)。
ok(!-e "/tmp/made.txt", "sanity: not written to an unrelated real path");

# 追記
open(my $ah, '>>', 'made.txt') or die "open a: $!";
print $ah "more\n";
close($ah);
open($rh, '<', 'made.txt') or die "open r2: $!";
my @lines = <$rh>;
close($rh);
ok(@lines == 2 && $lines[1] eq "more\n", "append mode appends via vfs");

# romfs埋め込みモジュールへの通常openでの読み取り(require経由ではなく
# 直接ファイルとして)。
open(my $lh, '<', 'lib/feature.pm') or die "open lib/feature.pm: $!";
my $first = <$lh>;
close($lh);
ok(defined $first, "regular open() can also read a ROMFS-embedded file directly");

# printf
open(my $ph, '>', 'pf.txt') or die "open pf: $!";
printf $ph "n=%d\n", 7;
close($ph);
open($rh, '<', 'pf.txt') or die "open pf r: $!";
my $pfline = <$rh>;
close($rh);
ok($pfline eq "n=7\n", "printf to a filehandle works via vfs");

print "ALL TESTS PASSED\n";
