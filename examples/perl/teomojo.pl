#!/usr/bin/perl
#
# @File teomojo.pl
# @Author Kirill Scherba <kirill@scherba.ru>
# @Created Feb 13, 2018 2:01:08 PM
#

use strict;
use Time::HiRes; # qw/gettimeofday/;
use Mojolicious::Lite;
use warnings;
use threads;
use Switch;
use teonet;

# Teonet + mojolicious test Application
my $TPERL_VERSION = "0.0.1";

# Pite to communicate betwean Teo and Mojo processes 
pipe (P_READ, P_WRITE);
P_WRITE->autoflush(1);

# Print time (to check latency)
sub print_time {
    my $t =  Time::HiRes::gettimeofday(); # Returns ssssssssss.uuuuuu in scalar context
    printf "%.04f ", $t;
}

# Mojolicious proccess
sub mojo {

#    my($any, $ke) = @_;

    # Route with placeholder
    get '/teo/:command' => sub {
      my $c = shift;
      my $cmd  = $c->stash('command');
      #my $cmd  = $c->param('teocommand');
      $c->render(text => "Teonet command: $cmd");
      # Send event to teonet
      print_time();
      print("Send cmd: $cmd\n");
      print P_WRITE "$cmd\n";
    };

    get '/:foo' => sub {
      my $c   = shift;
      my $foo = $c->param('foo');
      $c->render(text => "Hello from $foo.");
    };

    get '/' => sub {
      shift->render(text => "Hello!");
    };
    
    # Start the Mojolicious command system
    app->start;
}

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
        
        # EV_K_ASYNC
        case 12 {
            #printf("EV_K_ASYNC - Perl subroutine\n");
            print_time();
            printf("EV_K_ASYNC - Perl subroutine, length %d bytes: %s\n", 
                $data_len, teonet::rdToString($data));
        }
    }
}

# Teonet thread to read pipe and send pipes content to Teonet event loop
sub teo_thread {
    
    my($ke) = @_;
    
    while ( 1 ) {
        my $line;
        chomp($line = <P_READ>);
        print_time();
        print "Mojo send: $line\n";
        # Send teonet async event to teonet event loop
        teonet::ksnetEvMgrAsyncP($ke, $line);
    }
}

# Teonet process
sub teo {
    
#    my($any, $ke) = @_;
    
    print "Teoperl ver " . $TPERL_VERSION . ", based on Teonet ver " . 
        teonet::teoGetLibteonetVersion() . "\n";

    # Initialize teonet event manager and Read configuration
    my $argc = @ARGV;
    my $ke = teonet::ksnetEvMgrInitP($argc, \@ARGV, \&event_cb);

    # Set application type and version
    teonet::teoSetAppType($ke, "teo-perl");
    teonet::teoSetAppVersion($ke, $TPERL_VERSION);

    # Start teonet pipe read thread
    my $thr = threads->create('teo_thread', $ke);
    $thr->detach();

    # Start teonet
    teonet::ksnetEvMgrRun($ke);
}

# Main application code to start two process
my $pid = fork(); #you should probably test for undef for fork failure. 
if ( $pid == 0 ) { 
    ## in child:
    close (P_READ);
    mojo();
}
else {
    ## in parent:    
    close (P_WRITE);
    teo();
    #$thr->join();
    kill 1, $pid;
}
__END__
