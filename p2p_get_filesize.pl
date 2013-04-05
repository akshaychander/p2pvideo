#!/usr/bin/perl
use strict;
use warnings;

my $url = $ARGV[0] or die "\nError: You need to specify a YouTube URL\n\n";
my $prefix = defined($ARGV[1]) ? $ARGV[1] : "";
my $html = `wget -Ncq -e "convert-links=off" --keep-session-cookies --save-cookies /dev/null --no-check-certificate "$url" -O-`  or die  "\nThere was a problem downloading the HTML file.\n\n";

my ($title) = $html =~ m/<title>(.+)<\/title>/si;
$title =~ s/[^\w\d]+/_/g;
$title =~ s/_youtube//ig;
$title =~ s/^_//ig;
$title = lc ($title);


my ($download) = $html =~ /"url_encoded_fmt_stream_map"([\s\S]+?)\,/ig;

$download =~ s/\:\ \"//;
$download =~ s/%3A/:/g;
$download =~ s/%2F/\//g;
$download =~ s/%3F/\?/g;
$download =~ s/%3D/\=/g;
$download =~ s/%252C/%2C/g;
$download =~ s/%26/\&/g;
$download =~ s/sig=/signature=/g;
$download =~ s/\\u0026/\&/g;
$download =~ s/(type=[^&]+)//g;
$download =~ s/(fallback_host=[^&]+)//g;
$download =~ s/(quality=[^&]+)//g;


my ($signature) = $download =~ /(signature=[^&]+)/;
my ($youtubeurl) = $download =~ /(http.+)/;
$youtubeurl =~ s/&signature.+$//;


$download = "$youtubeurl\&$signature";

$download =~ s/&+/&/g;
$download =~ s/&itag=\d+&signature=/&signature=/g;


my ($perc) = `timeout 5s wget --progress=dot "$download" 2>&1 -O $prefix$title.mp4 | grep --line-buffered "% " | sed -u -e "s,\\.,,g" | sed -u -e "s,\\%,,g" | awk '{print \$2}' | tail -1`; 

my ($partfilesize) = -s "$prefix$title.mp4";

my ($actfilesize) = $partfilesize * 100 / $perc;
print "\n Actual File size : $actfilesize \n";
unlink("$prefix$title.mp4");
#### EOF #####
