#include "utf8.h"
uint32_t utf8_decode(const char *s,size_t n,size_t *p) {
 if(*p>=n) return 0;
 size_t start=*p;unsigned char c=(unsigned char)s[(*p)++];
 if(c<128) return c;
 int extra;uint32_t value,min;
 if(c>=0xc2 && c<=0xdf) {extra=1;value=c&31;min=0x80;}
 else if(c>=0xe0 && c<=0xef) {extra=2;value=c&15;min=0x800;}
 else if(c>=0xf0 && c<=0xf4) {extra=3;value=c&7;min=0x10000;}
 else return 0xffffffff;
 if(n-*p<(size_t)extra) return 0xffffffff;
 for(int i=0;i<extra;i++) {c=(unsigned char)s[*p];if((c&0xc0)!=0x80) {*p=start+1;return 0xffffffff;}(*p)++;value=(value<<6)|(c&63);}
 if(value<min || value>0x10ffff || (value>=0xd800 && value<=0xdfff)) return 0xffffffff;
 return value;
}
size_t utf8_next(const char *s,size_t n,size_t p) {utf8_decode(s,n,&p);return p;}
size_t utf8_prev(const char *s,size_t p) {if(p) {p--;while(p && ((unsigned char)s[p]&0xc0)==0x80) p--;}return p;}
size_t utf8_count(const char *s,size_t n) {size_t count=0;for(size_t p=0;p<n;count++) p=utf8_next(s,n,p);return count;}
size_t utf8_offset(const char *s,size_t n,size_t column) {size_t p=0;while(column-- && p<n) p=utf8_next(s,n,p);return p;}
int utf8_valid(const char *s,size_t n,int multiline) {
 for(size_t p=0;p<n;) {uint32_t c=utf8_decode(s,n,&p);
  if(c==0xffffffff || (c<32 && c!=9 && !(multiline && (c==10 || c==13))) || (c>=127 && c<=159)) return 0;
 }
 return 1;
}
