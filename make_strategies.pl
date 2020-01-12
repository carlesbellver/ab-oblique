#!/usr/bin/perl
# Usage: perl make_strategies.pl < strategies.txt > strategies.h

my @strategies;
my $count;
my $maxlen;

while(<>) {
  chomp;
  next if /^\s*$/ or /^#/;
  s/\.$//;
  $count++;
  push(@strategies, $_);
  $maxlen = length($_) if length($_) > $maxlen;
}

print "// Max length: $maxlen\n";
print "\n";

print "#define NO_STRATEGIES $count\n";
print "\n";

my $i = 0;
for my $strategy(@strategies) {
  print "const char strategy_$i\[\] PROGMEM = \"$strategy\";\n";
  $i++;
}

print "const char * const strategies\[] PROGMEM = {\n";
my $i = 0;
for($i=0;$i<$count;$i++) {
  print "  strategy_$i";
  print "," unless $i == $count - 1;
  print "\n";
}
print "};\n";