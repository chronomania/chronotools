<?php

/* This file is the source for some html file. Read it instead. */

$title = 'Chronotools - variable width 8pix font';
$progname = 'chronotools';

$text = Array(
   '1. Introduction' => "
   
If you find the standard item name length limit (10 characters plus symbol)
or the standard tech name length limit (10 characters plus symbol)
too restricting, forcing you to make ridiculous abbreviations,
you can install a support for variable width 8pix font.
  <p>
In <a href=\"http://bisqwit.iki.fi/source/chronotools.html\">Chronotools</a>,
this consists of the following components:

<ul>
 <li><a href=\"#font\">VWF8 font</a></li>
 <li><a href=\"#code\">VWF8 program module</a></li>
 <li><a href=\"#font\">Binding the program module to the ROM</a></li>
 <li><a href=\"#convert\">Conversion of the item / tech / monster tables</a></li>
</ul>

", 'font:1. VWF8 font' => "

You need a font. Take one from <tt>DOCS/ct8fnV.tga</tt> and
edit it to match your character set.
 <p>
Edit the <code>font8vfn</code> setting in the <code>[font]</code>
section of your configuration file so that the insertor knows the font.
 <p>
Please read <a href=\"fonts.html\">the fonts and character sets</a>
guide regarding the character mapping of this (and other) fonts.

", 'code:1. VWF8 code' => "

Add the following lines to your <tt>ct.cfg</tt>
in the <code>[linker]</code> section:

<pre>
# VWF8
load_code \"ct-vwf8.o65\"
# VWF for items: If your item list begins with *i11 (not *l11):
add_call_of \"EquipLeftItemFunc\"  \$C2A5AA 4  false
add_call_of \"EquipRightItemFunc\" \$C2F2DC 1  true
add_call_of \"ItemListFunc\"       \$C2B053 13 false
add_call_of \"EquipLeftHelper\"    \$C2A57D 14 false
add_call_of \"EquipLeftHelper2\"   \$C2A5B6 2  false
add_call_of \"BattleItemFunc\"     \$C109C1 32 false
add_call_of \"DialogItemCall\"     \$C25BA1 36 false

# VWF for techs: If your tech list begins with *t11 (not *l11):
add_call_of \"BattleTechFunc\"     \$C10B5B 32 false
add_call_of \"BattleTechHelper\"   \$C10A99 0 false
add_call_of \"DialogTechCall\"     \$C25A90 55 false
add_call_of \"\"                   \$C10ABE 16 false
add_call_of \"TechListFunc\"       \$C2BDB2 52 true

# For monsters: If your monster list begins with *m11 (not *l11):
add_call_of \"BattleMonsterFunc\"  \$CCED13 60 false
add_call_of \"DialogMonsterCall\"  \$C25AD2 35 false
</pre>

This tells the insertor that it must include code from <tt>ct-vwf8.o65</tt>
and insert calls of several routines to your ROM.<br>
By selecting the right <code>add_call_of</code> lines you select
which features of the ROM you want to patch.
  <p>
You can create a new <tt>ct-vwf8.o65</tt> file by
compiling the included <tt>ct-vwf8.a65</tt> file with
<a href=\"http://bisqwit.iki.fi/source/snescom.html\">snescom</a>.

", 'convert:1. Conversion of the tables in script' => "

Normally the items, techs and monsters are inserted as fixed-width
tables in the ROM, denoted by the label <code>*l11</code> in your script.
 <p>
When you are ready to get rid of the item length limit, replace the
<code>*l11</code> in the script with <code>i11</code>.<br>
Same goes for techs: <code>*l11</code> is changed to <code>t11</code>,<br>
and for monsters <code>*l11</code> is changed to <code>m11</code>.
 <p>
Note: Monster names are not displayed in VWF8, but their lon.

");
