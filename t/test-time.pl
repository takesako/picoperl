sub ok{die"NG $_[1]\n"unless$_[0];print"ok $_[1]\n"}

# libcのtime/localtimeに依存しない実装(picotime.c)の統合テスト。
# 実RTCが無いためtime()は固定エポックを返す設計。

my $t = time();
ok($t == 0, "time() returns the fixed epoch (0)");

my @gm = gmtime(0);
ok("@gm" eq "0 0 0 1 0 70 4 0 0", "gmtime(0) is 1970-01-01 00:00:00 Thursday");

my @lt = localtime(0);
ok("@lt" eq "@gm", "localtime(0) matches gmtime(0) (no timezone database, treated as UTC)");

# 2024-02-29 12:00:00 UTC (うるう年、2月29日)を検証。
my @leap = localtime(1709208000);
ok($leap[3] == 29 && $leap[4] == 1 && $leap[5] == 124,
    "localtime correctly handles a leap day (2024-02-29)");

print "ALL TESTS PASSED\n";
