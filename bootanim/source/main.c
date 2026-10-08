#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <xenos/xenos.h>
#include <time/time.h>
#include <png.h>

#define FB_BASE 0x1e000000
#define W 1280
#define H 720
#define BLACK 0xFF050505u
#define WHITE 0xFFFFFFFFu
#define GREEN 0xFF7CFF00u
#define RED 0xFFE22222u
#define BLUE 0xFF4DA6FFu

typedef volatile uint32_t fb_t;
static fb_t *fb=(fb_t*)FB_BASE;
extern const unsigned char _binary_badavatar_png_start[];
extern const unsigned char _binary_badavatar_png_end[];
extern const unsigned char _binary_phoenix_png_start[];
extern const unsigned char _binary_phoenix_png_end[];

static void clear(uint32_t c){for(int y=0;y<H;y++){fb_t*p=fb+y*W;for(int x=0;x<W;x++)p[x]=c;}}
static void rect(int x,int y,int w,int h,uint32_t c){if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}if(x+w>W)w=W-x;if(y+h>H)h=H-y;if(w<=0||h<=0)return;for(int j=0;j<h;j++){fb_t*p=fb+(y+j)*W+x;for(int i=0;i<w;i++)p[i]=c;}}
static void line(int x0,int y0,int x1,int y1,uint32_t c){int dx=x1-x0,dy=y1-y0,sx=dx>=0?1:-1,sy=dy>=0?1:-1,err=(dx>dy?dx:-dy)/2,e2;for(;;){if(x0>=0&&x0<W&&y0>=0&&y0<H)fb[y0*W+x0]=c;if(x0==x1&&y0==y1)break;e2=err;if(e2>-dx){err-=dy;x0+=sx;}if(e2<dy){err+=dx;y0+=sy;}}}
static void circle(int cx,int cy,int r,uint32_t c){int x=-r,y=0,err=2-2*r;do{for(int k=-1;k<=1;k++){if(cx-x>=0&&cx-x<W&&cy+y+k>=0&&cy+y+k<H)fb[(cy+y+k)*W+cx-x]=c;if(cx+x>=0&&cx+x<W&&cy+y+k>=0&&cy+y+k<H)fb[(cy+y+k)*W+cx+x]=c;if(cx-y>=0&&cx-y<W&&cy+x+k>=0&&cy+x+k<H)fb[(cy+x+k)*W+cx-y]=c;if(cx+y>=0&&cx+y<W&&cy+x+k>=0&&cy+x+k<H)fb[(cy+x+k)*W+cx+y]=c;}int e=err;if(e<=y)err+=++y*2+1;if(e>x||err>y)err+=++x*2+1;}while(x<0);}
static void xbox_icon(int cx,int cy,int s){int w=s*3/5,h=s;rect(cx-w/2,cy-h/2,w,h,0xFFBFC3C7);rect(cx-w/2+10,cy-h/2+10,w-20,h-20,0xFF202326);rect(cx-w/2+18,cy-h/2+20,w-36,8,0xFF0B0B0B);circle(cx,cy+h/2-35,18,GREEN);circle(cx,cy+h/2-35,8,0xFF101010);}
static void chain_icon(int cx,int cy,int s){int w=s/2,h=s/4;for(int k=0;k<2;k++){int ox=k?(w/2):-(w/2);rect(cx+ox-w/2,cy-h/2,w,h,0xFF9EA5AA);rect(cx+ox-w/2+10,cy-h/2+9,w-20,h-18,BLACK);line(cx+ox-w/2,cy-h/2,cx+ox+w/2,cy+h/2,0xFFE0E3E5);line(cx+ox-w/2,cy+h/2,cx+ox+w/2,cy-h/2,0xFF70777C);}}
static void gear_icon(int cx,int cy,int r,uint32_t c){circle(cx,cy,r,c);circle(cx,cy,r/2,BLACK);for(int a=0;a<8;a++){int x=cx,y=cy;if(a==0)x=cx+r+7;if(a==1)x=cx-r-7;if(a==2)y=cy+r+7;if(a==3)y=cy-r-7;rect(x-8,y-8,16,16,c);}}
static void wrench_icon(int x,int y,int s,uint32_t c){line(x,y+s,x+s*3/4,y+s/4,c);circle(x+s*3/4,y+s/4,s/6,c);circle(x+s*3/4,y+s/4,s/11,BLACK);rect(x-5,y+s-10,18,18,c);}
static void usb_icon(int x,int y,int s){rect(x,y,s/2,s,0xFFD7D9DA);rect(x+8,y+12,s/2-16,s-24,0xFF171A1D);line(x+s/4,y+s/2,x+s/4,y+s/2-28,WHITE);}

typedef struct {const unsigned char*p;size_t left;} PngMem;
static void png_read_mem(png_structp p,png_bytep out,png_size_t n){PngMem*m=(PngMem*)png_get_io_ptr(p);if(n>m->left){png_error(p,"short png");return;}memcpy(out,m->p,n);m->p+=n;m->left-=n;}
static void png_render(const unsigned char*start,const unsigned char*end,int dx,int dy,int dw,int dh){
 png_structp p=png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);if(!p)return;
 png_infop info=png_create_info_struct(p);if(!info){png_destroy_read_struct(&p,NULL,NULL);return;}
 if(setjmp(png_jmpbuf(p))){png_destroy_read_struct(&p,&info,NULL);return;}
 PngMem mem={start,(size_t)(end-start)};png_set_read_fn(p,&mem,png_read_mem);png_read_info(p,info);
 png_set_expand(p);png_set_gray_to_rgb(p);png_set_tRNS_to_alpha(p);png_set_filler(p,0xFF,PNG_FILLER_AFTER);png_read_update_info(p,info);
 png_uint_32 sw=png_get_image_width(p,info),sh=png_get_image_height(p,info);int channels=png_get_channels(p,info);size_t rowbytes=png_get_rowbytes(p,info);
 png_bytep rows=(png_bytep)malloc(rowbytes*sh);if(!rows){png_destroy_read_struct(&p,&info,NULL);return;}
 png_bytep*rp=(png_bytep*)malloc(sizeof(png_bytep)*sh);if(!rp){free(rows);png_destroy_read_struct(&p,&info,NULL);return;}
 for(png_uint_32 y=0;y<sh;y++)rp[y]=rows+y*rowbytes;png_read_image(p,rp);
 for(int y=0;y<dh;y++){unsigned sy=(unsigned)y*sh/dh;for(int x=0;x<dw;x++){unsigned sx=(unsigned)x*sw/dw;png_bytep q=rp[sy]+sx*channels;unsigned a=channels>=4?q[3]:255;if(a<16)continue;fb[(dy+y)*W+dx+x]=0xFF000000u|(q[0]<<16)|(q[1]<<8)|q[2];}}
 free(rp);free(rows);png_destroy_read_struct(&p,&info,NULL);
}

static const uint8_t font[27][7]={{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,2,4,8,16,16,31},{0,0,0,0,0,0,0}};
static void text(int x,int y,const char*s,int scale,uint32_t c){for(;*s;s++){char ch=*s;if(ch>='a'&&ch<='z')ch-=32;int idx=(ch>='A'&&ch<='Z')?ch-'A':26;for(int yy=0;yy<7;yy++)for(int xx=0;xx<5;xx++)if(font[idx][yy]&(1<<(4-xx)))rect(x+xx*scale,y+yy*scale,scale,scale,c);x+=6*scale;}}
static void final_screen(void){clear(BLACK);circle(W/2,250,210,0xFF0A9CFF);circle(W/2,250,195,0xFF72FF00);xbox_icon(W/2-105,250,250);wrench_icon(W/2+15,185,120,WHITE);usb_icon(W/2+115,175,120);gear_icon(W/2+180,330,42,0xFFBFC4C7);text(310,470,"Chase's Dev Box",7,WHITE);}
static void show(int which,int d){clear(BLACK);if(which==0)xbox_icon(W/2,H/2,280);else if(which==1)png_render(_binary_badavatar_png_start,_binary_badavatar_png_end,W/2-135,H/2-85,270,170);else if(which==2)chain_icon(W/2,H/2,260);else png_render(_binary_phoenix_png_start,_binary_phoenix_png_end,W/2-270,H/2-100,540,200);mdelay(d);}
int main(void){xenos_init(VIDEO_MODE_HDMI_720P);for(int cycle=0;cycle<5;cycle++){int d=180-cycle*32;if(d<30)d=30;show(0,d);show(1,d);show(2,d);show(3,d);}for(int i=0;i<8;i++){clear((i&1)?BLACK:WHITE);for(int j=0;j<80;j++){int x=(j*137+i*71)%W,y=(j*83+i*43)%H;rect(x,y,4+(j%15),4+(j%9),(j&1)?RED:BLUE);}mdelay(35);}final_screen();mdelay(1800);return 0;}
