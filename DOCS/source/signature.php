<?php

/* This file is the source for some html file. Read it instead. */

$title = 'Chronotools - signature feature';
$progname = 'chronotools';

$text = Array(
   '1. Introduction' => "
   
<a href=\"http://bisqwit.iki.fi/source/chronotools.html\">Chronotools</a>
allows you to insert arbitrary SNES code to your translation.
This includes a \"signature\" feature.
  <p>
Signature is a logo, displayed on the startup screen of the game,
as shown below for example:<br>
<img src=\"http://bisqwit.iki.fi/ctfin/dev/ctfin-screen2.png\" alt=\"Chronofin logo\">
  <p>
  &nbsp;
  <p>
ATTENTION: This feature is provided for making your publication
identified - preventing people claiming it as their own. Do not
use it for bragging. I recommend that you will ignore this feature
until your translation is ready for test releases.

", '1. How to' => "

If you want to add a signature, the easiest way is as follows:
  <p>
Add the following lines to your <tt>ct.cfg</tt>
in the <code>[linker]</code> section:

<pre>
# SIGNATURE
load_code \"ct-moglogo.o65\"
add_call_of \"show_moglogo\"   \$FDE62F 0 false
add_image   \"moglogo.tga\"    \"TILEDATA_ADDR\"  \"PALETTE_ADDR\" \"PALETTE_SIZE\"
</pre>

This tells the insertor that it must include code from <tt>ct-moglogo.o65</tt>
and insert a call of the routine <code>show_moglogo</code> at \$FDE62F in the ROM,
and that it should read and insert the image <tt>moglogo.tga</tt>, compress
it and assign several constants for referring to it.
  <p>
You can create <tt>ct-moglogo.o65</tt> by compiling the
included <tt>ct-moglogo.a65</tt> file with
<a href=\"http://bisqwit.iki.fi/source/snescom.html\">snescom</a>,
a SNES assembler, or asking me to send it to you.

");
