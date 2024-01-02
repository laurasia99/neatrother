/* imgbb.c: PS/EPS/PDF bounding box
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Knuth-Morris-Pratt substring search */

struct kmp{
  int n;
  char*w;
  int*t;
};

void _kmpbuild(int n,char*w,int*t){
  int p=1,c=0;
  t[0]=-1;
  while(p<n){
    if(w[p]==w[c])t[p]=t[c];
    else {
      t[p]=c;
      while(c>=0&&w[p]!=w[c])c=t[c];
    }
    ++p;++c;
  }
  t[p]=c;
}

void kmpinit(struct kmp*k,char*w){
  k->n=strlen(w);
  k->w=strcpy(malloc(k->n+1),w);
  k->t=calloc(k->n,sizeof(int));
  _kmpbuild(k->n,k->w,k->t);
}

void kmpfree(struct kmp*k){
  free(k->w);
  free(k->t);
}

int kmpfind(struct kmp*kmp,char*buf,int len){
  int j=0,k=0;
  while(j<len){
    if(kmp->w[k]==buf[j]){
      ++j;++k;
      if(k==kmp->n)return j-k;
    }else{
      if((k=kmp->t[k])<0){++j;++k;}
  } }
  return -1;
}

/* ASSUMPTION:
 *      A reasonable line buffer size is 258(=255+2+1) bytes
 *      (inc line termination characters and NUL-terminator)
 *
 * Red Book: PostScript Language Reference Manual (2nd edition)
 * Appendix G.4 Document Structure Rules
 *  "...a conforming PostScript language document description does not have
 *   lines exceeding 255 characters, excluding line termination characters."
 *
 * PDF 32000-1:2008 PDF 1.7 ISO Standard
 * 7.5.1 File Structure - General
 *  "To increase compatibility with compliant programs that process PDF files,
 *   lines that are not part of stream object data are limited to no more than
 *                  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ i.e. binary data exists!
 *   255 characters, with one exception" (12.8 "Digital Signatures")
 *
 * HOWEVER, to handle PDF's in general just read binary blocks.
 * This avoids the need to perform any tokenisation.
 */
#define LINELEN 258

/* Simple file buffer (fixed block size; forward traversal) */

struct fb{
  FILE *f;
  int len;
  char buf[4*1024];
};

void fbinit(struct fb*fb,FILE*f){fb->f=f;fb->len=0;}

void fbshift(struct fb*fb,int off){
  assert(off<fb->len);
  memcpy(fb->buf,fb->buf+off,fb->len-off);
  fb->len-=off;
}

int fbfill(struct fb*fb){
  int got=0;
  if(fb->len<sizeof(fb->buf)){
    got=(int)fread(fb->buf+fb->len,1,sizeof(fb->buf)-fb->len,fb->f);
    fb->len+=got;
  }
  return got;
}

/* Forward search file buffer for string (no backtracking) */
int fbfind(struct fb*fb,struct kmp*kmp){
  int i;
  while(1){
    (void)fbfill(fb);
    if(fb->len<kmp->n)return -1;
    else if(fb->len==kmp->n&&strncmp(fb->buf,kmp->w,kmp->n))return -1;
    if((i=kmpfind(kmp,fb->buf,fb->len))>=0){
      fbshift(fb,i); /* Relocate word to buffer start */
      if(fb->len<LINELEN)fbfill(fb);
      return 0;
    }
    fbshift(fb,fb->len-kmp->n);
  }
}

/* Find word once */
int fbfind1(struct fb*fb,char*w){
  struct kmp k;
  int i;
  kmpinit(&k,w);
  i=fbfind(fb,&k);
  kmpfree(&k);
  return i;
}

/* Return 0 if matched @ offset */
int fbmatch(struct fb*fb,int off,char*s){
  return strncmp(fb->buf+off,s,strlen(s));
}

int process(FILE*f){
  struct fb fb;
  float llx,lly,urx,ury;
  int i;
  fbinit(&fb,f);fbfill(&fb);
  if(!fbmatch(&fb,0,"%!PS-Adobe")){
    i=fbfind1(&fb,"%%BoundingBox:");
    if(4==sscanf(fb.buf+14," %f %f %f %f",&llx,&lly,&urx,&ury)){
      printf(".nr llx %fp\n.nr lly %fp\n.nr urx %fp\n.nr ury %fp\n",llx,lly,urx,ury);
      return 1;
    }
    /* else "(atend)" -- XXX continue search */
  }else
  if(!fbmatch(&fb,0,"%PDF-")){
    i=fbfind1(&fb,"/MediaBox");
    if(4==sscanf(fb.buf+9," [ %f %f %f %f",&llx,&lly,&urx,&ury)){
      printf(".nr llx %fp\n.nr lly %fp\n.nr urx %fp\n.nr ury %fp\n",llx,lly,urx,ury);
      return 1;
    }
  }
  return 0;
}

int main(int argc,char **argv){
  FILE*f;
  int i,rv=1;
  if(1==argc){
    fprintf(stderr,"imgbb <filename>\n\nExtract bounding box information from .eps/.ps/.pdf file\n");
    return 1;
  }
  for(i=1;i<argc;++i){
    if(!strcmp(argv[i],"-")){
      if(!(rv=process(stdin)))break;
    }else{
      f=fopen(argv[i],"rb");
      if(!f){
        fprintf(stderr,"imgbb: cannot open '%s'\n",argv[i]);
        return 1;
      }
      rv=process(f);
      fclose(f);
      if(!rv)break;
    }
  }
  if(!rv){
    fprintf(stderr,"imgbb: error processing '%s'\n",argv[i]);
    return 1;
  }

  return 0;
}
