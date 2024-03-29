#!/usr/bin/perl

use strict;
use warnings;

my $url = $ARGV[0] or die "\nError: You need to specify a YouTube URL\n\n";

 
my $prefix = defined($ARGV[1]) ? $ARGV[1] : "";


my $html = `wget -Ncq -e "convert-links=off" --tries 1 --keep-session-cookies --save-cookies /dev/null --no-check-certificate "$url" -O-`  or die  "\nThere was a problem downloading the HTML file.\n\n";


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


$download = "$youtubeurl\&$signature";


$download =~ s/&+/&/g;
$download =~ s/&itag=\d+&signature=/&signature=/g;



#print "\n Download: $prefix$title.webm\n\n";

#print "\n URL: $download \n";
my ($yurl) = `youtube-dl --skip-download "$url" -f 45 -g`;
#print $yurl;
#system("wget -Ncq -e \"convert-links=off\" --load-cookies /dev/null --tries=50 --timeout=45 --no-check-certificate \"$download\" -O $prefix$title.webm &");
#system("curl -I '$download' 2>/dev/null | egrep 'Content-Length' | awk '{print \$NF}'");
#my ($ret)=system("./curl_script '$download' 2>/dev/null");
#if ($ret != 0) {
#	print "ERROR";
#}
my ($size) = `./curl_script '$yurl' 2>/dev/null`;
print $size;

#my ($filesize) = -s "$prefix$title.webm";
#print $filesize;

#### EOF #####
