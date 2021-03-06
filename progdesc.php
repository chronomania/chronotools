<?php
//TITLE=Chrono Trigger (SNES) translation development system

$title = 'Chrono Trigger translation development system';
$progname = 'chronotools';
$git = 'git://bisqwit.iki.fi/chronotools.git';

require_once '/WWW/email.php';

$text = array(
   'what:1. Purpose' => "

Tools for translating Chrono&nbsp;Trigger to different languages.
<p>
Meant to be useful for anyone who wants to become
a translator for Chrono Trigger and translate the
game to their own language.
<p>
These programs do quite much more than just some little editing:
<ul>
 <li>Find data (images, text)</li>
 <li>Decompress data</li>
 <li>Recompress data</li>
 <li>Find space for the data</li>
 <li>Organize the data around in little pieces to ensure everything fits</li>
 <li>Reroute pointers and modify code to handle the data</li>
 <li>Write data</li>
</ul>

This project has grown together with
<a href=\"http://bisqwit.iki.fi/ctfin/\">Bisqwit's Finnish Chrono Trigger translation</a>,
and to a lesser degree together with Mziab's Polish translation (no link), but it's not limited to either Finnish or Polish.<br>
It has been designed to allow as much flexibility as possible.

", 'how:1. How to begin?' => "

So here's what you do if you want to translate Chrono Trigger.

<ul>
 <li><a href=\"#download\">Download</a> Chronotools.
     (Source code on this site. Windows binaries also
      available, but they are not supported.)</li>
 <li>You need the Chrono Trigger English ROM. (You won't get it from me.)</li>
 <li>Read <a href=\"http://bisqwit.iki.fi/ctfin/dev/quickstart.html\"
      >the quick start guide</a>.</li>
</ul>

Feel free to <a href=\"#copying\">contact me</a> in questions
you might have about translating/hacking Chrono Trigger.<br>

", 'status:1. Current status' => "

<img src=\"/src/chronotools-toad.gif\" align=left alt=\"\">
Chronotools is no longer under active development.
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
    <td>99%</td> <td>everything works well enough</td></tr>
<tr><td>Graphics handling</td>
    <td>95%</td> <td>minor details left</td></tr>
<tr><td>Signature feature</td>
    <td>100%</td> <td>everything works</td></tr>
<tr><td>Error recovery</td>
    <td>40%</td> <td>not all error situations are handled</td></tr>
<tr><td><a href=\"#docs\">Documentation</a></td>
    <td>40%</td> <td>it isn't good or complete, but it exists</td></tr>
<tr><td>Room/event modifying</td>
    <td>10%</td> <td>not easy, not necessary - but possible</td></tr>
<tr><td>Emulator compatibility</td>
    <td>80%</td> <td>tested extensively with SNES9x 1.43, slight VWF8-related bugs</td></tr>
<tr><td>Hardware compatibility</td>
    <td>60%</td> <td>Issues with the VWF8 scrolling and DMA updates outside NMI</td></tr>
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
# VERSION 1.10.0 implemented various assembly optimization techniques
# VERSION 1.10.1 updated the docs and the conj.code generator
# VERSION 1.10.2 creates more useful information when dumping
# VERSION 1.10.3 has technological updates but broken VWF8
# VERSION 1.11.0 has technological updates and new item list code with VWF8
# VERSION 1.11.1 adds the documentation core
# VERSION 1.11.2 is a backup before anything catastrophic happens
# VERSION 1.11.3 is another backup
# VERSION 1.11.4 is another backup again
# VERSION 1.11.5 another... big structural changes
# VERSION 1.11.6 backup copy... this version has bugs
# VERSION 1.11.7 unwraps the script when dumping, if configured so
# VERSION 1.11.8 has much more documentation than before
# VERSION 1.11.9 is faster than the few recent versions
# VERSION 1.12.0 allows some strings to be moved between pages
# VERSION 1.12.1 is an upgrade to support the new o65 extension...
# VERSION 1.12.2 fixes a bug in ctdump (battle message list not dumped properly)
# VERSION 1.12.3 fixes a bug that caused utils/codegen.cc not compile
# VERSION 1.13.0 added support for expansion to 48 Mbit or 64 Mbit
# VERSION 1.13.1 added support for free relocation of all script text
# VERSION 1.13.2 is a minor bugfix to the expansion patch
# VERSION 1.13.3 is a bugfix to the checksum fixer
# VERSION 1.13.4 is a fix to the dumper. 600ad castle texts are now again ok.
# VERSION 1.13.5 is another dumper fix, but also finishes the battle VWF8 support
# VERSION 1.13.6 is yet another fix, but also finishes the monster name code
# VERSION 1.13.7 brings an improvement to the sluggish VWF8 screens problem
# VERSION 1.13.8 C++ standard compliance upgrade... also a new eq-related feature
# VERSION 1.13.9 is a vwf8 bugfix, but seems to have other problems!
# VERSION 1.14.0 is a new \"stable\" release, at least for posix.
# VERSION 1.14.1 fixes the checksum generator problem and stabilizes the windows port.
# VERSION 1.14.2 fixes a bug related to Lucca's SightScope with long monster names.
# VERSION 1.14.3 supports changing the character names! Do a redump with ctdump and see.
# VERSION 1.15.0.0 preliminary support for location events.
# VERSION 1.15.0.1 improved compression. Configuration file changes.
# VERSION 1.15.0.2 improved support for location events.
# VERSION 1.15.0.3 improved location event decompiler.
# VERSION 1.15.1 location event support - preliminary release.
# VERSION 1.15.2 now dumps the button names and allows changing them.
# VERSION 1.15.2.1 conjugator supports now [member].
# VERSION 1.15.3 conjugator now supports definition by a table.
# VERSION 1.15.3.1 minor changes in conjugator for severe grammars.
# VERSION 1.15.3.2 a bugfix in ctdump (deleting the *c block). Added more documentation!
# VERSION 1.15.3.3 minor changes in default config.
# VERSION 1.15.3.4 changes in portability, documentation, and RLE IPS support.
# VERSION 1.15.3.5 drops support for Windows versions.
# VERSION 1.15.4 adds packedblob support and fixes compilation on certain platforms.
# VERSION 1.15.5 improves compilability on more modern gcc versions
# VERSION 1.15.5.1 improves compilability on more modern gcc versions
# VERSION 1.15.6 adds rawblob and spriteblob support (thanks Michal Ziabkowski)
# VERSION 1.15.6.1 improves the LZ-variant compression a little.
# VERSION 1.15.7 improves compilability on more modern gcc versions
</pre>

To use the character name changing feature, do a redump with ctdump
and copypaste the <code>*e</code>, <code>*c</code> and <code>*s1B</code>
blocks to your script.

", '1.1.1. Release notes' => "

<p>
 Users of versions prior to 1.15.0.1: Note the changed format of
 <code>packedimage</code> and <code>add_image</code> settings in
 the configuration file!
<p>
 Users of version 1.14.3: Ensure the following two lines
 in your <tt>ct.cfg</tt> are <em>disabled</em>!<br>
 <code>load_code \"relocstr.o65\"<br>
add_call_of \"RoomScriptFunctionB8\" \$C03557 20 true</code><br>
 Otherwise you'll see wrong texts in wrong places.
<p>
 Latest Windows version is 1.14.3. Windows versions are no longer supported.

", '1. Program list' => "

", 'ctdump:1.1. ctdump' => "

Dumps the script and fonts from a given ROM.<br>
Requires <tt>chrono-dumpee.smc</tt>.<br>
Produces the script file, the font files and a couple
of other image files.
<p>
Sample of produced script:<pre class=smallerpre
>*z;dialog
;-----------------
;1000ad (Lucca's home)
;2300ad (factory)
;1000ad (rainbow shell trial)
;-----------------
\$F1IO:
[nl]
            You got 1 [item]!
\$F1IQ:
LARA: Oh, hi Crono.[nl]
   Lucca's off at Leene Square with her
   father, Taban, unveiling her new invention.
\$F1IS:
LARA: Lucca and Taban only care about their silly toys!</pre>
(Dumped from the English ROM)
<p>
Usage example:
  <code>ctdump chrono-uncompressed.smc</code>

", 'ctinsert:1.1. ctinsert' => "

Reinserts the (edited) script and (edited) fonts to a ROM.<br>
Requires the files referenced by <tt>ct.cfg</tt>
(usually <tt>ct.txt</tt>, <tt>ct8fn.tga</tt> and <tt>ct16fn.tga</tt>,
elemental images and optional extra fonts and code files).<br>
Produces <tt>ctpatch-hdr.ips</tt> and <tt>ctpatch-nohdr.ips</tt>.<br>
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
originally developed by me for Pokémon hacking.

", '1.1.1. sramdump' => "

Views a sram dump file in a readable format.

", '1.1.1. base62' => "

Converts addresses between hex and base62 formats.
I.e. \$C2:5D4C -> 0eJI and vice versa.<br>
This development system uses base62 in the script
dumps to reduce the amount of code written.

", '1. Useful features' => "

", 'conj:1.1. Player name inflection' => "

<a href=\"/ctfin/ct-code.txt\">
<img src=\"/ctfin/dev/ct-taipus2.png\" alt=\"\" title=\"It works!\" align=right>
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
to the insertor.
<p>
Expanding the character set is a complicated thing because of its
configurability. Maybe less options would be better, but then it
wouldn't always work.
<p>
The compression, btw, generally shrinks the raw script
(which is about 2/3 of the size of <tt>ct.txt</tt>)
by a factor of 30...40%.<br>
For a 370&nbsp;kB script file this means about 30&nbsp;kB of
free ROM space or 74&nbsp;kB of free dialog text space.

", 'wrap:1.1. Automatic paragraph wrapping' => "

<img src=\"/src/chronotools-wrapdemo.png\" alt=\"\" title=\"It works!\" align=right>
The program takes automatically care of proper line
lengths, so you don't have to risk running into unexpected
too-long-lines or making too short lines in paranoia.<br>
You can force line breaks (like in HTML), but you don't have to.
<p>
The image on the right shows this script piece:
<pre class=smallerpre
>Tämä on Leenen Aukio. Sanotaan, että jos
joskus kuulee Leenen Kellon soivan, elää
mielenkiintoisen ja onnellisen elämän!
</pre>
And is equal to if I had wrote this:
<pre class=smallerpre
>Tämä on Leenen Aukio. Sanotaan, että[nl]
jos joskus kuulee Leenen Kellon soivan,[nl]
elää mielenkiintoisen ja onnellisen[nl]
elämän!</pre>

<br clear=all>

", 'vwf8:1.1. Variable width 8pix font' => "

<img src=\"/ctfin/dev/ctfin-items2.png\" alt=\"\" title=\"It works!\" align=right>
Item, monster and technique names in Chrono Trigger are limited to 10 characters
(restriction is enforced by both the screen layout and the ROM space).<br>
This is way too little for many languages with long words.
 <p>
For this reason Chronotools creates a vwf8 engine that allows
the game to draw the names in thinner font that fits on the screen.
 <p>
This feature is optional and can be applied to items, techniques
and monsters - each to them separately.

<br clear=all>

", 'expand:1.1. Expansion to 48 Mbit or 64 Mbit' => "

If by whatever reason 32 Mbits is not enough for you, you can expand
the ROM size with the <tt>romsize</tt> setting in the configuration
file. Chronotools will then automatically use the extra space when
needed for all relocatable objects.<br>
Even the script can now be freely relocated if it's declared
with <code>*Z</code> (capital Z).
 <p>
However Chronotools is designed to be able to use all the built features
within a 32 Mbit ROM. You only need to increase the ROM size if you're
doing a jumbo translation (increasing the text amount by a big factor)
or adding lots of custom images.

", '1.1. Location event support' => "

<img src=\"/ctfin/dev/newobj.png\" alt=\"\" title=\"That boy wasn't previously there\" align=right>
This is something that allows basically rewriting every scene
of the game - adding new characters and objects to scenes and
making them act differently.<br>
With certain limitations though.
 <p>
This feature is in its early phase and (ab)using it is
not yet recommended. Its syntax may (will) also vary,
meaning that your hacks would most probably not compile
in the next release.
 <p>
Some (technical) up-to-date documentation of interest is available
<a href=\"http://bisqwit.iki.fi/ctfin/dev/eventdata.xml\">here</a>.

<br clear=all>

", '1.1. Very configurable' => "

I have tried to put almost everything in text-only config files
instead of hardcoding it in the programs. You won't be depending
on me to do little updates for your purposes.

", '1. Summary of extra features' => "

These are the visible extra features that games patched by Chronotools
may have when compared to the standard English version:
  <ul>
   <li>Item names longer than 11 characters (new feature)</li>
   <li>Item names displayed in thinner font (new feature)</li>
   <li>Technique names longer than 11 characters (new feature)</li>
   <li>Technique names displayed in thinner font (new feature)</li>
   <li>Ayla using a special version of Crono's name (orig. in jap. only)</li>
   <li>Equip screen showing the item count (orig. in jap. only)</li>
   <li>Character names conjugated according to language rules (new feature)</li>
   <li>Translation team logo on startup screen (new feature)</li>
   <li>Expansion to 48 Mbit or 64 Mbit</li>
  </ul>

These things are not implemented:
  <ul>
   <li>Changing the length limit of character names</li>
   <li>Changing the length limit of place names (but you can use an alternate thinner font to fit more text)</li>
  </ul>

", 'req:1. Requirements' => "

For source code (if you're a developer):
<p style=\"margin-left:2em\">
A POSIX compatible system (like Linux or FreeBSD)
with GNU tools (GNU make, GCC etc) is required.<br>
These programs are archived as C++ source code.<br>
</p>

For binaries (if you're an unfortunate user stuck with some \"Windows\"):
<p style=\"margin-left:2em\">
I don't have a microsoft-operating system here on my hand,
but I have mingw32, which appears to produce working <em>commandline</em>
billware binaries. They should work in Windows 2000, Windows 98 and
possibly most other Windows systems as well.<br>
If the \"download\" section doesn't have a recent win32 version,
you can download the source and install mingw32 and try to compile
the source yourself.<br>
&nbsp;<br>
I don't offer binaries for any other platforms.<br>
If you fear the text mode and command line, you better
change your attitude and start learning :)
</p>
<p>
The <acronym title=\"Variable width 8pix tall font\">VWF8</acronym> code,
the <tt>[crononick]</tt> code (something that was removed
in the English release of CT) and the conjugator require an assembler,
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
 Chrono Trigger hacking, throw me email.
<p>
 ".GetEmail('My email address:', 'Joel Yliluoma', 'bisqwi'. 't@iki.fi')."<br>
 If you send me e-mail, please make sure that I can reply to you.

", 'parts:1.1. Ripping my hacks' => "

Among other things, Chronotools contains the following parts
which are more or less reusable:
<ul>
 <li>base16, base62 converters</li>
 <li>LZ compressor/decompressor (no assembler versions)</li>
 <li>configuration parser</li>
 <li>character set handler and optimizer (very hard to separate)</li>
 <li>conjugator engine (very hard to separate)</li>
 <li>dictionary generator (aka. DTE compressor) (very hard to separate)</li>
 <li>fixed-width font and image handlers (C++ code) (very hard to separate)</li>
 <li>assembly module linker library</li>
 <li>block-fitter (binpacking, space optimization)</li>
 <li>cross-referencing patches, reference handling and combining (hard to separate)</li>
 <li>XML parsing</li>
 <li>8pix variable width font engine (very hard to separate)</li>
 <li>font typeface generator (hard to reuse)</li>
 <li>ROM checksum fixer</li>
 <li><a href=\"http://bisqwit.iki.fi/source/snescom.html\">assembler, disassembler and a linker</a></li>
</ul>

You almost certainly need programming expertise to reuse anything of those.
Before doing anything, please
<a href=\"#copying\">send me email and explain your situation</a>.

", 'docs:1. Documentation' => "

<ul>
 <li><a href=\"http://bisqwit.iki.fi/ctfin/dev/quickstart.html\">Quick start guide</a></li>
 <li><a href=\"http://bisqwit.iki.fi/ctfin/dev/fonts.html\">Fonts and character sets guide</a></li>
 <li><a href=\"http://bisqwit.iki.fi/ctfin/dev/imageformat.html\">Image format</a> (the <tt>.tga</tt> files must obey these rules)</li>
 <li><a href=\"http://bisqwit.iki.fi/ctfin/dev/signature.html\">Startup screen logo</a></li>
 <li><a href=\"http://bisqwit.iki.fi/ctfin/dev/VWF8.html\">Item length expansion</a> (the small variable width font)</li>
 <li><a href=\"http://bisqwit.iki.fi/ctfin/dev/compression.html\">Compression</a> (script compression explained)</li>
 <li><a href=\"http://bisqwit.iki.fi/ctfin/dev/conjugation.html\">Conjugation</a> (inflecting character names according to grammar rules)</li>
 <li><a href=\"http://bisqwit.iki.fi/ctfin/dev/crononick.html\">Crononick</a> (Ayla's special version of Crono's name)</li>
 <li><a href=\"http://bisqwit.iki.fi/ctfin/dev/froggy.html\">Froggy</a> (What's up with \"Robos\" and \"Froggy\")</li>
</ul>

", 'undocs:1.1. Undocumented things' => "

Things that should be documented some day but currently are not:
<ul>
 <li>How does the character map actually work</li>
 <li>Known bugs and their resolutions</li>
 <li>Script format guidelines (how do the indents work)</li>
 <li>Tips and hints one should know</li>
 <li>The source code</li>
</ul>

", 'helpneeded:1. See also' => "

<ul>
 <li><a href=\"http://bisqwit.iki.fi/ctfin/\">Bisqwit's
  Finnish Chrono Trigger translation project</a>
  (uses these tools)</li>
 <li><a href=\"http://bisqwit.iki.fi/source/snescom.html\">snescom</a>
  - xa65-compatible 65816 assembler with free source code</li>
 <li><a href=\"http://bisqwit.iki.fi/jutut/ctcset.html\">Chrono Trigger
  technical document</a>
  (a very modest document that got me started in this whole thing)</li>
</ul>

<hr>
<h3>Screenshots</h3>

<div style=\"float:left\">
 <img src=\"/src/chronotools-fin.png\"
      alt=\"sample\" style=\"padding-right:10px\" ><br>
 <small> Example screenshot in Finnish </small>
</div>
<div style=\"float:left\">
 <img src=\"/src/chronotools-pol.png\"
      alt=\"sample\" style=\"padding-right:10px\" ><br>
 <small> Example screenshot in Polish </small>
</div>
<div style=\"float:left\">
 <img src=\"/src/chronotools-cro.png\"
      alt=\"sample\" style=\"padding-right:10px\" ><br>
 <small> Example screenshot in Croatian </small>
</div>
<div style=\"float:left\">
 <img src=\"/src/chronotools-rus.png\"
      alt=\"sample\" style=\"padding-right:10px\" ><br>
 <small> Example screenshot in Russian </small>
</div>
<br clear=all>

");

$pagebegin      = '';
$contentsheader = '';
$pageend        = '';

include '/WWW/progdesc.php';
