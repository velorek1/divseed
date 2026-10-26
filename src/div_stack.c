/* Extracted from DIV Games Studio, commit 77556912335d0a1b0efea855d4eb2490cf382128.
 * GPL-3.0. See NOTICE for adaptations. */
#include "ui.h"
struct tventana ventana[max_windows], vaux[4];
int ivaux=0;
void copy(int a,int b) {

  if(a==b)
	return;
	
  if (a==-1) {
    memcpy(&vaux[ivaux].tipo,&ventana[b].tipo,sizeof(struct tventana));
  } else if (b==-1) {
    memcpy(&ventana[a].tipo,&vaux[ivaux].tipo,sizeof(struct tventana));
  } else {
    memcpy(&ventana[a].tipo,&ventana[b].tipo,sizeof(struct tventana));
  }
  
}

void move(int a,int b) {
  if (a<b) {
    copy(-1,b);
    memmove(&ventana[a+1].tipo,&ventana[a].tipo,sizeof(struct tventana)*(b-a));
    copy(a,-1);
  } else if (b<a) {
    copy(-1,b);
    memmove(&ventana[b].tipo,&ventana[b+1].tipo,sizeof(struct tventana)*(a-b));
    copy(a,-1);
  }
}

void divdelete(int a) {
  memmove(&ventana[a].tipo,&ventana[a+1].tipo,sizeof(struct tventana)*(max_windows-1-a));
  ventana[max_windows-1].tipo=0;
}

void addwindow(void) {
  memmove(&ventana[1].tipo,&v.tipo,sizeof(struct tventana)*(max_windows-1));
}
