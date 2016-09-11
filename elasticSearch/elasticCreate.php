<?php
require("elastic.php");

$elastic = new elasticImport("http://localhost:9200");

$domains = split("\n", str_replace(array("\r"), array(""), file_get_contents("../tld.txt")));
foreach ($domains as $domain) {
    $domain = str_replace(".", "", $domain);
    $elastic->create($domain, $domain == "com" ? 64 : 32);
    $elastic->mapping($domain, $domain);
}

$domain = "other";
$elastic->create($domain, 32);
$elastic->mapping($domain, $domain);
