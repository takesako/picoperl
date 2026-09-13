#!/usr/bin/env perl
use strict;
use warnings;
use PPI;

local $/;
my $src = <STDIN>;
my $doc = PPI::Document->new(\$src) or die "PPI parse error\n";

# comments / POD / __END__
$doc->prune('PPI::Token::Comment');
$doc->prune('PPI::Token::Pod');
$doc->prune('PPI::Statement::End');

# Uncomment to delete __DATA__ too.
# $doc->prune('PPI::Statement::Data');

my$op = qr/^(?:=~|!~|==|!=|<=|>=|=>|\+=|-=|\*=|\/=|%=|\.=|&&|\|\||\*\*|[=<>+\-*%.?:,])$/;

# ヒアドキュメント (<<'EOT' 等) はボディが「その行の直後の物理行」に
# 続くという前提で本文が保存されている。空白/改行の詰め直しをすると
# ヒアドキュメント呼び出し文の直後にあるはずの改行が消え、後続のコードが
# ボディより前に来てしまい (my$x=<<'EOR';print"...";\nbody\nEOR;) 、
# 元のterminatorが見つからなくなって構文が壊れる。安全に判定する
# コストが高いため、ヒアドキュメントを含むファイルでは改行を触る
# 最適化(このファイルの2つのwhitespace関連ループ)を丸ごと無効化する。
my$has_heredoc = @{$doc->find('PPI::Token::HereDoc')||[]} ? 1 : 0;

# whitespace
for my$ws($has_heredoc ? () : @{$doc->find('PPI::Token::Whitespace')||[]}){
    my($p,$n)=($ws->previous_token,$ws->next_token);

    if(!$p||!$n){$ws->delete;next}

    my$del=0;

    # symbolic operators
    if(($p->isa('PPI::Token::Operator')&&$p->content=~$op)||
       ($n->isa('PPI::Token::Operator')&&$n->content=~$op)){
        $del=1 unless $p->isa('PPI::Token::Operator')&&
                      $n->isa('PPI::Token::Operator');
    }

    # ?: and comma can normally be packed tightly
    $del=1 if $p->isa('PPI::Token::Operator')&&$p->content=~/^[?:,]$/;
    $del=1 if $n->isa('PPI::Token::Operator')&&$n->content=~/^[?:,]$/;

    # word operators: $a eq 'x' -> $a eq'x'
    # ただし後続の最初の文字が英数字/アンダースコア/単一引用符だと、結合して
    # 1つの識別子("eqref"等)や旧式パッケージ区切り記号として誤認識される
    # ("eq'foo'"がeq::fooと解釈される等)ため、その場合だけ空白を残す。
    $del=1 if $p->isa('PPI::Token::Operator')&&
              $p->content=~/^(?:eq|ne|lt|le|gt|ge|cmp)$/&&
              $n->content!~/^[A-Za-z0-9_']/;

    # punctuation / structures
    $del=1 if $p->isa('PPI::Token::Structure')||
             ($n->isa('PPI::Token::Structure')&&$n->content=~/^[,;\)\]\}]$/);

    # if (1) -> if(1), foo (...) -> foo(...)
    if($n->isa('PPI::Token::Structure')&&$n->content eq'('){
        my$q=$n->parent;
        $del=1 if $p->isa('PPI::Token::Word')&&
            ($q->isa('PPI::Structure::Condition')||
             $q->isa('PPI::Structure::List')||
             $q->isa('PPI::Structure::For'));
    }

    # for my$x (@a) -> for my$x(@a)
    if($p->isa('PPI::Token::Symbol')&&
       $n->isa('PPI::Token::Structure')&&$n->content eq'('){
        my$q=$n->parent;
        $del=1 if $q->isa('PPI::Structure::List')||
                  $q->isa('PPI::Structure::For');
    }

    # return $x -> return$x
    $del=1 if $p->isa('PPI::Token::Word')&&
              $n->isa('PPI::Token::Symbol');

    # die "error" -> die"error"
    # 安全なのは "..." や `...` のように非英数字の区切り文字で始まる形式
    # だけ。'x' 形式(単一引用符始まり)は bareword'string' の形になり
    # 旧式パッケージ区切り記号 (Foo'bar は Foo::bar と同義) と誤認識され、
    # qq[...]/q(...)/qw(...)/m//のような英字始まりの引用形式は
    # bareword部分がくっついて1つの識別子(warnqq等)になり、どちらも
    # 構文が壊れる。ホワイトリスト方式で本当に安全な場合だけ空白を消す。
    $del=1 if $p->isa('PPI::Token::Word')&&
              $n->isa('PPI::Token::Quote')&&
              $n->content=~/^["`]/;

    $del?$ws->delete:$ws->set_content(' ');
}

# whitespace around structures and final ";" inside them
for my $type($has_heredoc ? () : ('PPI::Structure::Block','PPI::Structure::Condition','PPI::Structure::List')){
    for my $x(@{$doc->find($type)||[]}){
        my $a=$x->first_token;
        $a->delete if $a&&$a->isa('PPI::Token::Whitespace');

        my$z=$x->last_token;
        next unless$z;

        if($z->isa('PPI::Token::Whitespace')){
            $z->delete;
            $z=$x->last_token;
        }

        if($z){
            my$p=$z->previous_token;
            if($p&&($p->isa('PPI::Token::Whitespace')||
                    ($p->isa('PPI::Token::Structure')&&$p->content eq';'))){
                $p->set_content('');
            }
        }

        my$n=$z&&$z->next_token;
        $n->delete if$n&&$n->isa('PPI::Token::Whitespace');
    }
}

# foreach -> for
for my$s(@{$doc->find('PPI::Statement::Compound')||[]}){
    my$t=$s->first_token or next;
    $t->set_content('for')if$t->isa('PPI::Token::Word')&&$t->content eq'foreach';
}

# qw( a   b  c ) -> qw(a b c)
for my$q(@{$doc->find('PPI::Token::QuoteLike::Words')||[]}){
    my$s=$q->content;
    if($s=~/^qw(.)(.*)(.)$/s){
        my($l,$v,$r)=($1,$2,$3);
        $v=~s/^\s+|\s+$//g;
        $v=~s/\s+/ /g;
        $q->set_content("qw$l$v$r");
    }
}

my$out=$doc->serialize;
$out=~s/\s+\z//;
$out=~s/;*\z/;/;
print$out;
