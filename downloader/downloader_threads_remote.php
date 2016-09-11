<?php

class Downloader extends Worker {
    private $userAgent = 'Mozilla/5.0 (Windows NT 6.1; WOW64; rv:35.0) Gecko/20100101 Firefox/35.0';
    private $timeout = 30;
    
    protected $outDir;
    protected $reader;


    public function __construct(Reader $reader, $outDir) {
        echo "worker construct\n";
        $this->outDir = $outDir;
        $this->reader = $reader;
    }
    
    public function download($url) {
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
        
        $content = curl_exec($downloader);
        $downloaded = false;
        
        if(curl_getinfo($downloader, CURLINFO_HTTP_CODE)==200) {
            $contentType = curl_getinfo($downloader, CURLINFO_CONTENT_TYPE);
            if($contentType=="text/html" || $contentType === null) {                    
                file_put_contents($this->outDir.substr(urlencode($url), 0, 96).'_'.  crc32($url), $url."\r\n".time()."\r\n".$content, LOCK_EX);
                $downloaded = true;
            }
        }
        curl_close($downloader);
        return $downloaded;
    }
}

class Reader extends Stackable {
    private $feed_ip = '127.0.0.1';
    private $auth_secret = 'rWx1qHrhUbnUNxgrVgHu00vCzomRymQGVViEmcRPz1XDVJm6ARITxi';
        
    private $fileHandle = null;
    private $startTime;
    private $reads = 0;
    private $downloaded = 0;
    private $mutex;
    private $mutexUpdate;
    private $directory;
    private $minSpace = 268435456;
    
    private $f;
    
    public function __construct($dir) {
        echo "Reader construct\n";
        $this->mutex = Mutex::create();
        $this->mutexUpdate = Mutex::create();
        $this->startTime = time();
        $this->directory = $dir;
        
        echo "connecting...\n";
        $this->f = fsockopen($this->feed_ip, 32301);
        fwrite($this->f, $this->auth_secret);
        $out = fread($this->f, 100);
        if($out!="OK\n") {
            die("no auth\n");
        }
        echo "connected\n";
    }
    
    public function updateDownloaded() {
        Mutex::lock($this->mutexUpdate);
        $this->downloaded++;
        Mutex::unlock($this->mutexUpdate);
    }
    
    public function read() {
        Mutex::lock($this->mutex);
        if($this->minSpace > disk_free_space($this->directory)) {
            echo "NOT ENOUGH DISK SPACE, STOPPING!!!\n";
            Mutex::unlock($this->mutex);
            return false;
        }
        
        if((time()-$this->startTime) > 600) {
            echo "TAKE BREAK!!!\n";
            Mutex::unlock($this->mutex);
            return false;
        }        

        if(!feof($this->f)) {
            fwrite($this->f, 'a');
            $r = "";
            while(substr($r, -1)!="\n" && !feof($this->f)) {
                $r .= fread($this->f, 1500);
            }
            $url = trim($r);
            Mutex::lock($this->mutexUpdate);
            echo (time()-$this->startTime).' '.($this->reads++).' '.$this->downloaded.' '.$url."\n";
            Mutex::unlock($this->mutexUpdate);
            Mutex::unlock($this->mutex);
            return $url;
        }
        Mutex::unlock($this->mutex);
        return false;
    }
}

class WebWork extends Stackable {
    protected $complete;
    
    public function isComplete() {
	return $this->complete;
    }
    
    public function run()
    {
        do {
            $url = $this->worker->reader->read();
            //echo "Downloading: $url";
            if($url) {
                if($this->worker->download($url)) {
                    $this->worker->reader->updateDownloaded();
                }
            }
        } while($url!==false);
        $this->complete = true;
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

$pool = new Pool($argv[2], \Downloader::class, [new Reader($dir), $dir]);
for($i=0;$i<$argv[2];$i++) {
    try {
        $pool->submit(new WebWork());
    } catch(Exception $ex) {
	var_dump($ex);
	$i--;
    }
}

$pool->shutdown();

$pool->collect(function($work){
	return $work->isComplete();
});

sleep(5);
