#!/usr/bin/env perl
use strict;
use warnings;
use PPI;

local $/;
my$src=<STDIN>;
my$doc=PPI::Document->new(\$src)or die"PPI parse error\n";

$doc->prune('PPI::Token::Comment');
$doc->prune('PPI::Token::Pod');
$doc->prune('PPI::Statement::End');
# $doc->prune('PPI::Statement::Data');

my$op=qr/^(?:=~|!~|<=>|==|!=|<=|>=|=>|<<=?|>>=?|\|\|=?|&&=?|\/\/=?|\*\*=?|\+=|-=|\*=|\/=|%=|\.=|&=|\|=|\^=|->|\+\+|--|\.{2,3}|~~|[=<>+\-*\/%\.?:,&|^~!])$/;

# Do not join tokens into another operator.
sub clash{
    my($p,$n)=@_;
    my$s=substr($p->content,-1).substr($n->content,0,1);
    $s=~/^(?:\+\+|--|\*\*|&&|\|\||<<|>>|=~|!~|\/\/|\.\.|=>|<=|>=|==|!=|\+=|-=|\*=|\/=|%=|\.=|&=|\|=|\^=|->|::|~~|<>)$/;
}

# Preserve only the newline required to start each heredoc body.
# Heredoc bodies and terminators themselves are never modified here.
my$heredocs=$doc->find('PPI::Token::HereDoc')||[];
for my$h(@$heredocs){
    my$t=$h;
    while($t=$t->next_token){
        next unless$t->isa('PPI::Token::Whitespace')&&$t->content=~/\n/;
        $t->{_heredoc_nl}=1;
        $t->set_content("\n");
        last;
    }
}

for my$ws(@{$doc->find('PPI::Token::Whitespace')||[]}){
    next if$ws->{_heredoc_nl};

    my($p,$n)=($ws->previous_token,$ws->next_token);
    if(!$p||!$n){$ws->delete;next}

    my$del=0;

    # symbolic operators
    if(($p->isa('PPI::Token::Operator')&&$p->content=~$op)||
       ($n->isa('PPI::Token::Operator')&&$n->content=~$op)){
        $del=1 unless clash($p,$n)||
                      ($n->isa('PPI::Token::Operator')&&
                       $n->content eq'.'&&
                       $p->isa('PPI::Token::Number'));
    }

    # word operators
    # $a eq 'x' -> $a eq'x'
    $del=1 if$p->isa('PPI::Token::Operator')&&
              $p->content=~/^(?:eq|ne|lt|le|gt|ge|cmp)$/&&
              $n->content!~/^[A-Za-z0-9_]/;

    # $a and (...) -> $a and(...)
    $del=1 if$p->isa('PPI::Token::Operator')&&
              $p->content=~/^(?:and|or|xor|not|x|x=)$/&&
              $n->content!~/^[A-Za-z0-9_']/;

    # punctuation / structures
    $del=1 if$p->isa('PPI::Token::Structure')||
              ($n->isa('PPI::Token::Structure')&&
               $n->content=~/^[,;)\]}]$/);

    # if (1) -> if(1), foo (...) -> foo(...)
    if($n->isa('PPI::Token::Structure')&&$n->content eq'('){
        my$q=$n->parent;
        $del=1 if$p->isa('PPI::Token::Word')&&
            ($q->isa('PPI::Structure::Condition')||
             $q->isa('PPI::Structure::List')||
             $q->isa('PPI::Structure::For'));
    }

    # for my$x (@a) -> for my$x(@a)
    if($p->isa('PPI::Token::Symbol')&&
       $n->isa('PPI::Token::Structure')&&$n->content eq'('){
        my$q=$n->parent;
        $del=1 if$q->isa('PPI::Structure::List')||
                  $q->isa('PPI::Structure::For');
    }

    # return $x -> return$x
    $del=1 if$p->isa('PPI::Token::Word')&&
              $n->isa('PPI::Token::Symbol');

    # push @a -> push@a
    # keys %h -> keys%h
    # return \@a -> return\@a
    $del=1 if$n->isa('PPI::Token::Cast')&&
              ($p->isa('PPI::Token::Word')||
               $p->isa('PPI::Token::Operator'));

    # die "x" -> die"x"
    $del=1 if$p->isa('PPI::Token::Word')&&
              $n->isa('PPI::Token::Quote')&&
              $n->content=~/^["`]/;

    # return 'x' -> return'x'
    # Do not apply to arbitrary barewords because foo'x' is ambiguous.
    $del=1 if$p->isa('PPI::Token::Word')&&
              $n->isa('PPI::Token::Quote')&&
              $n->content=~/^'/&&
              $p->content=~/^(?:die|warn|print|printf|return|eval|require|do)$/;

    # sub f {      -> sub f{
    # else {       -> else{
    # eval {       -> eval{
    # grep { ... } -> grep{...}
    if($n->isa('PPI::Token::Structure')&&$n->content eq'{'){
        $del=1 if$p->isa('PPI::Token::Word')&&
          ($p->content=~/^(?:else|continue|do|eval|map|grep|sort|BEGIN|UNITCHECK|CHECK|INIT|END)$/||
           ($p->parent&&$p->parent->isa('PPI::Statement::Sub')));
    }

    # 'x' if$x       -> 'x'if$x
    # foo() unless$x -> foo()unless$x
    $del=1 if$n->isa('PPI::Token::Word')&&
              $n->content=~/^(?:if|unless|while|until|for)$/&&
              ($p->isa('PPI::Token::Quote')||
               ($p->isa('PPI::Token::Structure')&&
                $p->content=~/^[)\]}]$/));

    $del?$ws->delete:$ws->set_content(' ');
}

# Whitespace around structures and final ";" inside them.
for my$type(
    'PPI::Structure::Block',
    'PPI::Structure::Condition',
    'PPI::Structure::List'
){
    for my$x(@{$doc->find($type)||[]}){
        my$a=$x->first_token;

        $a->delete
            if$a&&
               $a->isa('PPI::Token::Whitespace')&&
               !$a->{_heredoc_nl};

        my$z=$x->last_token;
        next unless$z;

        if($z->isa('PPI::Token::Whitespace')){
            next if$z->{_heredoc_nl};
            $z->delete;
            $z=$x->last_token;
        }

        if($z){
            my$p=$z->previous_token;
            if($p&&!$p->{_heredoc_nl}&&
               ($p->isa('PPI::Token::Whitespace')||
                ($p->isa('PPI::Token::Structure')&&
                 $p->content eq';'))){
                $p->set_content('');
            }
        }

        my$n=$z&&$z->next_token;
        $n->delete
            if$n&&
               $n->isa('PPI::Token::Whitespace')&&
               !$n->{_heredoc_nl};
    }
}

# foreach -> for
for my$s(@{$doc->find('PPI::Statement::Compound')||[]}){
    my$t=$s->first_token or next;
    $t->set_content('for')
        if$t->isa('PPI::Token::Word')&&
           $t->content eq'foreach';
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

# A heredoc terminator must remain on its own physical line.
# Remove only whitespace AFTER the terminator, before the next Perl token.
#
#   END_OF_AUTOLOAD
#
#      package CGI...
#
# becomes:
#
#   END_OF_AUTOLOAD
#   package CGI...
#
for my$h(@$heredocs){
    my$t=quotemeta $h->terminator;
    $out=~s/(^$t\r?\n)[ \t\r\n]+/$1/mg;
}

$out=~s/\s+\z//;

# If the entire file ends at a heredoc terminator, the terminator still
# requires its newline. Put our final ";" on the following physical line.
my($last)=$out=~/([^\n]*)\z/;

if(grep{$last eq$_->terminator}@$heredocs){
    $out.="\n;";
}else{
    $out=~s/;*\z/;/;
}

print$out;
