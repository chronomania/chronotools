<?php

/* This file is the source for some html file. Read it instead. */

$title = 'Chronotools - signature feature';
$progname = 'chronotools';

$text = Array(
   '1. Introduction' => "
   
Chronotools allows you to insert arbitrary SNES code to your translation.
This includes a \"signature\" feature.
<p>
Signature is a logo, displayed on the startup screen of the game,
as shown below for example:<br>
<img src=\"http://bisqbot.stc.cx/img/ct/ctfin-screen2.png\" alt=\"Chronofin logo\">

", '1. How to' => "

If you want to add a signature, the easiest way is as follows:
<p>
Add the following lines to your <code>ct.cfg</code>
in the <code>[linker]</code> section:

<pre>
# SIGNATURE
load_code \"ct-moglogo.o65\"
add_call_of \"show_moglogo\"       \$FDE62F 0 false
add_image   \"moglogo.tga\"   \$7F \"TILEDATA_ADDR\"  \"PALETTE_ADDR\" \"PALETTE_SIZE\"
</pre>

This tells the insertor that it must include code from <code>ct-moglogo.o65</code>
and insert a call of the routine <code>show_moglogo</code> at \$FDE62F in the ROM,
and that it should read and insert the image <code>moglogo.tga</code>, compress
it (page \$7F) and assign several constants for referring to it.
<p>
You can create <code>ct-moglogo.o65</code> by compiling the
included <code>ct-moglogo.a65</code> file with
<a href=\"http://bisqwit.iki.fi/source/snescom.html\">snescom</a>,
a SNES assembler, or asking me to send it to you.

");
