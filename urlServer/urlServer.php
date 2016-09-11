<?php
class urlServer {
    private $lip = '0.0.0.0';
    private $lport = 32301;
    private $lsocket;
    
    private $lNumber = 5;
    
    private $read;
    private $clients;
    private $clientsAuthenticated;
    private $waitingForReplay;
    private $sendSomething;
    
    private $file;
    
    private $secret = 'rWx1qHrhUbnUNxgrVgHu00vCzomRymQGVViEmcRPz1XDVJm6ARITxi';
    
    public function __construct($filename) {
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
        $this->read = array();
        $this->clients = array();
        $this->clientsAuthenticated = array();
        $this->waitingForReplay = array();
        $this->sendSomething = array();
        $this->file = fopen($filename, "r");
    }
    
    public function getUrl() {
        if(!feof($this->file)) {
            return trim(fgets($this->file));
        }
        return "";
    }
    
    public function run() {
        while(true) {
            $read = array_merge(array($this->lsocket), $this->clients, $this->clientsAuthenticated);
            $write = array_merge($this->waitingForReplay, $this->sendSomething);
            socket_select($read, $write, $except=NULL, 180);
            if(in_array($this->lsocket, $read)) {
                $sock = socket_accept($this->lsocket);
                socket_set_nonblock($sock);
                $this->clients[] = $sock;
                echo "new connection ".((int)$sock)."\n";
                //socket_getpeername($sock, $ip, $port);
            }
            foreach($this->waitingForReplay as $replay) {
                if(in_array($replay, $write)) {
                    $url = $this->getUrl();
                    if(!$url) {
                        socket_close($replay);
                        $this->clientsAuthenticated = array_diff($this->clientsAuthenticated, array($replay));
                    } else {
                        $url.="\n";
                        echo $url;
                        socket_write($replay, $url, strlen($url));
                    }
                    $this->waitingForReplay = array_diff($this->waitingForReplay, array($replay));
                }
            }
            foreach($this->sendSomething as $replay) {
                if(in_array($replay, $write)) {
                    socket_write($replay, "OK\n", strlen("OK\n"));
                    $this->sendSomething = array_diff($this->sendSomething, array($replay));
                }
            }
            foreach($this->clientsAuthenticated as $client) {
                if(in_array($client, $read)) {
                    $r = socket_read($client, 2048);
                    if(!$r) {
                        echo "connection colose\n";
                        socket_close($client);
                        $this->clientsAuthenticated = array_diff($this->clientsAuthenticated, array($client));
                    } else {
                        echo "data: ".$r."\n";
                        $this->waitingForReplay[] = $client;
                    }
                }
            }
            foreach($this->clients as $client) {
                if(in_array($client, $read)) {
                    $r = socket_read($client, 2048);
                    if(!$r) {
                        echo "connection colose\n";
                        socket_close($client);
                    } else {
                        if($r==$this->secret) {
                            echo "auth\n";
                            $this->clientsAuthenticated[] = $client;
                            $this->sendSomething[] = $client;
                        } else {
                            echo "no auth\n";
                            socket_close($client);
                        }
                    }
                    $this->clients = array_diff($this->clients, array($client));
                }
            }                
        }
    }
}

if($argc<2) {
    echo "no list file\n";
}

$server = new urlServer($argv[1]);
$server->run();
