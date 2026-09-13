sub ok{die"NG $_[1]\n"unless$_[0];print"ok $_[1]\n"}

# lib/feature.pm はperl-5.12.5本体からコピーした本物 (プレースホルダではない)。
# ROMFS経由でrequire/useされ、実際に say (state/switchと並ぶfeatureの1つ) を
# 有効化できることまで確認する。
use feature 'say';
ok(defined $feature::VERSION, "use feature loads from ROMFS");
ok(exists $INC{'feature.pm'}, "%INC records the ROMFS-loaded module");

# このビルドはuseperlio='undef'でopen($fh,'>',\$scalar)によるSTDOUT
# キャプチャが使えない(PerlIOの:scalarレイヤーが無い)。'say'が使えない
# ままだとこの行自体がコンパイルエラーになるので、実行できて正しい値を
# 返すこと自体をテストする。
ok(say(0.123) && 1, "say (enabled via ROMFS feature.pm) runs without error");

eval { require NoSuchModule::AtAll; };
ok($@ =~ /Can't locate/, "missing module still fails normally (falls through \@INC)");

print "ALL TESTS PASSED\n";
