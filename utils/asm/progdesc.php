<?php
//TITLE=Free SNES assembler

$title = 'Free SNES assembler';
$progname = 'snescom';

$text = array(
   'purpose:1. Purpose' => "

This program reads symbolic 65816 or 65c816 machine code
and compiles (assembles) it into a relocatable object file.
<p>
The produced object file is binary-compatible with those
made with <a href=\"http://www.google.fi/search?q=xa65\">XA65</a>.

", 'syntax:1. Supported syntax' => "

", 'ops:1.1. Mnemonics' => "

The following mnemonics are supported:
<p>
<code>adc</code>, <code>and</code>, <code>asl</code>, <code>bcc</code>,
<code>bcs</code>, <code>beq</code>, <code>bit</code>, <code>bmi</code>,
<code>bne</code>, <code>bpl</code>, <code>bra</code>, <code>brk</code>,
<code>brl</code>, <code>bvc</code>, <code>bvs</code>, <code>clc</code>,
<code>cld</code>, <code>cli</code>, <code>clv</code>, <code>cmp</code>,
<code>cop</code>, <code>cpx</code>, <code>cpy</code>, <code>db </code>,
<code>dec</code>, <code>dex</code>, <code>dey</code>, <code>eor</code>,
<code>inc</code>, <code>inx</code>, <code>iny</code>, <code>jml</code>,
<code>jmp</code>, <code>jsl</code>, <code>jsr</code>, <code>lda</code>,
<code>ldx</code>, <code>ldy</code>, <code>lsr</code>, <code>mvn</code>,
<code>mvp</code>, <code>nop</code>, <code>ora</code>, <code>pea</code>,
<code>pei</code>, <code>per</code>, <code>pha</code>, <code>phb</code>,
<code>phd</code>, <code>phk</code>, <code>php</code>, <code>phx</code>,
<code>phy</code>, <code>pla</code>, <code>plb</code>, <code>pld</code>,
<code>plp</code>, <code>plx</code>, <code>ply</code>, <code>rep</code>,
<code>rol</code>, <code>ror</code>, <code>rti</code>, <code>rtl</code>,
<code>rts</code>, <code>sbc</code>, <code>sec</code>, <code>sed</code>,
<code>sei</code>, <code>sep</code>, <code>sta</code>, <code>stp</code>,
<code>stx</code>, <code>sty</code>, <code>stz</code>, <code>tax</code>,
<code>tay</code>, <code>tcd</code>, <code>tcs</code>, <code>tdc</code>,
<code>trb</code>, <code>tsb</code>, <code>tsc</code>, <code>tsx</code>,
<code>txa</code>, <code>txs</code>, <code>txy</code>, <code>tya</code>,
<code>tyx</code>, <code>wai</code>, <code>xba</code>, <code>xce</code>
</p>

", 'addrmodes:1.1. Addressing modes' => "

All the standard addressing modes of the 65816 cpu are supported.
<ul>
 <li>Implied: <code>nop</code>; <code>clc</code></li>
 <li>Immediate: <code>lda #value</code>; <code>rep #value</code> etc (size may depend on an <a href=\"#opsize\">operand size setting</a>)</li>
 <li>Short relative: <code>bra end</code></li>
 <li>Long relative: <code>brl end</code>; <code>per end+2</code></li>
 <li>Direct: <code>lda \$12</code></li>
 <li>Direct indexed: <code>lda \$12,x</code>; <code>lda \$12,y</code></li>
 <li>Direct indirect: <code>lda (\$12)</code>; <code>pei (\$12)</code></li>
 <li>Direct indexed indirect: <code>lda (\$12,x)</code></li>
 <li>Direct indirect indexed: <code>lda (\$12),y</code></li>
 <li>Direct indirect long: <code>lda [\$12]</code></li>
 <li>Direct indirect indexed long: <code>lda [\$12],y</code></li>
 <li>Absolute: <code>lda \$1234</code></li>
 <li>Absolute indexed: <code>lda \$1234,x</code>; <code>lda \$1234,y</code></li>
 <li>Absolute long: <code>lda \$123456</code></li>
 <li>Absolute indexed long: <code>lda \$123456,x</code></li>
 <li>Stack-relative: <code>lda \$12,s</code></li>
 <li>Stack-relative indirect indexed: <code>lda (\$12,s),y</code></li>
 <li>Absolute indirect: <code>lda (\$1234)</code></li>
 <li>Absolute indirect long: <code>lda [\$1234]</code></li>
 <li>Absolute indexed indirect: <code>lda (\$1234,x)</code></li>
 <li>MVN/MVP: <code>mvn \$7E,\$7F</code></li>
</ul>

", 'opsize:1.1. Operand size control' => "

The pseudo ops <code>.as</code>, <code>.al</code>, <code>.xs</code> &amp;
<code>.xl</code> are used to decide what size accumulator and index mode
for the assembler to use. <code>.as</code> and <code>.xs</code> are for
8bit operands, and <code>.al</code> and <code>.xl</code> are for 16 bit operands.
<p>
I've found it handy to define these macroes:
<pre class=smallerpre>#define SET_8_BIT_A()   sep #\$20 : .as
#define SET_16_BIT_A()  rep #\$20 : .al
#define SET_8_BIT_X()   sep #\$10 : .xs
#define SET_16_BIT_X()  rep #\$10 : .xl

#define SET_8_BIT_AX()  sep #\$30 : .xs : .as
#define SET_16_BIT_AX() rep #\$30 : .xl : .al
</pre>
In addition to these modes, there are several operand prefixes
that can be used to force a certain operand size/type.

<ul>
 <li><code>lda !\$f0</code> would use 16-bit address instead of direct page.</li>
 <li><code>lda @\$1234</code> would use a long (24-bit) address instead of absolute 16-bit address.</li>
 <li><code>lda #&lt;var</code> can be used to load the lower 8-bits of an external variable.</li>
 <li><code>lda #&gt;var</code> can be used to load the upper 8-bits of an external variable.</li>
 <li><code>lda #^var</code> can be used to load the segment part (8-bits) of an external variable.</li>
 <li><code>lda @var,x</code> can be used to use absolute-indexed-long mode instead of the default, absolute.</li>
</ul>

", 'eval:1.1. Expression evaluation' => "

Expressions are supported. These are valid code:
<ul>
 <li><code>bra somewhere+1</code></li>
 <li><code>lda #!address + \$100</code></li>
 <li><code>ldy #\$1234 + (\$6C * 3)</li>
</ul>

", 'segs:1.1. Segments' => "

Code, labels and data can be generated to four segments:
<code>text</code>, <code>data</code>, <code>zero</code> and <code>bss</code>.<br>
Use <code>.text</code>, <code>.data</code>, <code>.zero</code> and <code>.bss</code>
respectively to select the segment.

", 'comments:1.1. Comments' => "

Comments begin with a semicolon (;) and end with a newline.<br>
A colon is allowed to appear in comment.

", 'separation:1.1. Command separation' => "

Commands are separated by newlines and colons (:).

", 'branchlabels:1.1. Branch labels' => "

The label <code>-</code> can be defined for branches backward
and <code>+</code> for branches forward.<br>
Example:<br>
<pre class=smallerpre
>        ; Space-fill the buffer to end
        phx
         cpx #$0010
         bcs +     ;jumps to the next \"+\"
         SET_8_BIT_A()
         lda #$FF
-        sta $94A0,x
         sta $94B0,x
         inx
         cpx #$0010
         bcc -     ;jumps to the previous \"-\"
         SET_16_BIT_A()
+        lda W_VRAMADDR
         sta @$002116
        pla</pre>


", 'cpp:1.1. Preprocessor' => "

snescom uses <a href=\"http://gcc.gnu.org/\">GCC</a> as a preprocessor.<br>
You can use <code>#ifdef</code>, <code>#ifndef</code>, <code>#define</code>,
<code>#if</code>, <code>#endif</code> and <code>#include</code> like
in any C program.

", 'objfile:1.1. Object file format' => "

Currently snescom only produces relocatable object files.<br>
The file format has been
<a href=\"http://www.google.com/search?q=6502+binary+relocation+format\">documented</a>
by André Fachat
for the <a href=\"http://www.google.fi/search?q=xa65\">XA65</a> project.

", 'examples:1. Examples and more documentation' => "

In the source distribution there are some C++ modules
that can be used to handle the o65 files.<br>
There are also some example assembler files
(copied from <a href=\"http://bisqwit.iki.fi/source/chronotools.html\">Chronotools</a>).

", 'changelog:1. Changelog' => "

Sep 23 2003; 0.0.0 started working with the project.<br>
Sep 28 2003; 1.0.0 initial release. All features working.<br>
Sep 29 2003; 1.1.0 added feature: branch labels.<br>
Sep 29 2003; 1.2.0 bugfix: didn't resolve correctly when different scopes had a label with the same name; added warning options<br>

", 'copying:1. Copying' => "

snescom has been written by Joel Yliluoma, a.k.a.
<a href=\"http://iki.fi/bisqwit/\">Bisqwit</a>,<br>
and is distributed under the terms of the
<a href=\"http://www.gnu.org/licenses/licenses.html#GPL\">General Public License</a> (GPL).
<p>
If you happen to see this program useful for you, I'd
appreciate if you tell me :) Perhaps it would motivate
me to enhance the program.

");
include '/WWW/progdesc.php';
