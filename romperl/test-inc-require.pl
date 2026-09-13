sub ok{die"NG $_[1]\n"unless$_[0];print"ok $_[1]\n"}

use feature;
ok(defined $feature::VERSION, "use feature loads from ROMFS");
ok($feature::VERSION eq '0.01', "feature.pm content is the ROMFS placeholder");
ok(exists $INC{'feature.pm'}, "%INC records the ROMFS-loaded module");

eval { require NoSuchModule::AtAll; };
ok($@ =~ /Can't locate/, "missing module still fails normally (falls through \@INC)");

print "ALL TESTS PASSED\n";
