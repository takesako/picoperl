sub ok{die"NG $_[1]\n"unless$_[0];print"ok $_[1]\n"}
my $f=1+2**-24==1;
print "# NV=", $f?"float\n":"double\n";
ok(abs((.1+.2)-.3)<1e-6,"0.1+0.2");
ok(abs((1/3)-.33333334)<1e-6,"division");
ok(abs(sqrt(2)-1.41421356)<1e-6,"sqrt");
ok(abs(sin(1)-.84147098)<1e-6,"sin");
my $i=16777217.0;
ok($i==($f?16777216:16777217),"2^24+1 precision");
ok((1+2**-24==1)==$f,"epsilon");
ok((16777216.0+1==16777216)==$f,"2^24 boundary");
ok((1/3) eq ($f?"0.333333":"0.333333333333333"),"stringify precision (NV_DIG)");
print "ALL TESTS PASSED\n";
