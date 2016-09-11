<?php
require("elastic.php");

$elastic = new elasticImport("http://localhost:9200");

$domains = split("\n", str_replace(array("\r"), array(""), file_get_contents("1.txt")));

$db = new SQLite3("data.sqlite3");

$db->query("CREATE TABLE IF NOT EXISTS Imported (file TEXT UNIQUE)");

function importFile($file, $fileName) {
    global $db, $domains, $elastic;
    $db->query("INSERT OR IGNORE INTO Imported (file) VALUES ('".$db->escapeString($fileName)."');");
    if($db->changes()>0) {
        var_dump($file);
        $f = fopen($file, "r");
        $c = 0;
        $send = array();
        while(!feof($f)) {
            $s = fgets($f);
            if($s) {
                $d = explode("\t", rtrim($s, "\n"));
                $data = array(
                    'time'=>$d[1],
                    'to_domain'=>strrev(mb_check_encoding($d[2], 'UTF-8')?$d[2]:utf8_encode($d[2])),
                    'from_domain'=>strrev(mb_check_encoding($d[3], 'UTF-8')?$d[3]:utf8_encode($d[3])),
                    'to_proto'=>$d[4]?"1":"0",
                    'from_proto'=>$d[5]?"1":"0",
                    'to_loc'=>$d[6]?(mb_check_encoding($d[6], 'UTF-8')?$d[6]:utf8_encode($d[6])):'/',
                    'from_loc'=>$d[7]?(mb_check_encoding($d[7], 'UTF-8')?$d[7]:utf8_encode($d[7])):'/',
                    'atext'=>$d[8]?(mb_check_encoding($d[8], 'UTF-8')?$d[8]:utf8_encode($d[8])):"",
                    'arel'=>$d[9]?(mb_check_encoding($d[9], 'UTF-8')?$d[9]:utf8_encode($d[9])):"",
                    'img'=>$d[10]?"true":"false",
                    'alt'=>$d[11]?(mb_check_encoding($d[11], 'UTF-8')?$d[11]:utf8_encode($d[11])):"",
                );
                //var_dump($data);
                //die();
            	//    var_dump($d[0]);
                if(in_array($d[0], $domains)) {
                    $send[] = '{"index":{"_index":"'.str_replace(".", "", $d[0]).'", "_type":"'.str_replace(".", "", $d[0]).'"}'."\n".json_encode($data);
                } else {
                    $send[] = '{"index":{"_index":"other", "_type":"other"}'."\n".json_encode($data);
                }
                if(count($send)>200) {
                    echo "send ".($c++)."\n";
                    //var_dump($send);
                    $elastic->bulk($send);
                    $send = array();
                }
            }
        }
    }
}

function findFiles($dir) {
    if (is_dir($dir)) {
        if ($dh = opendir($dir)) {
            while (($file = readdir($dh)) !== false) {
                if($file!='.'&&$file!='..') {
                    if(is_file($dir . '/' . $file)) {
                        importFile($dir . '/' . $file, $file);
                    } else if(is_dir($dir . '/' . $file)) {
                        findFiles($dir . '/' . $file);
                    }
                }
            }
            closedir($dh);
        }
    }
}

findFiles('import');
