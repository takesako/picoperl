sub ok{die"NG $_[1]\n"unless$_[0];print"ok $_[1]\n"}

# Perlの例外機構(eval/die)はcop.hのJMPENV_PUSH/JMPENV_JUMPマクロ経由で
# 直接setjmp/longjmpに繋がっている(iperlsys.hのPerlProc_setjmp/longjmp、
# uconfig.hのHAS_SIGSETJMPが未定義のためsigsetjmp/siglongjmpではなく
# 素のsetjmp/longjmpが使われる)。ここではPerlレベルのeval/dieを様々な
# パターンで動かし、C側のsetjmp/longjmpによるスタック巻き戻しが
# 正しく行われることを確認する。

# 基本: die は eval に捕まり $@ にメッセージが入る
eval { die "boom\n" };
ok($@ eq "boom\n", "basic eval/die sets \$@");

# 正常終了時は $@ が空になる
eval { 1 };
ok($@ eq "", "\$@ is cleared after a successful eval");

# ネストしたeval: 内側のdieは内側のevalだけに捕まる
my $outer_caught = 0;
eval {
    eval { die "inner\n" };
    ok($@ eq "inner\n", "nested eval catches the inner die");
    $outer_caught = 1;
};
ok($outer_caught && $@ eq "", "outer eval is unaffected by a caught inner die");

# ネストしたeval: 内側で捕まらなかった例外は外側まで伝播する(longjmpが
# 複数のJMPENVフレームを飛び越えて正しい捕捉点まで戻ることの確認)
eval {
    eval {
        die "rethrown\n" if 1;
    };
    die $@ if $@;
};
ok($@ eq "rethrown\n", "an uncaught inner die propagates to the outer eval");

# 深い再帰呼び出しの途中からdieして、Cコールスタックを何段も巻き戻す。
# setjmp/longjmpがCの呼び出し規約通りにスタックを戻せているかの確認。
sub deep_die {
    my $n = shift;
    return deep_die($n - 1) if $n > 0;
    die "deep\n";
}
eval { deep_die(200) };
ok($@ eq "deep\n", "die from 200 levels of recursion is caught correctly");

# localの巻き戻し: dieでスコープを抜けてもlocal化した値が復元される
our $g = "outside";
eval {
    local $g = "inside";
    die "leaving\n";
};
ok($@ eq "leaving\n" && $g eq "outside",
   "local variables are restored correctly when unwinding via die");

# ループ中のlocal + die: 複数フレームのSAVE状態が正しく巻き戻る
our @stack;
eval {
    for my $i (1..5) {
        local $stack[$i] = $i * 10;
        die "loopdie\n" if $i == 3;
    }
};
ok($@ eq "loopdie\n", "die inside a loop with local is caught");
ok(!defined($stack[3]), "the local()-ized array element is restored after die");

# オブジェクト(blessedリファレンス)をdieして、$@経由でそのまま受け取れる
eval {
    die bless({ msg => "obj error" }, "MyError");
};
ok(ref($@) eq "MyError" && $@->{msg} eq "obj error",
   "die with a blessed reference is caught intact");

# 文字列evalの構文エラー: パーサ自体の異常もeval STRINGが捕まえる
eval "this is not valid perl syntax {{{";
ok($@ ne "", "a syntax error in eval STRING sets \$@ instead of crashing");

# $SIG{__DIE__}/$SIG{__WARN__}フック: PERL_MICRO(mg.cのPerl_magic_setsigが
# `#ifndef PERL_MICRO`で丸ごと除外される、fork/killと同じ既存の制限)では
# %SIGへの代入は素のハッシュ代入になるだけで何もフックしない仕様。
# これはこのビルド固有のバグでもsetjmp/longjmpの問題でもなく、upstreamの
# PERL_MICRO自体がそう設計しているため、「クラッシュせず、フックは
# 呼ばれない(ただしdie/evalの例外機構自体は正常に動く)」ことを確認する。
my $die_hook_calls = 0;
{
    local $SIG{__DIE__} = sub { $die_hook_calls++ };
    eval { die "hooked\n" };
}
ok($die_hook_calls == 0 && $@ eq "hooked\n",
   "\$SIG{__DIE__} is a no-op under PERL_MICRO but eval/die itself still works");

my $warn_msg;
{
    local $SIG{__WARN__} = sub { $warn_msg = $_[0] };
    warn "just a warning\n";
}
ok(!defined($warn_msg), "\$SIG{__WARN__} is a no-op under PERL_MICRO (warn doesn't crash)");

print "ALL TESTS PASSED\n";
