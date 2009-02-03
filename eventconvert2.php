<?php

#function xmlfix($s)
#{
#  return str_replace(Array('&','"'),Array('&amp;','&quot;'),$s);
#}

function echo_xml_tag($tag, $params, $end=false)
{
  echo "<$tag";
  if($params)
    foreach($params as $f=>$v)
    {
      echo " $f=\"", htmlspecialchars($v), '"';
    }
  if($end) echo ' /';
  echo '>';
}


$cmd = Array();
$cmds = Array();
foreach(explode("\n",join('',file('eventdata.inc'))) as $line)
{
  if($line == '') continue;
  ParseLine($line);
}

/*$ops=array();
foreach($cmds as $cmd)foreach($cmd['params'] as $param)
  $ops[$param['type']] = $param['type'];
print_r($ops);*/

print "<?xml version=\"1.0\"?>\n";
print "<?xml-stylesheet type='text/xsl' href='eventdata.xsl'?>\n";
print "<location_event_commands>\n";

foreach($cmds as $sequence)
{
  DumpSequence($sequence);
}

print " <positions>\n";
for($c=0;$c<19;++$c)print "  <value>$c</value>\n";
print " </positions>\n";

print "</location_event_commands>\n";

function InitCmd()
{
  global $cmd;
  $cmd = Array
  (
    'cmd'      => '',
    'comments' => '',
    'size'     => 0,
  );
}
function EndCmd()
{
  global $cmd, $cmds;
  if($cmd)
  {
    $cmds[] = $cmd;
    $cmd = Array();
  }
}

function ParseLine($line)
{
  global $cmd;
  
  if(ereg('^DECLARE "', $line))
  {
    // BEGIN NEW
    EndCmd();
    InitCmd();
    
    preg_match('@"(.*)"@', $line, $tmp);
    $cmd['cmd'] = $tmp[1];
  }
  elseif(ereg('^ *AT ', $line))
  {
    preg_match('@^ *AT *([0-9]+) *IS *(.*)@', $line, $tmp);
    $position = $tmp[1];
    $content  = $tmp[2];
    
    ParseContent($position, $content);
  }
  elseif(ereg('^ *AND ', $line))
  {
    preg_match('@^ *AND *(.*)@', $line, $tmp);
    $def = $tmp[1];
    ParseDef($def);
    return;
  }
  else
    $cmd['comments'] .= "$line\n";
  return;
}

function ParseContent($position, $content)
{
  global $cmd;
  
  $comment = '';
  if(preg_match('@// *(.*)@', $content, $tmp))
  {
    $comment = $tmp[1];
    $content = ereg_replace(' *//.*', '', $content);
  }
  
  $content = trim(eregi_replace('\.Annotate[a-z]*\(\)', '', $content));
  
  $op = Array
  (
    'pos'      => $position
  );
  
  if(preg_match('@^0x(..)$@', $content, $tmp))
  {
    $op['type']     = 'Immed';
    $op['size']     = 1;
    $op['min']      = $tmp[1];
    $op['max']      = $tmp[1];
  }
  elseif(preg_match('@^DeclareByte\("(.*)"\)$@', $content, $tmp))
  {
    $op['type']    = 'Immed';
    $op['param']   = $tmp[1];
    $op['size']    = 1;
  }
  elseif(preg_match('@^DeclareByte\("(.*)", *0x(..), *0x(..)\)$@', $content, $tmp))
  {
    $op['type']    = 'Immed';
    $op['param']   = $tmp[1];
    $op['size']    = 1;
    $op['min']     = $tmp[2];
    $op['max']     = $tmp[3];
  }
  elseif(preg_match('@^DeclareLong\("(.*)"\)$@', $content, $tmp))
  {
    $op['type']    = 'Immed';
    $op['param']   = $tmp[1];
    $op['size']    = 3;
  }
  elseif(preg_match('@^DeclareLong\("(.*)", *0x(......), *0x(......)\)$@', $content, $tmp))
  {
    $op['type']    = 'Immed';
    $op['param']   = $tmp[1];
    $op['size']    = 3;
    $op['min']     = $tmp[2];
    $op['max']     = $tmp[3];
  }
  elseif(preg_match('@^DeclareWord\("(.*)"\)$@', $content, $tmp))
  {
    $op['type']    = 'Immed';
    $op['param']   = $tmp[1];
    $op['size']    = 2;
  }
  elseif(preg_match('@^DeclareNibbleHi\("(.*)"\)$@', $content, $tmp))
  {
    $op['type']    = 'NibbleHi';
    $op['param']   = $tmp[1];
    $op['size']    = 1;
  }
  elseif(preg_match('@^DeclareNibbleLo\("(.*)"\)$@', $content, $tmp))
  {
    $op['type']    = 'NibbleLo';
    $op['param']   = $tmp[1];
    $op['size']    = 1;
  }
  elseif(preg_match('@^DeclareObjectNo\("(.*)"\)$@', $content, $tmp))
  {
    $op['type']    = 'ObjectNumber';
    $op['param']   = $tmp[1];
    $op['size']    = 1;
  }
  elseif(preg_match('@^Declare(7[EF]....)_B\("(.*)"\)$@', $content, $tmp))
  {
    $op['type']    = 'TableReference';
    $op['param']   = $tmp[2];
    $op['size']    = 1;
    $op['address'] = $tmp[1];
  }
  elseif(preg_match('@^Declare(7[EF]....)_W\("(.*)"\)$@', $content, $tmp))
  {
    $op['type']    = 'TableReference';
    $op['param']   = $tmp[2];
    $op['size']    = 2;
    $op['address'] = $tmp[1];
  }
  elseif(preg_match('@^Declare(7[EF]....)_2\("(.*)"\)$@', $content, $tmp))
  {
    $op['type']    = 'Table2Reference';
    $op['param']   = $tmp[2];
    $op['size']    = 1;
    $op['address'] = $tmp[1];
  }
  elseif(preg_match('@^DeclareIf\(\)$@', $content, $tmp))
  {
    $op['type']    = 'ConditionalGoto';
    $op['size']    = 1;
  }
  elseif(preg_match('@^DeclareElse\(\)$@', $content, $tmp))
  {
    $op['type']    = 'GotoForward';
    $op['size']    = 1;
  }
  elseif(preg_match('@^DeclareLoop\(\)$@', $content, $tmp))
  {
    $op['type']    = 'GotoBackward';
    $op['size']    = 1;
  }
  elseif(preg_match('@^Declare(?:Text)?Blob\("(.*)"\)$@', $content, $tmp))
  {
    $op['type']    = 'Blob';
    $op['param']   = $tmp[1];
    $op['size']    = 2;
    $cmd['has_blob'] = true;
  }
  elseif(preg_match('@^DeclareDialogBegin\("(.*)"\)$@', $content, $tmp))
  {
    $op['type']    = 'DialogBegin';
    $op['param']   = $tmp[1];
    $op['size']    = 3;
  }
  elseif(preg_match('@^DeclareDialogAddr\("(.*)"\)$@', $content, $tmp))
  {
    $op['type']    = 'DialogIndex';
    $op['param']   = $tmp[1];
    $op['size']    = 1;
  }
  elseif(preg_match('@^DeclareOperator\("(.*)"\)(\.SetHighbit\(\))?$@', $content, $tmp))
  {
    $op['type']    = 'Operator';
    $op['param']   = $tmp[1];
    $op['size']    = 1;
    if($tmp[2]) $op['highbit'] = true;
  }
  elseif(preg_match('@^DeclareOrBitnum\("(.*)"\)(\.SetHighbit\(\))?$@', $content, $tmp))
  {
    $op['type']    = 'OrBitNumber';
    $op['param']   = $tmp[1];
    $op['size']    = 1;
    if($tmp[2]) $op['highbit'] = true;
  }
  elseif(preg_match('@^DeclareAndBitnum\("(.*)"\)(\.SetHighbit\(\))?$@', $content, $tmp))
  {
    $op['type']    = 'AndBitNumber';
    $op['param']   = $tmp[1];
    $op['size']    = 1;
    if($tmp[2]) $op['highbit'] = true;
  }
  else
  {
    print "Unrecognized content '$content'\n";
    return;
  }
  if($op['param'] == '') unset($op['param']);
  
  if($comment != '') $op['comment'] = $comment;
  
  $max = $op['pos'] + $op['size'];
  if($max > $cmd['size']) $cmd['size'] = $max;

  $cmd['params'][] = $op;
}

function ParseDef($def)
{
  global $cmd;
  
  $comment = '';
  if(preg_match('@// *(.*)@', $def, $tmp))
  {
    $comment = $tmp[1];
    $def = ereg_replace(' *//.*', '', $def);
  }
  
  $def = trim(eregi_replace('\.Annotate[a-z]*\(\)', '', $def));
  
  $op = Array
  (
    
  );
  if(preg_match('@^DeclareProp\("(.*)", *0x([0-9A-F]+)\)$@', $def, $tmp))
  {
    $op['type']    = 'Prop';
    $op['param']   = $tmp[1];
    
    $val = $tmp[2];
    if(strlen($val) == 2) $val = "7E01$val";
    if(strlen($val) == 4) $val = "7E$val";
    $op['value']   = $val;
  }
  elseif(preg_match('@^DeclareConst\("(.*)", *(0x)?([0-9A-F]+)\)$@', $def, $tmp))
  {
    $op['type']    = 'Const';
    $op['param']   = $tmp[1];
    $op['value']   = $tmp[3];
  }
  else
  {
    print "Unrecognized def '$def'\n";
    return;
  }

  $cmd['params'][] = $op;
}

function DumpSequence($cmd)
{
  $seqparams = Array();
  $seqparams['length'] = $cmd['size'];
  $seqparams['cmd']    = $cmd['cmd'];
  if($cmd['has_blob']) $seqparams['has_blob'] = '1';
  
  echo ' '; echo_xml_tag('sequence', $seqparams); echo "\n";
  
  foreach($cmd['params'] as $param)
  {
    echo '  ';
    echo_xml_tag('param', $param, true);
    echo "\n";
  }
  
  if($cmd['comments'] != '')
  {
    echo "  <comments>\n";
    echo htmlspecialchars($cmd['comments']);
    echo "  </comments>\n";
  }
  
  echo " </sequence>\n\n";
}
