<?php

/* This file is the source for some html file. Read it instead. */

$title = 'Chronotools - conjugator';
$progname = 'chronotools';

$text = Array(
   '1. Introduction' => "
   
Chronotools supports conjugation - inflecting the character names depending on context.
 <p>
Here I'll explain how it was done for Finnish.
 <p>
This consists of the following sections:
<ul>
 <li><a href=\"#plan\">Planning</a></li>
 <li><a href=\"#cfg\">Configuration</a></li>
 <li><a href=\"#code\">Program code</a></li>
 <li><a href=\"#bind\">Binding the code to ROM</a></li>
</ul>

", 'plan:1. Planning' => "

First you need to find out which conjugations you're using.<br>
For Finnish, they were the following:

<pre>
Form               Function name   ZGX     R66     Pekka   Magus      Crono  
-----------------------------------------------------------------------------
genitive/accusative  Do_N          ZGX:n   R66:n   Pekan   Maguksen   Cronon
partitive            Do_A          ZGX:‰‰  R66:a   Pekkaa  Magusta    Cronoa
inessive             Do_SSA        ZGX:ss‰ R66:ssa Pekassa Maguksessa Cronossa
adessive             Do_LLA        ZGX:ll‰ R66:lla Pekalla Maguksella Cronolla
ablative             Do_LTA        ZGX:lt‰ R66:lta Pekalta Magukselta Cronolta
allative             Do_LLE        ZGX:lle R66:lle Pekalle Magukselle Cronolle
elative              Do_STA        ZGX:st‰ R66:sta Pekasta Maguksesta Cronosta
abessive             Do_TTA        ZGX:tt‰ R66:tta Pekatta Maguksetta Cronotta
translative          Do_KSI        ZGX:ksi R66:ksi Pekaksi Magukseksi Cronoksi
essive               Do_NA         ZGX:n‰  R66:na  Pekkana Maguksena  Cronona
illative             Do_HUN        ZGX:‰‰n R66:een Pekkaan Magukseen  Cronoon
</pre>

The \"Form\" here refers to the linguistic name of the conjugation.
Chronotools doesn't use it.<br>
\"Function name\" is the name of the function used in
your <a href=\"#code\">code file</a>. The rest of this table
are examples of how different names are conjugated in the given forms.

", 'cfg:1. Configuration' => "

Next you must configure each conjugation.
For this there's the <code>[conjugator]</code> section in your configuration file.
It is expected to consist of <code>setup</code> clauses.
<p>
Here's part of what it is in the Finnish version:
<pre>
[conjugator]
# Setup each conjugation.
#  First param:
#    The function name that handles the conjugation   
#    (as defined in file codefn)
#  Second param:
#    List of instances in your script that will
#    be replaced with conjugation code when inserting.
#  Third param:
#    String that represents the maximum length
#    the conjugation adds to the name
#
# c=crono m=marle l=lucca r=robo f=frog a=ayla u=magus e=epoch
# 1=member1 2=member2 3=member3
setup \"Do_A\" \"
 cCronoa,mMarlea,lLuccaa,rRobosta,fFrogia,fFroggia,
 aAylaa,uMagusta,uMaguksea,eEpochia,
 1[member1]:a,2[member2]:a,3[member3]:a,
 1[member1]:‰,2[member2]:‰,3[member3]:‰\" \":ta\"

setup \"Do_LLA\" \"
 cCronolla,mMarlella,lLuccalla,lLucalla,rRoboksella,fFrogilla,
 aAylalla,uMaguksella,uMagusilla,eEpochilla,
 1[member1]:lla,2[member2]:lla,3[member3]:lla,
 1[member1]:ll‰,2[member2]:ll‰,3[member3]:ll‰\" \"kella\"
</pre>

The first parameter of <code>setup</code> is the function name
that is responsible of handling this conjugation.
<p>
The second parameter is the list of all strings that the
conjugator will assume as a call to this conjugation.
The parameter is commaseparated and each string begins
with a letter: c for Crono, m for Marle, l for Lucca, r for Robo,
f for Frog, a for Ayla, u for Magus, e for Epoch, 1 for
the first member of party, 2 for the second member of party
and 3 for the third member of the party.
<p>
The third parameter is a string that is used by the script
wrapping code. It describes the <em>widest</em> possible string
that the conjugation could append to the character name.

", 'code:1. Creating the conjugator code' => "

This is the hardest part for average people.<br
You have to write your own conjugator!<br>
Have a look at <code>DOCS/ct-conj.code</code> (the Finnish conjugator
as an example) and try to grasp it.<br>
Sorry, I can't help you more than this.
<p>
Anyway, when you are done with your code file, you just compile
it with utils/compile, combine it with ct-conj.a65 and assemble
it with <a href=\"http://bisqwit.iki.fi/source/snescom.html\">snescom</a>.
Basically, once you have snescom installed, you can just do
<code>make ct-conj.o65</code> and that should do it.

", 'bind:1. Binding the code to ROM' => "

Add the following lines to your <code>ct.cfg</code>
in the <code>[linker]</code> section:

<pre>
# CONJUGATOR
load_code \"ct-conj.o65\"
add_call_of \"Conjugator\"         \$C258C2 1 false
</pre>

This tells the insertor that it must include code from <code>ct-conj.o65</code>
and that the Conjugator routine will be called from the ROM address <code>\$C258C2</code>.
<p>
I hope I didn't forget anything.
<p>
Ask me for help if you didn't get it :)

");
