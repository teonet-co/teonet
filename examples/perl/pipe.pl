#!/usr/bin/env perl
use strict;
use warnings;

# Pipe test

pipe ( my $reader, my $writer ); 
my $pid = fork(); #you should probably test for undef for fork failure. 
if ( $pid == 0 ) { 
    ## in child:
    close ( $writer );
    while ( my $line = <$reader> ) {
        print "Child got $line\n";
    }
}
else {
    ##in parent:
    close ( $reader );
    print {$writer} "Parent says hello!\n";
}
