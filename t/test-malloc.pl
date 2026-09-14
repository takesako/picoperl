sub ok{die"NG $_[1]\n"unless$_[0];print"ok $_[1]\n"}

# libcのmalloc/calloc/realloc/freeに依存しない固定ヒープアロケータ
# (malloc.c)の統合テスト。Perlレベルで実際にたくさんのSVを作っては
# 捨てる(malloc/freeが大量に走る)パターンで、クラッシュせず正しい
# 結果になることを確認する。

# 大量の文字列を作って捨てる(fragmentationやcoalescingの実地テスト)。
for my $i (1..2000) {
    my $s = "x" x ($i % 200 + 1);
    my $len = length($s);
    die "NG length mismatch at $i\n" unless $len == ($i % 200 + 1);
}
print "ok create/discard 2000 strings of varying size without corruption\n";

# 配列の伸縮(realloc相当のパスを繰り返し通す)。
my @a;
push @a, $_ for 1..5000;
ok(@a == 5000, "push 5000 elements (exercises realloc-style growth)");
ok($a[0] == 1 && $a[4999] == 5000, "array contents are correct after growth");
$#a = 10;
ok(@a == 11, "shrinking an array works");
push @a, $_ for 1..5000;
ok(@a == 5011, "growing again after shrinking works (reuses freed memory)");

# ハッシュ(hv.cのSV確保/解放パスも別途経由する)。
my %h;
$h{"key$_"} = $_ * 2 for 1..1000;
ok(scalar(keys %h) == 1000, "hash with 1000 keys");
ok($h{key500} == 1000, "hash value lookup is correct");
delete $h{"key$_"} for 1..500;
ok(scalar(keys %h) == 500, "deleting half the keys works (frees memory)");

# 大きな文字列の繰り返し連結(sv_catpvが内部でreallocを繰り返す)。
my $big = "";
$big .= "0123456789" for 1..2000;
ok(length($big) == 20000, "repeated string concatenation grows correctly (20000 bytes)");

print "ALL TESTS PASSED\n";
