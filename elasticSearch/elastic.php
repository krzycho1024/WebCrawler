<?php
class elasticImport {
    private $elasticUrl;
    
    public function __construct($elasticUrl) {
        $this->elasticUrl=$elasticUrl;
    }
    
    public function delete($index) {
        $ch = curl_init($this->elasticUrl . "/" . $index);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "DELETE");
        $response = curl_exec($ch);
        var_dump($response);
    }
    
    public function create($index, $shards) {
        $data = array(
            'settings' => array(
                'index' => array(
                    'analysis' => array(
                        'analyzer' => array(
                            'analyzer_startswith' => array(
                                'tokenizer'=>'keyword',
                                'filter'=>'lowercase',
                            )
                        )
                    ),
                    'number_of_shards'=> $shards,
                    'number_of_replicas'=> 1,
                ),
            ),
        );
        $ch = curl_init($this->elasticUrl."/".$index);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($data));
        $response = curl_exec($ch);
        var_dump($response);
        do {
            sleep(5);
            $ch = curl_init($this->elasticUrl."/".$index."/_status");
            curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
            curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "GET");
            $response = curl_exec($ch);
            $resp = json_decode($response);
            echo $resp->_shards->successful." of ".$resp->_shards->total."\n";
        } while($resp->_shards->successful<$resp->_shards->total);
    }
    
    public function mapping($index, $mapping) {
        $data = array(
            'properties' => array(
                'time'=>array(
                    "type" => "long", "store" => true, "index" => "no"
                ),
                'to_domain'=>array(
                    "type" => "string", "store" => true, "index" => "analyzed",
                    "search_analyzer"=>"analyzer_startswith",
                    "index_analyzer"=>"analyzer_startswith",
                ),
                'from_domain'=>array(
                    "type" => "string", "store" => true, "index" => "analyzed",
                    "search_analyzer"=>"analyzer_startswith",
                    "index_analyzer"=>"analyzer_startswith",
                ),
                'to_proto'=>array(
                    "type" => "boolean", "store" => true, "index" => "no"
                ),
                'from_proto'=>array(
                    "type" => "boolean", "store" => true, "index" => "no"
                ),
                'to_loc'=>array(
                    "type" => "string", "store" => true, "index" => "no"
                ),
                'from_loc'=>array(
                    "type" => "string", "store" => true, "index" => "no"
                ),
                'atext'=>array(
                    "type" => "string", "store" => true, "index" => "no"
                ),
                'arel'=>array(
                    "type" => "string", "store" => true, "index" => "no"
                ),
                'img'=>array(
                    "type" => "boolean", "store" => true, "index" => "no"
                ),
                'alt'=>array(
                    "type" => "string", "store" => true, "index" => "no"
                ),
            )
        );

        $ch = curl_init($this->elasticUrl."/".$index."/_mapping/".$mapping);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($data));
        $response = curl_exec($ch);
        var_dump($response);
    }
    
    public function bulk($data) {
        $ch = curl_init($this->elasticUrl."/_bulk?");
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
        curl_setopt($ch, CURLOPT_POSTFIELDS, implode("\n", $data)."\n");
        $response = curl_exec($ch);
        
        if(!$response) {
            echo "no resp!!!\n";
        }
        $ret = json_decode($response);
        if($ret->errors) {
            echo "errors\n";
            foreach($ret->items as $item) {
                if($item->create->error) {
                    var_dump($item->create->error);
                }
            }
        }
    }
}
