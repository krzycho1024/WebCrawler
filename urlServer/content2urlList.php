<?php
$f = fopen("content.rdf.u8", "r");
while(!feof($f)) {
    $line = fgets($f);
    if(strpos($line, "ExternalPage")!==false) {
	if(preg_match('/about="(.+?)"/ims', $line, $out)) {
	    $l = $out[1];
	    file_put_contents("data", $l."\n", FILE_APPEND);
	}
    }
}
fclose($f);