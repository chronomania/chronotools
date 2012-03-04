<?php

/* This file is the source for some html file. Read it instead. */

$title = 'Chronotools - conjugator';
$progname = 'chronotools';

$text = Array(
   '1. Introduction' => "
   
<a href=\"http://bisqwit.iki.fi/source/chronotools.html\">Chronotools</a>
supports conjugation - inflecting the character names depending on context.
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
You need to be a expert in the grammar of your language to do it properly.<br>
For Finnish, they were the following:

<pre>
                        Example names
Form                   ZGX     R66     Pekka   Magus      Crono  
-----------------------------------------------------------------
genitive/accusative    ZGX:n   R66:n   Pekan   Maguksen   Cronon
partitive              ZGX:ää  R66:a   Pekkaa  Magusta    Cronoa
inessive               ZGX:ssä R66:ssa Pekassa Maguksessa Cronossa
adessive               ZGX:llä R66:lla Pekalla Maguksella Cronolla
ablative               ZGX:ltä R66:lta Pekalta Magukselta Cronolta
allative               ZGX:lle R66:lle Pekalle Magukselle Cronolle
elative                ZGX:stä R66:sta Pekasta Maguksesta Cronosta
abessive               ZGX:ttä R66:tta Pekatta Maguksetta Cronotta
translative            ZGX:ksi R66:ksi Pekaksi Magukseksi Cronoksi
essive                 ZGX:nä  R66:na  Pekkana Maguksena  Cronona
illative               ZGX:ään R66:een Pekkaan Magukseen  Cronoon
</pre>

The \"Form\" here refers to the linguistic name of the conjugation.<br>
The other columns are examples of how different names are conjugated in that form.

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
 1[member1]:ä,2[member2]:ä,3[member3]:ä\" \":ta\"

setup \"Do_LLA\" \"
 cCronolla,mMarlella,lLuccalla,lLucalla,rRoboksella,fFrogilla,
 aAylalla,uMaguksella,uMagusilla,eEpochilla,
 1[member1]:lla,2[member2]:lla,3[member3]:lla,
 1[member1]:llä,2[member2]:llä,3[member3]:llä\" \"kella\"
</pre>

The first parameter of <code>setup</code> is the function name
that is responsible of handling this conjugation.
 <p>
The second parameter is the list of all strings that the
conjugator will <i>assume as a call to this conjugation</i>.
The parameter is commaseparated and each string begins
with a letter: c for Crono, m for Marle, l for Lucca, r for Robo,
f for Frog, a for Ayla, u for Magus, e for Epoch, 1 for
the first member of party, 2 for the second member of party
and 3 for the third member of the party.
 <p>
These names are expected to be <i>found</i> in the script,
and they will be <i>replaced</i> with calls to the conjugator
engine with the respective character name. In other words,
these names listed will not be rendered as-is.
 <p>
The third parameter is a string that is used by the script
wrapping code. It describes the <em>widest</em> possible string
that the conjugation could append to the character name.

", 'code:1. Creating the conjugator code' => "

This is the hardest part for average people.<br
You have to write your own conjugator!<br>
Have a look at <tt>DOCS/ct-conj.code</tt> (the Finnish conjugator
as an example) and try to grasp it.<br>
Sorry, I can't help you more than this. Yes, it requires some programming.
 <p>
Anyway, when you are done with your code file, you just compile
it with utils/compile, combine it with ct-conj.a65 and assemble
it with <a href=\"http://bisqwit.iki.fi/source/snescom.html\">snescom</a>.
Basically, once you have snescom installed, you can just do
<tt>make ct-conj.o65</tt> and that should do it.

", 'bind:1. Binding the code to ROM' => "

Add the following lines to your <tt>ct.cfg</tt>
in the <code>[linker]</code> section:

<pre>
# CONJUGATOR
load_code \"ct-conj.o65\"
add_call_of \"Conjugator\"         \$C258C2 1 false
</pre>

This tells the insertor that it must include code from <tt>ct-conj.o65</tt>
and that the Conjugator routine will be called from the ROM address <code>\$C258C2</code>.
  <p>
I hope I didn't forget anything.
  <p>
Ask me for help if you didn't get it :)

");
