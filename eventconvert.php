<?php

$s = <<<EOF
2C EvTwoBytes            [Unknown2C:%0:%1]
2D EvPosiGoto            [Goto:%0 [UnlessF8 == 00]]
2E EvCommand2E           [PaletteSet:%0:%1:%2:%3]
2F EvTwoBytes            [Unknown2F:%0:%1]
30 EvPosiGoto            [Goto:%0 [UnlessF8 & 02]]
31 EvPosiGoto            [Goto:%0 [UnlessF8 & 80]]
32 EvNoParams            [OrB:54:10]
33 EvOneByte             [PaletteSomething:%0]
34 EvPosiGoto            [Goto:%0 [UnlessButtonPressed:A]]
35 EvPosiGoto            [Goto:%0 [UnlessButtonPressed:B]]
36 EvPosiGoto            [Goto:%0 [UnlessButtonPressed:X]]
37 EvPosiGoto            [Goto:%0 [UnlessButtonPressed:Y]]
38 EvPosiGoto            [Goto:%0 [UnlessButtonPressed:L]]
39 EvPosiGoto            [Goto:%0 [UnlessButtonPressed:R]]
3A EvNoParams          [Crash:3A]
3B EvOneByte             [Unused3B:%0]
3C EvOneByte             [Unused3C:%0]
3D EvNoParams          [Crash:3D]
3E EvNoParams          [Crash:3E]
3F EvPosiGoto            [Goto:%0 [UnlessButtonStatus:A]]
40 EvPosiGoto            [Goto:%0 [UnlessButtonStatus:B]]
41 EvPosiGoto            [Goto:%0 [UnlessButtonStatus:X]]
42 EvPosiGoto            [Goto:%0 [UnlessButtonStatus:Y]]
43 EvPosiGoto            [Goto:%0 [UnlessButtonStatus:L]]
44 EvPosiGoto            [Goto:%0 [UnlessButtonStatus:R]]
45 EvNoParams          [Crash:45]
46 EvNoParams          [Crash:46]
47 EvOneByte             [PokeTo6B:%0]
48 EvLongWith7F0200      [LetB:%addr:%long]
49 EvLongWith7F0200      [LetW:%addr:%long]
4A EvLongWithByte        [LetB:%long:%byte]
4B EvLongWithWord        [LetW:%long:%word]
4C EvLongWith7F0200      [LetB:%long:%addr]
4D EvLongWith7F0200      [LetW:%long:%addr]
4E EvCommand4E           [StringStore:%long:%data]
4F EvByteWith7F0200      [LetB:%addr:%byte]
50 EvWordWith7F0200      [LetW:%addr:%word]
51 EvTwo7F0200           [LetB:%1:%0]
52 EvTwo7F0200           [LetW:%1:%0]
53 Ev7F0000And7F0200     [LetB:%1:%0]
54 Ev7F0000And7F0200     [LetW:%1:%0]
55 Ev7F0200              [GetStorylineCounter:%0]
56 EvByteWith7F0000      [LetB:%addr:%byte]
57 EvNoParams            [PCLoad:0]
58 Ev7F0200And7F0000     [LetB:%1:%0]
59 Ev7F0200And7F0000     [LetW:%1:%0]
5A EvOneByte             [SetStorylineCounter:%0]
5B EvByteWith7F0200      [AddB:%addr:%byte]
5C EvNoParams            [PCLoad:1]
5D EvTwoBytes            [Unused5D:%0:%1]
5E EvTwo7F0200           [AddW:%1:%0]
5F EvByteWith7F0200      [SubB:%addr:%byte]
60 EvWordWith7F0200      [SubW:%addr:%word]
61 EvTwo7F0200           [SubB:%1:%0]
62 EvNoParams            [PCLoad:2]
63 EvByteWith7F0200      [SetBit:%addr:%byte]
64 EvByteWith7F0200      [ClearBit:%addr:%byte]
65 EvCommands65And66     [SetBit:%addr:%byte]
66 EvCommands65And66     [ClearBit:%addr:%byte]
67 EvByteWith7F0200      [AndB:%addr:%byte]
68 EvNoParams            [PCLoad:3]
69 EvByteWith7F0200      [OrB:%addr:%byte]
6A EvNoParams            [PCLoad:4]
6B EvByteWith7F0200      [XorB:%addr:%byte]
6C EvNoParams            [PCLoad:5]
6D EvNoParams            [PCLoad:6]
6E EvNoParams          [Crash:6E]
6F EvByteWith7F0200      [ShrB:%addr:%byte]
70 EvNoParams          [Crash:70]
71 Ev7F0200              [IncB:%0]
72 Ev7F0200              [IncW:%0]
73 Ev7F0200              [DecB:%0]
74 EvNoParams          [Crash:74]
75 Ev7F0200              [LetB:%0:1]
76 Ev7F0200              [LetW:%0:1]
77 Ev7F0200              [LetB:%0:0]
78 EvNoParams          [Crash:78]
79 EvNoParams          [Crash:79]
7A EvThreeBytes          [NPCJump:%0:%1:%2]
7B EvTwoWords            [Unused7B:%0:%1]
7C EvOneByte             [SetObjectDrawingFor:%0:on]  // Sets 1A81 for given obj to 1
7D EvOneByte             [SetObjectDrawingFor:%0:off] // Sets 1A81 for given obj to 0
7E EvNoParams            [SetObjectDrawing:hide]
7F Ev7F0200              [GetRandom:%0]
80 EvOneByte             [PCLoad_IfInParty:%0]
81 EvOneByte             [PCLoad:%0]
82 EvOneByte             [NPCLoad:%0]
83 EvTwoBytes            [LoadEnemy:%0:%1]
84 EvOneByte             [NPCSolidProps:%0]
85 EvNoParams          [Crash:85]
86 EvNoParams          [Crash:86]
87 EvOneByte             [Unknown87:%0]
88 EvCommand88           [GFXCommand88:%0:%1:%2:%3]
89 EvOneByte             [NPCSetSpeed:%0] // Sets 1A00 from const
8A Ev7F0200              [NPCSetSpeed:%0] // Sets 1A00 from var
8B EvTwoBytes            [SetObjectCoord:%0:%1]
8C EvTwoBytes            [Unknown8C:%0:%1]
8D EvTwoWords            [SetObjectCoord2:%0:%1]
8E EvOneByte             [NPCHide:%0]
8F EvOneByte             [Unknown8F:%0]
90 EvNoParams            [SetObjectDrawing:on]
91 EvNoParams            [SetObjectDrawing:off]
92 EvTwoBytes            [MoveObject:%0:%1]
93 EvNoParams          [Crash:93]
94 EvOneByte             [Unknown94:%0]
95 EvOneByte             [NPCSetInviteFlag:%0]
96 EvTwoBytes            [NPCMove:%0:%1]
97 EvTwoBytes            [Unknown97:%0:%1]
98 EvTwoBytes            [Unknown98:%0:%1]
99 EvTwoBytes            [Unknown99:%0:%1]
9A EvThreeBytes          [Unknown9A:%0:%1:%2]
9B EvNoParams          [Crash:9B]
9C EvTwoBytes            [Unknown9C:%0:%1]
9D EvTwo7F0200           [Unknown9D:%0:%1]
9E EvOneByte             [Unknown9E:%0]
9F EvOneByte             [Unknown9F:%0]
A0 EvTwoBytes            [NPCMoveAnimated:%0:%1]
A1 EvTwo7F0200           [UnknownA1:%0:%1]
A2 EvNoParams          [Crash:A2]
A3 EvNoParams          [Crash:A3]
A4 EvNoParams          [Crash:A4]
A5 EvNoParams          [Crash:A5]
A6 EvOneByte             [NPCSetFacing:%0]
A7 Ev7F0200              [NPCSetFacing:%0]
A8 EvOneByte             [NPCSetRelativeFacing:%0]
A9 Ev7F0200              [NPCSetRelativeFacing:%0]
AA EvOneByte             [Animation:%0:1]
AB EvOneByte             [AnimationWait:%0]
AC EvOneByte             [PlayStaticAnimation:%0]
AD EvOneByte             [Pause:%0]
AE EvNoParams            [Animation:0:0]
AF EvNoParams            [UnknownAF]
B0 EvNoParams            [ForeverUnknownAF]
B1 EvNoParams            [LoopBreak]
B2 EvNoParams            [LoopEnd]
B3 EvNoParams            [Animation:0:1]
B4 EvNoParams            [Animation:1:1]
B5 EvOneByte             [ForeverUnknown94:%0]
B6 EvOneByte             [ForeverNPCSetInviteFlag:%0]
B7 EvLoopAnimation       [LoopAnimation:%0:%1]
B8 EvDialogBegin         [DialogSetTable:%0]
B9 EvNoParams            [Pause:250ms]
BA EvNoParams            [Pause:500ms]
BB EvDialogAddr          [DialogDisplay:%0]
BC EvNoParams            [Pause:1000ms]
BD EvNoParams            [Pause:2000ms]
BE EvNoParams          [Crash:BE]
BF EvNoParams          [Crash:BF]
C0 EvDialogAddrWithByte  [DialogAsk:%0:%1]       //stores to result (7F0A80+X)
C1 EvDialogAddr          [DialogDisplayTop:%0]
C2 EvDialogAddr          [DialogDisplayBottom:%0]
C3 EvDialogAddrWithByte  [DialogAskTop:%0:%1]    //stores to result (7F0A80+X)
C4 EvDialogAddrWithByte  [DialogAskBottom:%0:%1] //stores to result (7F0A80+X)
C5 EvNoParams          [Crash:C5]
C6 EvNoParams          [Crash:C6]
C7 Ev7F0200              [AddItem:%0]
C8 EvOneByte             [SpecialDialog:%0]
C9 EvIfOneByte           [Goto:%label [UnlessHasItem:%0]]
CA EvOneByte             [GiveItem:%0]
CB EvOneByte             [StealItem:%0]
CC EvIfOneWord           [Goto:%label [UnlessHasGold:%0]]
CD EvOneWord             [GiveGold:%0]
CE EvOneWord             [StealGold:%0]
CF EvIfOneByte           [Goto:%label [UnlessHasMember:%0]]
D0 EvOneByte             [GiveMember:%0]
D1 EvOneByte             [StealMember:%0]
D2 EvIfOneByte           [Goto:%label [UnlessHasActiveMember:%0]
D3 EvOneByte             [GiveActiveMember:%0]
D4 EvOneByte             [UnactivateMember:%0]
D5 EvTwoBytes            [EquipMember:%0:%1]
D6 EvOneByte             [StealActiveMember:%0]
D7 EvByteWith7F0200      [GetItemAmount:%byte:%addr]
D8 EvOneWord             [StartBattle:%0]
D9 EvSixBytes            [PartyMove:%0:%1:%2:%3:%4:%5]
DA EvNoParams            [PartySetFollow]
DB EvNoParams          [Crash:DB]
DC EvWordAndTwoBytes     [PartyTeleportDC:%0:%1:%2]
DD EvWordAndTwoBytes     [PartyTeleportDD:%0:%1:%2]
DE EvWordAndTwoBytes     [PartyTeleportDE:%0:%1:%2]
DF EvWordAndTwoBytes     [PartyTeleportDF:%0:%1:%2]
E0 EvWordAndTwoBytes     [PartyTeleportE0:%0:%1:%2]
E1 EvWordAndTwoBytes     [PartyTeleportE1:%0:%1:%2]
E2 EvFour7F0200          [UnknownE2:%0:%1:%2:%3]
E3 EvOneByte             [SetExploreMode:%0]
E4 EvCommandsE4andE5     [UnknownE4:%l:%t:%r:%b:%x:%y:%f]
E5 EvCommandsE4andE5     [UnknownE5:%l:%t:%r:%b:%x:%y:%f]
E6 EvWordAndTwoBytes     [ScrollLayers:%0:%1:%2]
E7 EvTwoBytes            [ScrollScreen:%0:%1]
E8 EvOneByte             [PlaySound:%0]
E9 EvNoParams          [Crash:E9]
EA EvOneByte             [PlaySong:%0]
EB EvTwoBytes            [SetVolume:%0:%1]
EC EvThreeBytes          [SoundCommand:%0:%1:%2]
ED EvNoParams            [WaitSilence]
EE EvNoParams            [WaitSongEnd]
EF EvNoParams          [Crash:EF]
F0 EvOneByte             [FadeOutScreen:%0]
F1 EvCommandF1           [BrightenScreen:%index:%duration]
F2 EvNoParams            [FadeOutScreen]
F3 EvNoParams            [UnknownF3]
F4 EvOneByte             [ShakeScreen:%0]
F5 EvNoParams          [Crash:F5]
F6 EvNoParams          [Crash:F6]
F7 EvNoParams          [Crash:F7]
F8 EvNoParams            [HealHPandMP]   // does opF9 and opFA
F9 EvNoParams            [HealHP]        // calls C28004,A=6
FA EvNoParams            [HealMP]        // calls C28004,A=7
FB EvNoParams          [Crash:FB]
FC EvNoParams          [Crash:FC]
FD EvNoParams          [Crash:FD]
FE EvCommandFE           [SetGeometry:%0:%1:%2:%3:%4:%5:%6:%7:%8:%9:%10:%11:%12:%13:%14:%15:%16]
FF EvMode7Scene          [Mode7Scene:%0:%1:%2]
EOF;

foreach(explode("\n", $s) as $s)
{
  $hex = substr($s, 0, 2);
  $s = substr($s, 3);
  $type = ereg_replace(' .*', '', $s);
  $text = ereg_replace('^[^[]* * \[', '[', $s);
  
  $comment = ereg_replace('.*// *', '', $text);
  if($comment != $text)
  {
    $text = ereg_replace(' *//.*', '', $text);
  }
  else
    $comment = '';
  
  print "<< \"$text\"\n";
  print "    << At[0] - 0x$hex\n";
  
  switch(strtolower($type))
  {
    case 'evnoparams':
      break;
    case 'evtwobytes':
      print "    << At[1] - DeclareByte(\"0\")\n";
      print "    << At[2] - DeclareByte(\"1\")\n";
      break;
    case 'evifonebyte':
      print "    << At[1] - DeclareByte(\"0\")\n";
      print "    << At[2] - DeclareGoto(\"label\", +1, 1)\n";
      break;
    case 'evifoneword':
      print "    << At[1] - DeclareWord(\"0\")\n";
      print "    << At[3] - DeclareGoto(\"label\", +1, 1)\n";
      break;
    case 'evwordandtwobytes':
      print "    << At[1] - DeclareWord(\"0\")\n";
      print "    << At[3] - DeclareWord(\"1\")\n";
      print "    << At[4] - DeclareByte(\"2\")\n";
      break;
    case 'evtwowords':
      print "    << At[1] - DeclareWord(\"0\")\n";
      print "    << At[3] - DeclareWord(\"1\")\n";
      break;
    case 'evoneword':
      print "    << At[1] - DeclareWord(\"0\")\n";
      break;
    case 'evthreebytes':
      print "    << At[1] - DeclareByte(\"0\")\n";
      print "    << At[2] - DeclareByte(\"1\")\n";
      print "    << At[3] - DeclareByte(\"2\")\n";
      break;
    case 'evsixbytes':
      print "    << At[1] - DeclareByte(\"0\")\n";
      print "    << At[2] - DeclareByte(\"1\")\n";
      print "    << At[3] - DeclareByte(\"2\")\n";
      print "    << At[4] - DeclareByte(\"3\")\n";
      print "    << At[5] - DeclareByte(\"4\")\n";
      print "    << At[6] - DeclareByte(\"5\")\n";
      break;
    case 'evonebyte':
      print "    << At[1] - DeclareByte(\"0\")\n";
      break;
    case 'evposigoto':
      print "    << At[1] - DeclareGoto(\"0\", +1, 0)\n";
      break;
    case 'ev7f0200':
      print "    << At[1] - Declare7F0200_2(\"0\")\n";
      break;
    case 'evbytewith7f0200':
      print "    << At[1] - DeclareByte(\"byte\")\n";
      print "    << At[2] - Declare7F0200_2(\"addr\")\n";
      break;
    case 'evwordwith7f0200':
      print "    << At[1] - DeclareWord(\"word\")\n";
      print "    << At[3] - Declare7F0200_2(\"addr\")\n";
      break;
    case 'evlongwith7f0200':
      print "    << At[1] - DeclareLong(\"long\")\n";
      print "    << At[4] - Declare7F0200_2(\"addr\")\n";
      break;
    case 'evbytewith7f0000':
      print "    << At[1] - DeclareByte(\"byte\")\n";
      print "    << At[2] - Declare7F0000_Wa(\"addr\")\n";
      break;
    case 'evlongwithbyte':
      print "    << At[1] - DeclareLong(\"long\")\n";
      print "    << At[4] - DeclareByte(\"byte\")\n";
      break;
    case 'evlongwithword':
      print "    << At[1] - DeclareLong(\"long\")\n";
      print "    << At[4] - DeclareWord(\"word\")\n";
      break;
    case 'evtwo7f0200':
      print "    << At[1] - Declare7F0200_2(\"0\")\n";
      print "    << At[2] - Declare7F0200_2(\"1\")\n";
      break;
    case 'evfour7f0200':
      print "    << At[1] - Declare7F0200_2(\"0\")\n";
      print "    << At[2] - Declare7F0200_2(\"1\")\n";
      print "    << At[3] - Declare7F0200_2(\"2\")\n";
      print "    << At[4] - Declare7F0200_2(\"3\")\n";
      break;
    case 'ev7f0200and7f0000':
      print "    << At[1] - Declare7F0200_2(\"0\")\n";
      print "    << At[2] - Declare7F0000_W(\"1\")\n";
      break;
    case 'ev7f0000and7f0200':
      print "    << At[1] - Declare7F0000_W(\"0\")\n";
      print "    << At[3] - Declare7F0200_2(\"1\")\n";
      break;
    case 'evdialogbegin':
      print "    << At[1] - DeclareDialogBegin(\"0\")\n";
      break;
    case 'evdialogaddr':
      print "    << At[1] - DeclareDialogAddr(\"0\")\n";
      break;
    case 'evdialogaddrwithbyte':
      print "    << At[1] - DeclareDialogAddr(\"0\")\n";
      print "    << At[2] - DeclareByte(\"1\")\n";
      break;
    default:
      print "    << ??? $type\n";
  }
  #print "hex($hex)type($type)text($text)comment($comment)\n";
  
  if($comment)
      print "    /* $comment */\n";
  
  print "\n";
}
