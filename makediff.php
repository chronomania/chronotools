<?php

# This is Bisqwit's generic makediff.php, activated from depfun.mak.
# The same program is used in many different projects to create
# a diff file version history (patches).
#
# makediff.php version 2.0.1

# Copyright (C) 2000,2002 Bisqwit (http://iki.fi/bisqwit/)

# Syntax 1:

# argv[1]: Newest archive if any
# argv[2]: Archive directory if any
# argv[3]: Disable /WWW/src linking if set

# Syntax 2:

# argv[1]: -d
# argv[2]: dir1
# argv[3]: dir2

if($REMOTE_ADDR)
{
  header('Content-type: text/plain');
?>
This test was added due to abuse :)
Looks like somebody's friends had fun.

xx.yy.cam.ac.uk - - [20/Nov/2001:02:17:51 +0200] "GET /src/tmp/makediff.php HTTP/1.1" 200 36858 "http://www.google.com/search?q=%22chdir%3A+No+such+file+or+directory%22&btnG=Google+Search" "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:0.9.5) Gecko/20011012"
207.195.43.xx - - [20/Nov/2001:02:18:18 +0200] "GET /src/tmp/makediff.php HTTP/1.1" 200 36858 "-" "Mozilla/4.0 (compatible; MSIE 5.0; Windows NT 5.1) Opera 5.12  [en]"
xx.yy.mi.home.com - - [20/Nov/2001:02:18:39 +0200] "GET /src/tmp/makediff.php HTTP/1.0" 200 36817 "-" "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)"
proxy1.slnt1.on.home.com - - [20/Nov/2001:02:18:53 +0200] "GET /src/tmp/makediff.php HTTP/1.0" 200 36817 "-" "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; MSOCD; AtHome021SI)"
xx-yy-ftw-01.cvx.algx.net - - [20/Nov/2001:02:19:01 +0200] "GET /src/tmp/makediff.php HTTP/1.1" 200 36858 "-" "Mozilla/4.0 (compatible; MSIE 5.0; Windows 98; DigExt; Hotbar 3.0)"
  
I still keep this text here for Google to index this happily.

ChDir: no such file or directory - have fun ;)

(The hostnames have been masked because the original person contacted
 me and explained me what was it about. Wasn't an abuse try.)
<?
  exit;
}

ob_implicit_flush(true);

if($argv[1]=='-d')
{
  MakeDiff($argv[2], $argv[3]);
  exit;
}

if(strlen($argv[2]))
{
  chdir($argv[2]);
  echo "\tcd ", $argv[2], "\n";
}

function ShellFix($s)
{
  return "'".str_replace("'", "'\''", $s)."'";
}

$fp=opendir('.');
$f=array();
while(($fn = readdir($fp)))
{
  if(ereg('\.tar\.gz$', $fn))
    $f[] = ereg_replace('\.tar\.gz$', '', $fn);
  elseif(ereg('\.tar\.bz2$', $fn))
    $f[] = ereg_replace('\.tar\.bz2$', '', $fn);
}
closedir($fp);
$f = array_unique($f);

function padder($k) { return str_pad($k, 9, '0', STR_PAD_LEFT); }
function cmp1($a, $b)
{
  $a1 = preg_replace('/([0-9]+)/e', "padder('\\1')", $a);
  $b1 = preg_replace('/([0-9]+)/e', "padder('\\1')", $b);
  if($a1 == $b1)return 0;
  if($a1 < $b1)return -1;
  return 1;
}
function cmp($a, $b)
{
  $k1 = ereg_replace('^patch-(.*)-[.0-9]*-([.0-9]*).*$', '\1-\2z', $a);
  $k2 = ereg_replace('^patch-(.*)-[.0-9]*-([.0-9]*).*$', '\1-\2z', $b);
  $k = cmp1($k1, $k2);
  if($k)return $k;
  return cmp1($a, $b);
}

usort($f, cmp);

function Eexec($s)
{
  print "\t$s\n";
  return exec($s);
}

function MakeDiffCmd($dir1, $dir2)
{
  return 'diff -NaHudr ' . shellfix($dir1) . ' ' . shellfix($dir2);
}

function MakeDiffCmd2($dir1, $dir2)
{
  return 'php -q ../../makediff.php -d '.shellfix($dir1).' '.shellfix($dir2);
}

function gentoucher($dir, $fn)
{
  $k = lstat($dir.'/'.$fn);
  printf("  tats %s %d.%d 0%o %s %s\n",
    shellfix($fn),
    $k[4], $k[5], $k[2]&07777,
    shellfix(date('Y-m-d H:i:s O', $k[8])),
    shellfix(date('Y-m-d H:i:s O', $k[9])));
}
function regexfix($s)
{
  return str_replace(array('^','[',']','$','\\'),
                     array('\\^','\\[','\\]','\\$','\\\\'),
                     $s);
}

function MakeDiff($dir1, $dir2)
{
  @chdir('archives/archives.tmp');

  print '
#!/bin/sh
reverse=0
if "$1" = "-R"; then reverse=1;shift;fi
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
cat<<EOF
 
 This is a difference (patch) file in
 a shell script format.

 The reason I don\'t support files made
 by GNU diff anymore is that GNU diff by itself
 is completely blind to symlinks, hardlinks,
 file permissions and other changes in the
 filesystem structure. It is not a recommendable
 tool for making updates in projects where that
 kind of things change often.

 This script will apply (and optionally reverse)
 all changes between two versions of directories.
 (Hardlinks, renames and accessmode/time changes
  are not supported yet though.)

EOF
  echo Usage: "$0" [--help] [-R]
  echo -R = reverse patch
fi
tats()
{
  chown $2 "$1"
  chmod $3 "$1"
  touch -d"$5" "$1"
  touch -a -d"$4" "$1"
}
';
  $tmp1='difftmp1.tmp';
  $tmp2='difftmp2.tmp';

  $diffs = array();
  exec('(cd '.shellfix($dir1).'&&find|sed s/^..//|sort) >'.shellfix($tmp1)
     .'&(cd '.shellfix($dir2).'&&find|sed s/^..//|sort) >'.shellfix($tmp2)
     .'&wait'
     .';diff -u`wc -l <'.shellfix($tmp1).'|tr -d \\ ` '.shellfix($tmp1).' '.shellfix($tmp2).'|tail +3|grep -v ^@'     .';rm -f '.shellfix($tmp1), $diffs);
  if(!count($diffs))
  {
    $diffs = file($tmp2);
    foreach($diffs as $a=>$b)$diffs[$a]=' '.substr($b,0,strlen($b)-1);
  }
  
  for($step = 1; ; $step++)
  {
    if($step == 1)
      echo 'if [ $reverse = 0 ]; then', "\n",
           "  revflag=''\n";
    elseif($step == 2)
    {
      echo "else\n",
           "  revflag=' -R'\n";
      $tmp=$dir1;$dir1=$dir2;$dir2=$tmp;
      foreach($diffs as $a=>$s)
      {
        if($s[0]=='+')$diffs[$a] = '-'.substr($s,1); else
        if($s[0]=='-')$diffs[$a] = '+'.substr($s,1);
      }
    }
    elseif($step == 3)
    {
      $tmp=$dir1;$dir1=$dir2;$dir2=$tmp;
      foreach($diffs as $a=>$s)
      {
        if($s[0]=='+')$diffs[$a] = '-'.substr($s,1); else
        if($s[0]=='-')$diffs[$a] = '+'.substr($s,1);
      }
      echo "fi\n";
      break;
    }
    
    $filediffs = array();
    $loppurmdirs = array();
    $makefiles = array();
    foreach($diffs as $s)
    {
      $fn = substr($s, 1);
      
      $doadd = 0;
      if($s[0] == '-')
      {
        if(!is_link($dir1.'/'.$fn) && is_dir($dir1.'/'.$fn))
          $loppurmdirs[] = $fn;
        else
        {
          /* This command deletes anything (be it sock, fifo, link...) */
          echo '  rm -vf ', shellfix($fn), "\n";
        }
      }
      elseif($s[0] == '+')
      {
        $doadd = 1;
      }
      else
      {
        $l1 = is_link($dir1.'/'.$fn);
        $l2 = is_link($dir2.'/'.$fn);
        if($l1)
        {
          if($l2)
          {
            $s1 = readlink($dir1.'/'.$fn);
            $s2 = readlink($dir2.'/'.$fn);
            if($s1 != $s2)
            {
              echo '  rm -v ', shellfix($fn), "\n",
                   '  ln -s ', shellfix($s2), ' ', shellfix($fn), "\n";
            }
          }
          else
          {
            echo '  rm -v ', shellfix($fn), "\n";
            $doadd = 1;
          }
        }
        else
        {
          if($l2)
          {
            echo '  rm -v ', shellfix($fn), "\n";
            $doadd = 1;
          }
          else
          {
            $m1 = lstat($dir1.'/'.$fn);
            $m2 = lstat($dir2.'/'.$fn);
            
            if(($m1[2] & ~07777) != ($m2[2] & ~07777))
            {
              echo '  rm -v ', shellfix($fn), "\n";
              $doadd = 1;
            }
            else
            {
              /* Now we can be sure that is_xxx() returns the same
               * thing for both files
               */
              $ftype = ($m2[2] >> 12) & 017;
              if($ftype == 014) // socket
              {
                /* nothing to check */
              }
              elseif($ftype == 012) // symlink
              {
                /* already verified */
                echo '# ', shellfix($fn), " is a symlink, this should not happen HERE!\n";
              }
              elseif($ftype == 010) // regular file
              {
                $filediffs[] = $fn;
              }
              elseif($ftype == 006  // block device
                  || $ftype == 002) // character device
              {
                $minor1 = $m1[0]&255; $major1=$m1[0]>>8;
                $minor2 = $m2[0]&255; $major2=$m2[0]>>8;
                if($minor1 != $minor2)
                {
                  echo '  rm -v ', shellfix($fn), "\n";
                  $doadd = 1;
                }
              }
              elseif($ftype == 004) // directory
              {
              }
              elseif($ftype == 001) // fifo
              {
                /* nothing to check */
              }
              else
              {
                echo '# ', shellfix($fn), ' is unknown type ', $ftype, ' file!', "\n";
              }
            }
          }
        }
      }
      if($doadd)
      {
        $m2 = lstat($dir2.'/'.$fn);
        $ftype = ($m2[2] >> 12) & 017;
        if($ftype == 014) // socket
        {
          echo '# ', shellfix($fn), " is a socket, can not make it\n";
        }
        elseif($ftype == 012) // symlink
        {
          echo '  ln -s ', readlink($dir2.'/'.$fn), ' ', shellfix($fn), "\n";
        }
        elseif($ftype == 010) // regular file
        {
          $makefiles[] = $fn;
        }
        elseif($ftype == 006)  // block device
        {
          $minor = $m2[0]&255; $major=$m2[0]>>8;
          printf("  mknod %s b %d %d\n", shellfix($fn), $major, $minor);
        }
        elseif($ftype == 002)  // character device
        {
          $minor = $m2[0]&255; $major=$m2[0]>>8;
          printf("  mknod %s c %d %d\n", shellfix($fn), $major, $minor);
        }
        elseif($ftype == 004) // directory
        {
          echo '  mkdir -v ', shellfix($fn), "\n";
        }
        elseif($ftype == 001) // fifo
        {
          echo '  mkfifo ', shellfix($fn), "\n";
        }
        else
        {
          echo '# ', shellfix($fn), ' is unknown type ', $ftype, ' file!', "\n";
        }
      }
    }

    $md5s = array();
    $md5f = array();
    /* Disable this code if you don't want to waste
     * resources on making md5sums
     */
    foreach($makefiles as $fn)
    {
      $md5sum = exec('md5sum < '.shellfix($fn).'|sed "s/ .*//"');
      $md5s[$fn] = $md5sum;
    }
    
    if(count($makefiles))
      echo "  patch -p0 << 'EOF'\n";
    $firstmd5 = array();
    foreach($makefiles as $fn)
    {
      $md5sum = $md5s[$fn];
      if($firstmd5[$md5sum])
      {
        $md5f[] = array($firstmd5[$md5sum], $fn);
        continue;
      }
      else
        $firstmd5[$md5sum] = $fn;
      
      passthru('cd '.shellfix($dir2).
               '&&echo -n "" >' . shellfix($tmp2).
               '&&diff -NaHud '.shellfix($tmp2).' '.shellfix($fn).
               '&&rm -f '.shellfix($tmp2));
    }
    if(count($makefiles))
      echo "EOF\n";
    rsort($loppurmdirs);
    foreach($loppurmdirs as $s)
      echo '  rmdir -v ', shellfix($s), "\n";
    foreach($firstmd5 as $tab)
        echo '  cp ', shellfix($tab[0]), ' ', shellfix($tab[1]), "\n";
  }
  if(count($filediffs))
  {
    echo 'patch -p0 $revflag'," << 'EOF'\n";
  }
  foreach($filediffs as $fn)
  {
    passthru('diff -NaHud '.shellfix($dir1.'/'.$fn).' '.shellfix($dir2.'/'.$fn).
             '|sed '.shellfix('s@^--- '.regexfix($dir1.'/').'@--- @'.
                             ';s@^--- '.regexfix($dir2.'/').'@--- @'.
                             ';s@^\\+\\+\\+ '.regexfix($dir2.'/').'@+++ @'.
                             ';s@^\\+\\+\\+ '.regexfix($dir2.'/').'@+++ @')
             );
  }
  if(count($filediffs))
    echo "EOF\n";
}

function FindInodes($directory)
{
  $inodes = array();
  $fp = @opendir($directory);
  if(!$fp)
  {
    print "OPENDIR $directory failed!\n";
    exit;
  }
  
  while(($fn = readdir($fp)))
  {
    if($fn=='.' || $fn=='..')continue;
    
    $fn = $directory.'/'.$fn;
    if(!is_link($fn))
    {
      $st = stat($fn);
      
      $inodes[$st[0].':'.$st[1]] = $fn;
    }
    if(is_dir($fn))
    {
      $inodes = $inodes + FindInodes($fn);
    }
  }
  closedir($fp);
  return $inodes;
}

function EraLinks($directory, &$inomap)
{
  $links = array();
  $fp = @opendir($directory);
  if(!$fp)
  {
    print "OPENDIR $directory failed!\n";
    exit;
  }
  while(($fn = readdir($fp)))
  {
    if($fn=='.' || $fn=='..')continue;
    
    $fn = $directory.'/'.$fn;
    if(is_link($fn))
    {
      $st = stat($fn); /* See if the target has already been included */
      if($inomap[$st[0].':'.$st[1]])
        $links[$fn] = $fn;
    }

    if(is_dir($fn))
      $links = $links + EraLinks($fn, $inomap);
  }
  closedir($fp);
  return $links;
}

$tmpdir = 'archives.tmp'; /* Subdir to not overwrite anything */

$tmpdir2= 'abcdefg.tmp';  /* Subdir to get the archive name safely
                           * This directory must not exist within
                           * the archive being analyzed.
                           */

mkdir($tmpdir, 0700);
mkdir($tmpdir.'/'.$tmpdir2, 0700);
$prev = '';
$madeprev = 0;
$lastprog = '';
foreach($f as $this)
{
  chdir($tmpdir);
  
  $v1 = ereg_replace('^.*-([0-9])', '\1', $prev);
  $v2 = ereg_replace('^.*-([0-9])', '\1', $this);
  $prog = ereg_replace('-[0-9].*', '', $this);
  
  if(!strlen($lastprog))$lastprog = $prog;
  
  if(strlen($prev) && ($this == $argv[1] || !strlen($argv[1])) && $prog == $lastprog)
  {
    if(!$madeprev)
    {
      chdir($tmpdir2);
      print 1;
      if(file_exists('../../'.$prev.'.tar.gz'))
        Eexec('gzip -d < ../../'.shellfix($prev).'.tar.gz | tar xf -');
      else
        Eexec('bzip2 -d < ../../'.shellfix($prev).'.tar.bz2 | tar xf -');
      $prevdirs = exec('echo *');
      exec('mv * ../');
      chdir('..');
    }
    if(1) /* Always true. Just for symmetry with the previous thing. */
    {
      $madeprev = 1;
      chdir($tmpdir2);
      $thisfn = '../'.$this.'.tar.gz';
      print 2;
      if(file_exists('../'.$thisfn))
        Eexec('gzip -d < ../'.shellfix($thisfn).' | tar xf -');
      else
      {
        $thisfn = '../'.$this.'.tar.bz2';
        Eexec('bzip2 -d < ../'.shellfix($thisfn).' | tar xf -');
      }
      $thisdirs = exec('echo *');
      exec('mv * ../');
      chdir('..');
    }

    $diffname = '../patch-'.$prog.'-'.$v1.'-'.$v2;
    
    //Eexec(MakeDiffCmd2($prevdirs, $thisdirs).
    //      '|gzip -9 >'.shellfix($diffname).'.sh.gz');

    $inomap = FindInodes($thisdirs) + FindInodes($prevdirs);
    $links = EraLinks($thisdirs, $inomap) + EraLinks($prevdirs, $inomap);
    $rmcmd = '';
    foreach($links as $linkname)
      $rmcmd .= shellfix($linkname).' ';
    if(strlen($rmcmd))
    {
      /* Erase files which of targets are already being diffed */
      Eexec('rm -f '.$rmcmd);
    }

    Eexec(MakeDiffCmd($prevdirs, $thisdirs).
          '|gzip -9 >'.shellfix($diffname).'.gz');

    Eexec('rm -rf '.$prev);

    Eexec('gzip -d <'.shellfix($diffname).'.gz|bzip2 -9 >'.shellfix($diffname).'.bz2');
    //Eexec('gzip -d <'.shellfix($diffname).'.sh.gz|bzip2 -9 >'.shellfix($diffname).'.sh.bz2');

    //Eexec('touch -r'.$thisfn.' '.shellfix($diffname).'.{sh.,}{gz,bz2}');
    //Eexec('chown --reference '.$thisfn.' '.shellfix($diffname).'.{sh.,}{gz,bz2}');
    //if(!$argv[3])Eexec('ln -f '.shellfix($diffname).'.{sh.,}{gz,bz2} /WWW/src/');

    Eexec('touch -r'.$thisfn.' '.shellfix($diffname).'.{gz,bz2}');
    Eexec('chown --reference '.$thisfn.' '.shellfix($diffname).'.{gz,bz2}');
    if(!$argv[3])Eexec('ln -f '.shellfix($diffname).'.{gz,bz2} /WWW/src/');
  }
  else
    $madeprev = 0;
  
  $lastprog = $prog;
  
  chdir('..');
  
  $prev     = $this;
  $prevdirs = $thisdirs;
}

exec('rm -rf '.shellfix($tmpdir));
echo "\tcd ..\n";
