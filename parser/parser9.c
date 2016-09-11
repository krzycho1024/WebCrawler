#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/file.h> 
#include <sys/types.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include <iconv.h>

#include <uriparser/Uri.h>

#include "urlencode.c"

enum protocol {
    http,
    https,
    current,
    error
};

char *data = NULL;
long len = 0;
char *global_url = NULL;
char *global_domain = NULL;
int s, ws;
char *downloadTime = NULL;
enum protocol srcProto;

int sockfd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;

char *get_domain(char *s) {
    int i = 0;
    int len = strlen(s);
    while(i<len&&s[i]!='/') i++;
    i++;
    if(i>len)
	return NULL;
    while(i<len&&s[i]!='/') i++;
    i++;
    if(i>len)
	return NULL;
    int j = i;
    while(i<len&&s[i]!='/'&&s[i]!=':') i++;
    if(i>j) {
	char *tmp = (char *) malloc(i-j+1);
	memcpy(tmp, s+j, i-j);
	tmp[i-j]=0;
	return tmp;
    }
    return NULL;
}

char *get_domain_nohttp(char *s) {
    int i = 0;
    int len = strlen(s);
    int j = i;
    while(i<len&&s[i]!='/'&&s[i]!=':') i++;
    if(i>j) {
	char *tmp = (char *) malloc(i-j+1);
	memcpy(tmp, s+j, i-j);
	tmp[i-j]=0;
	return tmp;
    }
    return NULL;
}

char *get_tld(char *domain) {
    int i = 0;
    int len = strlen(domain);
    int lastDotPos = 0;
    while(i<len) {
	if(domain[i]=='.') {
	    lastDotPos = i;
	}
	i++;
    }
    char *tld = (char *) malloc(len-lastDotPos+1);
    memcpy(tld, domain+lastDotPos, len-lastDotPos+1);
    return tld;
}


enum protocol getProtocol(char *url) {
    int len = strlen(url);
    if(len>=7 && url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p' && url[4] == ':' && url[5] == '/' && url[6] == '/') {
        return http;
    }
    if(len>=8 && url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p' && url[4] == 's' && url[5] == ':' && url[6] == '/' && url[7] == '/') {
        return https;
    }
    if(len>=3 && url[0] == ':' && url[1] == '/' && url[2] == '/') {
        return current;
    }
    return error;
}

typedef struct {
    int img;
    char *alt;
} IMG;

IMG* get_img(char *anchor) {
    IMG* ret = (IMG *) malloc(sizeof(IMG));
    ret->img = 0;
    ret->alt = NULL;
    int len = strlen(anchor);
    int i = 0;
    while(i+4<len) {
	if(anchor[i]=='<' &&
	    (anchor[i+1]=='i' || anchor[i+1]=='I') &&
	    (anchor[i+2]=='m' || anchor[i+2]=='M') &&
	    (anchor[i+3]=='g' || anchor[i+3]=='G') &&
	    anchor[i+4]==' ') {
	    i+=4;
	    ret->img = 1;
	    while(i+3<len) {
		if((anchor[i]=='a'||anchor[i]=='A') &&
		    (anchor[i+1]=='l'||anchor[i+1]=='L') &&
		    (anchor[i+2]=='t'||anchor[i+2]=='T') &&
		    anchor[i+3]=='=') {
		    i+=4;
		    char stopchar = ' ';
		    if(anchor[i]=='"') {
			stopchar = '"';
			i++;
		    }
		    if(anchor[i]=='\'') {
			stopchar = '\'';
			i++;
		    }
		    int j=i;
		    while(j<len&&anchor[j]!=stopchar&&anchor[j]!='>') {
			j++;
		    }
		    if(j-i-1>160) {
			j=i+160;
		    }
		    ret->alt = (char *) malloc(j-i+1);
		    memcpy(ret->alt, anchor+i, j-i);
		    ret->alt[j-i]=0x00;
		    return ret;
		}
		i++;
	    }
	}
	i++;
    }
    return ret;
}

char *dest_slash(char *s) {
    int i = 0;
    int len = strlen(s);
    while(i<len) {
	if(s[i]=='/')
	    return s;
	i++;
    }
    char *tmp = (char *) malloc(len+2);
    memcpy(tmp, s, len);
    tmp[len]='/';
    tmp[len+1]=0;
    free(s);
    return tmp;
}
 
char *charset = NULL;

void find_meta(int dataBegin) {
    int i=dataBegin;
    while(i<len) {
	if(data[i]=='<') {
	    i++;
	    while(i<len&&data[i]==' ') i++;
	    if((data[i]=='m'||data[i]=='M')&&(data[i+1]=='e'||data[i+1]=='E')&&(data[i+2]=='t'||data[i+2]=='T')&&(data[i+3]=='a'||data[i+3]=='A')) {
		i+=4;
		while(i<len&&data[i]!='>') {
		    if( (data[i]=='c'||data[i]=='C') &&
		    (data[i+1]=='h'||data[i+1]=='H') &&
		    (data[i+2]=='a'||data[i+2]=='A') &&
		    (data[i+3]=='r'||data[i+3]=='R') &&
		    (data[i+4]=='s'||data[i+4]=='S') &&
		    (data[i+5]=='e'||data[i+5]=='E') &&
		    (data[i+6]=='t'||data[i+6]=='T')) {
			while(i<len&&data[i]!='=') i++;
			if(data[i]=='=') {
			    i++;
			    while(i<len&&data[i]==' ') i++;
			    if(data[i]=='\'') {
				i++;
				int start = i;
				while(i<len&&data[i]!='\'') i++;
				charset = (char *)malloc(i-start+1);
				memcpy(charset, data+start, i-start);
				charset[i-start]=0;
			    } else if(data[i]=='"') {
				i++;
				int start = i;
				while(i<len&&data[i]!='"') i++;
				charset = (char *)malloc(i-start+1);
				memcpy(charset, data+start, i-start);
				charset[i-start]=0;
			    } else {
				int start = i;
				while(i<len&&((data[i]>='a'&&data[i]<='z')||(data[i]>='A'&&data[i]<='Z')||(data[i]>='0'&&data[i]<='9')||data[i]=='-')) i++;
				charset = (char *)malloc( i-start+1);
				memcpy(charset, data+start, i-start);
				charset[i-start]=0;
			    }
			    return;
			}
		    }
		    i++;
		}
	    }
	}
	i++;
    }
}

char *escape(char *data) {
    int i;
    int len = strlen(data);
    for(i=0;i<len;i++) {
	if(data[i]=='\r'||data[i]=='\n'||data[i]==0x01||data[i]=='\t') data[i]=' ';
    }
    return data;
}

char *clean_tags(char *d, int len) {
    if(len==0)
	return NULL;
    char *ret = (char *) malloc(len+1);
    int s = 0;
    int r = 0;
    int intag = 0;
    while(s<len) {
	if(d[s]=='<') {
	    intag = 1;
	} else if(d[s]=='>') {
	    intag = 0;
	} else if(!intag)
	    ret[r++] = d[s];
	s++;
    }
    ret[r]=0;
    return ret;
}

char *trim_spaces(char *d, int len) {
    if(len==0)
	return NULL;
    int i = 0;
    int l = len;
    while(l>0&&(d[l-1]==' '||d[l-1]=='\r'||d[l-1]=='\n'||d[l-1]=='\t')) l--;
    while((d[i]==' '||d[i]=='\r'||d[i]=='\n'||d[i]=='\t')&&i<l) i++;
    char *ret = (char *) malloc(l-i+1);
    ret[l-i]=0;
    memcpy(ret, d+i, l-i);
    return ret;
}


char *remove_http(char *url, int nofree) {
    int len = strlen(url);
    if(len>=7) {
	if((url[0]=='h'||url[0]=='H')&&
	    (url[1]=='t'||url[1]=='T')&&
	    (url[2]=='t'||url[2]=='T')&&
	    (url[3]=='p'||url[3]=='P')&&
	    url[4]==':'&&
	    url[5]=='/'&&
	    url[6]=='/') {
	    char *tmp = (char *)malloc(len-6);
	    memcpy(tmp, url+7, len-6);
	    if(!nofree) free(url);
	    return tmp;
	}
    }
    if(len>=3) {
	if(url[0]==':'&&
	    url[1]=='/'&&
	    url[2]=='/') {
	    char *tmp = (char *)malloc(len-2);
	    memcpy(tmp, url+3, len-2);
	    if(!nofree) free(url);
	    return tmp;
	}
    }
    free(url);
    return NULL;
}

void find_a(int dataBegin) {
    int i=dataBegin;
    while(i<len) {
	if(data[i]=='<') {
	    i++;
	    while(i<len&&data[i]==' ') i++;
	    if((data[i]=='a'||data[i]=='A')&&data[i+1]==' ') {
		i+=2;
		char *rel = NULL;
		char *href = NULL;
		char *adata = NULL;
		char *atext = NULL;
		char *dest = NULL;
		while(i<len&&data[i]!='>') {
		// rel
		    if( (data[i]=='r'||data[i]=='R')&&
		    (data[i+1]=='e'||data[i+1]=='E')&&
		    (data[i+2]=='l'||data[i+2]=='L')) {
			i+=3;
			while(i<len&&data[i]==' ') i++;
			if(data[i]=='=') {
			    i++;
			    while(i<len&&data[i]==' ') i++;
                            if(data[i]=='\'') {
				i++;
				int start = i;
			        while(i<len&&data[i]!='\''&&data[i]!='>') i++;
				rel = (char *)malloc(i-start+1);
                                if(i-start>128) {
                                    memcpy(rel, data+start, 128);
                                    rel[127]=0;
                                } else {
                                    memcpy(rel, data+start, i-start);
                                    rel[i-start]=0;
                                }
			    } else if(data[i]=='"') {
				i++;
				int start = i;
				while(i<len&&data[i]!='"'&&data[i]!='>') i++;
				rel = (char *)malloc(i-start+1);
                                if(i-start>128) {
                                    memcpy(rel, data+start, 128);
                                    rel[127]=0;
                                } else {
                                    memcpy(rel, data+start, i-start);
                                    rel[i-start]=0;
                                }
			    } else {
				int start = i;
				while(i<len&&data[i]!=' '&&data[i]!='>'&&data[i]!='<') i++;
				rel = (char *)malloc( i-start+1);
                                if(i-start>128) {
                                    memcpy(rel, data+start, 128);
                                    rel[127]=0;
                                } else {
                                    memcpy(rel, data+start, i-start);
                                    rel[i-start]=0;
                                }
			    }
			}
		    }
		// end rel
		// href
		    if( (data[i]=='h'||data[i]=='H')&&
		    (data[i+1]=='r'||data[i+1]=='R')&&
		    (data[i+2]=='e'||data[i+2]=='E')&&
		    (data[i+3]=='f'||data[i+3]=='F')) {
			i+=4;
			while(i<len&&data[i]==' ') i++;
			if(data[i]=='=') {
			    i++;
			    while(i<len&&data[i]==' ') i++;
			    if(data[i]=='\'') {
				i++;
				int start = i;
			        while(i<len&&data[i]!='\'') i++;
				href = (char *)malloc(i-start+1);
				memcpy(href, data+start, i-start);
				href[i-start]=0;
			    } else if(data[i]=='"') {
				i++;
				int start = i;
				while(i<len&&data[i]!='"') i++;
				href = (char *)malloc(i-start+1);
				memcpy(href, data+start, i-start);
				href[i-start]=0;
			    } else {
				int start = i;
				while(i<len&&data[i]!=' '&&data[i]!='>'&&data[i]!='<') i++;
				href = (char *)malloc( i-start+1);
				memcpy(href, data+start, i-start);
				href[i-start]=0;
			    }
			}
		    }
		// end href
		    i++;
		}
		i++;
		
		int st = i;
		int en = i;
		while(i<len) {
		    en = i;
		    if(data[i]=='<') {
			i++;
			while(i<len&&data[i]==' ') i++;
			if(data[i]=='/') {
			    i++;
			    while(i<len&&data[i]==' ') i++;
			    if(data[i]=='a'&&(data[i+1]==' '||data[i+1]=='>')) {
				while(data[i]!='>') i++;
				i++;
				goto end_a_found;
			    }
			}
		    }
		    i++;
		}
                // end a not found
                
                if(href)
		    free(href);
		if(rel)
		    free(rel);
		if(adata)
		    free(adata);
		if(atext)
		    free(atext);
		if(dest)
		    free(dest);
                return;
		
end_a_found:	adata = (char *)malloc(en-st+1);
		memcpy(adata, data+st, en-st);
		adata[en-st]=0;
		
		if(charset) {
		    iconv_t conv_desc = iconv_open ("UTF-8", charset);
		    if(conv_desc != (iconv_t)-1) {
			char *adata_tmp = adata;
			size_t adata_len = strlen(adata);
			size_t utf8len = 2*adata_len;
			char *utf8 = (char *) malloc(utf8len+1);
			bzero(utf8, utf8len+1);
			char *u8 = utf8;
			size_t iconv_value = iconv (conv_desc, & adata_tmp, & adata_len, & utf8, & utf8len);
			if (iconv_value != (size_t) -1) {
			    char *tmp = adata;
			    adata = u8;
			    free(tmp);
			} else {
			    free(u8);
			}
			iconv_close (conv_desc);
		    }
		}
		
		char *no_tags = clean_tags(adata, strlen(adata));
		if(no_tags) {
		    atext = trim_spaces(no_tags, strlen(no_tags));
		    free(no_tags);
		}
		if(adata) {
		    char *tmp = adata;
		    adata = trim_spaces(adata, strlen(adata));
		    free(tmp);
		}
		
		if(href) {
		    int t = 0;
		    while(href[t]!=0) {
			if(href[t]=='#') {
			    href[t]=0;
			    break;
			}
			t++;
		    }
		    char *tmp = href;
		    href = url_encode(tmp);
		    free(tmp);
		}
		
		if(href) {
		    UriParserStateA state;
		    //UriUriA uri;
		    UriUriA absoluteDest;
		    UriUriA relativeSource;
		    UriUriA absoluteBase;
		    state.uri = &absoluteBase;
		    if (uriParseUriA(&state, global_url) != URI_SUCCESS) {
			uriFreeUriMembersA(&absoluteBase);
		    } else {
			state.uri = &relativeSource;
			if (uriParseUriA(&state, href) != URI_SUCCESS) {
			    uriFreeUriMembersA(&relativeSource);
			} else {
			    if (uriAddBaseUriA(&absoluteDest, &relativeSource, &absoluteBase) != URI_SUCCESS) {
				uriFreeUriMembersA(&absoluteDest);
			    } else {
				if (uriNormalizeSyntaxA(&absoluteDest) != URI_SUCCESS) {
				    uriFreeUriMembersA(&absoluteDest);
				} else {
				//    char * uriString;
				    int charsRequired;
				    if (uriToStringCharsRequiredA(&absoluteDest, &charsRequired) != URI_SUCCESS) {
				    }
				    charsRequired++;
				    dest = malloc(charsRequired * sizeof(char));
				    if (uriToStringA(dest, &absoluteDest, charsRequired, NULL) != URI_SUCCESS) {
					if(dest)
					    free(dest);
					dest=NULL;
				    } else {
				    }
				    uriFreeUriMembersA(&absoluteDest);
				}
			    }
			    uriFreeUriMembersA(&relativeSource);
			}
			uriFreeUriMembersA(&absoluteBase);
		    }
		}

		if(dest) {
		    char *domain = get_domain(dest);
		    
		    if(href)
			href=escape(href);
		    if(rel)
			rel=escape(rel);
		    if(adata)
			adata=escape(adata);
		    if(atext)
			atext=escape(atext);
		    if(dest)
			dest=escape(dest);
		    if(domain)
			domain=escape(domain);
		
		    int t = strlen(dest);
		    int add = 1;
		    while(t>0) {
			if(dest[t]=='?') {
			    add=0;
			    //printf("found ? [%s]\n", dest);
			    break;
			}
			t--;
		    }
		    if(domain&&dest&&strcasecmp(global_domain, domain)&&strcasecmp(global_url, dest)) {
                        enum protocol destProto = getProtocol(dest);
			if(dest&&adata&&dest&&strlen(dest)<=1000&&strlen(adata)<=2000&&strlen(dest)<=1000) {
			    char *tld = get_tld(domain);
			    IMG *img = get_img(adata);
			    char *query = (char *)malloc(strlen(dest)+strlen(adata)*2+1024*8);
                            
                            if(destProto == http || destProto == https) {
                                sprintf(query, "%c%s\t%s\t%s\t%s\t%d\t%d\t%s\t%s\t%s\t%s\t%d\t%s%c", 0x01, tld, downloadTime, domain, global_domain, srcProto, destProto, dest+strlen(domain)+(destProto == http ? 7 : 8), global_url+strlen(global_domain)+(srcProto == http ? 7 : 8), atext?atext:"", rel?rel:"", img->img, img->alt?img->alt:"", 0x00);
                                n = write(sockfd,query,strlen(query)+1);
                                if (n < 0)
                                {
                                   perror("ERROR writing to socket");
                                   exit(1);
                                }
                                sprintf(query, "%c%s%c", 0x02, dest, 0x00);
                                n = write(sockfd,query,strlen(query)+1);
                                if (n < 0)
                                {
                                   perror("ERROR writing to socket");
                                   exit(1);
                                }
                            }
			    free(query);
			    if(img->alt) free(img->alt);
			    free(img);
			    free(tld);
			}
		    } else if(add&&dest&&strlen(dest)<=1000) {
                        enum protocol destProto = getProtocol(dest);
                        if(destProto == http || destProto == https) {
                            char *query = (char *) malloc(2048);
                            sprintf(query, "%c%s%c", 0x02, dest, 0x00);
                            n = write(sockfd,query,strlen(query)+1);
                            free(query);
                            if (n < 0)
                            {
                               perror("ERROR writing to socket");
                               exit(1);
                            }
                        }
		    }
		    free(domain);
		}
		if(href)
		    free(href);
		if(rel)
		    free(rel);
		if(adata)
		    free(adata);
		if(atext)
		    free(atext);
		if(dest)
		    free(dest);
	    }
	}
	i++;
    }
}

int findData() {
    int i = 4;
    while(i<len) {
        if(data[i-4]=='\r' && data[i-3]=='\n' && data[i-2]=='\r' && data[i-1]=='\n') {
            break;
        }
        i++;
    }
    if(i==len)
        return -1;
    else
        return i;
}

char *getUrl() {
    int i = 1;
    while(i<len) {
        if(data[i-1]=='\r' && data[i]=='\n') {
            break;
        }
        i++;
    }
    if(i>3) {
        char *ret = (char *)malloc(i);
        memcpy(ret, data, i-1);
        ret[i-1]=0x00;
        return ret;
    }
    return NULL;
}

char *getTime() {
    int i = 1;
    while(i<len) {
        if(data[i-1]=='\r' && data[i]=='\n') {
            break;
        }
        i++;
    }
    i++;
    int j = i;
    while(i<len) {
        if(data[i-1]=='\r' && data[i]=='\n') {
            break;
        }
        i++;
    }
    if(i>3) {
        char *ret = (char *)malloc(i-j);
        memcpy(ret, data+j, i-j-1);
        ret[i-j-1]=0x00;
        return ret;
    }
    return NULL;
}

void readFile(char *file) {
    FILE *f = fopen(file, "r");
    if(flock(fileno(f), LOCK_EX) != 0) {
        return;
    }
    
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if(len>0) {
        data = malloc(len + 1);
    }
    fread(data, len, 1, f);
    fclose(f);
    
    if(len>0) {
        int dataStart = findData();
        if(dataStart>0) {
            char *url = getUrl();
            if(url!=NULL) {
                global_url = url;
                global_domain = get_domain(url);
                downloadTime = getTime();
                if(downloadTime) {
		    if(charset) {
			free(charset);
		    }
                    charset = NULL;
                    find_meta(dataStart);
                    srcProto = getProtocol(global_url);
                    find_a(dataStart);

                    free(downloadTime);
                }
                free(url);
                free(global_domain);
            }
        }
        free(data);
    }
    
    unlink(file);
}

int main(int argc, char *argv[]) {
    DIR *dir;
    char tmp[1024];
    struct dirent *ent;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
       printf("ERROR opening socket");
       exit(1);
    }
    
    memset(&serv_addr, 0x00, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons( 32302 );
    
    if (connect(sockfd,(struct sockaddr *)&serv_addr , sizeof(serv_addr)) < 0)
    {
       printf("ERROR connecting");
       exit(1);
    }
    
    sprintf(tmp, "tVoGXdKarqza4TKEoJwugHTBMGeR5DHefm6MdaDfBHH8Mrnw5LCH9tpyA");
    
    n = write(sockfd,tmp,strlen(tmp));
    if (n < 0)
    {
       perror("ERROR writing to socket");
       exit(1);
    }
    
    n = read(sockfd, tmp, 1024);   
    if (n <= 0)
    {
       perror("ERROR reading from socket");
       exit(1);
    }
    
    
    if ((dir = opendir ("dl")) != NULL) {
        while(1) {
            while ((ent = readdir (dir)) != NULL) {
                if((ent->d_name[0]=='.' && ent->d_name[1]==0x00) || (ent->d_name[0]=='.' && ent->d_name[1]=='.' && ent->d_name[2]==0x00)) {
                    continue;
                }
                printf ("%s\n", ent->d_name);
                sprintf(tmp, "dl/%s", ent->d_name);
                readFile(tmp);
            }
            printf("finished dir, rewind\n");
            sleep(5);
            rewinddir(dir);
        }
        closedir (dir);
    }
    
    return 0;
}
