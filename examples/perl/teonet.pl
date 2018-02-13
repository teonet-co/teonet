#!/usr/bin/perl
use strict;
use warnings;
use Switch;
use teonet;

my $TPERL_VERSION = "0.0.1";

# Teonet event callback
#
# event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
#    size_t data_len, void *user_data, char* from);
# $1 -- ke        Pointer to ksnetEvMgrClass
# $2 -- event     ksnetEvMgrEvents (number)
# $3 -- data      Pointer to void data
# $4 -- data_len  Length of data
# $5 -- user_data Pointer to void user data
# $6 -- from      From peer (parsed form)
#
sub event_cb {
    my($ke, $event, $data, $data_len, $user_data, $from) = @_;

    switch ($event) {
        
        # EV_K_STARTED
        case 0 { printf("EV_K_STARTED - Perl subroutine\n"); }
        
        # EV_K_STOPPED
        case 2 { printf("EV_K_STOPPED - Perl subroutine\n"); }
        
        # EV_K_CONNECTED
        case 3 {
            # Parse `from peer`, the same as $from parameter
            my $from = teonet::rdFrom($data); 
            printf("EV_K_CONNECTED - Perl subroutine, from: %s, ".
                "data length: %d\n", 
                $from, $data_len);
        }
        
        # EV_K_DISCONNECTED
        case 4 {
            # Parse `from peer`, the same as $from parameter
            #my $from = teonet::rdFrom($data); 
            printf("EV_K_DISCONNECTED - Perl subroutine, ".
                "from: %s, data length: %d\n", 
                $from, $data_len);
        }
    }
}

print "Teoperl ver " . $TPERL_VERSION . ", based on Teonet ver " . 
    teonet::teoGetLibteonetVersion() . "\n";

# Initialize teonet event manager and Read configuration
my $argc = @ARGV;
my $ke = teonet::ksnetEvMgrInitP($argc, \@ARGV, \&event_cb);

# Set application type and version
teonet::teoSetAppType($ke, "teo-perl");
teonet::teoSetAppVersion($ke, $TPERL_VERSION);

# Start teonet
teonet::ksnetEvMgrRun($ke);

__END__
