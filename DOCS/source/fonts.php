<?php

/* This file is the source for some html file. Read it instead. */

$title = 'Chronotools - fonts and character sets';
$progname = 'chronotools';

$text = Array(
   '1. Introduction' => "

<a href=\"http://bisqwit.iki.fi/source/chronotools.html\">Chronotools</a>
has been designed to allow as much localization
of Chrono Trigger as possible. This includes the possibility
of using nearly any character set besides the English one.

", '1. Terms' => "

In this document (and many others), the following terms are used.

", '1.1. Script character set' => "

   This encoding is your system-dependent encoding of characters to
   byte values. It is used in your script files and in your configuration
   file. It is not used in the patch or ROM.
     <p>   
   The name of this encoding is written to the <tt>characterset</tt>
   field in your configuration file.<br>
   Some possible values include:
   <table>
    <tr> <th>iso-8859-1  </th> <td> Western European encoding, aka. \"latin1\" </td> </tr>
    <tr> <th>iso-8859-2  </th> <td> Polish, Czech, Slovak, Slovenia, Hungarian </td> </tr>
    <tr> <th>iso-8859-5  </th> <td> Cyrillic encoding: Russian, Ukrainian, Belarusian </td> </tr>
    <tr> <th>iso-8859-7  </th> <td> Greek encoding </td> </tr>
    <tr> <th>iso-8859-8  </th> <td> Modern Hebrew encoding </td> </tr>
    <tr> <th>iso-8859-9  </th> <td> Turkish, Maltese, Esperanto </td> </tr>
    <tr> <th>iso-8859-10 </th> <td> Estonian, Latvian, Lithuanian, Greenlandic, Saami </td> </tr>
    <tr> <th>iso-8859-11 </th> <td> Thai </td> </tr>
    <tr> <th>iso-8859-16 </th> <td> Albanian, Croatian, Romanian, Gaelic etc with Euro </td> </tr>
    <tr> <th>windows-1250 </th> <td> Microsoft version of iso-8859-2 </td> </tr>
    <tr> <th>windows-1251 </th> <td> Microsoft version of iso-8859-5 </td> </tr>
    <tr> <th>windows-1252 </th> <td> Microsoft version of latin1 and latin9 </td> </tr>
    <tr> <th>utf-8       </th> <td> The unicode character encoding </td> </tr>
    <tr> <th>shift-jis   </th> <td> Microsoft doublebyte Japanese encoding </td> </tr>
   </table>

", '1.1. 12pix font mapping' => "

   This is the array of characters specified using the <tt>font12_N</tt>
   settings in your configuration file.<br>
   Your 12pix font file (<tt>font12fn</tt>) <em>must</em> match
   exactly the contents of this array (excluding the <tt>typeface</tt>s).<br>
   In addition to the <tt>font12_N</tt> settings, also <tt>def12sym</tt>
   settings can be used to specify the contents of the font.
  <p>
   This mapping is used by:
   <ul>
    <li> Dialog text </li>
    <li> Player character names (in dialog) </li>
    <li> Item descriptions (in the item screen) </li>
    <li> Item names (in the \"got X\" messages) </li>
    <li> Monster names (when aiming in battle) </li>
    <li> Technique descriptions (in the technique screen) </li>
    <li> Technique names (in the \"learned tech X\" messages) </li>
    <li> Location names (in the overworld map) </li>
    <li> Chapter names (in the save/load screen) </li>
    <li> Game settings screen help texts </li>
   </ul>
   Therefore, all character symbols that appear in those texts
   <em>must</em> be present in the 12pix font mapping
   (and consequently, in the 12pix font).

", '1.1. 8pix font mapping' => "

   This is the array of characters specified using the <tt>font8_N</tt>
   settings in your configuration file.<br>
   Your 8pix font file (<tt>font8fn</tt>) <em>must</em> match
   exactly the contents of this array.<br>
   In addition to the <tt>font8_N</tt> settings, also <tt>def8sym</tt>
   settings can be used to specify the contents of the font.
  <p>
   This mapping is used by:
   <ul>
    <li> Player character names (in battle) </li>
    <li> Item names (in battle and item screen, regardless whether you use VWF8) </li>
    <li> Monster names (in battle) </li>
    <li> Technique names (in battle and tech screen, regardless whether you use VWF8) </li>
    <li> Status screen messages </li>
   </ul>
   Therefore, all character symbols that appear in those texts
   <em>must</em> be present in the 8pix font mapping
   (and consequently, in the 8pix font).

", '1.1. 12pix font file' => "

   This font file contains the actual character symbols of the
   12pix font. When you edit it, please remember the restrictions
   listed in <a href=\"imageformat.html\">the image format guide</a>
   and that it <em>must</em> match the contents of the <tt>font12_N</tt>
   map.

", '1.1. 8pix font file' => "

   This font file contains the actual character symbols of the
   constant-width 8pix font (used for most of the status screen content).
   When you edit it, please remember the restrictions
   listed in <a href=\"imageformat.html\">the image format guide</a>
   and that it <em>must</em> match the contents of the <tt>font8_N</tt>
   map.

", '1.1. VWF8 font file (optional)' => "

   This font file contains the actual character symbols of the
   variable-width 8pix font (used for the aspects you select it for).
   When you edit it, please remember the restrictions
   listed in <a href=\"imageformat.html\">the image format guide</a>.<br>
   Constraints for its character mapping are explained later in this document.

", '1.1. Locked symbols' => "

   When a symbol is \"locked\", using the <code>lock12syms</code>
   or <code>lock8syms</code> directive in the configuration file,
   it means that the position of the symbol in question will not
   be changed in the font in question.<br>
   This is useful for symbols that have positional constraints.
   These symbols are:
   <ul>
    <li>Symbols that are used in strings that are displayed
     using different fonts (explained later in this document).</li>
    <li>Symbols that are referred by the game code (for example,
     the star symbol and numbers in the 8pix font).</li>
    <li>Custom graphics you use using the <code>[gfx]</code>
     declaration in your <code>*r</code> section of the script file.</li>
   </ul>

", '1.1. Typefaces' => "

   Typeface in Chronotools is a technique to provide the same
   12pix text in different font styles. An example typeface
   could be italic.
     <p>
   This is accomplished by copying a region of the font to
   a different location in the font, editing it and then
   declaring it in the configuration file using the
   <code>typeface</code> statement.

", '1. Constraints' => "

", '1.1. Character set limits' => "

Due to how Chrono Trigger is built, there are several limitations in
how you can build the fonts and character tables.
  <p>
For understanding the explanations here you need to
have rudimentary understanding of hex numbers.

", '1.1.1. Limits in the 8pix font' => "

The 8pix font has 256 different indexes - \$00 to \$FF for characters.
There's no way around it. But not all of them can be used freely.

", '1.1.1.1. Reserved slots (in character set)' => "

This list tells which symbols may not be used in 8pix
character set, because they already have different meanings.
<ul>
 <li>Indexes \$00-\$0F are reserved for control bytes.</li>
 <li>Indexes \$2A-\$2F are used internally by the game.</li>
 <li>Indexes \$40-\$68 are used by Japanese legacy code (voiced versions of \$80..\$A8), unless the feature is disabled in configuration.</li>
 <li>Indexes \$5B-\$5C are used by item scrolling screens.</li>
 <li>Indexes \$60-\$63 are used by the item cursor.</li>
 <li>Indexes \$67-\$6F are used by the health and tech gauges.</li>
 <li>Indexes \$69-\$72 are used by Japanese legacy code (semivoiced versions of \$80..\$89), unless the feature is disabled in configuration.</li>
 <li>Indexes \$73-\$7C are used by the status numbers, if you enable <code>use_thin_numbers</code>.</li>
</ul>

I'm not sure of the status of areas \$10-\$1F and \$30-\$3F.

", '1.1.1.1. Reserved slots (in tilemap)' => "

This list tells which slots from the 8pix tilemap can not be
used, because they are already used for something else.
<ul>
 <li>Indexes \$00-\$0B are reserved by elemental symbol graphics</li>
 <li>Indexes \$15-\$1F are reserved by VWF8, if you use VWF8.</li>
 <li>Indexes \$2A-\$2F are used internally by the game.</li>
 <li>Indexes \$30-\$5B are reserved by VWF8, if you use VWF8.</li>
 <li>Indexes \$5B-\$5C are used by item scrolling screens.</li>
 <li>Indexes \$5C-\$66 are reserved by VWF8, if you use VWF8.</li>
 <li>Indexes \$60-\$63 are used by the item cursor.</li>
 <li>Indexes \$64-\$66 are used by Japanese legacy code (normal/voiced/semivoiced markers), unless the feature is disabled in configuration.</li>
 <li>Indexes \$67-\$6F are used by the health and tech gauges.</li>
 <li>Indexes \$67-\$71 are reserved by VWF8, if you use VWF8.</li>
 <li>Indexes \$73-\$7C are used by the status numbers, if you enable <code>use_thin_numbers</code>.</li>
 <li>Indexes \$7D-\$9D are reserved by VWF8, if you use VWF8.</li>
</ul>

", '1.1.1.1. Usable slots' => "

This leaves the following areas for your use:
<ul>
 <li>\$40-\$5A (27 slots) if you disabled the Japanese legacy code <em>and</em> don't use VWF8</li>
 <li>\$64-\$66 (3 slots) if you disabled the Japanese legacy code</li>
 <li>\$73-\$7C (10 slos) unless you use <code>use_thin_numbers</code></li>
 <li>\$7D-\$9D (33 slots) unless you use VWF8</li>
 <li>\$9E-\$FF (98 slots)</li>
</ul>

VWF8 needs quite many slots from the tile table (110, to be exact) to
draw the symbols on. This is an unavoidable technical limitation.

", '1.1.1. Limits in the 12pix font' => "

The 12pix font has 768 different indexes - \$000 to \$2FF for characters.<br>
It was this way in the Japanese version (yes, only 768!), and it wasn't
changed in the USA version.<br>
But there are some constraints regarding their use.
<ul>
 <li>Indexes \$00-\$20 are reserved for control bytes.</li>
 <li>Indexes \$21-\$9F are reserved for dictionary. You can change the
   upper number, but it affects the compressibility of your script.</li>
</ul>
This leaves the following areas for your use:
<ul>
 <li>\$A0-\$2FF (the lower number is determined by your dictionary
  size, configured by the <code>begin</code> setting in your configuration file.</li>
</ul>
However, characters in the \$100-\$2FF range take two bytes of ROM space
whereas characters below the \$100 marker take only 1 byte. This aspect
may well be ignored, because Chronotools automatically rearranges
the fonts optimally under the given constraints.

", '1.1. The positional constraints' => "

This is where it all gets problematic.
  <p>
Some texts are displayed in multiple fonts. Characters that are 
used to compose those strings must appear in exactly same positions
in all of the fonts the strings are written in.
  <p>
Character names (Crono, Marle, Lucca and so on) are displayed
in 8pix font (equipment screen, save-game) and 12pix font (dialog).
Therefore all the characters that <em>may</em> compose their names
<em>must</em> be in same slots in both of those fonts.
  <p>
This restriction applies to all the character symbols
that <em>are</em> or <em>could be</em> used in any of
the following things:
<ul>
 <li>Item names</li>
 <li>Technique names</li>
 <li>Monster names</li>
 <li>Player character names</li>
</ul>
Generally, this includes all the alphabet symbols as well
as some punctuation.
  <p>
This effectively means the following things:
<ul>
 <li>The said set of characters (hereon called as the SET)
   <em>must all</em> be <em>locked</em> using the <code>lock12syms</code>
   and the <code>lock8syms</code> directives.<br>
   This prevents Chronotools from moving them around.
  </li>
 <li>Characters of the SET <em>must all</em> appear in exactly
   the same positions in both the <code>font12_N</code> and
   the <code>font8_N</code> maps.
  </li>
 <li>Characters of the SET <em>must all</em> appear in the
    range that both the 8pix and 12pix fonts can agree on.<br>
    Normally, this means \$A0-\$FF (96 slots), but the lower limit
    may be smaller if your dictionary is smaller.
    It mustn't however spill on the area that is not
    allowed for the 8pix font.
  </li>
</ul>
This restriction may make things difficult for languages that
have lots of character symbols. Unfortunately, there's currently
no way around it in Chronotools.

", '1. Hints' => "

<ul>
 <li> You should put the symbols that are not part of the SET
     to outside of the constraint range to increase the number
     of symbols you can use in the SET.</li>

 <li> If you use <code>use_thin_numbers</code>, you can use
     the \$D3-\$DD range in 8pix font for something else
     (assuming you don't have numbers in the SET).<br>
     If you do that, you need to explicitly mark the
     \$73-\$7C range as numbers in your <code>font8_N</code> table.<br>
     Unfortunately, the \$D3-\$DD range can not currently
     be moved in the 12pix font because the game refers
     to it when it prints numbers. This might be changed
     in future Chronotools releases.
  </li>
</ul>

", '1. The VWF8 character mapping' => "

Since VWF8 is never used for printing text other than that is under
the \"positional constraints\" rule, the VWF8 character map
is practically the same as the 8pix font map. Technically though,
it allows 768 symbols just as the 12pix font does.

");
