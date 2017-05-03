#!/usr/bin/perl
use strict;
use warnings;
use teonet;

#my $area_of_cir = area::area_of_circle(5);
#my $area_of_squ = area::area_of_square(5);
#print "Area of Circle: $area_of_cir\n";
#print "Area of Square: $area_of_squ\n";
#print "$area::pi\n";

#my $TVPN_VERSION = "0.0.1"

print "Teovpn ver 0.0.1, based on teonet ver " . teonet::teoGetLibteonetVersion() . "\n";

# Initialize teonet event manager and Read configuration
my $ke = teonet::ksnetEvMgrInit(0, undef, undef, 3);

# Start teonet
ksnetEvMgrRun($ke);