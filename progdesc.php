<?php
//TITLE=Chrono Trigger translation development system

$title = 'Chrono Trigger translation development system';
$progname = 'chronotools';

$text = array(
   '1. Purpose' => "

Tools aiding in
<a href=\"http://bisqwit.iki.fi/ctfin/\">Bisqwit's Finnish Chrono Trigger translation</a>.
<p>
Meant to be useful for anyone who wants to become
a translator for Chrono Trigger and translate the
game to their own language.

", '1. Program list' => "

", '1.1. ctdump' => "

<img src=\"http://bisqwit.iki.fi/src/chronotools-bat.png\"
     align=right alt=\"sample\" style=\"padding-right:10px\" >

Dumps the script and fonts from a given ROM.<br>
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

Reinserts the (edited) script and (edited) fonts to a ROM.<br>
Requires <code>ct.txt</code>,
<code>ct8fn.tga</code> and <code>ct16fn.tga</code>.<br>
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
originally developed by me for Pokémon hacking.

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

", '1.1. Technical report' => "

My goal is to make a complete Finnish translation of Chrono Trigger.
<p>
The project currently depends on solving the following problems.  

", '1.1.1. Item/technique/monster names' => "

<img src=\"http://bisqwit.iki.fi/src/chronotools-vwf8.png\" alt=\"It works!\" align=right>
Item and technique names are limited to 10 characters
(restriction is enforced by both the screen layout and the ROM space).<br>
This is way too little for Finnish, which has long words.<br>   
Solution A: Leave the item names and technique names untranslated.<br>
This is not a considerable solution for me.<br>
Solution B: Move the names to a different location in ROM so that
there is no space limit. But: They still don't fit on screen.<br>
Solution C: Add code using <a href=\"/ctfin/ct8f.png\">variable width 8x8 font</a>
and rewrite the method the item names are looked up.
This means <em>lots of work</em>. But I attempt to do it anyway :)
<br clear=all>

", '1.1.1. Name conjugation' => "

<a href=\"/ctfin/ct-code.txt\">
<img src=\"/kala/snap/ctdevel/ct-taipus2.png\" alt=\"It works!\" align=right>
</a>  
Finnish is a language where words are conjugated.
Just adding a substring like \"'s\" doesn't make
correct language. The whole word stem changes a bit.<br>
Solution A: Ignore the problem. But: This is crude. I don't want it.<br>
Solution B: Add <a href=\"taipus.rb\">code that conjugates the names</a>.
This means <em>lots of work</em>. 30.6.2003 I did it! It works.
<br><a href=\"#conj\">See below.</a>
<br clear=all>

", '1. Useful features' => "

", 'conj:1.1. Name conjugation' => "

It currently has support for conjugating names on fly.<br>
It's very important in Finnish, where you can't just
add \"'s\" to anything to make a genitive.<br>
For example, genitive of name Matti is \"Matin\",
and genitive of name Crono is \"Cronon\".<br>
The conjugator-engine is a textual script file
translated to 65c816 assembly on demand.

", '1.1. Font/dictionary skew' => "

It's quite complicated to explain, but shortly said:
<p>
In normal Chrono Trigger,
<ul>
 <li>127 of them are assigned to the dictionary used to compress the script.</li>
 <li><b>96</b> of them are
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
If you are translating to a language that utilizes thousands of
different symbols, like Chinese, you might want to try to base on the
Japanese version instead. This utility pack bases on the
English version.<br>
(Sorry, I can't help you with issues regarding the Japanese ROM.)

", '1.1. Automatic paragraph wrapping' => "

The program takes automatically care of proper line
lengths, so you don't have to risk running into unexpected
too-long-lines or making too short lines in paranoia.<br>
You can force line breaks, but you don't have to.

", '1.1. Very configurable' => "

I have tried to put almost everything in text-only config files
instead of hardcoding it in the programs. You won't be depending
on me to do little updates for your purposes.

", '1. Copying' => "

If you're interested, throw me email.<br>
But <em>read the <a href=\"/ctfin/\">translation project page</a> first</em>!
<p>
My email-address (sigh) is:
<em>bisqwit a<b style=\"font-weight:lighter\">t i</b>ki <small>dot</small> fi</em>
</p>
I'm not publishing files on this web page, because it's a well-known fact
that many people in ROM hacking scene aren't very respectful to copyrights.

", '1. Requirements' => "

A POSIX compatible system (like Linux or FreeBSD)
with GNU tools (GNU make, GCC etc) is required.<br>
These programs are archived as C++ source code only.
<p>
I don't have a microsoft-operating system here on my hand,
so if you are an unfortunate user stuck with some Windows,
you just have to find a development system
(<a href=\"http://www.google.com/search?q=cygwin\">cygwin</a>?)
and compile the program on it to use it.
<p>
I'll soon try if I can get windows-binaries out with mingw32.
Currently it has problems with iconv.

", '1. Changelog' => "

Copypaste from the Makefile:

<pre class=smallerpre
># VERSION 1.0.3  was the first working! :D
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
</pre>

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
