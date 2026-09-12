sub ok{die"NG $_[1]\n"unless$_[0];print"ok $_[1]\n"}
my $forkrc = fork();
ok(!defined($forkrc),"fork disabled");
ok($!=~/not implemented/i,"fork errno ENOSYS");
ok(sleep(1)==0,"sleep is no-op");
ok($$==1,"fixed pid");
ok($<==0 && $>==0 && $(==0 && $)==0,"fixed uid/gid");
my $sysrc = system("true");
ok($sysrc==-1,"system() disabled");
my $out = `echo hi`;
ok(!defined($out),"backticks disabled");
eval { kill(0,$$) };
ok($@=~/unimplemented/i,"kill disabled");
print "ALL TESTS PASSED\n";
