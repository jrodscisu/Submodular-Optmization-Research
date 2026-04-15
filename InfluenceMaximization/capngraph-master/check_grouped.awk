#!/usr/bin/gawk -f
$1 in edges && current != $1 {
  print "File is not grouped.";
  exit;
}

{
  current = $1;
  edges[$1] = 1;
}
