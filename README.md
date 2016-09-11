# WebCrawler

I dodin't make this work fully automatic.<br/>
Exptect 80-260mbps download trafiic on i7-2660 and abuse comming to your ISP. Also you'll be oveloading Bind(or other DNS server).<br/>

How to start?<br/>
1. Get latest dump from http://rdf.dmoz.org/ (file: content.rdf.u8)<br/>
use: urlServer/content2urlList.php to build first url file<br/>
2. Run server providing url's:<br/>
urlServer/urlServer.php data <br/>
3. Create ramdrive "dl" folder in downloader directory. Size depends on how fast server can parse files.<br/>
On I7-2660 it does near real time- so 2gB is enouth.<br/>
4. Run parserServer/parserServer.php<br/>
4. Run parser9- it will scan dl directory.<br/>
5. Run downloader_remote.php- it will start downloading<br/>

parserServer.php produces 2 types of files:<br/>
.data -> this is backlink data<br/>
.queue -> this are url's to download<br/>
to create unique list of url's to download use: urlServer/uniq.c<br/>

urlServer/urlServer.php -> feeds downloaders with list of url<br/>
downloader/downloader_remote.php -> using curl multithread- curl has to be compiled with threaded resolver<br/>
downloader/downloader_threads_remote.php -> php downloader using pthread- php has some issues with multiple threads so it often crashed(internal php error)<br/>
uniq.c -> bst tree duplicate remover- when crawler producess mass amount of url's somehow list for urlServer has to be created<br/>

FreeBSD ramdrive setup:<br/>
mdconfig -a -t malloc -s 2048m -u 1<br/>
newfs -U md1<br/>
mount /dev/md1 /root/dl2/dl/<br/>

FreeBSD UDP tuning(Bind sends lots of packets):<br/>
sysctl net.inet.udp.recvspace=168320<br/>
sysctl net.inet.udp.maxdgram=36864<br/>
