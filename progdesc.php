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
Taistelun aikana jokaisen iskun teho vaihtelee iskun mukaan.
\$HSs6:
Ensinnäkin, voit iskeä myös useampaa, kuin
   yhtä vihollista kerralla.
\$HSs8:
Voit esimerkiksi tähdätä tätä hirviötä...</pre>
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

Compares two ROMs and produces a patch file in IPS format.

", '1.1.1. unmakeips' => "

Reads a ROM and an IPS file and produces a patched ROM file.

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
It conjugates names in Finnish. It's supposed to get translated
to SNES assembler by somebody.

", '1.1.1. sramdump' => "

Views a sram dump file in a readable format.

", '1.1.1. base62' => "

Converts addresses between hex and base62 formats.
I.e. \$C2:5D4C -> rRhU and vice versa.<br>
This development system uses base62 in the script
dumps to reduce the amount of code written.

", '1. Useful features' => "

", '1.1. Name conjugation' => "

It currently has support for conjugating names on fly.<br>
It's very important in Finnish, where you can't just
add \"'s\" to anything to make a genitive.<br>
For example, genitive of name Matti is \"Matin\",
and genitive of name Crono is \"Cronon\".<br>
The conjugator-engine is a textual script file
translated to 65c816 assembly on demand.

", '1.1. Font/dictionary skew' => "

In the script code, there are different 223 bytes available for the script.
<p>
In normal Chrono Trigger,
<ul>
 <li>127 of the are assigned to the dictionary</li>
 <li>96 of them are visible symbols.</li>
</ul>
Which means that you can only have 96 different characters.
take A-Z, a-z, 0-9 and punctuation symbols and count how
many you have left for umlauts and accents.
<p>
My insertor however allows to skew that ratio so
that the dictionary may be smaller and font bigger.
<p>
However, shrinking the dictionary means that the script
compresses worse, and could probably not fit in the ROM.<br>
It might still be a lifesaver for someone
doing an Estonian or Portuguese translation.
<p>
If you are translating to a language that utilizes <em>lots</em> of
different symbols, like Chinese, you might want to try to base on the
Japanese version instead. This utility pack bases on the
English version.<br>
(Sorry, I can't help you with the Japanese ROM.)

", '1.1. Automatic paragraph wrapping' => "

The program takes automatically care of proper line
lengths, so you don't have to risk running into unexpected
too-long-lines or making too short lines in paranoia.<br>
You can force line breaks, but you don't have to.

", '1. Copying' => "

If you're interested, throw me email.<br>
But <em>read the <a href=\"/ctfin/\">translation project page</a> first</em>!
<p>
My email-address (sigh) is:
<em>bisqwit a<b style=\"font-weight:lighter\">t i</b>ki <small>dot</small> fi</em>
</p>
I'm not publishing files, because it's a well-known
fact that many people in ROM hacking scene aren't
very respectful to copyrights.

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
