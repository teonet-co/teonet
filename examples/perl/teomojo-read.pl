#!/bin/perl
use LWP::Simple qw(get);

# Teomojo test

my $i = 0;
while(1) {
    my $url = 'http://localhost:3000/teo/echo';
    my $html = get $url;
    print "$i $html\n";
    $i++;
}

