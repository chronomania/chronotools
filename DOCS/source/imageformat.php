<?php
/* This file is the source for some html file. Read it instead. */

$title = 'Chronotools - image format';
$progname = 'chronotools';

$text = Array(
   '1. Introduction' => "
   
All files handled by Chronotools are TARGA (.tga) files.<br>
There are the following requirements:
<ul>
 <li>The files must be saved NON-COMPRESSED (not RLE encoded)</li>
 <li>They must be oriented as \"origin at bottom left\".</li>
 <li>They have to be paletted (less than 256 colours)</li>
 <li>The order of colours in the palette must not be changed</li>
</ul>
If you don't have an image manipulation program that lets you
specify these, use <a href=\"http://www.gimp.org\">GIMP</a>.<br>
It's free and available for many systems, including Windows systems.

");
