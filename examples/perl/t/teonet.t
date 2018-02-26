# Before 'make install' is performed this script should be runnable with
# 'make test'. After 'make install' it should work as 'perl Teonet.t'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use strict;
use warnings;
use teonet;

use Test::More tests => 1;
BEGIN { use_ok('teonet') };

#########################

# Insert your test code below, the Test::More module is use()ed here so read
# its man page ( perldoc Test::More ) for help writing this test script.

print "Teoperl ver 0.0.1, based on teonet ver " . teonet::teoGetLibteonetVersion() . "\n";