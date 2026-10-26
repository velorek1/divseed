/* Extracted from DIV Games Studio, commit 77556912335d0a1b0efea855d4eb2490cf382128.
 * GPL-3.0. See NOTICE for adaptations. */
#include "ui.h"

int big=0;
byte *text_font;
sscar glyphs[256];
char cbs[65],cbn,cgs[65],cgn,crs[65],crn;

char gradient[]={
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,
  0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,
  0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,
  0,1,0,1,1,0,0,0,0,1,0,1,0,0,0,0,
  0,1,0,1,1,0,0,0,0,1,0,1,0,0,1,0,
  0,1,0,1,1,0,0,0,0,1,0,1,1,0,1,0,
  0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,
  1,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,
  1,1,0,1,1,0,1,0,0,1,1,1,1,0,1,0,
  1,1,0,1,1,0,1,0,1,1,1,1,1,0,1,0,
  1,1,1,1,1,0,1,0,1,1,1,1,1,0,1,0,
  1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,
  1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,
  1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};


void wbox(byte*copia,int an_copia,int al_copia,byte c,int x,int y,int an,int al) {
  wbox_in_box(copia,an_copia,an_copia,al_copia,c,x,y,an,al);
}

void wbox_in_box(byte*copia,int an_real_copia,int an_copia,int al_copia,byte c,int x,int y,int an,int al) {

	byte *p;
	

	
	
/*	for (vn=0;vn<max_windows;vn++) {
		if(ventana[vn].ptr==copia) {
			tsurface = ventana[vn].surfaceptr;
			break;
		}
	}
*/
	// render ttf to copia_suface;	
	// ofset by the window x/y

	if (big) {
		an_real_copia*=2; an_copia*=2; al_copia*=2;
		x*=2; y*=2; an*=2; al*=2;
	}

	


  if (y<0) {al+=y; y=0;}
  if (x<0) {an+=x; x=0;}
  if (y+al>al_copia) al=al_copia-y;
  if (x+an>an_copia) an=an_copia-x;

  if (an>0 && al>0) {

    
			
	
	


    p=copia+y*an_real_copia+x;
    do {
      memset(p,c,an);
      p+=an_real_copia;
    } while (--al);
    



    
  }
}

void wgra(byte*copia,int an_copia,int al_copia,byte color,int x,int y,int an,int al) {
  int xx,yy,c,m;
  char * cs, cn;

  if (big) {
    an_copia*=2; al_copia*=2;
    x*=2; y*=2; an*=2; al*=2;
  }

  if (y<0) {al+=y; y=0;}
  if (x<0) {an+=x; x=0;}
  if (y+al>al_copia) al=al_copia-y;
  if (x+an>an_copia) an=an_copia-x;

  if (color==c_r_low) {
    cn=crn; cs=crs;
  } else if (color==c1) {
    cn=cgn; cs=cgs;
  } else {
    cn=cbn; cs=cbs;
  }

  if (an>0 && al>0) {
    xx=0;
    do {
      c=(cn-1)*xx*16/an; m=c%16; c/=16;
      yy=0;
      do {
        if (gradient[m*16+(xx%4)+(yy%4)*4]) {
          copia[(y+yy)*an_copia+(x+xx)]=cs[c+1];
        } else {
          copia[(y+yy)*an_copia+(x+xx)]=cs[c];
        }
      } while (++yy<al);
    } while (++xx<an);
  }
}

void wrectangulo(byte*copia,int an_copia,int al_copia,byte c,int x,int y,int an,int al) {
  wbox(copia,an_copia,al_copia,c,x,y,an,1);
  wbox(copia,an_copia,al_copia,c,x,y+al-1,an,1);
  wbox(copia,an_copia,al_copia,c,x,y+1,1,al-2);
  wbox(copia,an_copia,al_copia,c,x+an-1,y+1,1,al-2);
}

int text_len(byte * ptr) {

	int an;

	/*  struct {
	byte an;
	word dir;
	} * car;
	*/


	sscar *car;

	car=glyphs; an=0;
	while (*ptr) {
		an+=car[*ptr].an; 
		ptr++; 
	}


	if (big) 
		an/=2;

	if (an) 
		an--; 


//	printf("%d\n",an);
	
	return(an);

}

void wwrite(byte*copia,int an_copia,int al_copia,
            int x,int y,int centro,byte * ptr,byte c) {
				
//	printf("wwrite: %x %d %d %d %d %d %s %d\n",copia,an_copia, al_copia, x,y,centro, ptr, x);
	
	wwrite_in_box(copia,an_copia,an_copia,al_copia,x,y,centro,ptr,c);
}

void wwrite_in_box(byte*copia,int an_real_copia,int an_copia,int al_copia,
            int x,int y,int centro,byte * ptr,byte c) {

//	printf("Writing in box: %s\n",ptr);
	

	
	
	int an,al,boton,multi;

	

	/*
	struct {
	byte an;
	word dir;
	} * car;
	*/

	byte * font;

	sscar *car;

	if (centro>=10) { 
		centro-=10; 
		multi=1; 
	} else 
		multi=0;

	if (*ptr=='\xd') { 
		boton=1; 
		ptr++; 
	} else boton=0;

	car=glyphs;

	if (big&&!multi) {
		an=text_len(ptr); al=7;
	} else {
		font=ptr; 
		an=0; 
		
		while (*font) { 
			an+=car[*font].an; 
			font++; 
		}
		
		al=*text_font; if (an) an--;
	}

	font=text_font+1025;

	switch (centro) {
		case 0: 
		break;
		
		case 1: 
			x=x-(an>>1); 
		break;
		
		case 2: 
			x=x-an+1; 
		break;
		
		case 3: 
			y=y-(al>>1); 
		break;
		
		case 4: 
			x=x-(an>>1); 
			y=y-(al>>1); 
		break;
		
		case 5: 
			x=x-an+1; 
			y=y-(al>>1); 
		break;
		
		case 6: 
			y=y-al+1; 
		break;
		
		case 7: 
			x=x-(an>>1); 
			y=y-al+1; 
		break;
		
		case 8: 
			x=x-an+1; 
			y=y-al+1; 
		break;
	}

	if (boton) { 

		if (c!=c0) {
			wbox(copia,an_real_copia,al_copia,c2,x-2,y-2,an+4,al+4);
			wrectangulo(copia,an_real_copia,al_copia,c0,x-3,y-3,an+6,al+6);
			wrectangulo(copia,an_real_copia,al_copia,c3,x-2,y-2,an+3,1);
			wrectangulo(copia,an_real_copia,al_copia,c3,x-2,y-2,1,al+3);
			wrectangulo(copia,an_real_copia,al_copia,c4,x-2,y-2,1,1);
			wrectangulo(copia,an_real_copia,al_copia,c1,x-1,y+al+1,an+3,1);
			wrectangulo(copia,an_real_copia,al_copia,c1,x+an+1,y-1,1,al+3);
			if (big) {
				*(copia+(2*y-3)*an_real_copia*2+2*x-4)=c3;
				*(copia+(2*y-4)*an_real_copia*2+2*x-3)=c3;
				*(copia+(2*y-4)*an_real_copia*2+2*(x+an)+2)=c3;
				*(copia+(2*y-3)*an_real_copia*2+2*(x+an)+3)=c1;
				*(copia+(2*(y+al)+2)*an_real_copia*2+2*x-4)=c3;
				*(copia+(2*(y+al)+3)*an_real_copia*2+2*x-3)=c1;
			}
		} else {
			wbox(copia,an_real_copia,al_copia,c1,x-2,y-2,an+4,al+4);
			wrectangulo(copia,an_real_copia,al_copia,c0,x-3,y-3,an+6,al+6);
			wrectangulo(copia,an_real_copia,al_copia,c0,x-2,y-2,an+3,1);
			wrectangulo(copia,an_real_copia,al_copia,c0,x-2,y-2,1,al+3);
			wrectangulo(copia,an_real_copia,al_copia,c2,x-1,y+al+1,an+3,1);
			wrectangulo(copia,an_real_copia,al_copia,c2,x+an+1,y-1,1,al+3);
			wrectangulo(copia,an_real_copia,al_copia,c3,x+an+1,y+al+1,1,1);

			if (big) {
				*(copia+(2*(y+al)+3)*an_real_copia*2+2*(x+an)+2)=c2;
				*(copia+(2*(y+al)+2)*an_real_copia*2+2*(x+an)+3)=c2;
				*(copia+(2*y-4)*an_real_copia*2+2*(x+an)+2)=c0;
				*(copia+(2*y-3)*an_real_copia*2+2*(x+an)+3)=c2;
				*(copia+(2*(y+al)+2)*an_real_copia*2+2*x-4)=c0;
				*(copia+(2*(y+al)+3)*an_real_copia*2+2*x-3)=c2;
			}
		}
	}

	if (big&&!multi) {
		an_real_copia*=2; an_copia*=2; al_copia*=2;
		x*=2; y*=2; an*=2; al*=2;
	}





	if (y<al_copia && y+al>0) {
		if (y>=0 && y+al<=al_copia) { // El texto coge entero (coord. y)
			while (*ptr && x+car[*ptr].an<=0) { 
				x=x+car[*ptr].an; 
				ptr++; 
			}
			
			if (*ptr && x<0) {
				wtexc(copia,an_real_copia,an_copia,al_copia,font+car[*ptr].dir,x,y,car[*ptr].an,al,c);
				x=x+car[*ptr].an; 
				ptr++; 
			}
			
			while (*ptr && x+car[*ptr].an<=an_copia) {
				wtexn(copia,an_real_copia,font+car[*ptr].dir,x,y,car[*ptr].an,al,c);
				x=x+car[*ptr].an; 
				ptr++; 
			}
			
			if (*ptr && x<an_copia)
				wtexc(copia,an_real_copia,an_copia,al_copia,font+car[*ptr].dir,x,y,car[*ptr].an,al,c);
		} else {
			while (*ptr && x+car[*ptr].an<=0) { 
				x=x+car[*ptr].an; 
				ptr++; 
			}
			while (*ptr && x<an_copia) {
				wtexc(copia,an_real_copia,an_copia,al_copia,font+car[*ptr].dir,x,y,car[*ptr].an,al,c);
				x=x+car[*ptr].an; ptr++; 
			}
		}
	}



}

void wtexn(byte*copia,int an_real_copia,byte*p,int x,int y,byte an,int al,byte c) {

	byte *q=copia+y*an_real_copia+x;
	int ancho=an;

	do {
		do {
			if (*p) 
				*q=c; 
			p++; 
			q++;
		} while (--an);
		q+=an_real_copia-(an=ancho);
	} while (--al);
}
