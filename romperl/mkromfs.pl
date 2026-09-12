#!/usr/bin/perl
# mkromfs.pl - romfs.h に定義された ROMFS イメージを生成する開発ホスト側ツール
# (picoperl自身ではなく、システムのperlで実行する)。
#
#   mkromfs.pl <src-dir> <output.romfs>
#   mkromfs.pl --list <image.romfs>
#
use strict;
use warnings;
use File::Find;

use constant {
    MAGIC        => 'RFS1',
    NAME_MAX     => 32,
    HEADER_FMT   => 'a4 L L L',   # magic, file_count, total_size, reserved
    HEADER_SIZE  => 16,
    ENTRY_FMT    => 'a32 L L',    # name, offset, size
    ENTRY_SIZE   => 40,
};

sub list_romfs {
    my ($image) = @_;
    open my $fh, '<:raw', $image or die "open $image: $!\n";
    my $raw_header;
    read($fh, $raw_header, HEADER_SIZE) == HEADER_SIZE
        or die "$image: too short for a romfs header\n";
    my ($magic, $count, $total_size, $reserved) = unpack(HEADER_FMT, $raw_header);
    $magic eq MAGIC or die "$image: bad magic '$magic' (expected '".MAGIC."')\n";
    printf "magic=%s file_count=%d total_size=%d reserved=%d\n",
        $magic, $count, $total_size, $reserved;
    for my $i (1 .. $count) {
        my $raw_entry;
        read($fh, $raw_entry, ENTRY_SIZE) == ENTRY_SIZE
            or die "$image: truncated entry table\n";
        my ($name, $offset, $size) = unpack(ENTRY_FMT, $raw_entry);
        $name =~ s/\0.*$//s;
        printf "  %-31s offset=%-8d size=%d\n", $name, $offset, $size;
    }
    close $fh;
}

sub build_romfs {
    my ($srcdir, $outfile) = @_;
    -d $srcdir or die "$srcdir: not a directory\n";

    my @files;
    find({
        wanted => sub {
            return unless -f $_;
            my $rel = $File::Find::name;
            $rel =~ s{^\Q$srcdir\E/?}{};
            push @files, $rel;
        },
        no_chdir => 1,
    }, $srcdir);
    @files = sort @files;
    @files or die "$srcdir: no files found\n";

    my @entries;
    my $data = '';
    for my $rel (@files) {
        length($rel) < NAME_MAX
            or die "path too long for $rel (max ".(NAME_MAX-1)." chars): $rel\n";
        open my $fh, '<:raw', "$srcdir/$rel" or die "open $srcdir/$rel: $!\n";
        local $/;
        my $content = <$fh>;
        close $fh;
        push @entries, {
            name   => $rel,
            offset => length($data),
            size   => length($content),
        };
        $data .= $content;
    }

    my $header_and_table_size = HEADER_SIZE + ENTRY_SIZE * scalar(@entries);
    my $total_size = $header_and_table_size + length($data);

    open my $out, '>:raw', $outfile or die "open $outfile: $!\n";
    print $out pack(HEADER_FMT, MAGIC, scalar(@entries), $total_size, 0);
    for my $e (@entries) {
        print $out pack(ENTRY_FMT, $e->{name}, $header_and_table_size + $e->{offset}, $e->{size});
    }
    print $out $data;
    close $out;

    printf "wrote %s: %d files, %d bytes\n", $outfile, scalar(@entries), $total_size;
}

@ARGV or die "usage: $0 <src-dir> <output.romfs>\n       $0 --list <image.romfs>\n";
if ($ARGV[0] eq '--list') {
    @ARGV == 2 or die "usage: $0 --list <image.romfs>\n";
    list_romfs($ARGV[1]);
} else {
    @ARGV == 2 or die "usage: $0 <src-dir> <output.romfs>\n";
    build_romfs($ARGV[0], $ARGV[1]);
}
