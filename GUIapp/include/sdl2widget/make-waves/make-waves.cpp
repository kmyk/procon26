/*  
    Demo program for SDL-widgets-2.1
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
#include <math.h>
#include "../sdl-widgets.h"
#include "dump-wave.h"

bool debug;

template<class T,const int dim>
struct Array {  // can insert and remove
  int end;
  T buf[dim];
  Array():end(-1) {
  }
  T& operator[](int ind) {
    if (ind>=0 && ind<dim) {
      if (end<ind) end=ind;
      return buf[ind];
    }
    alert("Array: index=%d (>=%d) end=%d",ind,dim,end); if (debug) abort();
    return buf[0];
  }
/*
  T* operator+(int ind) {  // pointer arithmetic
    if (ind>=0 && ind<dim) {
      if (end<ind) end=ind;
      return buf+ind;
    }
    alert("Array: index=%d (>=%d)",ind,dim); if (debug) abort();
    return buf;
  }
*/
  void insert(int ind,T item) {
    if (end>=dim-1) {
      alert("Array insert: end >= %d-1",dim); if (debug) abort();
      return;
    }
    ++end;
    for (int j=end;j>ind;--j) buf[j]=buf[j-1];
    buf[ind]=item;
  }
  void remove(int ind) {
    if (end<0) return;
    for (int j=ind;j<end;++j) buf[j]=buf[j+1];
    --end;
  }
};


TopWin *top_win;

ExtRButCtrl *tab_ctr;
BgrWin *waves_ctr,*bgw1,*bgw2, // waves
       *bouncy_ctr,*bgw3,      // bouncy
       *harm_ctr,*bgw4,        // harmonics
       *filt_ctr,*bgw5,        // filters
       *scope_win;
Button *play_but,
       *save,
       *wave_out;
CheckBox *freeze,
         *low_frict,
         *non_lin_down,
         *open_ended,
         *randomize,
         *no_clip_harm,
         *log_a_scale,
         *log_f_scale,
         *do_change;
HSlider *am_amount,*fm_amount,
        *ab_detune,
        *node_mass,*asym_xy,
        *harm_offset,*ampl_mult,*harm_freq,*harm_clip,*harm_dist,*harm_n_mode,
        *filt_mode,*filt_range,*filt_freq,
        *filt_qfactor;
HVSlider *freqs_sl;
RButtons *mode_select,
         *bouncy_in,
         *test_filter;
TextWin *harm_info;
const int
  bg_w=128, // wave background width
  bg_h=50,  // wave background height/2
  filt_h=100, // filter display height 
  bncy_w=280, // bouncy bgr width
  harm_w=280, // harmonics bgr width
  filt_w=200, // filters bgr width
  SAMPLE_RATE=44100,
  scope_dim=bg_w,
  scope_h=30,
  wav_pnt_max=20, // points in waveforms
  harm_max=20;  // harmonics

int task,
    scope_buf1[scope_dim],scope_buf2[scope_dim],
    i_am_playing;
bool write_wave; // write wave file?

enum {
  eA,eB,eA_B,eB_am_A,eB_fm_A,eB_trig_A,  // wave display modes
  eString, eStringOEnd,         // bouncy mode
  eRunWaves, eRunBouncy, eRunHarmonics, eRunFilters, // task
  ePlaying, eDecaying,          // play modes
  eIdle, eMoving                // wave edit mode
};

SDL_cond *stop=SDL_CreateCond();
SDL_mutex *mtx=SDL_CreateMutex();

struct WavF;
SDL_Thread *audio_thread;

namespace we {  // wave edit
  int state;
  int lst_x,lst_y;
  WavF *wf;
  int act_pt;
  bool do_point(int x,int y,int but);
  int y2pt(int y) { return (y-bg_h)*20; }
  int pt2y(int y) { return y/20 + bg_h; }
}

struct Node {
  float d_x,nom_x,
        d_y,
        vx,
        vy,
        ax,
        ay,
        mass;
  bool fixed;
  void set(float dx, float dy, bool _fixed) {
    d_x=dx; d_y=dy; vx=0; vy=0; fixed=_fixed; 
  }
  void cpy(Node& src) { d_x=src.d_x; d_y=src.d_y; }
  void cpy(Node& old,Node& src,float mix) {
    d_x=int((1.-mix)*old.d_x+mix*src.d_x);
    d_y=int((1.-mix)*old.d_y+mix*src.d_y);
  }
};

struct Spring {
  Node *a,
       *b;
  void set(Node *_a, Node *_b) {
    a=_a; b=_b;
  }
  Spring() { }
};

void do_atexit() { puts("make_waves: bye!"); }

struct WavF {  // waveform buffer
  short wform[bg_w];
  Array<Point,wav_pnt_max> ptbuf;
  WavF() {
    ptbuf[0].set(0,0);
    ptbuf[1].set(bg_w/4,-400);
    ptbuf[2].set(bg_w*3/4,400);
    ptbuf[3].set(bg_w+1,0); // this is the guard
    ptbuf.end=3;
  }
} wavf1,wavf2;

static int max(int a, int b) { return a>b ? a : b; }
static int minmax(int a, int x, int b) { return x>b ? b : x<a ? a : x; }
static float minmax(float a, float x, float b) { return x>b ? b : x<a ? a : x; }
static float min(float a, float b) { return a>b ? b : a; }
static float max(float a, float b) { return a<b ? b : a; }

bool we::do_point(int x,int y,int but) {
  if (x<0 || x>=bg_w) return false;
  for (int i=0;i<wav_pnt_max && i<=wf->ptbuf.end;++i) {
    act_pt=i;
    Point *pt=&wf->ptbuf[i];
    //say("x=%d pt->x=%d y=%d pt->y=%d",x,act_pt->x,y,pt2y(act_pt->y));
    switch (but) {
      case SDL_BUTTON_LEFT:
        if (hypot(x-pt->x,y-pt2y(pt->y))<3) {
          lst_x=x; lst_y=y;
          state=eMoving;
          SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
          return false;
        }
        if (pt->x==x) return false;
        if (pt->x>x) {
          wf->ptbuf.insert(i,Point(x,y2pt(y))); // so inserted points have increasing x
          return true;
        }
        break;
      case SDL_BUTTON_MIDDLE:
        if (hypot(x-pt->x,y-pt2y(pt->y))<3) {
          if (wf->ptbuf.end==2) {
            alert("last point can't be removed");
            return false;
          }
          wf->ptbuf.remove(i);
          return true;
        }
        if (pt->x>=x)
          return false;
        break;
    }
  }
  return false;
}

void make_spline(
    int pt_end,  // dim of xs (points x buffer)
    int wdim,  // dim of ys (point val buffer)
    Point *pnt, 
    short *out // waveform buffer
  ) {
  int i,j,
      i0,i2,i3,
      delta;
  float x,xi,xi0,xi2,xi3;
  for (i=0;i<pt_end;++i) { // last point not used
    i0= i==0 ? pt_end-1 : i-1;
    i2= i==pt_end-1 ? 0 : i+1;
    i3= i2==pt_end-1 ? 0 : i2+1;
    delta= pnt[i2].x-pnt[i].x;
    if (delta<0) delta+=wdim;
    xi0=pnt[i0].y;
    xi=pnt[i].y;
    xi2=pnt[i2].y;
    xi3=pnt[i3].y;
    for (j=0;j<delta;++j) {
      x=float(j)/delta;
      // From: www.musicdsp.org/showone.php?id=49
      out[pnt[i].x+j]=int(
        ((((3 * (xi-xi2) - xi0 + xi3) / 2 * x) +
          2*xi2 + xi0 - (5*xi + xi3) / 2) * x +
          (xi2 - xi0) / 2) * x +
          xi);
    }
  }
}

void draw_wform(BgrWin *bgw,WavF *wf) { // supposed: bgw->parent = top_window
  bgw->clear();
  int i;
  for (i=0;i<wf->ptbuf.end;++i) {
    int x=wf->ptbuf[i].x,
        val=wf->ptbuf[i].y;
    circleColor(bgw->render,x,we::pt2y(val),2,0xff);
    if (i==0) circleColor(bgw->render,bg_w-1,we::pt2y(val),2,0xff);
  }
  int val2=we::pt2y(wf->wform[127]);
  for (i=0;i<bg_w;++i) {
    int val1=val2;
    val2=we::pt2y(wf->wform[i]);
    lineColor(bgw->render,i,val1,i+1,val2,0xff);
  }
  bgw->blit_upd();
}

namespace we {
  void draw_bgw1(BgrWin *wb) {
    make_spline(wavf1.ptbuf.end,bg_w,wavf1.ptbuf.buf,wavf1.wform);
    draw_wform(wb,&wavf1);
  }
  void draw_bgw2(BgrWin *wb) {
    make_spline(wavf2.ptbuf.end,bg_w,wavf2.ptbuf.buf,wavf2.wform);
    draw_wform(wb,&wavf2);
  }

  void down(BgrWin* bgw,int x,int y,int but) {
    wf= bgw==bgw1 ? &wavf1 : &wavf2;
    state=eIdle;
    if (do_point(x,y,but)) {
      make_spline(wf->ptbuf.end,bg_w,wf->ptbuf.buf,wf->wform);
      draw_wform(bgw,wf);
    }
  }

  void move(BgrWin* bgw,int x,int y,int but) {
    x=minmax(0,x,bg_w-2); // -2: less then last point (is first point)
    y=minmax(0,y,bg_h*2-1);
    if (act_pt==0) x=0;
    if (state!=eMoving || hypot(x-lst_x,y-lst_y)<2) return;
    lst_x=x; lst_y=y;
    Point *pt=&wf->ptbuf[act_pt];
    if ((act_pt>0 && pt[-1].x>=x) || pt[1].x<=x)
      return;
    pt->set(x,y2pt(y));
    make_spline(wf->ptbuf.end,bg_w,wf->ptbuf.buf,wf->wform);
    draw_wform(bgw,wf);
  }

  void up(BgrWin* bgw,int x,int y,int but) {
    state=eIdle;
    SDL_EventState(SDL_MOUSEMOTION,SDL_DISABLE);
  }
}

struct Scope {
  int pos,
      scope_start;
  bool display1,display2;
  void update(Sint16 *buffer,int i);
} scope;

void waves_mode_cmd(RButtons*,int nr,int fire) {
  switch (nr) {
    case 0: case 3: case 4: case 5:
      scope.display1=true; scope.display2=false; break;
    case 1:
      scope.display2=true; scope.display1=false; break;
    case 2:
      scope.display1=scope.display2=true; break;
  }
  switch (nr) {
    case 0: case 1: case 5:
      am_amount->hide(); fm_amount->hide(); ab_detune->hide(); break;
    case 2:
      am_amount->hide(); fm_amount->hide(); ab_detune->show(); break;
    case 3:
      fm_amount->hide(); am_amount->show(); ab_detune->show(); break;
    case 4:
      am_amount->hide(); fm_amount->show(); ab_detune->show(); break;
  }
}

void bgr_clear(BgrWin *bgr) { bgr->clear(); }

struct Waves {
  float freq1,
        freq2,
        fm_val,
        am_val,
        am_bias,
        ab_det,
        ind_f1,
        ind_f2,
        ampl;
  const float fmult;
  Waves();
  void audio_callback(Uint8 *stream, int len);
} *waves;

void draw_curve(Render rend,Rect *r,int *buf,int dim,Uint32 color) {
  SDL_Color sdlc=sdl_col(color);
  SDL_SetRenderDrawColor(rend.render,sdlc.r,sdlc.g,sdlc.b,0xff);
  SDL_Point pnt[dim];
  for (int i=0;i<dim;++i) { pnt[i].x=i+r->x; pnt[i].y=buf[i]+r->y; }
  SDL_RenderDrawLines(rend.render,pnt,dim);
}

void Scope::update(Sint16 *buffer,int i) {
  if (scope_start<0) return;
  Sint16 *test_buf=buffer;
  if (!display1) ++test_buf; // then other channel is tested for startup
  if (scope_start==0) { // set by handle_user_event()
    if (i>=2 && test_buf[i-2]<=0 && test_buf[i]>0) {
      scope_start=true;
      pos=0;
    }
    return;
  }
  if (i%4==0) {
    if (pos<scope_dim) {
      scope_buf1[pos]=buffer[i]/400 + scope_h;
      scope_buf2[pos]=buffer[i+1]/400 + scope_h;
      ++pos;
    }
    else {
      scope_start=-1;
      send_uev([](int) {
        Rect *sr=&scope_win->tw_area;
        top_win->clear(sr,cWhite,false);
        if (scope.display1) draw_curve(top_win->render,sr,scope_buf1,scope_dim,0xff);
        if (scope.display2) draw_curve(top_win->render,sr,scope_buf2,scope_dim,0xd0ff); // blue
        scope.scope_start=0;
        top_win->upd(sr);
      },0);
    }
  }
}

void Waves::audio_callback(Uint8 *stream, int len) {
  float fr1=freq1*fmult,
        fr2=freq2*fmult*ab_det;
  int val1,val2;
  bool trigger;
  Sint16 *buffer=reinterpret_cast<Sint16*>(stream);
  int mode=mode_select->value();
  for (int i=0;i<len/2;i+=2) {
    if (i_am_playing==ePlaying) { if (ampl<10.) ampl+=0.01; }
    else if (i_am_playing==eDecaying) {
      if (ampl>0.) ampl-=0.0004;
      else {
        i_am_playing=0;
        SDL_CondSignal(stop);
      }
    }
    // wave B
    ind_f2+=fr2;

    trigger=false;
    if (int(ind_f2)<0)
      ind_f2+=bg_w;
    else if (int(ind_f2)>=bg_w) {
      ind_f2-=bg_w;
      trigger=true;
    }
    if (mode==eB_trig_A) val2=0;
    else val2=int(wavf2.wform[int(ind_f2)]);

    // wave A
    if (mode==eB_trig_A) {
      if (trigger) ind_f1=0.;
      else ind_f1+=fr1;
      if (int(ind_f1)>=bg_w) val1=0.;
      else val1=int(wavf1.wform[int(ind_f1)]);
    }
    else {
      if (mode==eB_fm_A) 
        ind_f1+=fr1 + fm_val * val2/400.;
      else
        ind_f1+=fr1;
      if (int(ind_f1)>=bg_w) ind_f1-=bg_w;
      else if (int(ind_f1)<0) ind_f1+=bg_w;
      val1=int(wavf1.wform[int(ind_f1)]);
      if (mode==eB_am_A) val1 *= am_bias + am_val * val2/400.;
      //if (mode==eB_am_A) val1 *= am_bias + am_val * val2/400. * (1.+val1/300.); // <-- with feedback
    }
    val1=minmax(-30000,int(val1 * ampl),30000);
    val2=minmax(-30000,int(val2 * ampl),30000);
    switch (mode) {
      case eA:
      case eB_am_A:
      case eB_fm_A:
      case eB_trig_A:
        buffer[i]=val1; buffer[i+1]=0; break;
      case eB:
        buffer[i]=0; buffer[i+1]=val2; break;
      case eA_B:
        buffer[i]=val1; buffer[i+1]=val2; break;
    }
    scope.update(buffer,i);
  }
  if (write_wave && !dump_wav((char*)buffer,len/2)) write_wave=false;
}

void wav_audio_callback(void*, Uint8 *stream, int len) {
  waves->audio_callback(stream,len);
}

struct Noise {
  static const int ndim=100000;
  short pink_buf[ndim],
        white_buf[ndim];
  float nval() { return (rand() & 0xff)-127.5; }
  Noise() {
    const int pre_dim=100;
    float b1=0.,b2=0.,
          white,
          pre_buf[pre_dim];
    for (int i=-pre_dim;i<ndim;++i) {
      white=i<ndim-pre_dim ?  nval() : pre_buf[i-ndim+pre_dim];
      b1 = 0.98 * b1 + white * 0.4; // correct from about 100 Hz
      b2 = 0.6 * b2 + white;
      if (i<0) pre_buf[i+pre_dim]=white;
      else pink_buf[i] = b1 + b2;  // at most +/- 450
    }
    for (int i=-pre_dim;i<ndim;++i) {
      white=i<ndim-pre_dim ?  nval() : pre_buf[i-ndim+pre_dim];
      b2 = 0.6 * b2 + white;
      if (i<0) pre_buf[i+pre_dim]=white;
      else white_buf[i]=2 * b2;  // at most +/- 390
    }
  }
} noise;

namespace com {  // for Bouncy and PhysModel
  const float
    nom_friction = 0.9,
    radius = 10.,
    nom_springconst = 0.05;
  const int
    NN=9,  // nodes
    xdist=(bncy_w-40)/(NN-1);
  int mode=eString, // eString, eStringOEnd
    pickup=NN/3;
  float
    springconstant=0.8*nom_springconst,
    mass=5,
    asym=1;
}

struct Mass_Spring {
  Node nodes[com::NN];
  Spring springs[com::NN-1];
  int
    n_ind,
    phase,
    tim,      // time after mouse_up()
    nom_bdur; // noise burst time
  float
    friction,
    in_val,
    sync_val;
  Node *selnode;
  Mass_Spring():
      n_ind(0),
      phase(0),tim(0),
      nom_bdur(20),
      friction(com::nom_friction),
      in_val(0.),
      sync_val(0.),
      selnode(0) {
    int i;
    for (i=0;i<com::NN;++i) nodes[i].mass=com::mass;
    for (i = 0; i < com::NN; i++) {
      nodes[i].nom_x=20+i*com::xdist;
      nodes[i].set(nodes[i].nom_x,bg_h + 30 * (i % 3),false); 
    }
    nodes[0].fixed = nodes[com::NN-1].fixed = true;
    nodes[0].d_y = nodes[com::NN-1].d_y = bg_h;
    for (i = 0; i < com::NN-1; i++)
      springs[i].set(nodes+i, nodes+i+1);
  }
  void eval_model();
  void nburst() {
    if (phase==1) {  // set by bouncy_up()
      phase=2;
      tim=0;
    }
    if (phase==2) {
      if (!selnode) { phase=0; return; } // if clicked while phase = 2
      int n1=noise.white_buf[tim]*0.005,
          n2=noise.white_buf[tim+1000]*0.005;
      selnode->d_x+=n1;
      selnode->d_y+=n2;

      //int n1=noise.pink_buf[tim]*0.01,
      //    n2=noise.pink_buf[tim+1000]*0.01;
      //selnode->d_x+=(nom_bdur-tim)*n1/nom_bdur;
      //selnode->d_y+=(nom_bdur-tim)*n2/nom_bdur;
      //selnode->d_x=selnode->nom_x + (nom_bdur-tim)*n1/nom_bdur;
      //selnode->d_y=bg_h + (nom_bdur-tim)*n2/nom_bdur;

      if (++tim==nom_bdur) phase=0;
    }
  }
};

struct Bouncy:Mass_Spring {
  bool down;
  SDL_Thread *bouncy_thread;
  Bouncy();
  void audio_callback(Uint8 *stream, int len);
} *bouncy;

struct PhysModel:Mass_Spring {
  PhysModel() {
    friction=1.-(1.-com::nom_friction)/2200.;  // 44100 / 20 = 2205
    nom_bdur=1000;
  }
} *phys_model;

struct Sinus {
  static const int dim=500;
  float buf[dim];
  Sinus() {
    for (int i=0;i<dim;++i)
      buf[i]=5*sin(2*M_PI*i/dim);
  }
  float get(float &ind_f) { // ind_f: 0 -> dim
    int ind=int(ind_f);
    if (ind<0) {
      while (ind<0) ind+=dim;
      if (ind>=dim) { alert("Sinus: ind=%d",ind); return 0.; }
      ind_f+=float(dim);
    }
    else if (ind>=dim) {
      while (ind>=dim) ind-=dim;
      if (ind<0) { alert("Sinus: ind=%d",ind); return 0.; }
      ind_f-=float(dim);
    }
    return buf[ind];
  }
  float get(float &ind_f,float aa) { // ind_f: 0 -> dim
    int ind=int(ind_f);
    if (ind<0) {
      while (ind<0) ind+=dim;
      ind_f+=float(dim);
    }
    else if (ind>=dim) {
      while (ind>=dim) ind-=dim;
      ind_f-=float(dim);
    }
    ind=int(aa*ind/(1+(aa-1.)*ind/dim)); if (ind<0) ind+=dim; else if (ind>=dim) ind-=dim;
    return buf[ind];
  }
} sinus;

struct NoisySinus {
  static constexpr int
    nr_pts=200,  // points per sample
    nr_samp=500,
    dim=nr_pts*nr_samp,
    pre_dim=dim/2;
  static constexpr float
    freqcut=2 * M_PI / nr_pts,
    qres=0.05,
    fdelta=1.01;
  float buf[dim],
        pre_buf[pre_dim];
  NoisySinus() {
    float white=0,output,
          d1=0,d2=0,d3=0,d4=0;
    const float fc1=freqcut/fdelta,  // flatter peak
                fc2=freqcut*fdelta;
    srand(12345);
    for (int i=-pre_dim;i<dim;++i) { // such that end and begin of buf are matching
      if (i<dim-pre_dim) {
        if (i%(nr_pts/2)==0)
          white=(float(rand())/RAND_MAX - 0.5) * 0.2;
      }
      else {
        white=pre_buf[i-dim+pre_dim];
      }
      output=bp_section(fc1,d1,d2,white);
      output=bp_section(fc2,d3,d4,output);

      if (i<0)
        pre_buf[i+pre_dim]=white; 
      else
        buf[i]=output;
    }
  }
  float bp_section(const float fcut,float& d1,float& d2,const float white) {
    d2+=fcut*d1;                    // d2 = lowpass output
    float highpass=white-d2-qres*d1;
    d1+=fcut*highpass;              // d1 = bandpass output
    return d1;
  }
  float get(float &ind_f) { // ind_f: 0 -> dim
    int ind=int(ind_f);
    if (ind<0) {
      ind+=dim;
      if (ind<0 || ind>=dim) { alert("NoisySinus: ind=%d",ind); return 0.; }
      ind_f+=float(dim);
    }
    else if (ind>=dim) {
      ind-=dim;
      if (ind<0 || ind>=dim) { alert("NoisySinus: ind=%d",ind); return 0.; }
      ind_f-=float(dim);
    }
    return buf[ind];
  }
} noisy_sinus;

struct HLine {  // one harmonic
  int ampl,
      act_ampl;
  float offset;
  bool sel,
       info;
  float freq,
        ind_f;
  HLine() { reset(); }
  void reset() { ampl=act_ampl=0; offset=0.; sel=false; info=false; ind_f=0.; }
};

struct LoFreqNoise {
  float noise,  // -1. -> 1.
        d1,d2;  // filter state
  int cnt;
  LoFreqNoise():noise(0),d1(0),d2(0),cnt(-1) { }
};

namespace lfnoise {
  int div=10;
  const float qres=0.5;
  int n_mode=0;
  bool noisy=false;
  float n_mult=0.5,
        n_cutoff=0.3;
  const char* n_mname;
  LoFreqNoise lfn_buf[harm_max];
  float lp_section(const float fcut,float& d1,float& d2,const float white) {
    d2+=fcut*d1;                    // d2 = lowpass output
    float highpass=white-d2-qres*d1;
    d1+=fcut*highpass;              // d1 = bandpass output
    return d2;
  }
  void set_mode(int n) {
    n_mode=n;
    noisy=false;
    switch (n_mode) {
      case 0:
        n_mname="no noise";
        break;
      case 1: // noise hf freq mod
        n_mname="HF freq mod";
        n_mult=0.5;
        n_cutoff=0.3;
        div=10;
        break;
      case 2: // noise lf freq mod
        n_mname="LF freq mod";
        n_mult=2.;
        n_cutoff=0.02;
        div=50;
        break;
      case 3: // hf noise ampl mod
        n_mname="HF ampl mod";
        n_mult=0.5;
        n_cutoff=0.3;
        div=10;
        break;
      case 4: // lf noise ampl mod
        n_mname="LF ampl mod";
        n_mult=2.;
        n_cutoff=0.03;
        div=50;
        break;
      case 5: // noise distorsion mod
        n_mname="dist mod";
        n_mult=2.;
        n_cutoff=0.03;
        div=50;
        break;
      case 6:
        n_mname="noisy";
        noisy=true;
        break;
    }
  }
  void update(int har);
}

struct Harmonics {
  static const int
    ldist=harm_w/harm_max+1; // 280/20+1 = 15
  Array<HLine,harm_max> lines;
  int info_lnr;
  float sclip_limit,
        freq,
        ampl,
        aa;
  const float fmult,
              ns_fmult;
  Harmonics();
  void draw_line(int);
  void audio_callback(Uint8 *stream, int len);
  void print_info();
  int x2ind(int x) { return (x+ldist/2)/ldist; }

  int non_linear(int x) {
    const int level=10000;
    x *= 2/sclip_limit; // sclip_limit != 0
    if (x>0) {
      if (x>level) return level;
      return 2*(x-x*x/2/level);
    }
    else {
      if (x<-level) return -level;
      return 2*(x+x*x/2/level);
    }
  }
} *harm;

struct FilterTest {
  struct FilterBase *the_filter;
  int range,
      pos; // for audio
  float ampl,val; // for audio
  bool updating;
  float fq, // filter Q
        cutoff;
  FilterTest();
  void draw_fresponse();
  void audio_callback(Uint8 *stream, int len);
} *filt;

#include "filter-test.cpp"

void bnc_audio_callback(void*, Uint8 *stream, int len) {
  bouncy->audio_callback(stream,len);
}

void harm_audio_callback(void*, Uint8 *stream, int len) {
  harm->audio_callback(stream,len);
}

void filt_audio_callback(void*, Uint8 *stream, int len) {
  filt->audio_callback(stream,len);
}

void lfnoise::update(int har) {
  LoFreqNoise &lfn=lfn_buf[har];
  lfn.cnt=(lfn.cnt+1)%div;
  if (lfn.cnt==0) {
    float val= float(rand()) / RAND_MAX * 2. - 1.;
    lfn.noise=minmax(-1., n_mult * lp_section(n_cutoff,lfn.d1,lfn.d2,val), 1.);
    if (debug && lfn.noise>0.99) { printf("noise=%.2f\n",lfn.noise); fflush(stdout); }
  }
}

void Mass_Spring::eval_model() {
  int i;
  for (i = 0; i < com::NN; i++) 
    nodes[i].ax = nodes[i].ay = 0;
  switch (bouncy_in->value()) {
    case 0:  // mouse
      break;
    case 1:  // mouse, noise burst
      if (phase>0) nburst();
      break;
    case 2: {  // pink noise
        const float ns=0.005;
        short *buf=noise.pink_buf;
        n_ind=(n_ind+1)%Noise::ndim;
        int ind_y=(n_ind+1000)%Noise::ndim;
        nodes[0].d_x=nodes[0].nom_x + buf[n_ind] * ns;
        nodes[0].d_y=bg_h + buf[ind_y] * ns;
      }
      break;
    case 3: {  // feedback
        Node *nod=nodes+com::pickup;
        const float fb=0.3;
        nodes[0].d_x=minmax(-1.,(nod->d_x-nod->nom_x) * fb,1.) + nodes[0].nom_x;
        nodes[0].d_y=minmax(-1.,(nod->d_y-bg_h) * fb,1.) + bg_h;
      }
      break;
  }
  for (i = 0; i < com::NN-1; i++) {
    Spring *s = springs+i;
    float dx,dy;
    dx = s->b->d_x - s->a->d_x - com::xdist;
    dy = s->b->d_y - s->a->d_y;
    if (non_lin_down->value()) {
      float ddy = dy*dy/20;
      dy += dy>0 ? ddy : -ddy;
      float dx1 = dx*dx/20;
      dx += dx>0 ? dx1 : -dx1;
    }
    dx *= com::springconstant;
    dy *= com::springconstant;
    s->a->ax += dx / (s->a->mass/com::asym);
    s->a->ay += dy / (s->a->mass*com::asym);
    s->b->ax -= dx / (s->b->mass/com::asym);
    s->b->ay -= dy / (s->b->mass*com::asym);
  }
  Node *n;
  for (i = 1; i < com::NN; i++) {
    n = nodes+i;
    if (n->fixed || (bouncy->down && n == bouncy->selnode))
      continue;
    n->vy = (n->vy + n->ay) * friction;
    n->vx = (n->vx + n->ax) * friction;
    n->d_x += n->vx;
    n->d_y += n->vy;

    float val=n->d_x - n->nom_x;
    const float limit=bg_h;
    if (val>limit) n->d_x -= (val-limit)*0.5;
    else if (val<-limit) n->d_x -= (val+limit)*0.5;
    val=n->d_y - bg_h;
    if (val>limit) n->d_y -= (val-limit)*0.5;
    else if (val<-limit) n->d_y -= (val+limit)*0.5;
  }
}

namespace smooth {
  int phase=0,
      tim=0;
  const int delta=100;
  Node b_nodes[com::NN],
       pm_nodes[com::NN];
  void smooth() {
    if (phase==1) {  // set by bouncy_up()
      for (int i=0;i<com::NN;++i) {
        pm_nodes[i].cpy(phys_model->nodes[i]);
        b_nodes[i].cpy(bouncy->nodes[i]);
      }
      phase=2;
      tim=0;
    }
    if (phase==2) {
      for (int i=0;i<com::NN;++i) {
        phys_model->nodes[i].cpy(pm_nodes[i],b_nodes[i],float(tim)/delta);
      }
      if (++tim==delta) phase=0;
    }
  }
}

void Bouncy::audio_callback(Uint8 *stream, int len) {
  if (i_am_playing==eDecaying) { // set by play_cmd()
    i_am_playing=0;
    SDL_CondSignal(stop);        // audio_thread killed
  }
  Sint16 *buffer=reinterpret_cast<Sint16*>(stream);
  const float scale=300.;
  if (freeze->value())
    for (int i=0;i<len/2;++i) buffer[i]=0;
  else
    for (int i=0;i<len/2;i+=2) {
      if (smooth::phase>0) smooth::smooth();
      else
        phys_model->eval_model();
      Node *nod=phys_model->nodes+com::pickup;
      buffer[i]  =minmax(-30000,int((nod->d_y-bg_h) * scale),30000);
      buffer[i+1]=minmax(-30000,int((nod->d_x-nod->nom_x) * scale),30000);
      scope.update(buffer,i);
    }
  if (write_wave && !dump_wav((char*)buffer,len/2)) write_wave=false;
}

void init_audio() {
  SDL_AudioSpec *ask=new SDL_AudioSpec,
                *got=new SDL_AudioSpec;
  ask->freq=SAMPLE_RATE;
  ask->format=AUDIO_S16SYS;
  ask->channels=2;
  ask->samples=2048;
  switch (task) {
    case eRunWaves: ask->callback=wav_audio_callback; break;
    case eRunBouncy: ask->callback=bnc_audio_callback; break;
    case eRunHarmonics: ask->callback=harm_audio_callback; break;
    case eRunFilters: ask->callback=filt_audio_callback; break;
    default: ask->callback=0;
  }
  ask->userdata=0;
  if ( SDL_OpenAudio(ask, got) < 0 ) {
     alert("Couldn't open audio: %s",SDL_GetError());
     exit(1);
  }
  //printf("samples=%d channels=%d freq=%d format=%d (LSB=%d) size=%d\n",
  //       got->samples,got->channels,got->freq,got->format,AUDIO_S16LSB,got->size);
  SDL_PauseAudio(0);
}

int play_threadfun(void* data) {
  if (SDL_GetAudioStatus()!=SDL_AUDIO_PLAYING)
    init_audio();
  SDL_mutexP(mtx);
  SDL_CondWait(stop,mtx);
  SDL_mutexV(mtx);
  SDL_CloseAudio();
  if (write_wave) {
    close_dump_wav();
    write_wave=false;
  }
  return 0;
}

void right_arrow(Render rend,int par,int y_off) {
  trigonColor(rend,6,6,12,10,6,14,0x000000ff);
}

void square(Render rend,int par,int y_off) {
  rectangleColor(rend,6,6,14,14,0xff0000ff);
}

void play_cmd(Button* but) {
  if (i_am_playing) {
    but->label.draw_cmd=right_arrow;
    i_am_playing=eDecaying; // then: audio_thread will be killed
    SDL_WaitThread(audio_thread,0);
    wave_out->hide();
  }
  else {
    i_am_playing=ePlaying;
    play_but->label.draw_cmd=square;
    wave_out->show();
    scope.scope_start=0;
    audio_thread=SDL_CreateThread(play_threadfun,"play_threadfun",0);
  }
}

void save_cmd(Button*) {
  FILE *sav=fopen("x.mw","w");
  if (!sav) { alert("x.mw can't be written"); return; }
  fprintf(sav,"Format:1.0\n");

  fprintf(sav,"Waves\n");
  fprintf(sav,"  A freq:%d\n",freqs_sl->value().x);
  fprintf(sav,"  B freq:%d\n",freqs_sl->value().y);
  fprintf(sav,"  mode:%d\n",mode_select->value());
  fprintf(sav,"  AM amount:%d\n",am_amount->value());
  fprintf(sav,"  FM amount:%d\n",fm_amount->value());
  fprintf(sav,"  detune:%d\n",ab_detune->value());
  fprintf(sav,"  A:");
  for (int i=0;i<=wavf1.ptbuf.end;++i)
    fprintf(sav,"%d,%d;",wavf1.ptbuf[i].x,wavf1.ptbuf[i].y);
  putc('\n',sav);
  fprintf(sav,"  B:");
  for (int i=0;i<=wavf2.ptbuf.end;++i)
    fprintf(sav,"%d,%d;",wavf2.ptbuf[i].x,wavf2.ptbuf[i].y);
  putc('\n',sav);

  fprintf(sav,"Harmonics\n");
  fprintf(sav,"  freq:%d (%.1fHz)\n",harm_freq->value(),harm->freq);
  fprintf(sav,"  clip:%d\n",harm_clip->value());
  fprintf(sav,"  rand:%d\n",randomize->value());
  fprintf(sav,"  add_dist:%d\n",harm_dist->value());
  fprintf(sav,"  n_mode:%d\n",harm_n_mode->value());
  fprintf(sav,"  harm:");
  for (int i=1;i<harm_max;++i) {
    HLine *hl=&harm->lines[i];
    if (hl->ampl) fprintf(sav,"%d,%d,%.2f;",i,hl->act_ampl,hl->offset);
  }
  putc('\n',sav);
  fclose(sav);
}

void wave_out_cmd(Button* but) {
  if (init_dump_wav("out.wav",2,SAMPLE_RATE)) {
    write_wave=true;
    but->style.param=4;  // rose background
    but->draw_blit_upd();
  }
}

void freqs_cmd(HVSlider *sl,int fire,bool rel) {
   const float
     C=SAMPLE_RATE/float(lrint(SAMPLE_RATE/260)), // middle C, 259.412Hz
     G=SAMPLE_RATE/float(lrint(SAMPLE_RATE/(260*1.5))),
     freqs[10]={ C/4.f, G/4.f, C/2.f, G/2.f, C, G, C*2.f, G*2.f, C*4.f, G*4.f };
   static int xval,yval;
   if (xval!=sl->value().x) {
     xval=sl->value().x;
     float freq=freqs[xval];
     set_text(sl->text_x,"%.0fHz/%.0fHz",freq,freqs[yval]);
     if (waves) waves->freq1=freq;
   }
   if (yval!=sl->value().y) {
     yval=sl->value().y;
     float freq=freqs[yval];
     set_text(sl->text_x,"%.0fHz/%.0fHz",freqs[xval],freq);
     if (waves) waves->freq2=freq;
   }
}

void amval_cmd(HSlider *sl,int fire,bool rel) {
   float amval[6]={  0.,0.1,0.2,0.5,1. ,2. },
         ambias[6]={ 1.,1. ,1. ,0.7,0.5,0. };
   set_text(sl->text,"%.1f",amval[sl->value()]);
   if (waves) {
     waves->am_val=amval[sl->value()];
     waves->am_bias=ambias[sl->value()];
   }
}

void fmval_cmd(HSlider* sl,int fire,bool rel) {
   float fmval[10]={ 0.,0.1,0.2,0.3,0.5,0.7,1.,1.5,2.,3. };
   set_text(sl->text,"%.1f",fmval[sl->value()]);
   if (waves)
     waves->fm_val=fmval[sl->value()];
}

void mass_spring_cmd(HSlider* sl,int fire,bool rel) { // val 0 - 7
  static const float m[8]={ 0.3,0.4,0.6,0.8,1.2,1.6,2.4,3.2 };
  float fval=m[sl->value()];
  com::mass=4./fval;
  if (bouncy)
    for (int i=0;i<com::NN;++i) bouncy->nodes[i].mass=phys_model->nodes[i].mass=com::mass;
  com::springconstant=com::nom_springconst*fval;
  set_text(sl->text,"%.1f",fval);
}

void asym_cmd(HSlider *sl,int fire,bool rel) { // val 0 - 8
  static const float as[9]={ 1/1.99, 1/1.5, 0.98, 0.99, 1., 1.01, 1.02, 1.5, 1.99 };
  com::asym = as[sl->value()];
  set_text(sl->text,"%.2f",as[sl->value()]);
}

void topw_disp() {
  top_win->clear(rp(0,0,300,17),cGrey,false);
  top_win->draw_raised(rp(0,17,top_win->tw_area.w,top_win->tw_area.h-17),top_win->bgcol,true);
  top_win->border(scope_win);
}

void bouncy_disp(int) {
  if (task!=eRunBouncy) return;
  int i;
  bgw3->clear();
  SDL_Color sdlc=sdl_col(0xff);
  SDL_SetRenderDrawColor(bgw3->render.render,sdlc.r,sdlc.g,sdlc.b,0xff);
  for (i = 0; i < com::NN-1; i++) {
    Spring *s = bouncy->springs+i;
    SDL_RenderDrawLine(bgw3->render.render, s->a->d_x, s->a->d_y, s->b->d_x, s->b->d_y);
  }
  for (i = 0; i < com::NN; i++) {
    Node *n = bouncy->nodes+i;
    if (n->fixed)
      boxColor(bgw3->render,n->d_x-5,n->d_y-10,n->d_x+5,n->d_y+10,0x00ff00ff);
    else {
      int rrx=int(sqrt(8 * n->mass/com::asym)),
          rry=int(sqrt(8 * n->mass*com::asym));
      filledEllipseColor(bgw3->render,n->d_x,n->d_y,rrx,rry,0xff6060ff);
    }
    if (i==com::pickup)
      filledCircleColor(bgw3->render,n->d_x,n->d_y,2,0xff);
  }
  bgw3->blit_upd();
};

int bouncy_threadfun(void* data) {
  while (task==eRunBouncy) {
    SDL_Delay(50);
    if (!freeze->value())
      bouncy->eval_model();
    send_uev(bouncy_disp,0);
  }
  return 0;
}

void hide(int what) {
  switch (what) {
    case 'wavs':
      if (i_am_playing) { // audio_thread is running?
        i_am_playing=eDecaying;
        SDL_WaitThread(audio_thread,0);
        i_am_playing=0;
        SDL_CondSignal(stop);
      }
      bgw1->hide(); bgw2->hide(); waves_ctr->hide();
      break;
    case 'bncy':
      if (i_am_playing) {
        i_am_playing=0;
        SDL_CondSignal(stop);
        SDL_WaitThread(audio_thread,0);
      }
      if (task==eRunBouncy) {
        task=0;
        SDL_WaitThread(bouncy->bouncy_thread,0);
      }
      bgw3->hide(); bouncy_ctr->hide();
      break;
    case 'harm':
      if (i_am_playing) { // audio_thread running?
        i_am_playing=0;
        SDL_CondSignal(stop);
        SDL_WaitThread(audio_thread,0);
      }
      bgw4->hide(); harm_ctr->hide();
      break;
    case 'filt':
      bgw5->hide(); filt_ctr->hide();
      break;
  }
  scope_win->clear();
  if (sdl_running) {
    play_but->label.draw_cmd=right_arrow; // reset play_but->label
    play_but->draw_blit_upd();
  }
}

void extb_cmd(RExtButton* rb,bool is_act) { 
  if (!is_act)
    return;
  switch (rb->id) {
    case 'wavs':
      if (task==eRunWaves) break;
      hide('bncy'); hide('harm'); hide('filt');
      task=eRunWaves;
      bgw1->show(); bgw2->show(); waves_ctr->show();
      break;
    case 'bncy':
      if (task==eRunBouncy) break;
      hide('wavs'); hide('harm'); hide('filt');
      task=eRunBouncy;
      bgw3->show(); bouncy_ctr->show();
      scope.display1=scope.display2=true;
      bouncy->bouncy_thread=SDL_CreateThread(bouncy_threadfun,"bouncy_threadfun",0);
      if (!phys_model) phys_model=new PhysModel();
      break;
    case 'harm':
      if (task==eRunHarmonics) break;
      hide('wavs'); hide('bncy'); hide('filt');
      task=eRunHarmonics;
      scope.display1=true; scope.display2=false;
      bgw4->show(); harm_ctr->show();
      break;
    case 'filt':
      if (task==eRunFilters) break;
      hide('wavs'); hide('bncy'); hide('harm');
      task=eRunFilters;
      scope.display1=true; scope.display2=false;
      bgw5->show(); filt_ctr->show();
      break;
  }
}

void draw_bgw4(BgrWin *wb) {
  harm->print_info();
  wb->clear();
  for (int i=1;i<harm_max;++i) harm->draw_line(i);
}

void draw_bgw5(BgrWin *bg) {
  bg->hide(); filt_ctr->hide();
}

Waves::Waves():
    freq1(260.),freq2(1.5*freq1),
    fm_val(0.7),am_val(0.5),am_bias(1.),ab_det(1.),
    ind_f1(0),ind_f2(0.),ampl(0.),fmult(float(bg_w)/SAMPLE_RATE) {
  bgw1=new BgrWin(top_win, Rect(10,40,bg_w,bg_h*2),"A",we::draw_bgw1,we::down,we::move,we::up,cWhite);
  bgw2=new BgrWin(top_win, Rect(150,40,bg_w,bg_h*2),"B",we::draw_bgw2,we::down,we::move,we::up,cWhite);
  waves_ctr=new BgrWin(top_win, Rect(50,150,240,160),0,bgr_clear,0,0,0,cForeground);
  freqs_sl=new HVSlider(waves_ctr,1,Rect(10,20,86,92),Int2(9,9),"A/B frequency",freqs_cmd);
  freqs_sl->set_hvsval(Int2(4,5),1,false);
  am_amount=new HSlider(waves_ctr,1,Rect(10,130,60,0),5,"AM amount",
    [](HSlider *sl,int fire,bool rel) {
      float amval[6]={  0.,0.1,0.2,0.5,1. ,2. },
            ambias[6]={ 1.,1. ,1. ,0.7,0.5,0. };
      set_text(sl->text,"%.1f",amval[sl->value()]);
      if (waves) {
        waves->am_val=amval[sl->value()];
        waves->am_bias=ambias[sl->value()];
      }
    }
  );
  am_amount->hidden=true;
  am_amount->set_hsval(3,1,false);
  fm_amount=new HSlider(waves_ctr,1,Rect(10,130,80,0),9,"FM amount",
    [](HSlider* sl,int fire,bool rel) {
      float fmval[10]={ 0.,0.1,0.2,0.3,0.5,0.7,1.,1.5,2.,3. };
      set_text(sl->text,"%.1f",fmval[sl->value()]);
      if (waves)
        waves->fm_val=fmval[sl->value()];
    });
  fm_amount->hidden=true;
  fm_amount->set_hsval(5,1,false);
  ab_detune=new HSlider(waves_ctr,1,Rect(100,130,60,0),5,"A/B detune",
    [](HSlider* sl,int fire,bool rel) {
      static const float det[6]={ 1,1.003,1.005,1.007,1.01,1.02 };
      set_text(sl->text,"%.3f",det[sl->value()]);
      if (waves) waves->ab_det=det[sl->value()];
    }
  );
  mode_select=new RButtons(waves_ctr,0,Rect(135,20,90,6*TDIST+2),"Mode",false,waves_mode_cmd);
  mode_select->add_rbut("A");
  mode_select->add_rbut("B");
  mode_select->add_rbut("A and B");
  mode_select->add_rbut("B mods A, AM");
  mode_select->add_rbut("B mods A, FM");
  mode_select->add_rbut("B triggers A");
  mode_select->set_rbutnr(0,1,false);
}

void bouncy_down(BgrWin *bgw,int x,int y,int but) {
  int i,
      nearest = -1;
  bouncy->down=false;
  bouncy->selnode=0;
  for (i = 1; i < com::NN; i++) {
    Node *n = bouncy->nodes+i;
    int dx = n->d_x - x, dy = n->d_y - y;
    if (sqrt((dx * dx) + (dy * dy)) < com::radius) {
      nearest = i;
      break;
    }
  }
  if (nearest>=0) {
    bouncy->selnode=bouncy->nodes+nearest;
    phys_model->selnode=phys_model->nodes+nearest;
  }
  else
    bouncy->selnode=phys_model->selnode=0;
  switch (but) {
  case SDL_BUTTON_LEFT:
    if (nearest>=0) {
      bouncy->down = true;
      SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
    }
    break;
  case SDL_BUTTON_MIDDLE:
    if (nearest>=0) {
      bool fixed=bouncy->nodes[nearest].fixed;
      phys_model->nodes[nearest].fixed=bouncy->nodes[nearest].fixed= !fixed;
    }
    break;
  }
}

void bouncy_moved(BgrWin *bgw,int x,int y,int but) {
  if (bouncy->down && bouncy->selnode) {
    bouncy->selnode->d_x = x;
    bouncy->selnode->d_y = y;
  }
}


void bouncy_up(BgrWin *bgw,int x,int y,int but) {
  if (bouncy->selnode) {
    smooth::phase=1;
    if (bouncy_in->value()==1) // noise burst?
      bouncy->phase=phys_model->phase=1;
  }
  bouncy->down=false;
  SDL_EventState(SDL_MOUSEMOTION,SDL_IGNORE);
}

Bouncy::Bouncy():
    down(false) {
  bgw3=new BgrWin(top_win,Rect(10,40,bncy_w,bg_h*2),"masses, springs",0,bouncy_down,bouncy_moved,bouncy_up,cWhite);
  bouncy_ctr=new BgrWin(top_win,Rect(50,150,240,154),0,bgr_clear,0,0,0,cForeground);
  node_mass=new HSlider(bouncy_ctr,1,Rect(8,20,100,0),7,"spring/mass",mass_spring_cmd);
  node_mass->set_hsval(3,1,false);
  asym_xy=new HSlider(bouncy_ctr,1,Rect(8,58,100,0),8,"x/y asymmetry",
    [](HSlider *sl,int fire,bool rel) { // val 0 - 8
      static const float as[9]={ 1/1.99, 1/1.5, 0.98, 0.99, 1., 1.01, 1.02, 1.5, 1.99 };
      com::asym = as[sl->value()];
      set_text(sl->text,"%.2f",as[sl->value()]);
    }
  );
  asym_xy->set_hsval(4,1,false);
  bouncy_in=new RButtons(bouncy_ctr,1,Rect(10,92,120,4*TDIST),"Input",false,0);
  bouncy_in->add_rbut("mouse");
  bouncy_in->add_rbut("mouse, noise burst");
  bouncy_in->add_rbut("pink noise");
  bouncy_in->add_rbut("feedback");
  freeze=new CheckBox(bouncy_ctr,0,Rect(122,6,0,14),"freeze",
    [](CheckBox *chb) {
      for (int i=0;i<com::NN;++i) {
        phys_model->nodes[i].d_x=bouncy->nodes[i].d_x;
        phys_model->nodes[i].d_y=bouncy->nodes[i].d_y;
      }
    }
  );
  low_frict=new CheckBox(bouncy_ctr,0,Rect(122,22,0,14),"low friction",
    [](CheckBox *chb) {
      bouncy->friction= chb->value() ? phys_model->friction : com::nom_friction;
    }
  );
  open_ended=new CheckBox(bouncy_ctr,0,Rect(122,38,0,14),"open ended",
    [](CheckBox *chb) {
      Node &bn=bouncy->nodes[com::NN-1],
      &pmn=phys_model->nodes[com::NN-1];
      if (chb->value()) {
        com::mode=eStringOEnd;
        com::pickup=com::NN-2;
        bn.fixed=pmn.fixed=false;
      }
      else {
        com::mode=eString;
        com::pickup=com::NN/3;
        bn.fixed=pmn.fixed=true;
      }
    }
  );
  non_lin_down=new CheckBox(bouncy_ctr,0,Rect(122,54,0,14),"non-linear",0);
}

void harm_down(BgrWin*,int x,int y,int but) {
  int amp,
      ind=harm->x2ind(x);
  HLine *lin;
  if (ind>0) {
    switch (but) {
      case SDL_BUTTON_LEFT:
        amp=2*bg_h-y;
        lin=&harm->lines[ind];
        if (amp<5) {
          lin->ampl=lin->act_ampl=0;
          lin->sel=false;
          lin->offset=0;
        }
        else
          lin->ampl=lin->act_ampl=amp;
        if (lin->info)
          harm->print_info();
        harm->draw_line(ind);
        break;
      case SDL_BUTTON_MIDDLE:
        harm->lines[harm->info_lnr].info=false;
        harm->draw_line(harm->info_lnr);
        harm->info_lnr=ind;
        harm->lines[ind].info=true;
        harm->draw_line(ind);
        harm->print_info();
        break;
      case SDL_BUTTON_RIGHT:
        if (harm->lines[ind].ampl) {
          harm->lines[ind].sel=!harm->lines[ind].sel;
          harm->draw_line(ind);
        }
        break;
    }
  }
}

Harmonics::Harmonics():
    info_lnr(0),
    freq(260.),
    ampl(0.),
    aa(1.),
    fmult(float(Sinus::dim)/SAMPLE_RATE),
    ns_fmult(float(NoisySinus::nr_pts)/SAMPLE_RATE) {
  bgw4=new BgrWin(top_win,Rect(10,40,harm_w,bg_h*2),"harmonics",draw_bgw4,harm_down,0,0,cWhite);
  harm_ctr=new BgrWin(top_win,Rect(50,150,240,160),0,bgr_clear,0,0,0,cForeground);
  harm_freq=new HSlider(harm_ctr,1,Rect(10,20,100,0),11,"frequency",
    [](HSlider* sl,int fire,bool rel) {
      static const float fr[12]={ 20,30,40,60,80,120,160,240,320,480,640,960 };
      if (sdl_running)
        harm->freq=fr[sl->value()];
      set_text(sl->text,"%.0f",fr[sl->value()]);
      if (rel) harm->print_info();
    },true
  );
  harm_freq->set_hsval(7,1,false);
  harm_offset=new HSlider(harm_ctr,1,Rect(10,58,100,0),12,"sel: freq offset",
    [](HSlider* sl,int fire,bool rel) {
      static const float off_arr[13]= { -0.4, -0.2, -0.1, -0.04, -0.02, -0.01, 0, 0.01, 0.02, 0.04, 0.1, 0.2, 0.4 };
      float val=off_arr[sl->value()];
      if (sdl_running)
        for (int i=0;i<harm_max;++i) {
          if (harm->lines[i].sel) {
            harm->lines[i].offset=val;
            if (rel && harm->lines[i].info) harm->print_info();
          }
        }
      if (sl->value()==6) set_text(sl->text,"0");
      else set_text(sl->text,"%0.2f",val);
    },true
  );
  harm_offset->set_hsval(6,1,false);
  ampl_mult=new HSlider(harm_ctr,1,Rect(10,96,100,0),8,"sel: ampl mult",
    [](HSlider* sl,int fire,bool rel) {
      static const float ampl_mult_arr[9]={ 0.2,0.4,0.6,0.8,1,1.2,1.5,2,3 };
      if (sdl_running)
        for (int i=0;i<harm_max;++i) {
          if (harm->lines[i].sel) {
            harm->lines[i].act_ampl=min(int(harm->lines[i].ampl * ampl_mult_arr[sl->value()]), bg_h*2);
            harm->draw_line(i);
            if (rel && harm->lines[i].info) harm->print_info();
          }
        }
      set_text(sl->text,"%0.1f",ampl_mult_arr[sl->value()]);
    },true
  );
  ampl_mult->set_hsval(4,1,false);
  harm_dist=new HSlider(harm_ctr,1,Rect(10,132,60,0),4,"distorsion",
    [](HSlider* sl,int fire,bool rel) {
      static const float a_arr[5]={ 1.,1.2,1.5,2,3 };
      harm->aa=a_arr[sl->value()];
      if (sl->value()) set_text(sl->text,"%0.2f",harm->aa-1.);
      else sl->text=0;
    }
  );
  harm_n_mode=new HSlider(harm_ctr,1,Rect(80,132,74,0),6,"noise mode",
    [](HSlider* sl,int fire,bool rel) {
      lfnoise::set_mode(sl->value());
      set_text(sl->text,"%s",lfnoise::n_mname);
    }
  );
  harm_clip=new HSlider(harm_ctr,1,Rect(164,132,60,0),4,"soft clipping",
    [](HSlider* sl,int fire,bool rel) {
      static float lim[5]={ 0,2,1,0.7,0.5 };
      float val=lim[sl->value()];
      if (harm) harm->sclip_limit=val;
      if (sl->value()) set_text(sl->text,"%.2f",val);
      else sl->text=0;
    }
  );
  harm_clip->set_hsval(0,0,false);
  harm_info=new TextWin(harm_ctr,1,Rect(120,20,100,2*TDIST+4),4,"info");
  harm_info->bgcol=top_win->bgcol;
  randomize=new CheckBox(harm_ctr,0,Rect(120,66,0,14),"randomized freq",
    [](CheckBox* chb) {
      if (chb->value())
        for (int i=1;i<harm_max;++i)
          harm->lines[i].freq=i * (1.+(float(rand())/RAND_MAX-0.5)/100.);
      else
        for (int i=1;i<harm_max;++i)
          harm->lines[i].freq=i;
      if (debug) {
        printf("harm:");
        for (int i=1;i<harm_max;++i) printf(" %.2f",harm->lines[i].freq);
        putchar('\n');
      }
    }
  );
  no_clip_harm=new CheckBox(harm_ctr,0,Rect(120,84,0,14),"only clip base freq",0);

  for (int i=1;i<4;++i) lines[i].ampl=lines[i].act_ampl=bg_h;
  for (int i=1;i<harm_max;++i) lines[i].freq=i;
  lines[1].info=true;
  info_lnr=1;
}

void FilterTest::draw_fresponse() {
  bgw5->clear();
  if (!the_filter) return;
  updating=true;
  const int
    wid=bgw5->tw_area.w;
  int
    freq,
    tfer,
    xpos=1,
    delta_f=range * 2 / wid,
    x_nyq=0;
  float 
    tIn, tOut,
    sIn, sOut;
  const float
    mult_f=1.04;
  bool
    log_f=log_f_scale->value(); // log freq scale?
  the_filter->init(filt->cutoff,filt_qfactor->value(),&filt_qfactor->text);
  for (freq=log_f ? range/10 : 0;
       xpos < wid && freq<2*44100;
       freq=log_f ? freq*mult_f : freq+delta_f,xpos+=2) {
    tIn=tOut=0;
    for (int spos=0;spos<TESTSAMPLES;++spos) {
      sIn  = freq==0 ? 0.1 : sin((2 * M_PI * spos * freq) / SAMPLE_RATE);
      sOut = the_filter->getSample(sIn);
      if (fabs(sOut)>50.) { alert("filter out = %.1f",sOut); updating=false; return; }
      tIn+=fabs(sIn);
      tOut+=fabs(sOut);
    }
    if (log_a_scale->value())
      tfer=max(0,int(30*log(tOut / tIn))+20);
    else
      tfer=int(20. * tOut / tIn);
    if (tfer)
      vlineColor(bgw5->render,xpos,max(0,bgw5->tw_area.h-tfer-1),bgw5->tw_area.h-1,freq>range ? 0x808080ff : 0xff);
    if (!x_nyq && freq>22050.) x_nyq=xpos;
  }
  if (x_nyq && x_nyq<wid)
    vlineColor(bgw5->render,x_nyq,0,10,0xff0000ff);
  bgw5->blit_upd();
  updating=false;
}

void test_filt_cmd(RButtons*,int nr,int fire) {
  filt->updating=true;
  delete filt->the_filter;
  filt->the_filter=0;
  switch (nr) {
    case 0: filt->the_filter=new Filter2(); break;
    case 1: filt->the_filter=new Biquad(); break;
    case 2: filt->the_filter=new PinkNoise(); break;
    case 3: filt->the_filter=new ThreePoint(); break;
    case 4: filt->the_filter=new MoogVCF(); break;
  }
  filt_mode->set_hsval(0,2); // will call draw_fresponse()
}

FilterTest::FilterTest():
    the_filter(0),
    range(10000),
    pos(0),
    ampl(0.),val(0),
    updating(false),
    fq(0.6),
    cutoff(3200) {
  bgw5=new BgrWin(top_win,Rect(10,40,filt_w,filt_h),"amplitude",draw_bgw5,0,0,0,cWhite);
  filt_ctr=new BgrWin(top_win,Rect(50,150,240,160),0,bgr_clear,0,0,0,cForeground);
  test_filter=new RButtons(filt_ctr,0,Rect(10,18,120,5.5*TDIST),"filter",true,test_filt_cmd);
  test_filter->add_rbut("state-var 24dB/oct");
  test_filter->add_rbut("biquad 12dB/oct");
  test_filter->add_rbut("lowpass 3dB/oct");
  test_filter->add_rbut("2,3,4-point");
  test_filter->add_rbut("Moog 24dB/oct");
  test_filter->set_rbutnr(0,0,false);
  do_change=new CheckBox(filt_ctr,0,Rect(30,98,0,14),"change param",
    [](CheckBox *chb) {
      filt->draw_fresponse();
    });
  do_change->d=&change_param;
  filt_mode=new HSlider(filt_ctr,1,Rect(10,130,74,0),6,"mode",
    [](HSlider* sl,int fire,bool rel) {
      int fmod=sl->value();
      if (filt) {
        if (!filt->the_filter) { bgw5->clear(); bgw5->blit_upd(); return; }
        const char *mm= filt->the_filter->get_mode(fmod);
        if (mm) {
          set_text(sl->text,mm);
          if (rel || fire==2) {          // fire == 2: from test_filt_cmd()
            filt->the_filter->mode=fmod;
            filt->draw_fresponse();
          }
        }
        else {
          set_text(sl->text,"-");
          bgw5->clear(); bgw5->blit_upd();
        }
      }
    },true
  );
  filt_mode->set_hsval(0,1,false);
  filt_range=new HSlider(filt_ctr,1,Rect(145,18,80,0),4,"range",
    [](HSlider* sl,int fire,bool rel) {
      static int range[]={ 1500,3000,6000,12000,24000 };
      int rng=range[sl->value()];
      if (filt) {
        filt->range=rng;
        if (rel) filt->draw_fresponse();
      }
      if (log_f_scale->value())
        set_text(sl->text,"%.1f-%d KHz",rng/10000.,rng/1000);
      else
        set_text(sl->text,"0-%d KHz",rng/1000);
    },true
  );
  filt_qfactor=new HSlider(filt_ctr,1,Rect(145,58,70,0),5,"filter Q",
    [](HSlider* sl,int fire,bool rel) {
      if (rel && filt) {
        filt->draw_fresponse();
        sl->draw_blit_upd(); // text may have been modified
      }
    },true
  );
  filt_qfactor->set_hsval(2,1,false);
  log_a_scale=new CheckBox(filt_ctr,0,Rect(135,80,0,14),"log ampl scale",
    [](CheckBox *chb) { filt_range->cmd(filt_range,1,true); filt_range->draw_blit_upd(); });
  log_f_scale=new CheckBox(filt_ctr,0,Rect(135,98,0,14),"log freq scale",
    [](CheckBox *chb) { filt_range->cmd(filt_range,1,true); filt_range->draw_blit_upd(); });
  filt_range->set_hsval(2,1,false);
  filt_freq=new HSlider(filt_ctr,1,Rect(92,130,138,0),11,"corner freq",
    [](HSlider* sl,int fire,bool rel) {
      static int ffreq[12]={ 100,200,400,800,1500,3000,6000,8000,10000,15000,18000,21000 };
      int ffr=ffreq[sl->value()];
      if (filt) filt->cutoff=ffr;
      if (rel && filt) filt->draw_fresponse();
      set_text(sl->text,"%d Hz",ffr);
    },true
  );
  filt_freq->set_hsval(5,1,false);
}

void Harmonics::audio_callback(Uint8 *stream, int len) {
  int i,
      har;
  bool noisy=lfnoise::noisy,
       no_clip_h=no_clip_harm->value();
  float val=0;
  Sint16 *buffer=reinterpret_cast<Sint16*>(stream);
  for (i=0;i<len/2;++i)
    buffer[i]=0;
  for (har=1;har<harm_max;++har) {
    if (!lines[har].ampl && har!=1) continue;
    float har1=lines[har].freq*(1.+lines[har].offset/ldist), // shifted harmonic
          add=freq * (noisy ? ns_fmult : fmult) * har1;
    for (i=0;i<len/2;i+=2) {
      if (noisy) {
        lines[har].ind_f+=add;
        val=noisy_sinus.get(lines[har].ind_f) * lines[har].act_ampl;  // can modify lines[i].ind_f
      }
      else {
        lfnoise::update(har);
        float n_val=lfnoise::lfn_buf[har].noise; // range: -1 -> 1
        switch (lfnoise::n_mode) {
          case 0: // noise: off
            lines[har].ind_f+=add;
            val=sinus.get(lines[har].ind_f,aa) * lines[har].act_ampl;
            break;
          case 1: // noise: hf freq mod 
            //lines[har].ind_f += har==1 ? add : add * (1. + n_val * 0.3); // sounds better
            lines[har].ind_f += add * (1. + n_val * 0.3);
            val=sinus.get(lines[har].ind_f,aa) * lines[har].act_ampl;
            break;
          case 2: // noise: lf freq mod 
            lines[har].ind_f += add * (1. + n_val * 0.03);
            val=sinus.get(lines[har].ind_f,aa) * lines[har].act_ampl;
            break;
          case 3: // noise: hf ampl mod
            lines[har].ind_f+=add;
            val=sinus.get(lines[har].ind_f,aa) * lines[har].act_ampl * (n_val + 1.);
            break;
          case 4: // noise: lf ampl mod
            lines[har].ind_f+=add;
            val=sinus.get(lines[har].ind_f,aa) * lines[har].act_ampl * (n_val + 1.);
            break;
          case 5: // noise: distorsion mod
            lines[har].ind_f+=add;
            val=sinus.get(lines[har].ind_f,aa * (1. + n_val * 0.5)) * lines[har].act_ampl;
            break;
        }
      }
      if (no_clip_h && har==1 && harm_clip->value()>0)
        buffer[i] += non_linear(val*10)/10;
      else buffer[i] += val;
    }
  }
  for (i=0;i<len/2;i+=2) {
    if (i_am_playing==ePlaying) { if (ampl<10.) ampl+=0.01; }
    else if (i_am_playing==eDecaying) {
      if (ampl>0.) ampl-=0.0004;
      else {
        i_am_playing=0;
        SDL_CondSignal(stop);
      }
    }
    if (!no_clip_h && harm_clip->value()>0)
      buffer[i]=non_linear(buffer[i] * ampl);
    else
      buffer[i]*=ampl;
    buffer[i+1]=buffer[i];
    scope.update(buffer,i);
  }
  if (write_wave && !dump_wav((char*)buffer,len/2)) write_wave=false;
}

void FilterTest::audio_callback(Uint8 *stream, int len) {
  int i;
  const int period=SAMPLE_RATE/220,   // 200 Hz
            edge=period/50,           // = 4, rising/falling
            flat=period/5;            // uppper flat part of puls
  const float top=1.,
              bot=-0.5;
  Sint16 *buffer=reinterpret_cast<Sint16*>(stream);
  for (i=0;i<len/2;i+=2) {
    if (i_am_playing==ePlaying) { if (ampl<1.) ampl+=0.001; }
    else if (i_am_playing==eDecaying) {
      if (ampl>0.) ampl-=0.00004;
      else {
        i_am_playing=0;
        SDL_CondSignal(stop);
      }
    }
    else if (updating) {
      if (ampl>0.) ampl-=0.001;
    }
    if (pos==0) val=bot;
    else if (pos<edge) val=min(val+(top-bot)/edge,top);
    else if (pos<flat) val=top;
    else if (pos<flat+edge) val=max(val-(top-bot)/edge,bot);
    else val=bot;
    pos=(pos+1)%period;
    if (the_filter && !updating)
      buffer[i]=buffer[i+1]=int(ampl*the_filter->getSample(val)*7000);
    else
      buffer[i]=buffer[i+1]=0;
    scope.update(buffer,i);
  }
  if (write_wave && !dump_wav((char*)buffer,len/2)) write_wave=false;
}

void Harmonics::draw_line(int ind) {
  Rect rect(ind*ldist-ldist/2,0,ldist,2*bg_h);
  bgw4->clear(&rect);
  Uint32 col;
  HLine *lin=&lines[ind];
  static Uint32 cSel=0x707070ff;
  if (lin->ampl) {
    int x=ind*ldist, // int(ind*ldist+lin->offset),
        h=lin->act_ampl;
    col=lin->sel ? cSel : fabs(lin->offset)>0.001f ? 0xffff : 0xff;
    boxColor(bgw4->render,x-1,bg_h*2-h,x+2,bg_h*2,col);
  }
  if (lin->info)
    circleColor(bgw4->render,ind*ldist,3,2,0xf00000ff);
  vlineColor(bgw4->render,ind*ldist-ldist/2,0,bg_h*2,0x70ff70ff); // green
  bgw4->blit_upd(&rect);
}

void Harmonics::print_info() {
  char buf[100],*bp=buf;
  harm_info->reset();
  HLine *lin=&lines[info_lnr];
  bp+=sprintf(bp,"harmonic: %.2f\n",info_lnr+lin->offset);
  bp+=sprintf(bp,"ampl: %.2f",lin->ampl/2./bg_h);
  if (lin->ampl!=lin->act_ampl)
    bp+=sprintf(bp,"* %.1f",float(lin->act_ampl)/lin->ampl);
  harm_info->add_text(buf,sdl_running);
}

bool set_config(FILE *conf) {  // no drawing!
  char buf[20];
  int i,
      n1,n2;
  if (fscanf(conf,"Format:%20s\n",buf)!=1) { alert("Format missing"); return false; }
  if (strcmp(buf,"1.0")) { alert("%s: bad format (expected: 1.0)",buf); return false; }
  while (true) {
    if (1!=fscanf(conf,"%20s\n",buf)) break;
    if (!strcmp(buf,"Waves")) {
      if (1!=fscanf(conf," A freq:%d\n",&n1)) return false;
      if (1!=fscanf(conf," B freq:%d\n",&n2)) return false;
      freqs_sl->set_hvsval(Int2(n1,n2),1,false);
      if (1!=fscanf(conf," mode:%d\n",&n1)) return false;
      mode_select->set_rbutnr(n1,1,false);
      if (1!=fscanf(conf," AM amount:%d\n",&n1)) return false;
      am_amount->set_hsval(n1,1,false);
      if (1!=fscanf(conf," FM amount:%d\n",&n1)) return false;
      fm_amount->set_hsval(n1,1,false);
      if (1!=fscanf(conf," detune:%d\n",&n1)) return false;
      ab_detune->set_hsval(n1,1,false);
      if (0!=fscanf(conf," A:"));
      for (i=0;;++i) {
        if (2!=fscanf(conf,"%d,%d;",&n1,&n2)) return false;
        if (i>=wav_pnt_max) {
          alert("waves A: %d points (should be < %d)",i,wav_pnt_max);
          while (getc(conf)!='\n');
          break;
        }
        wavf1.ptbuf[i].x=n1; wavf1.ptbuf[i].y=n2;
        n1=getc(conf); if (n1=='\n') break; ungetc(n1,conf);
      }
      if (0!=fscanf(conf," B:"));
      for (i=0;;++i) {
        if (2!=fscanf(conf,"%d,%d;",&n1,&n2)) return false;
        wavf2.ptbuf[i].x=n1; wavf2.ptbuf[i].y=n2;
        n1=getc(conf); if (n1=='\n') break; ungetc(n1,conf);
      }
    }
    else if (!strcmp(buf,"Harmonics")) {
      char key[10];
      while (fscanf(conf," %[^:]%*c",key)==1) { // read upto-and-incl ':'
        if (!strcmp(key,"freq")) {
          if (fscanf(conf,"%d (%*s)\n",&n1)!=1) return false;
          harm_freq->set_hsval(n1,1,false);
        }
        else if (!strcmp(key,"clip")) {
          if (fscanf(conf,"%d\n",&n1)!=1) return false;
          harm_clip->set_hsval(n1,1,false);
        }
        else if (!strcmp(key,"add_dist")) {
          if (fscanf(conf,"%d\n",&n1)!=1) return false;
          harm_dist->set_hsval(n1,1,false);
        }
        else if (!strcmp(key,"n_mode")) {
          if (fscanf(conf,"%d\n",&n1)!=1) return false;
          harm_n_mode->set_hsval(n1,1,false);
        }
        else if (!strcmp(key,"rand")) {
          if (fscanf(conf,"%d\n",&n1)!=1) return false;
          randomize->set_cbval(n1,1,false);
        }
        else if (!strcmp(key,"harm")) {
          float f1;
          HLine *hl;
          for (i=0;i<harm_max;++i) {
            hl=&harm->lines[i];
            hl->ampl=hl->act_ampl=0; hl->offset=0;
          }
          for (i=1;;++i) {
            if (fscanf(conf,"%d,%d,%f;",&n1,&n2,&f1)!=3) return false;
            if (n1>=harm_max) {
              alert("harmonic %d (should be < %d)",n1,harm_max);
              while (getc(conf)!='\n');
              break;
            }
            if (n2) {
              hl=&harm->lines[n1];
              hl->ampl=hl->act_ampl=n2; hl->offset=f1;
            }
            n1=getc(conf); if (n1=='\n') break;
            ungetc(n1,conf);
          }
        }
        else { alert("unknown keyword: %s",key); return false; }
      }
    }
  }
  return true;
}

int main(int argc,char** argv) {
  const char* inf=0;
  for (int an=1;an<argc;an++) {
    if (!strcmp(argv[an],"-h")) {
      puts("This is make-waves, an audio test suite using SDL-widgets");
      puts("Usage:");
      puts("  make_waves [<config-file>]");
      puts("  (the config-file can be created by this program)");
      puts("Option:");
      puts("  -db  - extra debug info");
      exit(1);
    }
    if (!strcmp(argv[an],"-db")) debug=true;
    else inf=argv[an];
  }
  top_win=new TopWin("Make Waves",Rect(100,100,300,390),SDL_INIT_AUDIO,0,false,topw_disp);
  top_win->bgcol=cBackground;

  // tabs
  tab_ctr=new ExtRButCtrl(Style(0,int_col(cBackground)),extb_cmd);
  tab_ctr->maybe_z=false;
  RExtButton *wrex=tab_ctr->add_extrbut(top_win,Rect(0,1,60,17),"waves",'wavs'),
             *brex=tab_ctr->add_extrbut(top_win,Rect(64,1,60,17),"bouncy",'bncy'),
             *hrex=tab_ctr->add_extrbut(top_win,Rect(128,1,64,17),"harmonics",'harm'),
             *frex=tab_ctr->add_extrbut(top_win,Rect(196,1,60,17),"filters",'filt');

  play_but=new Button(top_win,0,Rect(10,150,24,20),right_arrow,play_cmd);
  scope_win=new BgrWin(top_win,Rect(10,320,scope_dim,scope_h*2),"sound",bgr_clear,0,0,0,cGrey);
  save=new Button(top_win,1,Rect(145,320,0,0),"save settings (x.mw)",save_cmd);
  wave_out=new Button(top_win,1,Rect(145,340,0,0),"write sound (out.wav)",wave_out_cmd);
  wave_out->hidden=true;

  waves=new Waves();
  bouncy=new Bouncy();
  harm=new Harmonics();
  filt=new FilterTest();
  test_filt_cmd(0,0,2); // needs filt

  const int startup=1;
  switch (startup) {
    case 1: tab_ctr->act_lbut=wrex; tab_ctr->reb_cmd(wrex,true); break; // task = eRunWaves
    case 2: tab_ctr->act_lbut=brex; tab_ctr->reb_cmd(brex,true); break; //        eRunBouncy
    case 3: tab_ctr->act_lbut=hrex; tab_ctr->reb_cmd(hrex,true); break; //        eHarmonics
    case 4: tab_ctr->act_lbut=frex; tab_ctr->reb_cmd(frex,true); break; //        eFilters
  }
  if (inf) {
    FILE *config=fopen(inf,"r");
    if (!config) alert("config file %s not opened",inf);
    else {
      if (!set_config(config)) {
        alert("problems reading %s",inf);
        fclose(config);
      }
      else
        fclose(config);
    }
  }
  get_events();
  return 0;
}
