#!/usr/bin/perl -w
 
use strict; 
use IO::Socket::Multicast; 

print "Starting ...\n";

#my $host = shift
#my $port = shift

my $host="205.209.220.44";
my $port=10000;
my $if="eth1";

print "Preparing socket for messages on: $host :: $port :: $if \n";

# set up socket
# 
my $s = IO::Socket::Multicast->new(LocalPort=>$port);
$s->mcast_add($host,$if);

print "Entering data loop\n";

my $data;
while (1) {     
   next unless $s->recv($data,1024); 
   print "Received: $data\n";
}
