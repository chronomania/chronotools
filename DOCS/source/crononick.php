<?php

/* This file is the source for some html file. Read it instead. */

$title = 'Chronotools - crononick';
$progname = 'chronotools';

$text = Array(
   '1. Introduction' => "
   
<b>Q:</b>
What exactly is <code>[crononick]</code>? Can I just replace it
with Crono? From what I saw only Ayla uses it in her dialogue.
<p>
<b>A:</b>
Ayla talks simply language, and from the very first moment she
sees Crono, she likes him a lot and gives him a new name, a
simpler name.<br>
From that on, she calls Crono always by that nickname.
<p>
In the Japanese version, Crono's default name is Kurono, and
Ayla's version of that name is Kuro.<br>
If the user gives Crono a new name and the name is longer
than two syllables, the <code>[crononick]</code> handler
only displays the first two syllables of it.
<p>
In the English version you don't see this happening because the
localizers weren't able to invent an algorithm to simplify the name.<br>
However, the skeleton for that is still present in CT ROM,
and for this reason <a href=\"http://bisqwit.iki.fi/source/chronotools.html\">Chronotools</a> supports it.
<p>
If you happen to invent a way to convert any 1-5 letter name
into a nickname using the rules of your native language, you
can write (or have it written) the rules and routines to
handle that and Chronotools inserts that and uses it to
display <code>[crononick]</code>.
<p>
Even if you don't intend to do that, it's good to keep all
<code>[crononick]</code> there and not change them to \"Crono\",
because it allows
you to quickly find all dialog where Ayla talks about Crono :)

", '1. How to' => "

The instructions here are similar to the
<a href=\"conjugation.html\">conjugation instructions</a>, but
the only configuration file section associated with this is
<code>[linker]</code>.<br>
Here's what I have there:
<pre>
# CRONONICK
load_code \"ct-crononick.o65\"
add_call_of \"CrononickHandler\"   \$C25B3B 13 false
</pre>
You can find my code file in the <code>DOCS/ct-crononick.code</code> file.

");
