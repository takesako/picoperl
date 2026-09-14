sub ok{die"NG $_[1]\n"unless$_[0];print"ok $_[1]\n"}

# libcのgetenv/putenvに依存しない自前の環境変数ストア(env.c)経由で
# $ENV{...}の読み書きが動くことを確認する。

$ENV{PICOPERL_TEST_VAR} = "hello";
ok($ENV{PICOPERL_TEST_VAR} eq "hello", "set then get \$ENV{...} round-trips");

$ENV{PICOPERL_TEST_VAR} = "updated";
ok($ENV{PICOPERL_TEST_VAR} eq "updated", "overwriting an existing \$ENV{...} key works");

$ENV{ANOTHER} = "x";
ok($ENV{PICOPERL_TEST_VAR} eq "updated" && $ENV{ANOTHER} eq "x",
    "two different keys don't clobber each other");

print "ALL TESTS PASSED\n";
