<?php
class parserClient {
    public $socket;
    public $authenticated = false;
    public $fileData;
    public $fileQueue;
    public $fileOpened = 0;
    
    public $receivedData;
    public $write = array();
    
    public function __construct($socket) {
        $this->socket = $socket;
    }
    
    private function openFileHandles() {
        socket_getpeername($this->socket, $ip, $port);
        $this->fileOpened = time();
	echo "out/".$ip."_".$this->fileOpened.".data\n";
        $this->fileData = fopen("out/".$ip."_".$this->fileOpened.".data", "a");
	echo "out/".$ip."_".$this->fileOpened.".queue\n";
        $this->fileQueue = fopen("out/".$ip."_".$this->fileOpened.".queue", "a");
    }
    
    private function closeFileHandles() {
        if($this->fileData) {
            fclose($this->fileData);
        }
        if($this->fileQueue) {
            fclose($this->fileQueue);
        }
    }
   
    public function processData($data) {
        //echo "data: ".$data." [".  bin2hex($data)."]\n";
        if(isset($this->receivedData)) {
            $this->receivedData .= $data;
        } else {
            $this->receivedData = $data;
        }
        $rpos = strrpos($this->receivedData, "\x00");
        if($rpos!==false) {
            if($this->fileOpened+600<time()) {
                $this->closeFileHandles();
                $this->openFileHandles();
            }
            $d = explode("\x00", substr($this->receivedData, 0, $rpos));
            foreach($d as $data) {
                $type = substr($data, 0, 1);
                if($type=="\x01") {
                    fwrite($this->fileData, substr($data, 1)."\n");
                } else if($type=="\x02") {
                    fwrite($this->fileQueue, substr($data, 1)."\n");
                } else {
                    file_put_contents("error.log", "data error? [".$data."]\n", FILE_APPEND);
                }                
            }
            if(strlen($this->receivedData)+1==$rpos) {
                unset($this->receivedData);
            } else {
                $this->receivedData = substr($this->receivedData, $rpos+1);
            }
        }
    }

    public function __destruct() {
        $this->closeFileHandles();
    }
}

class parserServer {
    private $lip = '0.0.0.0';
    private $lport = 32302;
    private $lsocket;
    
    private $lNumber = 5;
    
    private $clients;
    
    private $secret = 'tVoGXdKarqza4TKEoJwugHTBMGeR5DHefm6MdaDfBHH8Mrnw5LCH9tpyA';
    
    public function __construct() {
        if (($this->lsocket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP)) < 0) {
            die("socket_create() failed: " . socket_strerror($this->lsocket) . "\n");
        }
        if ( ! socket_set_option($this->lsocket, SOL_SOCKET, SO_REUSEADDR, 1)) 
        { 
            echo socket_strerror(socket_last_error($this->lsocket)); 
            exit; 
        }
        if ( ! socket_bind($this->lsocket, $this->lip, $this->lport)) {
            die("socket_bind() failed: " . socket_strerror($ret) . "\n");
        }
        if ( ! socket_listen($this->lsocket, $this->lNumber)) {
            die("socket_listen() failed: reason: " . socket_strerror($ret) . "\n");
        }
        $this->clients = array();
    }
    
    public function run() {
        while(true) {
            $read = array($this->lsocket);
            $write = array();
            foreach($this->clients as $client) {
                /* @var $client parserClient */
                $read[] = $client->socket;
                if(count($client->write)>0) {
                    $write[] = $client->socket;
                }
            }
            socket_select($read, $write, $except=NULL, 180);
            if(in_array($this->lsocket, $read)) {
                $sock = socket_accept($this->lsocket);
                socket_set_nonblock($sock);
                $this->clients[] = new parserClient($sock);
                echo "new connection ".((int)$sock)."\n";
                //socket_getpeername($sock, $ip, $port);
            }
            foreach($this->clients as $client) {
                /* @var $client parserClient */
                if(in_array($client->socket, $write) && count($client->write)>0) {
                    $data = array_pop($client->write);
                    socket_write($client->socket, $data, strlen($data));
                }
            }
            foreach($this->clients as $key => $client) {
                /* @var $client parserClient */
                if(in_array($client->socket, $read)) {
                    if($client->authenticated==false) {
                        $r = socket_read($client->socket, 2048);
                        if($r==$this->secret) {
                            $client->authenticated = true;
                            $client->write[] = "OK\n";
                        } else {
                            socket_close($client->socket);
                            //$this->clients = array_diff($this->clients, array($client));
                            unset($this->clients[$key]);
                            unset($client);
                        }
                    } else {
                        $r = socket_read($client->socket, 16384);
                        if(!$r) {
                            socket_close($client->socket);
                            //$this->clients = array_diff($this->clients, array($client));
                            unset($this->clients[$key]);
                            unset($client);
                        } else {
                            $client->processData($r);
                        }
                    }
                }
            }
        }
    }
}

$server = new parserServer();
$server->run();
