<?php
class Downloader {
    private $feed_ip = '127.0.0.1';
    private $auth_secret = 'rWx1qHrhUbnUNxgrVgHu00vCzomRymQGVViEmcRPz1XDVJm6ARITxi';
    
    private $mh;
    private $handleUrl = array();
    private $userAgent = 'Mozilla/5.0 (Windows NT 6.1; WOW64; rv:35.0) Gecko/20100101 Firefox/35.0';
    private $timeout = 15;
    private $minSpace = 1073741824;
    private $stop = 0;

    private $f;
    private $outDir;
    private $startTime;
    private $dlComplete = 0;
    private $totalComplete = 0;
    
    public function __construct($outDir) {
        $this->mh = curl_multi_init();
        //curl_multi_setopt($this->mh, CURLMOPT_MAXCONNECTS, 200);
        $this->outDir = $outDir;
        $this->startTime = time();
        
        echo "connecting...\n";
        $this->f = fsockopen($this->feed_ip, 32301);
        fwrite($this->f, $this->auth_secret);
        $out = fread($this->f, 100);
        if($out!="OK\n") {
            die("no auth\n");
        }
        echo "connected\n";
    }
    
    public function __destruct() {
        curl_multi_close($this->mh);
    }
    
    public function readNextUrl() {
        if($this->minSpace > disk_free_space($this->outDir)) {
            echo "NOT ENOUGH DISK SPACE, STOPPING!!!\n";
            $this->stop = 1;
            return null;
        }
        if(!feof($this->f)) {
            fwrite($this->f, 'a');
            $r = "";
            while(substr($r, -1)!="\n" && !feof($this->f)) {
                $r .= fread($this->f, 1500);
            }
            $url = trim($r);
            return $url;
        }
    }
    
    public function addDownloader() {
        if(file_exists("stop")) {
            return;
        }
        $url = $this->readNextUrl();
        if(!$url) {
            return;
        }
        
        $downloader = curl_init();
        curl_setopt($downloader, CURLOPT_URL, $url);
        curl_setopt($downloader, CURLOPT_HEADER, true);
        curl_setopt($downloader, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($downloader, CURLOPT_CONNECTTIMEOUT, $this->timeout);
        curl_setopt($downloader, CURLOPT_TIMEOUT, $this->timeout);
        curl_setopt($downloader, CURLOPT_BUFFERSIZE, 65536);
        curl_setopt($downloader, CURLOPT_NOPROGRESS, false);
        curl_setopt($downloader, CURLOPT_FOLLOWLOCATION, false);
        curl_setopt($downloader, CURLOPT_USERAGENT, $this->userAgent);
        curl_setopt($downloader, CURLOPT_SSL_VERIFYHOST, 0);
        curl_setopt($downloader, CURLOPT_SSL_VERIFYPEER, 0);
        curl_setopt($downloader, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1);
        curl_setopt($downloader, CURLOPT_TCP_NODELAY, true);
        curl_setopt($downloader, CURLOPT_NOSIGNAL , 1);
        curl_setopt($downloader, CURLOPT_PROGRESSFUNCTION, function($handle, $DownloadSize, $Downloaded, $UploadSize, $Uploaded){
            return $Downloaded > 524288 ? 1 : 0;
        });
        curl_setopt($downloader, CURLOPT_HEADERFUNCTION, function($handle, $string){
            if(strpos($string, "HTTP/")!==false && substr($string, 9, 3)!="200") {
                return 0;
            }
            if(strpos($string, "Content-Type: ")!==false && substr($string, 14, 9)!="text/html") {
                return 0;
            }
            return strlen($string);
        });
        
        
        $this->handleUrl[(int)$downloader] = $url;
        curl_multi_add_handle($this->mh, $downloader);
    }
    
    public function infoRead() {
        while ($info = curl_multi_info_read($this->mh)) {
            if($info['result']==CURLE_OK || $info['result']==CURLE_ABORTED_BY_CALLBACK) {
                if(curl_getinfo($info['handle'], CURLINFO_HTTP_CODE)==200) {
                    $contentType = curl_getinfo($info['handle'], CURLINFO_CONTENT_TYPE);
                    if($contentType=="text/html" || $contentType === null) {                    
                        $content = curl_multi_getcontent($info['handle']);
                        file_put_contents($this->outDir.substr(urlencode($this->handleUrl[(int)$info['handle']]), 0, 96).'_'.  crc32($this->handleUrl[(int)$info['handle']]),
                                $this->handleUrl[(int)$info['handle']]."\r\n".time()."\r\n".$content,
                                LOCK_EX);
                        $this->dlComplete++;
                    }
                }
            }

            echo (time()-$this->startTime)." ".($this->totalComplete++)." ".$this->dlComplete." ".$this->handleUrl[(int)$info['handle']]."\n";
            
            curl_multi_remove_handle($this->mh, $info['handle']);
            unset($this->handleUrl[(int)$info['handle']]);
            curl_close($info['handle']);
            
            $this->addDownloader();
        }
    }
    
    public function execute() {
        do {
            $mrc = curl_multi_exec($this->mh, $active);
            var_dump($mrc);
            echo "multi exec...\n";
        } while ($mrc == CURLM_CALL_MULTI_PERFORM);
        $this->infoRead();

        while ($active && $mrc == CURLM_OK) {
            $s = curl_multi_select($this->mh);
            $s = -1;
            if ($s == -1 || $s < 100) {
                usleep(100);
            }
            do {
                $mrc = curl_multi_exec($this->mh, $active);
                //var_dump($mrc);
                echo "multi exec(".$mrc.")...\n";
            } while ($mrc == CURLM_CALL_MULTI_PERFORM);
            $this->infoRead();
        }
    }
}

if(!@$argv[1]) {
    die("out directory missing\n");
}
if(!(@$argv[2]>=1)) {
    die("invalid number of threads\n");
}
$dir = $argv[1].DIRECTORY_SEPARATOR;
@mkdir($dir);

global $downloader;
$downloader = new Downloader($dir);

for($i=0;$i<$argv[2];$i++) {
    $downloader->addDownloader();
}

$downloader->execute();

