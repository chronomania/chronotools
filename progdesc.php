<?php
//TITLE=Chrono Trigger translation development system

$title = 'Chrono Trigger translation development system';
$progname = 'chronotools';

require_once '/WWW/email.php';

$text = array(
   'what:1. Purpose' => "

Tools aiding in
<a href=\"http://bisqwit.iki.fi/ctfin/\">Bisqwit's Finnish Chrono Trigger translation</a>.
<p>
Meant to be useful for anyone who wants to become
a translator for Chrono Trigger and translate the
game to their own language.

", 'how:1. How to begin?' => "

So here's what you do if you want to translate Chrono Trigger.

<ul>
 <li><a href=\"#download\">Download</a> Chronotools. (Source code on this site. Windows binaries available upon request.)</li>
 <li>You need the Chrono Trigger English ROM. (You won't get it from me.)</li>
 <li>You dump the script and images from the ROM using
     <a href=\"#ctdump\">ctdump</a>, included in Chronotools.</li>
 <li>You redesign the font using an image manipulation
     program such as <a href=\"http://www.gimp.org\">GIMP</a>,
     editing <code>ct16fn.tga</code> and <code>ct8fn.tga</code>,
     and update the configuration to match the new character set
     in your font and script files.</li>
 <li>You translate the script, once in a while testing it
     by creating patches with <a href=\"#ctinsert\">ctinsert</a>.</li>
 <li>You ask me how to do this and that what you don't know. :)<br>
     For example, what to do if ctinsert reports errors,
     or if you want features like
     <a href=\"#conj\">name conjugation</a> or
     <a href=\"#vwf8\">variable width 8pix font</a>.</li>
</ul>

Feel free to <a href=\"#copying\">contact me</a> in questions
you might have about translating/hacking Chrono†Trigger.<br>

", 'status:1. Current status' => "

<img src=\"/src/chronotools-toad.gif\" align=left alt=\"\">
Chronotools is under active development.
Here's the current situation.<br>
Last updated:
".date('Y-m-d', filemtime('/WWW/src/.desc-chronotools.php'))."

<p>
<table>
 <tr>
  <th align=left style=\"background:#EEE\">Subject</th>
  <th align=left style=\"background:#EEE\">Percentage</th>
  <th align=left style=\"background:#EEE\"></th>
 </tr>
<tr><td>Dialog handling</td>
    <td>120%</td> <td>everything works, extra features</td></tr>
<tr><td>Dialog font handling</td>
    <td>120%</td> <td>everything works, extra features</td></tr>
<tr><td>Status font handling</td>
    <td>100%</td> <td>everything works</td></tr>
<tr><td>Character set issues</td>
    <td>100%</td> <td>everything works</td></tr>
<tr><td>Compression algorithms</td>
    <td>100%</td> <td>everything works</td></tr>
<tr><td>Item/tech/monster font handling</td>
    <td>80%</td> <td>useless in its current state</td></tr>
<tr><td>Graphics handling</td>
    <td>95%</td> <td>minor details left</td></tr>
<tr><td>Signature feature</td>
    <td>100%</td> <td>everything works</td></tr>
<tr><td>Error recovery</td>
    <td>10%</td> <td>silent ignore</td></tr>
<tr><td>Documentation</td>
    <td>0%</td> <td>next to none</td></tr>
</table>

", 'changes:1.1. Version history' => "

Copypaste from the Makefile:

<pre class=smallerpre
># VERSION 1.0.0  first archived version. dumper works.
# VERSION 1.0.1  working with recompressor, added tools.
# VERSION 1.0.2  updates to patcher
# VERSION 1.0.3  bugfixes to patcher - first working version!
# VERSION 1.0.4  handled fixed strings too
# VERSION 1.0.5  found item descriptions
# VERSION 1.0.6  compressed better
# VERSION 1.0.7  compressed more carefully
# VERSION 1.0.8  documented the script
# VERSION 1.0.9  fixed \"...\" handling and located the font
# VERSION 1.0.10 had knowledge of character sets
# VERSION 1.0.11 had a working font insertor
# VERSION 1.0.12 had better knowledge of special codes
# VERSION 1.0.13 used 62-base numbers
# VERSION 1.0.14 added taipus.cc
# VERSION 1.0.15 updated FIN/README and ct_fin.txt, but neither are archived
# VERSION 1.0.16 added taipus.rb, fixed homepage urls and fixed mmap error checking.
# VERSION 1.0.17 working again; uses space better; little modularized
# VERSION 1.0.18 more of above
# VERSION 1.0.19 code organising... improved 'i' in 8x8 font.
# VERSION 1.0.20 binpacker changes, some translation done too
# VERSION 1.0.21 more translation, some documentation, font palette changes.
# VERSION 1.0.22 more translation, autowrapping support, conjugation detection code
# VERSION 1.1.0  did some assembly hacking, support for code patching
# VERSION 1.1.1  conjugating conjugating conjugating... work goes on
# VERSION 1.1.2  and so on
# VERSION 1.1.3  and so on... almost working! \"case\" still doesn't work.
# VERSION 1.1.4  conjugating finally works!
# VERSION 1.1.5  some bugfixes
# VERSION 1.1.6  fixed an allocation bug and optimized the code generator a bit
# VERSION 1.1.7  some translation, more asm changes
# VERSION 1.1.8  syntax changes in the compiler, optimizations
# VERSION 1.1.9  support for font/dictionary size skew
# VERSION 1.1.10 new configuration system. Time to squash bugs.
# VERSION 1.1.11 configuration works, font-enhancement works.
# VERSION 1.2.0  variable-width 8pix font has stepped in, but has many many bugs.
# VERSION 1.2.1  more vwf stuff, backup before doing big changes
# VERSION 1.2.2  vwf stability++, also techniques now vwf. Scrolling bugs.
# VERSION 1.2.3  lots of more translation
# VERSION 1.2.4  8pix system deciphered, more bugs introduced
# VERSION 1.2.5  characterset enlarged by 512, only vwf8 bugs still
# VERSION 1.2.6  using nonstandard hash_map for greatly improved performance
# VERSION 1.2.7  creating another compiler
# VERSION 1.2.8  improved dictionary compression
# VERSION 1.2.9  compiler progress, first windows binaries are working
# VERSION 1.2.10 cursive font support
# VERSION 1.2.11 some translation, compression optimizations
# VERSION 1.3.0  new compression options, font reorganizer, generic typeface engine
# VERSION 1.4.0  image patching support, more font reorganizing support
# VERSION 1.4.1  lots of more translation (I'm archiving it here for my convenience)
# VERSION 1.5.0  end of the compiler project; using assembler (xa65) now.
# VERSION 1.5.1  vwf8 optimizations, assembly experiments
# VERSION 1.5.2  compressed graphics support: decompressor and compressor
# VERSION 1.5.3  better graphics compressor
# VERSION 1.5.4  another archive-only version
# VERSION 1.6.0  signature support (custom compressed image on startup screen)
# VERSION 1.6.1  some remodularizing of code
# VERSION 1.6.2  fixed the vwf8 scrolling problems and some other bugs
# VERSION 1.6.3  battle item lister fixed - no vwf8 problems there now
# VERSION 1.6.4  battle tech lister almost done; dumper: partial jap ROM support
# VERSION 1.6.5  is working on an assembler
# VERSION 1.6.6  has an almost working assembler
# VERSION 1.6.7  has a complete assembler, doesn't require xa65 anymore
# VERSION 1.6.8  patch version
# VERSION 1.6.9  conjugater now partially asm; its compiler is a separate program
# VERSION 1.7.0  some error checking; windows build of the assembler
# VERSION 1.8.0  is GPL
# VERSION 1.8.1  requires separate snescom (not bundled anymore)
# VERSION 1.9.0  unified some configuration parts; added crononick-code support
# VERSION 1.9.1  improved the signature feature; added checksum and ROM name feature
# VERSION 1.9.2  has only documentation updates
# VERSION 1.9.3  includes the forgotten snescode and dictionary modules.
# VERSION 1.10.0 implements various assembly optimization techniques
</pre>

", '1. Program list' => "

", 'ctdump:1.1. ctdump' => "

<table cellspacing=0 cellpadding=0 width=\"100%\">
<tr><td valign=top>

Dumps the script and fonts from a given ROM.<br>
Requires <code>chrono-dumpee.smc</code>.<br>
Produces <code>ct_eng.txt</code>,
<code>ct8fn.tga</code> and <code>ct16fn.tga</code>.
<p>
Sample of produced script:<pre class=smallerpre
>;1000ad (Lucca's home)
*z;106 pointerstrings (12pix font)
\$F1IO:
[nl]
            You got 1 [item]!
\$F1IQ:
LARA: Oh, hi Crono.[nl]
   Lucca's off at Leene Square with[nl]
   her father, Taban, unveiling her new[nl] 
   invention.
\$F1IS:
LARA: Lucca and Taban only care[nl]
   about their silly toys!</pre>
(Dumped from the English ROM)
<p>
The windows version of this program is
<a href=\"#download\">downloadable</a> on this page.<br>
Usage example:
  <code>ctdump chrono-uncompressed.smc &gt; ct_eng.txt</code>

</td><td valign=top align=right>

<table><tr><td align=center>
<img src=\"/src/chronotools-esp.png\"
     alt=\"sample\" style=\"padding-right:10px\" >
<br>
<small> Example screenshot in Spanish </small>
</td></tr></table>

</td></tr></table>

", 'ctinsert:1.1. ctinsert' => "

Reinserts the (edited) script and (edited) fonts to a ROM.<br>
Requires the files referenced by <code>ct.cfg</code>
(usually <code>ct.txt</code>, <code>ct8fn.tga</code> and <code>ct16fn.tga</code>
and optional extra fonts and code files).<br>
Produces <code>ctpatch-hdr.ips</code>
and <code>ctpatch-nohdr.ips</code>.<br>
Curiously, it doesn't require the ROM.

", '1.1. other' => "

", '1.1.1. makeips' => "

<a href=\"/src/makeips.cc\">makeips</a>
compares two ROMs and produces a patch file in IPS format.

", '1.1.1. unmakeips' => "

<a href=\"/src/unmakeips.cc\">unmakeips</a>
reads a ROM and an IPS file and produces a patched ROM file.

", '1.1.1. xray' => "

xray is a libggi-requiring application
for browsing the ROM contents.

", '1.1.1. viewer' => "

viewer requires S-Lang and is a textmode ROM browser
originally developed by me for PokÈmon hacking.

", '1.1.1. spacefind' => "

A <a href=\"/source/jaa3.html\">bin-packing</a> algorithm test.

", '1.1.1. taipus.rb' => "

This one is <a href=\"/ctfin/taipus.rb\">publicly available</a>.
It conjugates names in Finnish.<br>It has now been
<a href=\"/ctfin/ct-code.txt\">translated</a> to
SNES assembly (or sort of).<br>It works. :)

", '1.1.1. sramdump' => "

Views a sram dump file in a readable format.

", '1.1.1. base62' => "

Converts addresses between hex and base62 formats.
I.e. \$C2:5D4C -> 0eJI and vice versa.<br>
This development system uses base62 in the script
dumps to reduce the amount of code written.

", '1. Useful features' => "

", 'conj:1.1. Name conjugation' => "

<a href=\"/ctfin/ct-code.txt\">
<img src=\"/ctfin/dev/ct-taipus2.png\" alt=\"It works!\" align=right>
</a>  
It currently has support for conjugating names on fly.<br>
It's very important in Finnish, where you can't just
add \"'s\" to anything to make a genitive.<br>
For example, genitive of name Matti is \"Matin\",
and genitive of name Crono is \"Cronon\".<br>
The conjugator-engine is a textual script file
translated to 65c816 assembly on demand. It can
be customized to do conjugation in any language,
not just Finnish.
<br clear=all>

", 'skew:1.1. Font/dictionary skew' => "

It's quite complicated to explain, but shortly said:
<p>
In normal Chrono Trigger, the character set is as follows:
<ul>
 <li>127 of them are assigned to the dictionary used to compress the script.</li>
 <li><b>96</b> of them are possible
     <a href=\"/src/chronotools-16en.png\">visible symbols</a>.</li>
</ul>
<p>
In Chronotools,
<ul>
 <li>Dictionary size may be anything between 0-223</li>
 <li>The amount of different characters the script
     may use is <b>735</b> minus dictionary size.</li>
 <li>Though, 8pix strings (items, techs, monsters, character names and so on)
     can only use about 128 different characters.</li>
</ul>
<p>
There are drawbacks though.
<ul>
 <li>Shrinking the dictionary gives more available characters,
     but makes compression worse. This could cause the script
     not fit in the ROM.</li>
 <li>Extracharacters (anything above 223 minus dictionary size)
     require twice the space of normal characters. This has
     similar effect as above.</li>
</ul>

Despite these hazards this system might be a lifesaver
for someone doing an Estonian, Portuguese or even Thai translation.
<p>
Note: The Japanese version of the game is almost exactly the same
as the English version. Both have the same engine with only minor
changes. If you are going to translate to Chinese or something else
that uses thousands of different symbols, you are either going to
have to manage with ~700 symbols or have to request some changes
to the insertor.<br>

", 'wrap:1.1. Automatic paragraph wrapping' => "

<img src=\"/src/chronotools-wrapdemo.png\" alt=\"It works!\" align=right>
The program takes automatically care of proper line
lengths, so you don't have to risk running into unexpected
too-long-lines or making too short lines in paranoia.<br>
You can force line breaks (like in HTML), but you don't have to.
<p>
The image on the right shows this script piece:
<pre class=smallerpre
>T‰m‰ on Leenen Aukio. Sanotaan, ett‰ jos
joskus kuulee Leenen Kellon soivan, el‰‰
mielenkiintoisen ja onnellisen el‰m‰n!
</pre>
And is equal to if I had wrote this:
<pre class=smallerpre
>T‰m‰ on Leenen Aukio. Sanotaan, ett‰[nl]
jos joskus kuulee Leenen Kellon soivan,[nl]
el‰‰ mielenkiintoisen ja onnellisen[nl]
el‰m‰n!</pre>

<br clear=all>

", 'vwf8:1.1. Variable width 8pix font' => "

<img src=\"/src/chronotools-vwf8.png\" alt=\"It works!\" align=right>
Item, monster and technique names in Chrono Trigger are limited to 10 characters
(restriction is enforced by both the screen layout and the ROM space).<br>
This is way too little for Finnish, which has long words.
<p>
For this reason Chronotools creates a vwf8 engine that allows
the game to draw the names in thinner font that fits on the screen.
<p>
It still has the length limitation in the ROM though,
but this will be fixed eventually.

<br clear=all>

", '1.1. Very configurable' => "

I have tried to put almost everything in text-only config files
instead of hardcoding it in the programs. You won't be depending
on me to do little updates for your purposes.

", 'req:1. Requirements' => "

For source code (if you're a developer):
<blockquote>
A POSIX compatible system (like Linux or FreeBSD)
with GNU tools (GNU make, GCC etc) is required.<br>
These programs are archived as C++ source code.<br>
</blockquote>

For binaries (if you're an unfortunate user stuck with some \"Windows\"):
<blockquote>
I don't have a microsoft-operating system here on my hand,
but I have mingw32, which appears to produce working <em>commandline</em>
billware binaries. They should work in Windows 2000, Windows 98 and
possibly most other Windows systems as well.<br>
I don't offer binaries for any other platforms.<br>
If you fear the text mode and command line, you better
change your attitude and start learning :)
</blockquote>
<p>
The <acronym title=\"Variable width 8pix tall font\">VWF8</acronym> code
and the conjugator require an assembler,
<a href=\"http://bisqwit.iki.fi/source/snescom.html\">snescom</a>.
Snescom is a GPL'd xa65-compatible 65816 assembler program,
and it can be downloaded at
<a href=\"http://bisqwit.iki.fi/source/snescom.html\"
  >http://bisqwit.iki.fi/source/snescom.html</a> .

", 'copying:1. Copying' => "

Chronotools has been written by Joel Yliluoma, a.k.a.
<a href=\"http://iki.fi/bisqwit/\">Bisqwit</a>,<br>
and is distributed under the terms of the
<a href=\"http://www.gnu.org/licenses/licenses.html#GPL\">General Public License</a> (GPL).
<p>
If you have questions or just want to talk about
Chrono†Trigger hacking, throw me email.
<p>
".GetEmail('My email address:', 'Joel Yliluoma', 'bisqwi'. 't@iki.fi')."
</p>
I've previously stated here that I'm not publishing files because it's
a†well-known fact that many people in ROM†hacking scene aren't very
respectful to copyrights.<br>
Well, left to be seen...

", 'parts:1.1. If you only are interested in some features/parts' => "

This system doesn't contain much reusable material,
although it now does contain some separate assembly code.<br>
I have made my own systems to compile and link code.<br>
If you're uncertain, <a href=\"#copying\">send me email and explain your situation</a>.

", '1. See also' => "

<ul>
 <li><a href=\"http://bisqwit.iki.fi/ctfin/\">Bisqwit's
  Finnish Chrono Trigger translation project</a>
  (uses these tools)</li>
 <li><a href=\"http://bisqwit.iki.fi/source/snescom.html\">snescom</a>
  - xa65-compatible 65816 assembler with free source code</li>
 <li><a href=\"http://bisqwit.iki.fi/jutut/ctcset.html\">Chrono Trigger
  technical document</a>
  (publicly available information that these tools base on)</li>
</ul>

");

$pagebegin      = '';
$contentsheader = '';
$pageend        = '';

include '/WWW/progdesc.php';
