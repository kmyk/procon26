/*
    Demo program for SDL-widgets-2.1
    Original authors: Sed Barbouky, Christian Klein
    Copyright 2011-2013 W.Boeke

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program.
*/

#include <stdio.h>
#include <SDL_stdinc.h>  // for Sint16 etc
#include "dump-wave.h"

static bool inited=false;
static FILE *dumpfd;
static int size;

void f_close(FILE*& fil) {
  if (!fil) return;
  fclose(fil);
  fil=0;
}

// return false if error
bool init_dump_wav(const char *fname,int nr_chan,int sample_rate) {
  Sint16 dum16;
  Uint32 dum32;

  inited=false;
  if ((dumpfd=fopen(fname,"w"))==0) { alert("init_dump_wav: %s not opened",fname); return false; }
  if (
     fwrite("RIFF", 4,1,dumpfd)!=1 ||
     (dum32=36, fwrite(&dum32, 4,1,dumpfd)!=1) ||       // header size
     fwrite("WAVEfmt ", 8,1,dumpfd)!=1 ||
     (dum32=16, fwrite(&dum32, 4,1,dumpfd)!=1) ||       // chunk size  
     (dum16=1, fwrite(&dum16, 2,1,dumpfd)!=1) ||        // format tag (1 = uncompressed PCM)
     (dum16=nr_chan, fwrite(&dum16, 2,1,dumpfd)!=1) ||  // no of channels (1 or 2)
     (dum32=sample_rate, fwrite(&dum32, 4,1,dumpfd)!=1) ||    // rate
     (dum32=sample_rate*2*2, fwrite(&dum32, 4,1,dumpfd)!=1) ||  // average bytes/sec
     (dum16=(2*16+7)/8, fwrite(&dum16, 2,1,dumpfd)!=1) || // block align
     (dum16=16, fwrite(&dum16, 2,1,dumpfd)!=1) ||       // bits per sample
     fwrite("data", 4,1,dumpfd)!=1 ||
     (dum32=0, fwrite(&dum32, 4,1,dumpfd)!=1)) {        // sample length (0 for now)
    f_close(dumpfd);
    alert("init_dump_wav: initialization failed");
    return false;
  }
  inited=true;
  size=36;
  return true;
}

bool close_dump_wav(void) {
  if (!inited) { alert("close_dump_wav: not inited"); return false; }
  inited=false;
  if (!dumpfd) { alert("close_dump_wav: file?"); return false; }

  // update the wav header
  fseek(dumpfd, 4, SEEK_SET);  // first place to update
  if (fwrite(&size, 4,1,dumpfd)!=1) goto error;
  size-=36;
  fseek(dumpfd, 40, SEEK_SET); // second place
  if (fwrite(&size, 4,1,dumpfd)!=1) goto error;

  f_close(dumpfd);
  return true;

  error:
  alert("close_dump_wav: header update failed");
  f_close(dumpfd);
  return false;
}

bool dump_wav(char *buf, int sz) {
  if (!inited) { alert("dump_wav: not inited"); return false; }
  if (fwrite(buf, sz,1,dumpfd)!=1) {
    alert("dump_wav: write problem");
    f_close(dumpfd);
    return false;
  }
  size+=sz;
  return true;
}
#ifdef TEST_DUMPWAV
#include <math.h>
#define NB_SAMPLE 2048
int main() {
  int n,m,res;
  Sint16 buf[NB_SAMPLE],
         val1,val2;
  res=init_dump_wav("out.wav");
  printf("init:%d\n",res);
  for (n=val1=val2=0;n<30;++n) {
    for (m=0;m<NB_SAMPLE;m+=2) {
      val1=(val1+6)%628; // 100 * 2 * PI = 628
      val2=(val2+9)%628;
      buf[m]=Sint16(10000. * sinf(val1/100.));
      buf[m+1]=Sint16(10000. * sinf(val2/100.));
    }
    res=dump_wav((char*)buf,NB_SAMPLE*2);
    if (!res) break;
  }
  printf("dump:%d\n",res);
  res=close_dump_wav();
  printf("close:%d\n",res);
}
#endif
