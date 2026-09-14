sub ok{die"NG $_[1]\n"unless$_[0];print"ok $_[1]\n"}

# libcのlocale依存is*系/to*系(__ctype_b_loc経由)に依存しない実装
# (ctype.c)の統合テスト。handy.hのisALPHA_LC/isDIGIT_LC等
# (POSIX文字クラス`[[:alpha:]]`や`\w`/`\d`等が最終的に使う)経由で
# 間接的に検証する。

my $s = "Hello, World! 123";
(my $t = $s) =~ tr/a-zA-Z0-9//cd;
ok($t eq "HelloWorld123", "tr///cd (uses isALNUM_LC) strips non-alnum correctly");

ok(uc("hello") eq "HELLO", "uc (toUPPER_LC) works");
ok(lc("WORLD") eq "world", "lc (toLOWER_LC) works");
ok(ucfirst("hello") eq "Hello", "ucfirst works");
ok(lcfirst("WORLD") eq "wORLD", "lcfirst works");

ok("5" =~ /\d/, "\\d (digit class) matches a digit");
ok("a" =~ /\w/, "\\w (word class) matches a letter");
ok(" " =~ /\s/, "\\s (space class) matches a space");
ok("abc123" =~ /^[[:alnum:]]+$/, "POSIX [[:alnum:]] character class works");
ok(!("!!!" =~ /[[:alnum:]]/), "POSIX [[:alnum:]] correctly excludes punctuation");

print "ALL TESTS PASSED\n";
