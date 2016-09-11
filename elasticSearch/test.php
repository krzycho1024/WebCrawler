<?php

$domain = "yahoo.com";
$index = "com";

$data = array(
    'from' => 0,
    'size' => 1000,
    'query' => array(
	'bool' => array(
	    'should' => array(
		array('match_phrase_prefix' => array(
		    'to_domain' => strrev('.'.$domain)
		)),
		array('term' => array(
		    'to_domain' => strrev($domain)
		))
	    ),
	    'must_not' => array(
		array('match_phrase_prefix' => array(
		    'from_domain' => strrev('.'.$domain)
		)),
		array('term' => array(
		    'from_domain' => strrev($domain)
		))		
	    )
	)
    )
);

$ch = curl_init("http://localhost:9200/".$index."/".$index."/_search?pretty=yes");
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "GET");
curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($data));
$response = curl_exec($ch);
var_dump($response);
