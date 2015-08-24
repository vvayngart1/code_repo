#!/usr/bin/env perl

use strict;
use File::Spec::Functions qw(rel2abs);
use File::Spec::Functions qw(curdir);

# ar.pl CFG/obj $(AR) cr libname (obj)*

my $dir = rel2abs(curdir());
my $root = rel2abs(shift @ARGV);
my $ar = shift @ARGV;
my $opt = shift @ARGV;
my $lib = shift @ARGV;
my @objs = @ARGV;

my @links = ();
foreach my $obj ( @objs ) {
  my $link = rel2abs($obj);
  $link =~ s/^$root\///;
  $link =~ s/\//__/g;
  $link = "$root/" . $link;
  $link =~ s/^$dir\///;
  system("ln $obj $link");
  push @links, $link;
}

my $linkstr = join(' ', @links);
system("$ar $opt $lib $linkstr");
system("rm -f $linkstr");
