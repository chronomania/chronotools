<?php

/* This file is the source for some html file. Read it instead. */

$title = 'Chronotools - what to make of "Froggy"';
$progname = 'chronotools';

$text = Array(
   '1. Froggy' => "
   
<pre>On Sat, 5 Mar 2005, Dmitri Nikulin wrote:
> Oh, one more misfeature I forgot to report... while you were careful
> to protect 'Robo' from being misused in other words, 'Frog' wasn't so
> lucky, so 'Froggy' can become (say) 'Fredgy'. And this happens a lot.
> It happens for 'Tata and the Frog' too.

This is something I don't know how to solve.
Using a [name] is something I wouldn't like to do, because
1. it makes conjugation clumsy
2. it makes script writing clumsy
I know the 2., because I had to use it in ToP.
The point is, I don't know what to make of \"Froggy\" and such
(how to translate it).

But you are right, Frog should be changed to something safer
for the same reason as Robo was.

It happens in four lines:
7140:OZZIE: Welcome, Glenn! Or should I say, Sir Froggy! Mwa, ha![nl]
7235:FLEA: Poor little Froggie! You must be lonely now that Cyrus is gone.
8368:   Frogs haven't belly buttons!
9018:   Froggy, you weren't such a bad guy either.

There's one solution:

Write \"Fr[10]oggy\" when you want to say \"Froggy\" without
substituting \"Frog\" with Frog's name.

[10] is a zero-width opcode which I don't exactly know what it does.
It seems to work, at least in the only scene where I tested it.

I didn't think of this feature when I wrote the Robos trick
(to protect \"Robo\" in \"Robots\" be substituted).

It's not much dirtier than surrounding each substituted & conjugated
name by [].</pre>

");
