/* Small lexical C highlighter, GPL-3.0. No parsing or source transformation. */
#include "syntax.h"
#include <string.h>
static int letter(unsigned char c) {return (c>='a' && c<='z') || (c>='A' && c<='Z') || c=='_' || c>=128;}
static int digit(unsigned char c) {return c>='0' && c<='9';}
static int keyword(const char *s,size_t n) {
 static const char *words[]={"auto","break","case","char","const","continue","default","do","double","else","enum","extern","float","for","goto","if","inline","int","long","register","restrict","return","short","signed","sizeof","static","struct","switch","typedef","union","unsigned","void","volatile","while","_Alignas","_Alignof","_Atomic","_Bool","_Complex","_Generic","_Imaginary","_Noreturn","_Static_assert","_Thread_local","alignas","alignof","bool","constexpr","false","nullptr","static_assert","thread_local","true","typeof","typeof_unqual","_BitInt"};
 for(size_t i=0;i<sizeof words/sizeof words[0];i++) if(strlen(words[i])==n && !memcmp(s,words[i],n)) return 1;
 return 0;
}
int c_filename(const char *path) {if(!path)return 0;const char *ext=strrchr(path,'.');return ext && (!strcmp(ext,".c") || !strcmp(ext,".h"));}
void c_highlight(const char *s,size_t n,unsigned char *out) {
 size_t i=0;int bol=1,pp=0,header=0;
 while(i<n) {
  size_t start=i;unsigned char color=pp?C_PREPROCESSOR:C_TEXT;
  if(s[i]=='\\' && i+1<n && s[i+1]=='\n') {i+=2;} /* logical line continuation */
  else if(s[i]=='\n') {i++;bol=1;pp=0;header=0;}
  else if(s[i]==' ' || s[i]=='\t') {i++;}
  else if(i+1<n && s[i]=='/' && s[i+1]=='*') {
   i+=2;while(i<n && !(s[i-1]=='*' && s[i]=='/')) {if(s[i]=='\n'){bol=1;pp=0;header=0;}i++;}if(i<n)i++;color=C_COMMENT;
  } else if(i+1<n && s[i]=='/' && s[i+1]=='/') {
   i+=2;while(i<n && s[i]!='\n') {if(s[i]=='\\' && i+1<n && s[i+1]=='\n')i+=2;else i++;}color=C_COMMENT;
  } else if(s[i]=='"' || s[i]=='\'' || (header && s[i]=='<')) {
   char end=s[i]=='<'?'>':s[i];i++;
   while(i<n && s[i]!='\n') {if(s[i]=='\\' && i+1<n){i+=2;continue;}if(s[i++]==end)break;}
   color=C_STRING;bol=0;
  } else if(bol && s[i]=='#') {pp=1;color=C_PREPROCESSOR;bol=0;i++;}
  else if(letter((unsigned char)s[i])) {
   i++;while(i<n && (letter((unsigned char)s[i]) || digit((unsigned char)s[i])))i++;
   if(pp && i-start==7 && !memcmp(s+start,"include",7))header=1;
   if(!pp && keyword(s+start,i-start))color=C_KEYWORD;
   bol=0;
  } else if(digit((unsigned char)s[i]) || (s[i]=='.' && i+1<n && digit((unsigned char)s[i+1]))) {
   i++;while(i<n) {char c=s[i],prev=s[i-1];if(letter((unsigned char)c)||digit((unsigned char)c)||c=='.'||((c=='+'||c=='-')&&(prev=='e'||prev=='E'||prev=='p'||prev=='P')))i++;else break;}color=C_NUMBER;bol=0;
  } else {i++;bol=0;}
  memset(out+start,color,i-start);
 }
}
