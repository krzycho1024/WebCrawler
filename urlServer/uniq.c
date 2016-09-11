#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdint.h>

#define FLAG_END 0x01
#define FLAG_USED 0x02


static uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t crc32(uint32_t crc, const void *buf, size_t size)
{
	const uint8_t *p;

	p = buf;
	crc = crc ^ ~0U;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}


typedef struct _tree {
    char ch;
    char flag;
    struct _tree *up;
    struct _tree *left;
    struct _tree *right;
    struct _tree *prevRight;
} tree;

tree *root;
unsigned int memory;


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
    while(i<len&&s[i]!='/'&&s[i]!=':'&&s[i]!='?') i++;
    if(i>j) {
        int n, dotCount = 0;
        for(n=j;n<i;n++) {
            if(s[n]=='.') {
                dotCount++;
            }
        }
        if(dotCount>1) {
            int dc = 0;
            for(n=i;n>j;n--) {
                if(s[n]=='.') {
                    dc++;
                }
                if(dc==2) {
                    if(i-n>7) {
                        char *tmp = (char *) malloc(i-n);
                        memcpy(tmp, s+n+1, i-n-1);
                        tmp[i-n-1]=0;
                        return tmp;
                    }
                    break;
                }
            }
        }
	char *tmp = (char *) malloc(i-j+1);
	memcpy(tmp, s+j, i-j);
	tmp[i-j]=0;
	return tmp;
    }
    return NULL;
}

typedef struct _URL {
    struct _URL *next;
    char *url;
    int len;
} URL;

typedef struct _DOMAIN {
    struct _DOMAIN *next;
    struct _URL *url;
    struct _URL *last_url;
    char *domain;
    int len;
    uint32_t crc;
} DOMAIN;

DOMAIN *domains;

FILE *outfile;
FILE *flushfile;

void force_flush() {
    int found = 1;
    int count_domain = 0;
    while(found) {
        DOMAIN *d = domains;
        DOMAIN *prev = NULL;
        found = 0;
        count_domain = 0;
        while(d) {
            if(d->url!=NULL) {
                count_domain++;
                found = 1;
                fprintf(flushfile, "%s\n", d->url->url);
                URL *tmp = d->url;
                d->url = d->url->next;
                free(tmp->url);
                free(tmp);
            }
            if(d->url==NULL) {
                free(d->domain);
                if(prev!=NULL) {
                    prev->next=d->next;
                    free(d);
                    d=prev->next;
                } else {
                    domains=d->next;
                    d=domains;
                }
            } else {
                prev = d;
                d=d->next;
            }
        }
    }    
}

void flush_domains(int force) {
    int found = 1;
    int count_domain = 0;
    while(found) {
        DOMAIN *d = domains;
        DOMAIN *prev = NULL;
        found = 0;
        count_domain = 0;
        while(d) {
            if(d->url!=NULL) {
                count_domain++;
                found = 1;
                fprintf(outfile, "%s\n", d->url->url);
                URL *tmp = d->url;
                d->url = d->url->next;
                free(tmp->url);
                free(tmp);
            }
            if(d->url==NULL) {
                free(d->domain);
                if(prev!=NULL) {
                    prev->next=d->next;
                    free(d);
                    d=prev->next;
                } else {
                    domains=d->next;
                    d=domains;
                }
            } else {
                prev = d;
                d=d->next;
            }
        }
	if(!force && count_domain<12000) {
            force_flush();
            break;
        } else if(!force && count_domain<25000) {
            break;
        }
    }
}

void add_domain_url(char *domain, char *url) {
    if (!domains) {
        domains = (DOMAIN *) malloc(sizeof (DOMAIN));
        domains->next = NULL;
        domains->len = strlen(domain);
        domains->crc = crc32(0, domain, domains->len);
        domains->domain = (char *) malloc(domains->len+1);
        memcpy(domains->domain, domain, domains->len+1);
        domains->url = (URL *) malloc(sizeof (URL));
	domains->last_url = domains->url;
        domains->url->next = NULL;
        domains->url->len = strlen(url);
        domains->url->url = (char *) malloc(domains->url->len+1);
        memcpy(domains->url->url, url, domains->url->len+1);
        return;
    } else {
        DOMAIN *d = domains;
        int len = strlen(domain);
        uint32_t crc = crc32(0, domain, len);
        for (;;) {
            if (len == d->len && d->crc == crc && !strncmp(domain, d->domain, len)) {
                //URL *u = d->url;
                //int ulen = strlen(url);
                //for (;;) {
                    //if (u->len == ulen && !strncmp(url, u->url, ulen)) {
                        //			printf("existing url\n");
                        //return;
                    //}
                    //if (u->next) {
                        //u = u->next;
                    //} else {
                    //    u->next = (URL *) malloc(sizeof (URL));
                    //    u->next->next = NULL;
                    ///    //u->next->url = url;
                    //    u->next->len = ulen;
                    //    u->next->url = (char *) malloc(u->next->len+1);
                    //    memcpy(u->next->url, url, u->next->len+1);
                        //			printf("new url\n");
                    //    return;
                    //}
                //}
		d->last_url->next = (URL *) malloc(sizeof (URL));
		d->last_url->next->next = NULL;
		d->last_url->next->len = strlen(url);
		d->last_url->next->url = (char *) malloc(d->last_url->next->len+1);
		memcpy(d->last_url->next->url, url, d->last_url->next->len+1);
		d->last_url = d->last_url->next;
                return;
            }
            if (d->next) {
                d = d->next;
            } else {
                d->next = (DOMAIN *) malloc(sizeof (DOMAIN));
                d->next->next = NULL;
                //d->next->domain = domain;
                d->next->domain = (char *) malloc(len+1);
                memcpy(d->next->domain, domain, len+1);
                d->next->crc = crc;
                d->next->len = len;
                d->next->url = (URL *) malloc(sizeof (URL));
		d->next->last_url = d->next->url;
                d->next->url->next = NULL;
                //d->next->url->url = url;
                d->next->url->len = strlen(url);
                d->next->url->url = (char *) malloc(d->next->url->len+1);
                memcpy(d->next->url->url, url, d->next->url->len+1);
                //		printf("new domain\n");
                return;
            }
        }
    }
}

void add(char *add) {
    int i = 0;
    int len = strlen(add);

    tree *tmp = root;
    tree *prev = tmp;
    for (i = 0; i < len; i++) {
        if (tmp) {
            int found = 0;
            while (tmp && !found) {
                prev = tmp;
                if(tmp->ch == add[i]) {
                    tmp = tmp->left;
                    found = 1;
                } else {
                    tmp = tmp->right;
                }
            }
            if(!found) {
                memory += sizeof (tree);
                prev->right = (tree *) malloc(sizeof (tree));
                prev->right->up = prev->up;
                prev->right->left = NULL;
                prev->right->right = NULL;
                prev->right->flag = 0x00;
                prev->right->ch = add[i];
                prev->right->prevRight = prev;
                prev = prev->right;
                tmp = NULL;
            }
        } else {
            memory += sizeof (tree);
            tmp = (tree *) malloc(sizeof (tree));
            tmp->ch = add[i];
            tmp->up = prev;
            tmp->left = NULL;
            tmp->right = NULL;
            tmp->prevRight = NULL;
            tmp->flag = 0x00;
            if(prev==NULL) {
                root = tmp;
            } else {
                prev->left = tmp;
            }
            prev = tmp;
            tmp = NULL;
        }
    }
    if(prev) {
        prev->flag = FLAG_END;
    }
}

void removeNode(tree *node) {
    if(!node) {
        return;
    }
    if(node->left!=NULL) {
        return;
    }
    if(node->flag & FLAG_END && !(node->flag & FLAG_USED)) {
        return;
    }
    if(node->right!=NULL) {
        node->right->prevRight = node->prevRight;
        if(node->prevRight) {
            node->prevRight->right = node->right;
        } else if(node->up && node->up->left == node) {
            node->up->left = node->right;
        }
        if(node==root) {
            root = node->right;
        }
        free(node);
    } else {
	if(node->up->left == node) {
            node->up->left = NULL;
        }
        if(node->prevRight) {
            node->prevRight->right = NULL;
        }
        tree *tmp = node->up;
        free(node);
        removeNode(tmp);
    }
}

void match(char *add) {
    int i = 0;
    int len = strlen(add);

    tree *tmp = root;
    tree *prev = tmp;
    for (i = 0; i < len; i++) {
        if (tmp) {
            int found = 0;
            while (tmp && !found) {
                prev = tmp;
                if(tmp->ch == add[i]) {
                    tmp = tmp->left;
                    found = 1;
                } else {
                    tmp = tmp->right;
                }
            }
            if(!found) {
                return;
            }
        } else {
            return;
        }
    }
    if(prev) {
        prev->flag |= FLAG_USED;
        removeNode(prev);
    }
}

char *getStringUp(tree *t) {
    char ret[1050];
    int i = 0, j = 0;
    ret[i++] = t->ch;
    while (t->up) {
        t = t->up;
        ret[i++] = t->ch;
    }
    char *ret_tmp = (char *) malloc(i + 1);
    i--;
    while (i >= 0) {
        ret_tmp[j++] = ret[i--];
    }
    ret_tmp[j] = 0x00;
    return ret_tmp;
}

int findctr = 0;

void find_free(tree *t) {
    if (!t) {
        return;
    }
    if (t->flag & FLAG_END) {
        if(!(t->flag & FLAG_USED)) {
            char *s = getStringUp(t);
            //printf("%s\n", s);
            char *domain = get_domain(s);
            if (domain) {
                int domainlen = strlen(domain);
                findctr++;
                if(findctr%100000 == 0) {
                    printf("findctr: %d\n", findctr);
                    flush_domains(0);
                }
                add_domain_url(domain, s);
                //fprintf(outfile, "%s\t%s\n", domain, s);
                free(domain);
            }
            free(s);
        }
    }
    if (t->left) {
        find_free(t->left);
    }
    if (t->right) {
        find_free(t->right);
    }
    free(t);
    memory -= sizeof (tree);
}
/*
int main(int argc, char** argv) {
    add("ab");
    add("ac");
    add("ad");
    match("ab");
    find_free(root);
    return (EXIT_SUCCESS);
}
*/

int main(int argc, char** argv) {
    if (argc != 3) {
        return 0;
    }
        
    DIR *dir;
    struct dirent *entry;

    int ctr = 0;
            FILE *f = fopen(argv[1], "r");
            if (!f) {
                return 0;
            }
            char line[4096];
            printf("reading: %s\n", argv[1]);
            while (!feof(f)) {
                if(!fgets(line, 4096, f)) {
                    continue;
                }
                int i = 0;
                while (line[i] != 0x00 && line[i] != '\r' && line[i] != '\n') {
                    i++;
                }
                    line[i] = 0x00;
                        add(line);
                        ctr++;
                if (ctr % 100000 == 0) {
                    printf("read ctr: %d\n", ctr);
                }
            }
            fclose(f);
    
    int checkctr = 0;
    if ((dir = opendir(argv[2])) == NULL) {
        printf("opendir() error");
    } else {
        while ((entry = readdir(dir)) != NULL) {
            if ((entry->d_name[0] != 0x00 && entry->d_name[0] == '.' && entry->d_name[1] == 0x00) ||
                    (entry->d_name[0] != 0x00 && entry->d_name[1] != 0x00 && entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == 0x00)) {
                continue;
            }
            char fname[1024];
            sprintf(fname, "%s/%s", argv[2], entry->d_name);
            FILE *f = fopen(fname, "r");
            if (!f) {
                continue;
            }
            char line[4096];
            printf("reading: %s\n", fname);
            while (!feof(f)) {
                if(!fgets(line, 4096, f)) {
                    continue;
                }
                int i = 0;
                while (line[i] != 0x00 && line[i] != '\r' && line[i] != '\n') {
                    i++;
                }
                    line[i] = 0x00;
                    match(line);
                    checkctr++;
                if (checkctr % 100000 == 0) {
                    printf("ctr: %d\n", checkctr);
                }
            }
            fclose(f);
        }
        closedir(dir);
    }
    
    if ((dir = opendir("out")) == NULL) {
        printf("opendir() error");
    } else {
        while ((entry = readdir(dir)) != NULL) {
            if ((entry->d_name[0] != 0x00 && entry->d_name[0] == '.' && entry->d_name[1] == 0x00) ||
                    (entry->d_name[0] != 0x00 && entry->d_name[1] != 0x00 && entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == 0x00)) {
                continue;
            }
            char fname[1024];
            sprintf(fname, "%s/%s", "out", entry->d_name);
            FILE *f = fopen(fname, "r");
            if (!f) {
                continue;
            }
            char line[4096];
            printf("reading: %s\n", fname);
            while (!feof(f)) {
                if(!fgets(line, 4096, f)) {
                    continue;
                }
                int i = 0;
                while (line[i] != 0x00 && line[i] != '\r' && line[i] != '\n') {
                    i++;
                }
                    line[i] = 0x00;
                        match(line);
                        checkctr++;
                if (checkctr % 100000 == 0) {
                    printf("ctr: %d\n", checkctr);
                }
            }
            fclose(f);
            //break;
        }
        closedir(dir);
    }

    char fname[1024];
    sprintf(fname, "out/%d.flushqueue", time(NULL));
    flushfile = fopen(fname, "w");
    
    sprintf(fname, "out/%d.queue", time(NULL));
    outfile = fopen(fname, "w");
    printf("find\n");
    find_free(root);
    flush_domains(0);
    fclose(outfile);
    printf("flush rest\n");
    sprintf(fname, "out/%d.restqueue", time(NULL));
    outfile = fopen(fname, "w");
    flush_domains(1);
    fclose(outfile);
    fclose(flushfile);
    return (EXIT_SUCCESS);
}

