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
<code>ADC</code>, <code>AND</code>, <code>ASL</code>, <code>BCC</code>,
<code>BCS</code>, <code>BEQ</code>, <code>BIT</code>, <code>BMI</code>,
<code>BNE</code>, <code>BPL</code>, <code>BRA</code>, <code>BRK</code>,
<code>BRL</code>, <code>BVC</code>, <code>BVS</code>, <code>CLC</code>,
<code>CLD</code>, <code>CLI</code>, <code>CLV</code>, <code>CMP</code>,
<code>COP</code>, <code>CPX</code>, <code>CPY</code>, <code>DB </code>,
<code>DEC</code>, <code>DEX</code>, <code>DEY</code>, <code>EOR</code>,
<code>INC</code>, <code>INX</code>, <code>INY</code>, <code>JML</code>,
<code>JMP</code>, <code>JSL</code>, <code>JSR</code>, <code>LDA</code>,
<code>LDX</code>, <code>LDY</code>, <code>LSR</code>, <code>MVN</code>,
<code>MVP</code>, <code>NOP</code>, <code>ORA</code>, <code>PEA</code>,
<code>PEI</code>, <code>PER</code>, <code>PHA</code>, <code>PHB</code>,
<code>PHD</code>, <code>PHK</code>, <code>PHP</code>, <code>PHX</code>,
<code>PHY</code>, <code>PLA</code>, <code>PLB</code>, <code>PLD</code>,
<code>PLP</code>, <code>PLX</code>, <code>PLY</code>, <code>REP</code>,
<code>ROL</code>, <code>ROR</code>, <code>RTI</code>, <code>RTL</code>,
<code>RTS</code>, <code>SBC</code>, <code>SEC</code>, <code>SED</code>,
<code>SEI</code>, <code>SEP</code>, <code>STA</code>, <code>STP</code>,
<code>STX</code>, <code>STY</code>, <code>STZ</code>, <code>TAX</code>,
<code>TAY</code>, <code>TCD</code>, <code>TCS</code>, <code>TDC</code>,
<code>TRB</code>, <code>TSB</code>, <code>TSC</code>, <code>TSX</code>,
<code>TXA</code>, <code>TXS</code>, <code>TXY</code>, <code>TYA</code>,
<code>TYX</code>, <code>WAI</code>, <code>XBA</code>, <code>XCE</code>
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

", 'comments:1.1. Comments' => "

Comments begin with a semicolon (;) and end with a newline.<br>
A colon is allowed to appear in comment.

", 'separation:1.1. Command separation' => "

Commands are separated by newlines and colons (:).

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
