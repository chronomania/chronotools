<?php
//TITLE=Chrono Trigger translation development system

$title = 'Chrono Trigger translation development system';
$progname = 'chronotools';

$text = array(
   '1. Purpose' => "

Tools aiding in
<a href=\"http://bisqwit.iki.fi/ctfin/\">Bisqwit's Finnish Chrono Trigger translation</a>.

", '1. Program list' => "

", '1.1. ctdump' => "

Dumps the script from a given ROM.<br>
Requires <code>chrono-dumpee.smc</code>.<br>
Produces <code>ct_eng.txt</code>,
<code>ct8fn.tga</code> and <code>ct16fn.tga</code>.
<p>
Sample of produced script:<pre class=smallerpre
>;Battle tutorials, Zeal stuff, party stuff
*z;39 pointerstrings (12pix font)
\$HSs4:
Taistelun aikana jokaisen iskun[nl]
   teho vaihtelee iskun mukaan.
\$HSs6:
Ensinnäkin, voit iskeä myös[nl]
   useampaa, kuin yhtä vihollista[nl]
   kerralla.
\$HSs8:
Voit esimerkiksi tähdätä tätä[nl]
   hirviötä...</pre>
(Dumped from
<a href=\"http://hallucinat.ionstream.fi/teemu/engine.html?page=13\"
>inf's Finnish Chrono Trigger translation</a>).

", '1.1. ctinsert' => "

Reinserts the script to a ROM.<br>
Requires <code>ct.txt</code>,
<code>ct8fn.tga</code> and <code>ct16fn.tga</code>.<br>
Produces <code>ctpatch-hdr.ips</code>
and <code>ctpatch-nohdr.ips</code>.<br>
Curiously, it doesn't require the ROM.

", '1.1. xray' => "

xray is a libggi-requiring application
for browsing the ROM contents.

", '1.1. viewer' => "

viewer requires S-Lang and is a textmode ROM browser
originally developed by me for Pokémon hacking.

", '1.1. spacefind' => "

A bin-packing algorithm test.

", '1.1. makeips' => "

Compares two ROMs and produces a patch file in IPS format.

", '1.1. unmakeips' => "

Reads a ROM and an IPS file and produces a patched ROM file.

", '1.1. taipus.rb' => "

This one is <a href=\"/ctfin/taipus.rb\">publicly available</a>.
It conjugates names in Finnish. It's supposed to get translated
to SNES assembler by somebody.

", '1. Copying' => "

If you're interested, throw me email.<br>
Read the <a href=\"/ctfin/\">translation project page</a> first!
<p>
My email-address (sigh) is:
<em>bisqwit a<b style=\"font-weight:lighter\">t i</b>ki <small>dot</small> fi</em>
</p>
I'm not publishing files, because it's a well-known
fact that many people in ROM hacking scene aren't
very respective to copyrights.

", '1. Requirements' => "

A POSIX compatible system (like Linux or FreeBSD)
with GNU tools (GNU make, GCC etc) is required.<br>
These programs are archived as C++ source code only.

", '1. Technical limitations' => "

", '1.1. Space' => "

The ROM doesn't have much space for text.<br>
It does apply a compression method, but some text
compresses better than some other.

", '1.1. Font' => "

The 12-pix font has only 96 characters available.<br>
Originally the characters are arranged like this:<br>
<img src=\"http://bisqwit.iki.fi/src/chronotools-16en.png\"
     alt=\"(Chrono Trigger English 12pix font)\"><br clear=all>
My font contains currently this slightly rearranged character map:<br>
<img src=\"http://bisqwit.iki.fi/src/chronotools-16fi.png\"
     alt=\"(Chrono Trigger Finnish 12pix font)\"><br clear=all>
You can imagine in your mind, about which characters you would replace
with another to get support for Thai for example...
<p>
Similarly the 8-pix font has undergone some changes.<br>
It has lots more redundant space though.<br>
Originally:<br>
<img src=\"http://bisqwit.iki.fi/src/chronotools-8en.png\"
     alt=\"(Chrono Trigger English 8pix font)\"><br clear=all>
Bisqwit's Finnish 8pix Chrono Trigger font:<br>
<img src=\"http://bisqwit.iki.fi/src/chronotools-8fi.png\"
     alt=\"(Chrono Trigger Finnish 8pix font)\"><br clear=all>

<p>
If you are translating to a language that utilizes lots of
different symbols, you might want to try to base on the
Japanese version instead. This utility pack bases on the
English version.<br>
(Sorry, I can't help you with the Japanese ROM.)

", '1. See also' => "

<ul>
 <li><a href=\"http://bisqwit.iki.fi/ctfin/\">Bisqwit's Finnish Chrono Trigger translation project</a>
  (uses these tools)</li>
 <li><a href=\"http://bisqwit.iki.fi/jutut/ctcset.html\">Chrono Trigger character set and encoding explained</a>
  (publicly available information that these tools base on)</li>
</ul>

");

$pagebegin      = '';
$contentsheader = '';
$pageend        = '';

include '/WWW/progdesc.php';
