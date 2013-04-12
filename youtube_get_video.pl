#!/usr/bin/perl

use strict;
use warnings;

my $url = $ARGV[0] or die "\nError: You need to specify a YouTube URL\n\n";
#my $blocknum = $ARGV[1] or die "\nError: You need to specify a block number\n\n";
#my $start = $ARGV[2] or die "\nError: You need to specify a start offset\n\n";
#my $end = $ARGV[3] or die "\nError: You need to specify an end offset\n\n";

my $blockdest = $ARGV[1];
#print $blockdest;
my $start = $ARGV[2];
my $end = $ARGV[3]; 
#my $prefix = defined($ARGV[1]) ? $ARGV[1] : "";


my $html = `wget -Ncq -e "convert-links=off" --keep-session-cookies --save-cookies /dev/null --no-check-certificate "$url" -O-`  or die  "\nThere was a problem downloading the HTML file.\n\n";


my ($title) = $html =~ m/<title>(.+)<\/title>/si;
$title =~ s/[^\w\d]+/_/g;
$title =~ s/_youtube//ig;
$title =~ s/^_//ig;
$title = lc ($title);


my ($download) = $html =~ /"url_encoded_fmt_stream_map"(([\s\S]+?)\,){3}/ig;

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


$download = "$youtubeurl\&$signature\&range=$start-$end";


$download =~ s/&+/&/g;
$download =~ s/&itag=\d+&signature=/&signature=/g;



#print "\n Download: $prefix$title.webm\n\n";

#print "\n URL: $download \n";
#system("curl -I '$download' 2>/dev/null | egrep 'Content-Length' | awk '{print \$NF}'");
#system("wget -Ncq -e \"convert-links=off\" --load-cookies /dev/null --tries=50 --timeout=45 --no-check-certificate \"$download\" -O $prefix$title.webm &");
my $ret;
$ret = system("wget -N -e \"convert-links=off\" --load-cookies /dev/null --tries 10 --no-check-certificate \"$download\"  -O \"$blockdest\" 2>/dev/null");
#print $ret;
if ($ret != 0) {
	unlink($blockdest);
}
#my ($filesize) = -s "$prefix$title.webm";
#print $filesize;

#### EOF #####
