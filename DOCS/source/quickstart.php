<?php

/* This file is the source for some html file. Read it instead. */

$title = 'Chronotools - quickstart guide';
$progname = 'chronotools';

$text = Array(
   '1. Introduction' => "
   
Here's a quickstart guide on how to use
<a href=\"http://bisqwit.iki.fi/source/chronotools.html\">Chronotools</a>.

", '1. Quickstart guide' => "

", '1.1. Decompress and compile' => "

Decompress and compile Chronotools.<br>
Compilation is not needed if you are running a Windows
system or your Chronotools package came precompiled.<br>
In the command line examples of this page, \$ represents
the command prompt.
  <p>
<pre
>\$ tar xfj ../archives/chronotools-1.15.3.1.tar.bz2
\$ mv chronotools-1.15.3.1 ct
\$ make -C ct</pre>

You only need to do this once.

", '1.1. Import your ROM' => "

Copy or link the Chrono Trigger ROM to your working directory.<br>
You can use <tt>md5sum</tt> (standard unix utility)
to test whether your ROM is the right one. The output
below is the expected one.

<pre
>\$ ln -s ../FIN/chrono-uncompressed.smc chrono.smc
\$ md5sum chrono.smc
395bf4d0a75717b03c0c2131495faf7a  chrono.smc</pre>

You only need to do this once.

", '1.1. Import the example configuration file' => "

<pre
>\$ cp ct/etc/ct.cfg .</pre>

That is, copy the <tt>ct.cfg</tt> from
Chronotools's <tt>etc</tt> to your working directory.

", '1.1. Dump the ROM contents' => "

<pre
>\$ ct/ctdump chrono.smc
\$ cp ctdump.out ct.txt</pre>

You only need to do this once.

", '1.1. Customize your script and the configuration file' => "

... by editing your <tt>ct.txt</tt> and <tt>ct.cfg</tt>
files. Not necessary if you don't want to change anything.

", '1.1. Reinsert the resources' => "

<pre
>\$ ct/ctinsert</pre>
  <p>
<tt>ctinsert</tt> will say lots of things.
Here's a sample run:
  <p>
<pre
>Reading ct.cfg... done
Chrono Trigger script insertor version 1.15.3.1
Copyright (C) 1992,2005 Bisqwit (http://iki.fi/bisqwit/)
> ct.cfg: Missing \"typeface\" in [font]
Loading 'ct8fn.tga'...
Loading 'ct8fnV.tga'...
> ct8fnV.tga doesn't exist, ignoring
Loading 'ct16fn.tga'...
> ct.cfg: Missing \"setup\" in [conjugator]
> Conjugator has nothing to do. Not using.
Loading dialog... done.                     
Analyzing character usage... Loading 'ct16fn.tga'...
Loading 'ct8fn.tga'...
Loading 'ct8fnV.tga'...
> ct8fnV.tga doesn't exist, ignoring
done.
Calculating script size... done.
Applying dictionary... done.
Calculating script size... done.
> Original script size: 241642 bytes; new script size: 178785 bytes
> Saved: 62857 bytes (26.0% off); dictionary size: 509 bytes
Free space: 02:96/1 06:854/1 0C:7981/11 0E:180/1 18:11459/2 1B:219/1 1E:14784/7
36:9747/2 37:60211/2 38:24084/2 39:18174/1 3C:15536/2 3F:28965/12 - total: 192290 bytes
--
Loading images... done.        
Writing strings... done.               
> ct.cfg: Missing \"load_code\" in [linker]
> ct.cfg: Missing \"add_image\" in [linker]
Writing dictionary... done.
Linking 5838 modules... done.
O65 linker: Warning: Symbol \"RELOCATED_STRING_SIGNATURE\" was defined but never used.
O65 linker: Warning: Symbol \"VWF8_DISPLAY_EQ_COUNT\" was defined but never used.
O65 linker: Warning: Symbol \"DECOMPRESS_FUNC_ADDR\" was defined but never used.
O65 linker: Warning: Symbol \"CHAR_OUTPUT_FUNC\" was defined but never used.
--
Creating a virtual ROM...
Inhabitating the ROM image...
--
Free space: 01:30/1 02:8/1 03:40/1 06:45/1 0C:262/4 0E:1/1 18:50/1 1B:7/1
1E:42/2 36:45/1 37:600/1 38:161/1 39:141/1 3C:18/1 3E:25/1 3F:6183/3 - total: 7658 bytes
--
Unallocating insertor data...
Creating ctpatch-nohdr.ips
Creating ctpatch-hdr.ips</pre>

", '1.1. Fixing the checksum of the patch (OPTIONAL)' => "

ctinsert has been designed to work without requiring the ROM,
and therefore if you to fix the checksum of your patch, you
need to use the <tt>fixchecksum</tt> program.
<p>
<pre
>\$ ct/utils/fixchecksum ctpatch-hdr.ips chrono.smc ctpatch-hdr.ips</pre>

", '1.1. Patching the ROM' => "

You can use your favourite method of patching the ROM
now that the IPS file is ready. You can also use the
<tt>unmakeips</tt> program that comes with Chronotools:
 <p>
<pre
>\$ ct/utils/unmakeips ctpatch-hdr.ips chrono.smc chrono-patched.smc</pre>

After this step, you can test your patch,
and then resume editing and repeat earlier steps.

", '1. Translating the game' => "

Besides editing the script, there are a myriad of tasks to do:

<ul>
 <li>
   Redesign the font using an image manipulation
   program such as <a href=\"http://www.gimp.org\">GIMP</a>,
   editing <tt>ct16fn.tga</tt> and <tt>ct8fn.tga</tt>,
   and update the configuration to match the new character set
   in your font and script files.<br>
   See <a href=\"fonts.html\">the fonts and character sets guide</a>.
  </li>
 <li>
   Redesign the images, such as the elementals.
   See <a href=\"imageformat.html\">image format requirements</a>.
  </li>
 <li>
   <a href=\"conjugation.html\">Conjugation</a>, which is a quite
   complex thing and requires knowledge of your language's grammar.<br>
   It's needed in a few languages, where the names of player characters
   may be written differently depending on grammatical context.
   Because of the level of language experdise required, many
   translators choose not to do conjugation.
  </li>
</ul>

");
