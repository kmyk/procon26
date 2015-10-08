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
#include <string.h>
#include <math.h>
#include "../sdl-widgets.h"
#include "../make-waves/dump-wave.h"
#include "templates.h"

static TopWin *top_win;

static BgrWin
  *bview,
  *infoview,
  *scoreview,
  *scope_win,
  *pan_view,  // panel
  *tfer_curve;
static TextWin
  *messages;
static RButtons
  *voice_col,
  *tfer_mode;
static CheckBox
  *freeze,
  *ignore,
  *write_wav;
static Button
  *play_but,
  *unselect,
  *del_sel,
  *recol_sel;
static ExtRButCtrl
  *mode_ctr,
  *tab_ctr,
  *scv_edit_ctr;
static RExtButton
  *eb_sved,
  *eb_ssel,
  *eb_mmas,
  *eb_edma,
  *eb_edsp;
static HScrollbar *sv_scroll;
static HSlider *tempo;
static Lamp *wav_indic;
static Message
  *txtmes1,*txtmes2,*txtmes3;
static DialogWin *dialog;

enum {
  ePlayMode=1, eEditMass, eEditSpring,
  eWavOutStart, eWavRecording, eWavOutNoAudio,
  eEndReached, eStopAudio,
  eNote, ePause, eEnd,  // note category
  eIdle, eTracking, eErasing, eTrackingCol, eErasingCol, eCollectSel, eCollectSel1,  // mouse states
  eMoving, eCopying, eScroll,
  eEdit, eSelect      // scoreview edit mode
};

enum { ePlay, eSilent }; // ScSection

static const int
  bg_w=300,
  bg_h=190,
  SAMPLE_RATE=44100,
  scope_dim=128,
  scope_h=30,
  tfer_h=20,  // transfer curve
  tfer_w=25,
  NN=50,  // nr nodes
  NS=50,  // nr springs
  sclin_max=60, // score lines
  sect_len=8,   // ScSection
  sclin_dist=5,
  scv_yoff=3,   // y offset of score lines
  nom_snr_max=44,
  scv_w=nom_snr_max*sect_len, // scoreview width
  scv_h=(sclin_max-1)*sclin_dist+2*scv_yoff,  // scoreview height
  voice_max=6,
  meter=8,
  tempo_offset=6;

static const float
  nominal_friction=0.9,
  radius=10.,
  nom_mass=6.4,
  nominal_springconst=0.05,
  mid_C=261;

static float
  springconstant=nominal_springconst;

static char
  mode=ePlayMode,  // bouncy window
  scv_mode=eEdit,  // scoreview edit mode
  wav_mode, // eWavRecording, eWavOutNoAudio, eWavOutStart
  stop_req; // eEndReached, eStopAudio

static bool
  debug,
  i_am_playing,
  play_tune,
  one_col;  // show only act instr

enum { eBlack_x, eBlack_y, eBlue_x, eBlue_y, eGreen_x, eGreen_y };
enum { eBlack, eBlue, eGreen };  // instruments
static const char *col_name[6]={ "black-X","black-Y","blue-X","blue-Y","green-X","green-Y" };
static const Uint32 col2color[voice_max]={ 0xff,0x707070ff,0xffff,0x7070ffff,0xc000ff,0x50ff50ff };
enum {  // panel sliders etc
  ePizz,eNonl,ePaph,ePnoi,eTun,eAsym,eFric,eLstr,eExl,eAmpl,eTfer,
  ctrl_max // = array length
};
static const char
  *slider_tag_name[ctrl_max]={ "pizz","nonl","paph","pnoi","tun","asym","fric","lstr","exl","ampl","tfer" },
  *config_file=0;
static int
  scope_buf1[scope_dim],scope_buf2[scope_dim],
  act_color=eBlack_x,
  leftside=0,      // scoreview
  cur_leftside,    // set when mouse down
  act_snr,
  act_instr=0;

static SDL_Color
  cMesBg=sdl_col(0xf0f0a0ff),
  cAlert=sdl_col(0xffa0a0ff);

static void audio_callback(Uint8 *stream, int len);

static SDL_Thread
  *bouncy_thread,
  *audio_thread,
  *wav_thread;

//static SDL_mutex *audio_mtx=SDL_CreateMutex();

static WinBase *CONT[ctrl_max]; // panel sliders and checkboxes

static int min(int a, int b) { return a>b ? b : a; }
static int max(int a, int b) { return a<b ? b : a; }
static int minmax(int a, int x, int b) { return x>b ? b : x<a ? a : x; }

struct Node {
  float d_x,nom_x,
        d_y,nom_y,
        vx,
        vy,
        ax,
        ay,
        mass;
  bool fixed;
  int spr_end;
  struct Spring *spr[10];
  Node():
    mass(nom_mass),
    spr_end(-1) {
  }
  void set(float x,float y,float m,bool fix) {
    d_x=d_y=0;
    nom_x=x; nom_y=y; vx=vy=0; mass=m; fixed=fix;
  }
  void connect(struct Spring *sp) { 
    if (spr_end==9) alert("too much springs");
    else spr[++spr_end]=sp;
  }
  void reset() {
    spr_end=-1;
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
  bool weak;
  void set(Node *_a, Node *_b) {
    a=_a; b=_b;
    a->connect(this); b->connect(this);
    weak=false;
  }
};

struct Noise {
  static const int ndim=100000;
  short pink_buf[ndim];
  float nval() { return (rand() & 0xff)-127; }
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
  }
} noise;

struct Note {
  float freq;
  int dur,
      start_snr;
  char cat; // eNote, ePause, eEnd
  void set(float f,int d,int c,int snr) {
    freq=f; dur=d; cat=c; start_snr=snr;
  }
};

struct NoteBuffer {
  Note *notes,
       *cur_note;
  int busy,     // note duration
      note_lnr, // line nr of a note
      note_snr, // start of a note
      nlen,     // dimension of notes[]
      end_snr;  // ending snr of last note
  bool begin;
  float the_freq;
  NoteBuffer():
      notes(new Note[nom_snr_max]),
      nlen(nom_snr_max) {
    reset();
  }
  void reset() {
    cur_note=0;  // notes[] not freed
    busy=0;
    end_snr=0;
    note_snr=-1;
    the_freq=mid_C;
    begin=true;
  }
  void omit_sect(int lnr,int snr,int col);
  void add_note(float freq,int fst_snr,int lst_snr) {
    const int mult=60000;
    if (debug) printf("add_note: f=%.1f,snr=%d,lst=%d (cur_n=%p)\n",freq,fst_snr,lst_snr,cur_note);
    if (!cur_note) { 
      cur_note=notes;
      if (fst_snr>leftside) {
        cur_note->set(0,(fst_snr-leftside)*mult,ePause,0);
        ++cur_note;
      }
      cur_note->set(freq,(lst_snr-fst_snr+1)*mult,eNote,fst_snr);
    }
    else {
      if (cur_note-notes>=nlen-2) { // room for 2 notes
        int old_len=nlen;
        notes=re_alloc(notes,nlen);
        cur_note=notes+old_len;
      }
      if (fst_snr>end_snr+1) {
        ++cur_note;
        cur_note->set(0,(fst_snr-end_snr-1)*mult,ePause,0);
      }
      ++cur_note;
      cur_note->set(freq,(lst_snr-fst_snr+1)*mult,eNote,fst_snr);
    }
    end_snr=lst_snr;
  }
};

namespace freq {
const float
  A=440, Bes=466.2, B=493.9, C=523.3, Cis=554.4, D=587.3,
  Dis=622.3, E=659.3, F=698.5, Fis=740.0, G=784.0, Gis=830.6,
  freqs[sclin_max]= {
                                                      D*2 ,Cis*2 ,C*2,
    B*2,Bes*2,A*2,Gis   ,G   ,Fis   ,F   ,E   ,Dis   ,D   ,Cis   ,C  ,
    B  ,Bes  ,A  ,Gis/2 ,G/2 ,Fis/2 ,F/2 ,E/2 ,Dis/2 ,D/2 ,Cis/2 ,C/2,
    B/2,Bes/2,A/2,Gis/4 ,G/4 ,Fis/4 ,F/4 ,E/4 ,Dis/4 ,D/4 ,Cis/4 ,C/4,
    B/4,Bes/4,A/4,Gis/8 ,G/8 ,Fis/8 ,F/8 ,E/8 ,Dis/8 ,D/8 ,Cis/8 ,C/8,
    B/8,Bes/8,A/8,Gis/16,G/16,Fis/16,F/16,E/16,Dis/16
  };
}

struct NoteBuffers {
  NoteBuffer nbuf[voice_max];
  NoteBuffers() { reset(); }
  void reset() {
    for (int i=0;i<voice_max;i++) nbuf[i].reset();
  }
  bool fill_note_bufs();
  void add_note(int col,int lnr,int fst_snr,int lst_snr) {
    nbuf[col].add_note(freq::freqs[lnr],fst_snr,lst_snr);
  }
  void report(int v) {
    NoteBuffer *nb=nbuf+v;
    if (nb->notes) {
      printf("voice %d:\n",v);
      for (Note *n=nb->notes;n-nb->notes<=nb->nlen;++n) {
        printf("  { f=%.1f,dur=%d,cat=%s }\n",n->freq,n->dur,
          n->cat==eNote ? "Note" : n->cat==ePause ? "Pause" : n->cat==eEnd ? "End" : "?");
        if (n->cat==eEnd) break;
      }
    }
  }
} nbufs;

struct Mass_Spring { // base class of Bouncy and PhysModel
  Node nodes[NN];
  Spring springs[NS];
  int
    n_ind;
  float
    friction,
    asym,
    tuning,
    low_sprc;
  Mass_Spring(struct Instrument*);
  void eval_model(struct Instrument*,NoteBuffer**,float freq_x,float freq_y);
  void reconnect();
};

namespace scope {
  int pos,
      scope_start;
  void draw_curve(Render rend,Rect *r,int *buf,int dim,Uint32 color) {
    SDL_Color sdlc=sdl_col(color);
    SDL_SetRenderDrawColor(rend.render,sdlc.r,sdlc.g,sdlc.b,0xff);
    SDL_Point pnt[dim];
    for (int i=0;i<dim;++i) { pnt[i].x=i+r->x; pnt[i].y=buf[i]+r->y; }
    SDL_RenderDrawLines(rend.render,pnt,dim);
  }
  void update(Sint16 *buffer,int i) {  // thread-safe
    if (scope_start<0) return;
    if (scope_start==0) {  // set by send_uev()
      if (i>=2 && ((buffer[i-2]<=0 && buffer[i]>0) || (buffer[i-1]<=0 && buffer[i+1]>0))) {
        scope_start=1;
        pos=0;
      }
      return;
    }
    if (i%8==0) {
      if (pos<scope_dim) {
        static const int div=32000/scope_h;
        scope_buf1[pos]=buffer[i]/div + scope_h;
        scope_buf2[pos]=buffer[i+1]/div + scope_h;
        ++pos;
      }
      else {
        scope_start=-1;
        send_uev([](int){
          Rect *sr=&scope_win->tw_area;
          top_win->clear(sr,scope_win->bgcol,false);
          draw_curve(top_win->render,sr,scope_buf1,scope_dim,0xa000ff); // green
          draw_curve(top_win->render,sr,scope_buf2,scope_dim,0xd0ff); // blue
          scope_win->upd();
          scope_start=0;
        },0);
      }
    }
  }
}

struct Bouncy:Mass_Spring {
  int down,
      selected;
  Node *selnode;
  Bouncy(struct Instrument*);
};

struct PhysModel:Mass_Spring {
  int damp_cnt;
  PhysModel(struct Instrument* inst):
      Mass_Spring(inst) {
    friction=1.-(1.-nominal_friction)/2200.;  // 44100 / 20 = 2205
  }
};

struct Instrument {
  int pickup_left,pickup_right,
      excite_mass,   // the excited mass
      tf_mode,
      active_mass,   // the selected mass
      active_spring, // the selected spring
      excit,         // excitation value
      Nend,
      Send;
  float ampl;
  Bouncy *bouncy;
  PhysModel *phys_model;
  struct Panel *inst_panel;
  Instrument():
      pickup_left(1),pickup_right(2),
      excite_mass(8),
      tf_mode(0),
      active_mass(-1),
      active_spring(-1),
      excit(30),
      Nend(10),
      Send(9),
      ampl(1.),
      bouncy(new Bouncy(this)),
      phys_model(new PhysModel(this)),
      inst_panel(0) {
  }
  void draw();
  void delete_mass();
  void delete_spring();
  void reloc_pickup(int &pickup);
} *INST[voice_max/2];

struct ScSection {
  Uint32 buf;
  ScSection():buf(0) { }
  Uint8 get_c(int col) {
    return (buf >> (col*4)) & 0xf;
  }
  void set(int col,bool play,bool stacc) {
    buf &= ~(0xf << (col*4));
    if (play) buf |= 1 << (col*4);
    if (stacc) buf |= 2 << (col*4);
  }
  void set_sel(bool b) { // unselect if !b
    for (int c=0;c<voice_max;++c) {
      if (b) buf |= 4 << (c*4);
      else buf &= ~(4 << (c*4));
    }
  }
  void set_sel(bool b,int col) { // unselect if !b
    if (b) buf |= 4 << (col*4);
    else buf &= ~(4 << (col*4));
  }
  void get(int col,bool* play,bool* stacc) {
    Uint8 uc=get_c(col);
    if (play) *play= (uc & 1)!=0;
    if (stacc) *stacc= (uc & 2)!=0; // does'nt work without !=0
  }
  void copy(ScSection *from,int col) { // sel also copied
    buf &= ~(0xf << (col*4));
    buf |= from->buf & (0xf << (col*4));
  }
  void copy(ScSection *from) {
    buf=from->buf;
  }
  bool is_sel(int col) {
    return get_c(col) & 4;
  }
  bool is_sel() {
    for (int c=0;c<voice_max;++c)
      if (is_sel(c)) return true;
    return false;
  }
  void reset() { buf=0; }
  void reset(int col) {
    buf &= ~(0xf << (col*4));
  }
  void draw_sect(int lnr,int snr);
  void draw_playsect(int x,int y);
  void draw_ghost(int lnr,int snr,bool erase);
  bool more1() {  // more then 1 color occupied?
    bool b=false;
    for (int col=0;col<voice_max;++col)
      if (occ(col)) {
        if (b) return true;
        b=true;
      }
    return false;
  }
  bool occ(int col) { return get_c(col) & 1; }
  bool occ(int* c=0) {  // at least 1 color occupied?
    if (one_col) {
      for (int col=0;col<voice_max;++col)
        if (occ(col) && col==act_color) { if (c) *c=col; return true; }
    }
    for (int col=0;col<voice_max;++col)
      if (occ(col)) { if (c) *c=col; return true; }
    return false;
  }
};

struct SectData { // used by struct Selected
  int snr,
      lnr;
  ScSection sect;
  SectData(int _lnr,int _snr,ScSection *sec):
      snr(_snr),lnr(_lnr) {
    if (sec) sect=*sec;
  }
  bool operator==(SectData& sd) { return lnr==sd.lnr && snr==sd.snr; }
  bool operator<(SectData& sd) {
    return snr==sd.snr ? lnr<sd.lnr : snr<sd.snr;
  }
};

struct Selected {
  SLinkedList<SectData> sd_list; 
  void to_rose() {
    if (!sd_list.lis) {
      unselect->style.param=4;  // rose background
      unselect->draw_blit_upd();
    }
  }
  void to_blue() {
    if (!sd_list.lis) {
      unselect->style.param=0;  // blue background
      unselect->draw_blit_upd();
    }
  }
  void insert(int lnr,int snr,ScSection *sec) {
    sd_list.insert(SectData(lnr,snr,sec));
  }
  void remove(int lnr,int snr) {
    sd_list.remove(SectData(lnr,snr,0));
    to_blue();
  }
  void reset() {
    sd_list.reset();
    if (unselect) to_blue(); // 0 at startup
  }
  void restore_sel();
  void modify_sel(int);
  int min_snr() {
    SLList_elem<SectData> *sd;
    int snr=sd_list.lis->d.snr;
    for (sd=sd_list.lis;sd;sd=sd->nxt)
      if (snr>sd->d.snr) snr=sd->d.snr;
    return snr;
  }
} selected;

struct ScoreView {
  int state,             // set when mouse down
      cur_lnr, cur_snr,  // set when mouse down
      prev_snr,          // previous visited section nr
      delta_lnr, delta_snr,
      left_snr;          // new snr of leftmost selected section
  Point cur_point,       // set when mouse down
        prev_point;      // set when mouse moved
  ScSection proto;
  void mouse_down(int x,int y,int but);
  void mouse_moved(int x,int y,int but);
  void mouse_up(int x,int y,int but);
  void select_down(ScSection*,const int lnr,int snr,int but);
  void edit_down(ScSection* sec,int lnr,int snr,int but);
  void sel_column(int snr,int col=-1);
} sv;

static int sectnr(int x) { return (x+2)/sect_len + leftside; }
static int linenr(int y) { return (y-scv_yoff+2)/sclin_dist; }

struct Score {
  const char* name;
  int len,        // number of sections
      lst_sect;   // last written section
  ScSection (*block)[sclin_max];
  Score():
    len(40),
    lst_sect(-1),
    block(new ScSection[len][sclin_max]) {
  }
  ScSection* get_section(int lnr,int snr) {
    if (snr>=len) {
      int old_len=len;
      do len*=2; while (snr>=len);
      ScSection (*new_block)[sclin_max]=new ScSection[len][sclin_max];
      for (int i=0;i<old_len;++i)
        for (int j=0;j<sclin_max;++j)
          new_block[i][j]=block[i][j];
      delete[] block;
      block=new_block;
      sv_scroll->set_range(len*sect_len+20,false);
      sv_scroll->set_xpos(leftside*sect_len);
    }
    return block[snr] + lnr;
  }
  void reset() {
    lst_sect=-1;
    for (int snr=0;snr<len;++snr)
      for (int lnr=0;lnr<sclin_max;++lnr)
        get_section(lnr,snr)->reset();
  }
} *score;

union WValue {
  bool b;
  int i;
  WValue():i(0){ }
};

struct InfoVal {
  Sint8 instr,  // eBlack etc.
        slider_tag; // eNonl etc.
  WValue val;
  InfoVal *next;
  InfoVal():slider_tag(-1),next(0) { }
  InfoVal(int col,int tag):instr(col),slider_tag(tag),next(0) { }
  ~InfoVal() { delete next; }
  bool operator==(InfoVal& inf) { return slider_tag==inf.slider_tag && instr==inf.instr; }
  void add(InfoVal& info) {
    InfoVal *sci;
    if (slider_tag<0) *this=info;
    else {
      for (sci=this;sci->next;sci=sci->next) 
        if (*sci==info) { val=info.val; return; }
      sci->next=new InfoVal(info);
    }
  }
  void reset() {
    delete next;
    slider_tag=-1; val.i=0; next=0;
  }
};

struct Info {
  int len,
      lst_info;
  InfoVal *buf;
  Info():
    len(40),
    lst_info(-1),
    buf(new(InfoVal[len])) {
  };
  void mouse_down(int snr);
  InfoVal& get(int ind) {
    while (ind>=len) { // not: re_alloc()
      InfoVal *new_buf=new InfoVal[len*2];
      for (int i=0;i<len;++i) { new_buf[i]=buf[i]; buf[i].next=0; }
      delete[] buf;
      buf=new_buf;
      len*=2;
    }
    if (lst_info<ind) lst_info=ind;
    return buf[ind];
  }
  void reset() {
    for (int i=0;i<=lst_info;++i) buf[i].reset();
    lst_info=-1;
  }
  void remove(int snr,int col,int tag=-1) {
    InfoVal *inf=buf+snr;
    if (inf->instr==col && (tag<0 || inf->slider_tag==tag)) {
      if (inf->next) *inf=*inf->next;
      else inf->reset();
      return;
    }
    InfoVal *prev,*inf1;
    for (prev=inf,inf1=inf->next;inf1;prev=inf1,inf1=inf1->next) {
      if (inf1->instr==col && (tag<0 || inf1->slider_tag==tag)) {
        if (inf1->next) *inf1=*inf1->next;
        else prev->next=0;
        return;
      }
    }
    // removed item is not deleted, so inf->next stays valid
  }
} info_buf;

struct Panel {
  WValue SET0[ctrl_max],  // settings at time 0
         SET[ctrl_max];   // actual settings
  Panel();
  template<class T> bool is_a(T*& ptr,int ind) {
    return (ptr=dynamic_cast<T*>(CONT[ind])) != 0;
  }
  void upd_sliders(int inr) {  // inr = instr nr
    bool upd= inr==act_instr;
    const int the_act_instr=act_instr;
    act_instr=inr;  // used by hsl->cmd() etc.
    for (int i=0;i<ctrl_max;++i) {
      CheckBox *cb;
      HSlider *hsl;
      VSlider *vsl;
      RButtons *rbw;
      if (is_a(cb,i))       {
        cb->d=&SET[i].b;
        if (cb->cmd) cb->cmd(cb);
        if (upd) cb->draw_blit_upd();
      }
      else if (is_a(hsl,i)) {
        hsl->d=&SET[i].i;
        hsl->cmd(hsl,upd,false);
        if (upd) hsl->draw_blit_upd();
      }
      else if (is_a(vsl,i)) {
        vsl->d=&SET[i].i;
        vsl->cmd(vsl,upd,false);
        if (upd) vsl->draw_blit_upd();
      }
      else if (is_a(rbw,i)) {
        rbw->d=&SET[i].i;
        if (rbw->rb_cmd) rbw->rb_cmd(rbw,rbw->value(),upd);
        if (upd) rbw->draw_blit_upd();
      }
    }
    act_instr=the_act_instr;
    // afterwards, sliders will be re-connected to act_instr  
  }
  void connect_sliders(bool fire) {
    for (int i=0;i<ctrl_max;++i) {
      CheckBox *cb;
      HSlider *hsl;
      VSlider *vsl;
      RButtons *rbw;
      if (is_a(cb,i))       { cb->d=&SET[i].b; if (cb->cmd) cb->cmd(cb); }
      else if (is_a(hsl,i)) { hsl->d=&SET[i].i; hsl->cmd(hsl,fire,false); }
      else if (is_a(vsl,i)) { vsl->d=&SET[i].i; vsl->cmd(vsl,fire,false); }
      else if (is_a(rbw,i)) { rbw->d=&SET[i].i; if (rbw->rb_cmd) rbw->rb_cmd(rbw,rbw->value(),fire); }
    }
  }
} *PAN[voice_max/2];

void exec_info(int snr,bool do_draw) { // maybe in audio thread
  InfoVal *inf=&info_buf.get(snr);
  if (inf->slider_tag<0) return;
  for (;inf;inf=inf->next) {
    Panel *pan=PAN[inf->instr];
    pan->SET[inf->slider_tag].i=inf->val.i;
    const bool upd= inf->instr==act_instr;
    const int the_act_instr=act_instr;
    act_instr=inf->instr;  // used by hsl->cmd() etc.
    CheckBox *cb;
    HSlider *hsl;
    VSlider *vsl;
    RButtons *rbw;
    if (pan->is_a(cb,inf->slider_tag)) {
      if (cb->cmd) {
        cb->d=&pan->SET[inf->slider_tag].b;
        cb->cmd(cb);
      }
    }
    else if (pan->is_a(hsl,inf->slider_tag)) {
      hsl->d=&pan->SET[inf->slider_tag].i;
      hsl->cmd(hsl,upd,false);
    }
    else if (pan->is_a(vsl,inf->slider_tag)) {
      vsl->d=&pan->SET[inf->slider_tag].i;
      vsl->cmd(vsl,upd,false);
    }
    else if (pan->is_a(rbw,inf->slider_tag)) {
      rbw->d=&pan->SET[inf->slider_tag].i;
      if (rbw->rb_cmd) rbw->rb_cmd(rbw,rbw->value(),upd);
    }
    if (do_draw && upd)
      send_uev([](int ind){
        CONT[ind]->draw_blit_upd();
      },inf->slider_tag);
    act_instr=the_act_instr;
  }
}

int nr_keys;
const Uint8 *keystate = SDL_GetKeyboardState(&nr_keys);

void Info::mouse_down(const int snr) {
  Panel *pan=PAN[act_instr];
  int i;
  if (keystate[SDL_SCANCODE_I]) {
    char b[60];
    messages->add_text("Modifications:",false);
    for (int snr1=0;snr1<=info_buf.lst_info;++snr1) {
      InfoVal *inf=&info_buf.get(snr1);
      if (inf->slider_tag>=0)
        for (;inf;inf=inf->next) {
          snprintf(b,60,"  time=%d:%d instr=%d widget=%s val=%d",
            snr1/meter,snr1%meter,inf->instr,slider_tag_name[inf->slider_tag],inf->val.i);
          messages->add_text(b,false);
        }
    }
    messages->draw_blit_recur(); messages->upd();
    return;
  }
  if (keystate[SDL_SCANCODE_D]) {
    info_buf.remove(snr,act_instr);
    infoview->draw_blit_upd();
    return;
  }
  WValue SET_act[ctrl_max];
  for (i=0;i<ctrl_max;++i)
    SET_act[i]=pan->SET0[i];  // init SET_act
  for (int snr1=0;snr1<=snr;++snr1) {
    InfoVal *inf=&info_buf.get(snr1);
    if (inf->slider_tag<0) continue;
    for (;inf;inf=inf->next) {
      if (inf->instr!=act_instr) continue;
      if (snr1==snr) inf->val.i=pan->SET[inf->slider_tag].i;
      SET_act[inf->slider_tag].i=inf->val.i;
    }
  }
  // now SET_act contains last values
  bool redraw=false;
  for (i=0;i<ctrl_max;++i) {
    if (pan->SET[i].i!=SET_act[i].i) {
      InfoVal info(act_instr,i);
      info.val=SET_act[i]=pan->SET[i];
      info_buf.get(snr).add(info);
      redraw=true;
    }
  }
  for (int snr1=snr+1;snr1<=info_buf.lst_info;++snr1) {
    InfoVal *inf=&info_buf.get(snr1);
    if (inf->slider_tag<0) continue;
    for (;inf;inf=inf->next) {
      if (inf->instr!=act_instr) continue;
      if (SET_act[inf->slider_tag].i==inf->val.i) {
        remove(snr1,inf->instr,inf->slider_tag);
        redraw=true;
      }
      SET_act[inf->slider_tag].i=-99; // not to be used again
    }
  }
  if (redraw) infoview->draw_blit_upd();
}

void NoteBuffer::omit_sect(int lnr,int snr,int col) { // remove overlapping notes
  for (;snr<=score->lst_sect;++snr) {
    ScSection *sec=score->get_section(lnr,snr);
    bool play;
    sec->get(col,&play,0);
    if (!play) break;
    sec->reset(col);
    sec->draw_sect(lnr,snr);
  }
  alert("overlapping %s note shortened at %d.%d",col_name[col],snr/meter,snr%meter);
}

bool set_mass_spring(FILE *conf) {
  char buf[20];
  int n,
      n1,n2,n3,n4;
  float fl1;
  char ch;
  Node *nod;
  Spring *spr;
  Instrument *ins=0;
  int inr=0;
  while (true) {
    if (1!=fscanf(conf,"%20s\n",buf)) return true;
    if (1==sscanf(buf,"tempo=%d",&inr)) {
      tempo->set_hsval(inr-tempo_offset,1,false);
      continue;
    }
    if (1==sscanf(buf,"Instrument=%d",&inr)) {
      ins=INST[inr];
      for (n=0;n<=ins->Nend;++n) {
        ins->bouncy->nodes[n].reset();
        ins->phys_model->nodes[n].reset();
      }
      continue;
    }
    if (!strcmp(buf,"Nodes")) {
      bool end=false;
      while (!end) {
        if (1!=fscanf(conf,"%d:",&n)) return false;
        ins->Nend=n;
        if (4!=fscanf(conf,"x=%d,y=%d,m=%f,fix=%d,s=",&n1,&n2,&fl1,&n3)) return false;
        ins->bouncy->nodes[n].set(n1,n2,fl1,n3);
        ins->phys_model->nodes[n].set(n1,n2,fl1,n3);
        while (true) {
          if (2!=fscanf(conf,"%d%c",&n1,&ch)) return false; // "spr=1,2" not used
          if (ch==' ') break;
          if (ch=='\n') { end=true; break; }
          if (ch!=',') return false;
        }
      }
    }
    else if (!strcmp(buf,"Springs")) {
      while (true) {
        if (1!=fscanf(conf,"%d:",&n)) return false;
        ins->Send=n;
        if (4!=fscanf(conf,"a=%d,b=%d,st=%d%c",&n1,&n2,&n3,&ch)) return false;
        spr=ins->bouncy->springs+n;
        nod=ins->bouncy->nodes;
        spr->set(nod+n1,nod+n2); spr->weak=n3;
        spr=ins->phys_model->springs+n;
        nod=ins->phys_model->nodes;
        spr->set(nod+n1,nod+n2); spr->weak=n3;
        if (ch=='\n') break;
        if (ch!=' ') return false;
      }
    }
    else if (!strcmp(buf,"Tune")) {
      int lnr,snr,col,dur,stacc;
      if (sdl_running) {
        selected.reset();
        selected.to_blue();
      }
      while (true) {
        int nr=fscanf(conf," L%dS%dc%dd%ds%d%c",&lnr,&snr,&col,&dur,&stacc,&ch);
        if (nr==0) break; // all notes deleted
        if (nr!=6) {
          alert("note syntax error");
          return false;
        }
        if (stacc!=0 && stacc!=1) return false;
        if (debug) printf("lnr=%d snr=%d col=%d dur=%d stacc=%d ch=[%c]\n",lnr,snr,col,dur,stacc,ch);
        for (int i=0;i<dur;++i)
          score->get_section(lnr,snr+i)->set(col,true,i==dur-1 ? stacc : false);
        if (score->lst_sect<snr+dur-1)
          score->lst_sect=snr+dur-1;
        if (ch=='\n') break;
        if (ch!=' ') return false;
      }
      if (sdl_running) {
        scoreview->draw_blit_upd();
        infoview->draw_blit_upd();
      }
    }
    else if (!strcmp(buf,"Settings")) {
      Panel *pan=PAN[inr];
      while (true) {
        if (3!=fscanf(conf," %[^=]=%d%c",buf,&n1,&ch)) return false;
        WValue *set=pan->SET,
               *set0=pan->SET0;
        if (!strcmp(buf,"stsp"));
        else if (!strcmp(buf,"pizz")) { set0[ePizz].b=set[ePizz].b=n1; }
        else if (!strcmp(buf,"nonl")) { set0[eNonl].b=set[eNonl].b=n1; }
        else if (!strcmp(buf,"paph")) { set0[ePaph].i=set[ePaph].i=n1; }
        else if (!strcmp(buf,"pnoi")) { set0[ePnoi].b=set[ePnoi].b=n1; }
        else if (!strcmp(buf,"tun"))  { set0[eTun].i=set[eTun].i=n1; }
        else if (!strcmp(buf,"asym")) { set0[eAsym].i=set[eAsym].i=n1; }
        else if (!strcmp(buf,"fric")) { set0[eFric].i=set[eFric].i=n1; }
        else if (!strcmp(buf,"lstr")) { set0[eLstr].i=set[eLstr].i=n1; }
        else if (!strcmp(buf,"exl"))  { set0[eExl].i=set[eExl].i=n1; }
        else if (!strcmp(buf,"ampl")) { set0[eAmpl].i=set[eAmpl].i=n1; }
        else if (!strcmp(buf,"fb"));
        else if (!strcmp(buf,"tfer")) { set0[eTfer].i=set[eTfer].i=n1; }
        else if (!strcmp(buf,"pu_l")) ins->pickup_left=n1;
        else if (!strcmp(buf,"pu_r")) ins->pickup_right=n1;
        else if (!strcmp(buf,"exc")) ins->excite_mass=n1;
        else {
          alert("unexpected setting: %s",buf);
          return false;
        }
        if (ch=='\n') break;
        if (ch!=' ') return false;
      }
      if (act_instr==inr && sdl_running) {
        pan_view->draw_blit_recur(); pan_view->upd();
      }
    }
    else if (!strcmp(buf,"Modifications")) {
      while (true) {    // " S%dc%dt%d:%d",snr,inf->col,inf->slider_tag,inf->val);
        int nr=fscanf(conf," S%dc%dt%d:%d%c",&n1,&n2,&n3,&n4,&ch);
        if (nr==0) break;
        if (nr!=5) return false;
        InfoVal info(n2,n3); 
        info.val.i=n4;
        info_buf.get(n1).add(info);
        if (ch=='\n') break;
        if (ch!=' ') return false;
      }
    }
  }
}

static void topw_disp() {
  top_win->clear();
  top_win->border(bview,2);
  top_win->border(messages,2);
  top_win->border(infoview,1);
  top_win->border(scoreview,1);
  top_win->border(scope_win,2);
  wav_indic->draw();
  txtmes1->draw_label();
  txtmes2->draw_label();
  txtmes3->draw_label();
  draw_ttf->draw_string(top_win->render,"keys: m, c, k, p",Point(eb_sved->tw_area.x,eb_sved->tw_area.y+43));
  draw_ttf->draw_string(top_win->render,"keys: d, up, down",Point(eb_edma->tw_area.x,eb_edma->tw_area.y+43));
  draw_ttf->draw_string(top_win->render,"keys: d",Point(eb_edsp->tw_area.x,eb_edsp->tw_area.y+43));
  if (config_file) txtmes3->draw_mes(config_file);
}

Mass_Spring::Mass_Spring(Instrument *inst):
    n_ind(0),
    friction(nominal_friction),
    asym(1.),
    tuning(1.),
    low_sprc(0.01) {
  int i;
  for (i=0; i <= inst->Nend; i++) {
    nodes[i].set(20+i*24,bg_h/2,nom_mass,false);
  }
  nodes[0].fixed=nodes[inst->Nend].fixed=true;
  for (i=0; i <= inst->Send; i++)
    springs[i].set(nodes+i, nodes+i+1);
}

void Mass_Spring::reconnect() {
  int i;
  Instrument *ins=INST[act_instr];
  for (i=0;i<=ins->Nend;++i)
    nodes[i].reset();
  for (i=0;i<=ins->Send;++i) {
    Spring *sp=springs+i;
    sp->a->connect(sp); sp->b->connect(sp);
  }
}

Bouncy::Bouncy(Instrument* inst):
    Mass_Spring(inst),
    down(0),selected(0),selnode(0) {
}

void right_arrow(Render rend,int par,int y_off) {
  i_am_playing ? rectangleColor(rend,6,7,12,13,0xff0000ff) : trigonColor(rend,6,6,12,10,6,14,0xff);
}

// nbuf = 0 if !play_tune or if this = bouncy
void Mass_Spring::eval_model(Instrument *inst,NoteBuffer **nbuf,float freq_x,float freq_y) {
  int i,
      note_cat_x= nbuf && nbuf[0]->cur_note ? nbuf[0]->cur_note->cat : 0,
      note_cat_y= nbuf && nbuf[1]->cur_note ? nbuf[1]->cur_note->cat : 0;
  Panel *pan=inst->inst_panel;
  bool pizz= pan->SET[ePizz].b;
  Node *n;
  for (i=0; i <= inst->Nend; i++) {
    n=nodes+i;
    n->ax = n->ay = 0;
  }
  if (pan->SET[ePnoi].b) {  // pink noise
    n_ind=(n_ind+1)%Noise::ndim;
    bool add_x=!nbuf || note_cat_x==eNote,
         add_y=!nbuf || note_cat_y==eNote;
    if (add_x)
      nodes[inst->excite_mass].d_x += noise.pink_buf[n_ind] * 0.0006;
    if (add_y)
      nodes[inst->excite_mass].d_y += noise.pink_buf[n_ind] * 0.0006;
  }
  for (i = 0; i <= inst->Send; ++i) {
    Spring *s = springs+i;
    float dx = s->b->d_x - s->a->d_x,
          dy = s->b->d_y - s->a->d_y;
    if (pan->SET[eNonl].b) {
      float dd=dx*dx/20;
      dx += dx>0 ? dd : -dd;
      dd=dy*dy/20;
      dy += dy>0 ? dd : -dd;
    }
    float sprc=springconstant*tuning;
    if (s->weak) sprc *= low_sprc;
    if (s->a->fixed || s->b->fixed) sprc *= 0.5;  // better frequency of the harmonics
    dx *= sprc * freq_x;
    dy *= sprc * freq_y;
    s->a->ax += dx;
    s->a->ay += dy;
    s->b->ax -= dx;
    s->b->ay -= dy;
  }
  const float fric_x= note_cat_x==eNote && !pizz ? 0.99999 : stop_req==eEndReached ? 0.999 : friction,
              fric_y= note_cat_y==eNote && !pizz ? 0.99999 : stop_req==eEndReached ? 0.999 : friction,
              mult_x=freq_x * asym,
              mult_y=freq_y / asym;
  for (i = 0; i <= inst->Nend; ++i) {
    n = nodes+i;
    if (n->fixed) {
      n->d_x=n->d_y=0;
      continue;
    }
    if (fabs(n->ax)>0.0002)
      n->vx = (n->vx + n->ax / n->mass * mult_x) * fric_x;
    n->d_x += n->vx;
    if (fabs(n->ay)>0.0002)
      n->vy = (n->vy + n->ay / n->mass * mult_y) * fric_y;
    n->d_y += n->vy;
  }
}

void audio_callback(void*, Uint8 *stream, int len) {
  audio_callback(stream,len);
}

void init_audio() {
  SDL_AudioSpec *ask=new SDL_AudioSpec,
                *got=new SDL_AudioSpec;
  ask->freq=SAMPLE_RATE;
  ask->format=AUDIO_S16SYS;
  ask->channels=2;
  ask->samples=2048; // 1024: xruns!
  ask->callback=audio_callback;
  ask->userdata=0;
  if (SDL_OpenAudio(ask, got) < 0) {
     alert("Couldn't open audio: %s",SDL_GetError());
     exit(1);
  }
  //printf("samples=%d channels=%d freq=%d format=%d (LSB=%d) size=%d\n",
  //       got->samples,got->channels,got->freq,got->format,AUDIO_S16LSB,got->size);
  SDL_PauseAudio(0);
}

int bouncy_threadfun(void* data) {
  while (true) {
    SDL_Delay(50);
    if (freeze && !freeze->value())
      INST[act_instr]->bouncy->eval_model(INST[act_instr],0,1,1);
    send_uev([](int){
      bview->clear();
      INST[act_instr]->draw();
      bview->blit_upd(0);
    },0);
  }
  return 0;
}

int play_threadfun(void* data) {
  if (SDL_GetAudioStatus()!=SDL_AUDIO_PLAYING) {
    init_audio();
  }
  scope::scope_start=0;
  while (stop_req!=eStopAudio)
    SDL_Delay(10);
  SDL_CloseAudio();
  i_am_playing=false;
  if (wav_mode==eWavRecording)
    send_uev([](int){ write_wav->set_cbval(false,1); },0);
  send_uev([](int){ play_but->draw_blit_upd(); },0);
  return 0;
}

int wav_threadfun(void*) {
  scope::scope_start=0;
  Uint8 buf[1024];
  while (wav_mode && stop_req!=eStopAudio) {
    audio_callback(buf,1024);
    if (!dump_wav((char*)buf,1024)) wav_mode=0;
  }
  i_am_playing=false;
  send_uev([](int){
    write_wav->set_cbval(false,1);
    play_but->draw_blit_upd();
  },0);
  return 0;
}

void write_note(FILE *conf,int col,int lnr,int& snr) {
  ScSection *sec=score->get_section(lnr,snr);
  bool play,
       stacc;
  sec->get(col,&play,&stacc);
  if (play) {
    int snr1=snr;
    for (;;++snr) {
      if (snr==score->lst_sect || stacc) {
//        fprintf(conf," l%ds%dd%dc%ds%d",lnr,snr1,snr-snr1+1,col,stacc);
        fprintf(conf," L%dS%dc%dd%ds%d",lnr,snr1,col,snr-snr1+1,stacc);
        break;
      }
      score->get_section(lnr,snr+1)->get(col,&play,&stacc);
      if (!play) {
//        fprintf(conf," l%ds%dd%dc%ds0",lnr,snr1,snr-snr1+1,col);
        fprintf(conf," L%dS%dc%dd%ds0",lnr,snr1,col,snr-snr1+1);
        break;
      }
    }
  }
}

bool write_config(FILE *conf) {
  int i;
  Node *n;
  Spring *s;
  Instrument *inst;
  fprintf(conf,"tempo=%d\n",tempo->value()+tempo_offset);
  for (int nr=0;nr<voice_max/2;++nr) {
    fprintf(conf,"Instrument=%d\n",nr);
    inst=INST[nr];
    fputs("Nodes\n ",conf);
    n=inst->bouncy->nodes;
    s=inst->bouncy->springs;
    for (i=0;i<=inst->Nend;++i) {
      fprintf(conf," %d:x=%.0f,y=%.0f,m=%.1f,fix=%d,s=",i,n[i].nom_x,n[i].nom_y,n[i].mass,n[i].fixed);
      Spring *sp;
      for (int j=0;;++j) {
        sp=n[i].spr[j]; fprintf(conf,"%d",sp-s);
        if (j<n[i].spr_end) putc(',',conf);
        else break;
      }
    }
    putc('\n',conf);
    fputs("Springs\n ",conf);
    for (i=0;i<=inst->Send;++i)
      fprintf(conf," %d:a=%d,b=%d,st=%d",i,s[i].a-n,s[i].b-n,s[i].weak);
    putc('\n',conf);
    fputs("Settings\n",conf);
    Panel *pan=PAN[nr];
    Instrument *ins=INST[nr];
    fprintf(conf,"  pizz=%d nonl=%d paph=%d pnoi=%d",
      pan->SET0[ePizz].b,pan->SET0[eNonl].b,pan->SET0[ePaph].i,pan->SET0[ePnoi].b);
    fprintf(conf," tun=%d asym=%d fric=%d lstr=%d exl=%d ampl=%d tfer=%d",
      pan->SET0[eTun].i,pan->SET0[eAsym].i,pan->SET0[eFric].i,pan->SET0[eLstr].i,
      pan->SET0[eExl].i,pan->SET0[eAmpl].i,pan->SET0[eTfer].i);
    fprintf(conf," pu_l=%d pu_r=%d exc=%d\n",ins->pickup_left,ins->pickup_right,ins->excite_mass);
  }
  if (info_buf.lst_info>=0) {
    bool add_mod=false;
    for (i=0;i<voice_max/2;++i)
      for (int j=0;j<ctrl_max;++j)
        PAN[i]->SET[j]=PAN[i]->SET0[j];
    for (int snr=0;snr<=info_buf.lst_info;++snr) {
      InfoVal *inf=&info_buf.get(snr);
      if (inf->slider_tag>=0) {
        CheckBox *cb; HSlider *hsl; VSlider *vsl; RButtons *rbw;
        for (;inf;inf=inf->next) {  // not write
          Panel *pan=PAN[inf->instr];
          int tag=inf->slider_tag;
          bool add=false;
          if (pan->is_a(cb,tag)) {
            if (inf->val.b!=pan->SET[tag].b) add=true;
          }
          else if (pan->is_a(hsl,tag)) {
            if (inf->val.i!=pan->SET[tag].i) add=true;
          }
          else if (pan->is_a(vsl,tag)) {
            if (inf->val.i!=pan->SET[tag].i) add=true;
          }
          else if (pan->is_a(rbw,tag)) {
            if (inf->val.i!=pan->SET[tag].i) add=true;
          }
          if (add) {
            pan->SET[tag]=inf->val;
            if (!add_mod) {
              add_mod=true;
              fputs("Modifications\n ",conf);
            }
            fprintf(conf," S%dc%dt%d:%d",snr,inf->instr,inf->slider_tag,inf->val.i);
          }
        }
      }
    }
    putc('\n',conf);
  }
  if (score->lst_sect>=0) {
    fputs("Tune\n ",conf);
    for (int lnr=0;lnr<sclin_max;++lnr) {
      for (int col=0;col<voice_max;++col) {
        for (int snr=0;snr<=score->lst_sect;++snr)
          write_note(conf,col,lnr,snr); // may increase snr
      }
    }
  }
  putc('\n',conf);
  return true;
/*
  puts("Audio:");
  n=phys_model->nodes;
  s=phys_model->springs;
  for (i=0;i<=Nend;++i) printf("node %d: nom_x=%.0f nom_y=%.0f fixed=%d\n",i,n[i].nom_x,n[i].nom_y,n[i].fixed);
  for (i=0;i<=Send;++i) printf("spring %d: a=%d b=%d\n",i,s[i].a-n,s[i].b-n);
*/
}

void Instrument::reloc_pickup(int &pickup) {
  if (pickup > active_mass) --pickup;
  else if (pickup==active_mass) pickup=-1;
}

bool NoteBuffers::fill_note_bufs() {
  int snr,lnr,col;
  ScSection *sec,
            *sec_prev,*sec_nxt;
  for (snr=leftside;snr<=score->lst_sect;++snr) {
    for (lnr=0;lnr<sclin_max;++lnr) {
      sec=score->get_section(lnr,snr);
      for (col=0;col<voice_max;++col) {
        bool play,stacc;
        sec->get(col,&play,&stacc);
        if (play) {
          sec_prev= snr>leftside ? score->get_section(lnr,snr-1) : 0; // previous section
          bool play1,stacc1;
          if (sec_prev) sec_prev->get(col,&play1,&stacc1);

          if (!sec_prev || stacc1 || !play1) {
            if (nbuf[col].note_snr>=0) {  // overlapping notes?
              if (nbuf[col].note_snr<snr)
                add_note(col,nbuf[col].note_lnr,nbuf[col].note_snr,snr-1);
              nbuf[col].omit_sect(nbuf[col].note_lnr,snr,col);
            }
            nbuf[col].note_lnr=lnr;
            nbuf[col].note_snr=snr;
          }
        }
      }
    }
    for (lnr=0;lnr<sclin_max;++lnr) {
      sec=score->get_section(lnr,snr);
      for (col=0;col<voice_max;++col) {
        bool play,stacc;
        sec->get(col,&play,&stacc);
        if (play) {
          bool play1=false;
          sec_nxt= snr<score->lst_sect && snr<score->len-1 ? score->get_section(lnr,snr+1) : 0;
          if (sec_nxt) sec_nxt->get(col,&play1,0);
          if (stacc || !sec_nxt || !play1) {
            add_note(col,lnr,nbuf[col].note_snr,snr); 
            nbuf[col].note_snr=-1;
          }
        }
      }
    }
  }
  for (int i=0;i<voice_max;++i) {  // add eEnd notes
    NoteBuffer *nb=nbuf+i;
    if (nb->cur_note) {
      nb->add_note(0,nbuf[i].end_snr+1,nbuf[i].end_snr+1);
      nb->cur_note->cat=eEnd;
      nb->cur_note=nb->notes; // reset cur_note
      if (debug) nbufs.report(i);
    }
  }
  return true;
}

void dial_cmd(const char* text,int cmd_id) {
  switch(cmd_id) {
    case 'w_cf': {
        FILE *conf=fopen(text,"w");
        if (!conf) { alert("%s can't be written",text); break; }
        if (write_config(conf)) {
          dialog->dialog_label("saved");
          if (!config_file || strcmp(text,config_file)) {
            config_file=strdup(text);
            txtmes3->draw_mes(config_file);
          }
        }
        fclose(conf);
      }
      break;
    case 'r_cf': {
        FILE *conf=fopen(text,"r");
        if (conf) {
          score->reset();
          info_buf.reset();
          if (!set_mass_spring(conf)) {
            alert("error in %s",text); fclose(conf);
            break;
          }
          fclose(conf);
          dialog->dialog_label("loaded");
          if (!config_file || strcmp(config_file,text)) {
            config_file=strdup(text);
            txtmes3->draw_mes(config_file);
          }
        }
        else alert("project file %s not opened",text);
      }
      break;
    default: alert("dial_cmd?");
  }
}

void play_cmd(Button *but) {
  if (i_am_playing) {
    stop_req=eEndReached;
    if (wav_mode==eWavOutNoAudio)
      SDL_WaitThread(wav_thread,0);
    else
      SDL_WaitThread(audio_thread,0); // sets i_am_playing = false
    return;
  }
  for (int i=0;i<voice_max/2;++i) { // reset phys_model
    INST[i]->phys_model->damp_cnt=0;
    for (int j=0;j<=INST[i]->Nend;++j) {
      Node *n=INST[i]->phys_model->nodes+j;
      n->d_x=n->d_y=n->vx=n->vy=0;
    }
  }
  nbufs.reset();
  act_snr=0;
  if (!ignore->value()) {
    for (int i=0;i<voice_max/2;++i) {
      Panel *pan=PAN[i];
      for (int j=0;j<ctrl_max;++j) pan->SET[j]=pan->SET0[j];
      for (int snr=0;snr<=leftside;++snr) {
        InfoVal *inf=&info_buf.get(snr);
        if (inf->slider_tag<0) continue;
        for (;inf;inf=inf->next)
          pan->SET[inf->slider_tag].i=inf->val.i;
      }
      // now SET contains start values
      pan->upd_sliders(i);
    }
    PAN[act_instr]->connect_sliders(false);
  }
  if (play_tune && !nbufs.fill_note_bufs())
    return;
  i_am_playing=true;
  stop_req=0;
  if (wav_mode==eWavOutStart) {
    wav_mode=eWavRecording;
    wav_indic->set_color(0xff0000ff); // red
    audio_thread=SDL_CreateThread(play_threadfun,"play_thread",0);
  }
  else if (wav_mode==eWavOutNoAudio) {
    wav_indic->set_color(0xff0000ff); // red
    wav_thread=SDL_CreateThread(wav_threadfun,"wav_thread",0);
  }
  else {
    audio_thread=SDL_CreateThread(play_threadfun,"play_thread",0);
  }
}
namespace smooth { // copy node location from bouncy to phys model
  int phase=0,
      tim=0;
  const int delta=100;
  Node b_nodes[NN],
       pm_nodes[NN];
  void smooth(Instrument *instr) {
    PhysModel *phys_model=instr->phys_model;
    Bouncy *bouncy=instr->bouncy;
    if (phase==3) { // set by bouncy_down()
      Node &nod=instr->phys_model->nodes[instr->excite_mass];
      nod.d_x=nod.d_y=instr->excit;
      phase=0;
    }
    else if (phase==1) {  // set by bouncy_up()
      for (int i=0;i<=instr->Nend;++i) {
        pm_nodes[i].cpy(phys_model->nodes[i]);
        b_nodes[i].cpy(bouncy->nodes[i]);
      }
      phase=2;
      tim=0;
    }
    if (phase==2) {
      for (int i=0;i<=instr->Nend;++i) {
        phys_model->nodes[i].cpy(pm_nodes[i],b_nodes[i],float(tim)/delta);
      }
      if (++tim==delta) phase=0;
    }
  }
}
void bouncy_down(BgrWin*,int x,int y,int but) {
  int i,
      nearest=-1,
      dx=0,dy;
  Instrument *act_ins=INST[act_instr];
  Bouncy *act_bnc=act_ins->bouncy;
  PhysModel *act_pm=act_ins->phys_model;
  act_bnc->down=0;
  act_bnc->selnode=0;
  if (mode==eEditSpring && but!=SDL_BUTTON_RIGHT) // find nearest spring
    for (i=0; i <= act_ins->Send; i++) {
      Spring *s=act_bnc->springs+i;
      dx=(s->a->d_x+s->a->nom_x + s->b->d_x+s->b->nom_x)/2 - x;
      dy=(s->a->d_y+s->a->nom_y + s->b->d_y+s->b->nom_y)/2 - y;
      if (hypot(dx,dy) < radius) {
        nearest=i;
        break;
      }
    }
  else                                   // find nearest mass
    for (i=0; i <= act_ins->Nend; i++) {
      Node *n=act_bnc->nodes+i;
      dx=n->d_x+n->nom_x - x;
      dy=n->d_y+n->nom_y - y;
      if (hypot(dx,dy) < radius) {
        nearest=i;
        break;
      }
    }
  switch (but) {
    case SDL_BUTTON_LEFT:
      act_ins->active_mass=act_ins->active_spring=-1;
      if (nearest>=0) {
        if (mode==ePlayMode) {
          act_bnc->selnode=act_bnc->nodes+nearest;
          act_bnc->down=2;
          SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
        }
        else if (mode==eEditMass) {
          act_ins->active_mass=nearest;
          if (act_ins->active_mass>=0) {
            if (keystate[SDL_SCANCODE_D]) {
              act_ins->delete_mass();
            }
            else if (keystate[SDL_SCANCODE_UP]) {
              act_ins->bouncy->nodes[act_ins->active_mass].mass *= 1.4;
              act_ins->phys_model->nodes[act_ins->active_mass].mass=act_ins->bouncy->nodes[act_ins->active_mass].mass;
            }
            else if (keystate[SDL_SCANCODE_DOWN]) {
              act_ins->bouncy->nodes[act_ins->active_mass].mass /= 1.4;
              act_ins->phys_model->nodes[act_ins->active_mass].mass=act_ins->bouncy->nodes[act_ins->active_mass].mass;
            }
          }
        }
        else if (mode==eEditSpring) {
          act_ins->active_spring=nearest;
          if (keystate[SDL_SCANCODE_D]) {
            if (act_ins->active_spring>=0)
            act_ins->delete_spring();
          }
        }
      }
    break;
  case SDL_BUTTON_MIDDLE:
    if (nearest>=0) {
      if (mode==ePlayMode) {
        bool fixed=act_bnc->nodes[nearest].fixed;
        act_pm->nodes[nearest].fixed=act_bnc->nodes[nearest].fixed= !fixed;
      }
      else if (mode==eEditSpring) {
        act_pm->springs[nearest].weak=act_bnc->springs[nearest].weak=!act_bnc->springs[nearest].weak;
      }
    }
    else if (mode==eEditMass) {
      if (act_ins->Nend==NN-1) { alert("nodes > %d",NN); break; }
      ++act_ins->Nend;
      act_bnc->nodes[act_ins->Nend].set(x,y,nom_mass,false);
      act_pm->nodes[act_ins->Nend].set(x,y,nom_mass,false);
    }
    break;
  case SDL_BUTTON_RIGHT:
    if (nearest>=0) {
      if (mode==ePlayMode) {
        if (dx>0) {
          if (act_ins->pickup_left==nearest) act_ins->pickup_left=-1;
          else act_ins->pickup_left=nearest;
        }
        else {
          if (act_ins->pickup_right==nearest) act_ins->pickup_right=-1;
          else act_ins->pickup_right=nearest;
        }
      }
      else if (mode==eEditMass) {
        if (act_ins->active_mass>=0) {
          if (act_ins->Send<NS-1) { // connect 2 nodes
            ++act_ins->Send;
            act_bnc->springs[act_ins->Send].set(act_bnc->nodes+act_ins->active_mass,act_bnc->nodes+nearest);
            act_pm->springs[act_ins->Send].set(act_pm->nodes+act_ins->active_mass,act_pm->nodes+nearest);
            act_ins->active_mass=nearest;
          }
          else alert("springs > %d",NS);
        }
      }
      else if (mode==eEditSpring) {
        act_ins->excite_mass=nearest;
      }
    }
    break;
  }
  if (nearest>=0 && !(mode==ePlayMode && but==SDL_BUTTON_LEFT)) // mass in physical model excited
    smooth::phase=3;
}

void bouncy_moved(BgrWin*,int x,int y,int but) {
  if (INST[act_instr]->bouncy->down == 2 && INST[act_instr]->bouncy->selnode) {
    Node *n=INST[act_instr]->bouncy->selnode;
    n->d_x=x - n->nom_x;
    n->d_y=y - n->nom_y;
    //if (hard_attack->value()) bouncy->selnode->vy=y<bg_h ? 10 : -10; 
  }
}

Bouncy *act_bnc() { return INST[act_instr]->bouncy; }
PhysModel *act_phm() { return INST[act_instr]->phys_model; }

void bouncy_up(BgrWin*,int x,int y,int but) {
  SDL_EventState(SDL_MOUSEMOTION,SDL_IGNORE);
  Bouncy *bcy=act_bnc();
  if (bcy->selnode) { // set by bouncy_down() if mode = ePlayMode
    smooth::phase=1;
    bcy->selnode=0;
    if (wav_mode==eWavOutStart) {
      wav_mode=eWavRecording;
      wav_indic->set_color(0xff0000ff); // red
    }
  }
}

static float non_linear(float x,int trfmode) {
  const float limit=30000,
              limit2=10000;
  switch (trfmode) {
    case 0:
      if (x>limit) return limit;
      if (x<-limit) return -limit;
      break;
    case 1:
      if (x>0.) {
        if (x>limit2) return limit2;
        return 2*x-x*x/limit2;
      }
      if (x<-limit) return -limit;
      break;
    case 2:
      if (x>0.) {
        if (x>limit2) return limit2;
        return 2*x-x*x/limit2;
      }
      if (x<-limit2) return -limit2;
      return 2*x+x*x/limit2;
    case 3:
      if (x>limit || x<-limit) return limit;
      if (x<0) return -x;
      break;
  }
  return x;
}

void Instrument::delete_mass() {
  Node *nod=bouncy->nodes+active_mass;
  Spring *sp,*sp2;
  int i,j;
  for (i=0;i<=Send;) {
    sp=bouncy->springs+i;
    if (sp->a==nod || sp->b==nod) { // remove connected springs
      for (j=i;j<Send;++j) {
        bouncy->springs[j]=bouncy->springs[j+1];
        phys_model->springs[j]=phys_model->springs[j+1];
      }
      --Send;
    }
    else ++i;
  }
  for (i=active_mass;i<Nend;++i) {
    for (j=0;j<=Send;++j) { // shift spring connections
      sp=bouncy->springs+j;
      sp2=phys_model->springs+j;
      if (sp->a==bouncy->nodes+i+1) { sp->a=bouncy->nodes+i; sp2->a=phys_model->nodes+i; }
      else if (sp->b==bouncy->nodes+i+1) { sp->b=bouncy->nodes+i; sp2->b=phys_model->nodes+i; }
    }
  }
  for (i=active_mass;i<Nend;++i) {  // shift masses to the left
    bouncy->nodes[i]=bouncy->nodes[i+1];
    phys_model->nodes[i]=phys_model->nodes[i+1];
  }
  --Nend;
  bouncy->reconnect();   // restore spr[]'s in nodes
  phys_model->reconnect();
  reloc_pickup(pickup_left);
  reloc_pickup(pickup_right);
  reloc_pickup(excite_mass);
  active_mass=-1;
}

void Instrument::delete_spring() {
  for (int i=active_spring;i<Send;++i) {
    bouncy->springs[i]=bouncy->springs[i+1];
    phys_model->springs[i]=phys_model->springs[i+1];
  }
  --Send;
  bouncy->reconnect();   // restore spr[] in nodes
  phys_model->reconnect(); 
  active_spring=-1;
}

void Instrument::draw() {
  int i;
  for (i=0; i <= Send; i++) {
    Spring *s=bouncy->springs+i;
    int col= s->weak ? 0xe000ff : 0xff;
    lineColor(bview->render, s->a->d_x+s->a->nom_x, s->a->d_y+s->a->nom_y, s->b->d_x+s->b->nom_x, s->b->d_y+s->b->nom_y,col);
    if (i==active_spring) 
      circleColor(bview->render,(s->a->d_x+s->a->nom_x + s->b->d_x+s->b->nom_x)/2,
                            (s->a->d_y+s->a->nom_y + s->b->d_y+s->b->nom_y)/2,2,0xff);
  }
  for (i=0; i <= Nend; i++) {
    Node *n=bouncy->nodes+i;
    int rr=int(sqrt(8 * n->mass));
    if (n->fixed) {
      boxColor(bview->render,n->d_x+n->nom_x-6, n->d_y+n->nom_y-6,n->d_x+n->nom_x+6,n->d_y+n->nom_y+6,0xe000ff);
      if (i==active_mass)
        rectangleColor(bview->render,n->d_x+n->nom_x-5,n->d_y+n->nom_y-10,n->d_x+n->nom_x+5,n->d_y+n->nom_y+10,0xff);
    }
    else {
      filledCircleColor(bview->render,n->d_x+n->nom_x,n->d_y+n->nom_y,rr,0xff6060ff);
      if (i==active_mass)
        circleColor(bview->render,n->d_x+n->nom_x,n->d_y+n->nom_y,rr,0xff);
    }
    if (i==pickup_left)
      filledCircleColor(bview->render,n->d_x+n->nom_x-2,n->d_y+n->nom_y,2,0xff);
    if (i==pickup_right)
      filledCircleColor(bview->render,n->d_x+n->nom_x+2,n->d_y+n->nom_y,2,0xff);
    if (i==excite_mass) {
      int x=n->d_x+n->nom_x,
          y=n->d_y+n->nom_y-rr-6;
      trigonColor(bview->render,x-2,y,x+2,y,x,y+4,0xff);
    }
  }
}

void audio_callback(Uint8 *stream, int len) {
  int buffer[len/2];
  const int
    voice_fst= play_tune ? 0 : act_instr*2,
    voice_lst= play_tune ? voice_max-1 : voice_fst,
    act_chan=act_color%2;
  for (int i=0;i<len/2;++i) buffer[i]=0;
  bool anynote=!play_tune;
  for (int voice=voice_fst;voice<=voice_lst;voice+=2) {
    if (one_col && voice!=act_instr*2) continue;
    Instrument *inst=INST[voice/2];
    if (inst->pickup_left<0 && inst->pickup_right<0)
      continue;
    Panel *pan=PAN[voice/2];
    NoteBuffer *nbuf[2]={ nbufs.nbuf+voice, nbufs.nbuf+voice+1 },
               *nb;
    for (int i=0;i<len/2;i+=2) {
      if (play_tune) {
        for (int chan=0;chan<2;++chan) { // initialize note buffer
          if (one_col && act_chan!=chan) continue;
          nb=nbuf[chan];
          if (!nb->cur_note || nb->cur_note->cat==eEnd) continue;
          anynote=true;
          if (nb->begin) {
            nb->begin=false;
            if (nb->cur_note->cat==eNote) {
              nb->the_freq=nb->cur_note->freq;
              if (inst->excite_mass>=0) {
                Node &nod=inst->phys_model->nodes[inst->excite_mass];
                if (chan==0) nod.d_x=inst->excit;
                else nod.d_y=inst->excit;
              }
            }
          }
        }
        float freq0=nbuf[0]->the_freq/mid_C,
              freq1=nbuf[1]->the_freq/mid_C;
        inst->phys_model->eval_model(inst,nbuf,freq0,freq1);
        for (int chan=0;chan<2;++chan) { // read phys_model
          if (one_col && act_chan!=chan) continue;
          nb=nbuf[chan];
          if (!nb->cur_note) continue;
          nb->busy+=tempo->value()+tempo_offset;
          if (nb->busy>=nb->cur_note->dur) {
            nb->busy-=nb->cur_note->dur;
            if (nb->cur_note->cat!=eEnd)
              ++nb->cur_note;
            if (nb->cur_note->cat==eNote) {
              nb->begin=true;
              if (nb->cur_note->start_snr > act_snr) {
                do {
                  ++act_snr;
                  if (!ignore->value()) exec_info(act_snr,true);
                } while (act_snr < nb->cur_note->start_snr);
                send_uev([](int actsnr){
                  txtmes2->draw_mes("%d:%d",actsnr/meter,actsnr%meter);
                },act_snr);
              }
            }
          }
        }
      }
      else {
        if (smooth::phase>0) smooth::smooth(inst);
        else 
          inst->phys_model->eval_model(inst,0,1,1);
      }
      Node *nod1= inst->pickup_left>=0 ? inst->phys_model->nodes+inst->pickup_left : 0,
           *nod2= inst->pickup_right>=0 ? inst->phys_model->nodes+inst->pickup_right : 0;
      float left=0,
            right=0;
      if (pan->SET[ePaph].i==0 && nod1 && nod2) { // in-phase?
        left=nod1->d_y + nod2->d_y;
        right=nod1->d_x + nod2->d_x;
      }
      else if (pan->SET[ePaph].i==1 && nod1 && nod2) { // anti-phase?
        left=nod1->d_y - nod2->d_y;
        right=nod1->d_x - nod2->d_x;
      }
      else {
        if (nod1) left=nod1->d_y;
        if (nod2) right=nod2->d_x;
      }
      int val0=int(non_linear(left*400*inst->ampl,inst->tf_mode)),
          val1=int(non_linear(right*400*inst->ampl,inst->tf_mode));
      if (play_tune) {
        buffer[i]+=val0;
        buffer[i+1]+=val1; 
      }
      else {
        buffer[i]=val0;
        buffer[i+1]=val1;
      }
    }
    if (stop_req==eEndReached) {
      if (++inst->phys_model->damp_cnt > 10)
        stop_req=eStopAudio;
    }
  }
  Sint16 *out=reinterpret_cast<Sint16*>(stream);
  for (int i=0;i<len/2;i+=2) {
    out[i]=minmax(-30000,buffer[i],30000);
    out[i+1]=minmax(-30000,buffer[i+1],30000);
    scope::update(out,i);
  }
  if (wav_mode==eWavRecording && !dump_wav((char*)out,len))
    wav_mode=0;
  if (!anynote && !stop_req) stop_req=eEndReached;
}

void extb_cmd(RExtButton *eb,bool is_act) {
  if (!is_act)
    return;
  switch (eb->id) {
    case 'mmas':
      mode=ePlayMode;
      break;
    case 'edma':
      mode=eEditMass;
      break;
    case 'edsp':
      mode=eEditSpring;
      break;
    case 'sved':
      scv_mode=eEdit;
      break;
    case 'ssel':
      scv_mode=eSelect;
      break;
    default:
      alert("extb_cmd '%s'?",id2s(eb->id));
  }
}

void handle_key_event(SDL_Keysym *key,bool down) {
  switch (key->sym) {
    case SDLK_c:
    case SDLK_d:
    case SDLK_k:
    case SDLK_m:
    case SDLK_p:
    case SDLK_v:
    case SDLK_UP:
    case SDLK_DOWN:
    case SDLK_LCTRL:
      break;
    case SDLK_i:
      if (down) { bview->hide(); messages->show(); }
      break;
    default:
      alert("unexpected key");
      return;
  }
}

void bgr_clear(BgrWin *bgr) { bgr->clear(); }

void extrb_label(Render rend,int nr,int) {
  const char *lab=0;
  switch (nr) {
    case 3:
      lab="L: move mass\nM: toggle fixed\nR: toggle pickup"; break;
    case 4:
      lab="L: select mass\nM: insert mass\nR: connect"; break;
    case 5:
      lab="L: select spring\nM: toggle strength\nR: select excited mass"; break;
    case 1:
      lab="L: insert/remove note\nM: staccato note\nR: ins/rm note, color"; break;
    case 2:
      lab="L: select note, column\nM: select color\nR: select all >>"; break;
  }
  draw_ttf->draw_string(rend,lab,Point(3,1));
}

void draw_tfer_curve(BgrWin *tfc) {
  int i,
      x,
      tf_mode=INST[act_instr]->tf_mode;
  const int xstep=40000/tfer_w,
            ydiv=32000/tfer_h;
  tfc->clear();
  hlineColor(tfc->render,0,2*tfer_w-1,tfer_h,0xff00ff);
  vlineColor(tfc->render,tfer_w,0,2*tfer_h-1,0xff00ff);
  SDL_SetRenderDrawColor(tfc->render.render,0,0,0,0xff);
  for (i=0,x=-40000;i<2*tfer_w-1;x+=xstep,++i)
    SDL_RenderDrawPoint(tfc->render.render,i,tfer_h-int(non_linear(x,tf_mode))/ydiv);
  tfc->blit_upd(0);
}

void mouse_down(BgrWin *bgw,int x,int y,int but) {
  switch (bgw->id) {
    case 'scv': sv.mouse_down(x,y,but); break;
    case 'infv': info_buf.mouse_down(sectnr(x)); break;
    default: alert("mouse_down?");
  }
}

void mouse_moved(BgrWin *bgw,int x,int y,int but) {
  switch (bgw->id) {
    case 'scv': sv.mouse_moved(x,y,but); break;
  }
}

void mouse_up(BgrWin *bgw,int x,int y,int but) {
  switch (bgw->id) {
    case 'scv': sv.mouse_up(x,y,but); break;
  }
}

static const int // d   c b   a   g   f e
  line_col[12]=   { 1,0,2,1,0,1,0,1,0,1,1,0 };

void draw_notes() {
  for (int lnr=0;lnr<sclin_max;++lnr) {
    int y=scv_yoff+lnr*sclin_dist;
    for (int snr=leftside;snr<score->len && snr<leftside+nom_snr_max;++snr) {
      ScSection *sec=score->get_section(lnr,snr);
      if (sec->occ()) {
        int x=(snr-leftside)*sect_len+1;
        sec->draw_playsect(x,y);
      }
    }
  }
}

void draw_score(BgrWin *scv) {
  scv->clear();
  txtmes1->draw_mes("%d:%d",leftside/meter,leftside%meter);

  if (score->len-leftside<=0)
    return;
  for (int snr=leftside+1;snr<score->len && snr<leftside+nom_snr_max;++snr) {
    if (snr%meter==0) {
      int x=(snr-leftside)*sect_len;
      vlineColor(scv->render,x,0,scv_h-1,0xb0b0b0ff);
    }
  }
  for (int lnr=0;lnr<sclin_max;++lnr) {
    int lcol=line_col[lnr%12];
    if (lcol) {
      const int xend=min((score->len-leftside)*sect_len,scv_w),
                y=scv_yoff+lnr*sclin_dist;
      hlineColor(scv->render,0,xend-1,y,lcol==2 ? 0xff : 0xb0b0b0ff);
    }
  }
  draw_notes();
}

void draw_info(BgrWin *infv) {
  infv->clear();
  if (score->len-leftside<=0)
    return;
  int snr;
  for (snr=leftside+1;snr<score->len && snr<leftside+nom_snr_max;++snr) {
    if (snr%meter==0) {
      int x=(snr-leftside)*sect_len;
      vlineColor(infv->render,x,0,infv->tw_area.h-1,0xb0b0b0ff);
    }
  }
  for (snr=leftside;snr<=info_buf.lst_info && snr<score->len && snr<leftside+nom_snr_max;++snr) {
    InfoVal *inf=&info_buf.get(snr);
    if (inf->slider_tag>=0) {
      bool done[voice_max/2];
      for (int i=0;i<voice_max/2;++i) done[i]=false;
      for (;inf;inf=inf->next) {
        if (!done[inf->instr]) {
          done[inf->instr]=true;
          int x=(snr-leftside)*sect_len+4,
              y=inf->instr*5+1;
          filledTrigonColor(infv->render,x-3,y,x+3,y,x,y+4,col2color[inf->instr*2]);
        }
      }
    }
  }
  infv->blit_upd(0);
}

void ScSection::draw_playsect(int x,int y) {
  int col;
  if (occ(&col)) {
    bool more=more1(),
         stacc=false;
    if (!more) get(col,0,&stacc);
    int gap= stacc ? 4 : 2,
        color= one_col ? col==act_color ? col2color[col] : 0xb0b0b0ff
                       : more ? 0xf08080ff : col2color[col];
    boxColor(scoreview->render,x,y-2,x+sect_len-gap,y+2,color);
    if (is_sel()) {
      boxColor(scoreview->render,x+2,y-1,x+sect_len-gap-2,y+1,0xffffffff); // erase middle
    }
  }
}

void ScSection::draw_sect(int lnr,int snr) {
  int x=(snr-leftside)*sect_len+1;
  if (x<0 || x>scv_w-sect_len+1) return;
  int y=scv_yoff+lnr*sclin_dist,
      lcol=line_col[lnr%12];
  boxColor(scoreview->render,x,y-2,x+sect_len-2,y+2,0xffffffff); // erase section
  if (lcol)
    hlineColor(scoreview->render,x,x+sect_len,y,lcol==2 ? 0xff : 0xb0b0b0ff); // needed if stacc or not occupied
  if (occ())
    draw_playsect(x,y);
  scoreview->blit_upd(rp(x,y-2,sect_len,5));
}

void ScSection::draw_ghost(int lnr,int snr,bool erase) {
  int x=(snr-leftside)*sect_len+1;
  if (x<0 || x>=scv_w-sect_len || occ()) return;
  int y=scv_yoff+lnr*sclin_dist,
      lcol=line_col[lnr%12];
  boxColor(scoreview->render,x,y-2,x+sect_len-2,y+2,erase ? 0xffffffff : 0xa0a0a0ff);
  if (lcol && erase)
    hlineColor(scoreview->render,x,x+sect_len,y,lcol==2 ? 0xff : 0xb0b0b0ff);
  scoreview->blit_upd(rp(x,y-2,sect_len,5));
}

void ScoreView::sel_column(int snr,int col) { // default: col = -1
  ScSection *sec;
  for (int lnr=0;lnr<sclin_max;++lnr) {
    sec=score->get_section(lnr,snr);
    bool b;
    if (col<0) b=sec->occ();
    else b=sec->occ(act_color);
    if (b) {
      if (col<0) sec->set_sel(true);
      else sec->set_sel(true,col);
      sec->draw_sect(lnr,snr);
      ScSection sect;
      if (col<0) sect.copy(sec);
      else sect.copy(sec,col);
      selected.to_rose();
      selected.insert(lnr,snr,&sect);
    }
  }
}

void ScoreView::edit_down(ScSection* sec,int lnr,int snr,int but) {
  if (but==SDL_BUTTON_RIGHT) {
    if (sec->occ(act_color)) {
      state=eErasingCol;
      sec->reset(act_color);
    }
    else {
      state=eTrackingCol;
      sec->set(act_color,true,false);
    }
  }
  else if (but==SDL_BUTTON_MIDDLE || but==SDL_BUTTON_LEFT) {
    if (sec->occ()) {
      state=eErasing;
      sec->reset();
    }
    else {
      state=eTracking;
      sec->set(act_color,true,false);
    }
  }
  else
    return;
  if (score->lst_sect<snr) score->lst_sect=snr;
  sec->draw_sect(lnr,snr);
  SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
}


void ScoreView::select_down(ScSection *sec,const int lnr,int snr,int but) {
  if (but==SDL_BUTTON_RIGHT) { // select all notes at right side
    for (;snr<score->len;++snr) sel_column(snr);
  }
  else if (but==SDL_BUTTON_MIDDLE) {
    if (sec->occ(act_color)) { // select rest of note with act_color
      bool b=sec->is_sel(act_color);
      while (true) {
        if (b) {
          if (sec->is_sel(act_color)) selected.remove(lnr,snr);
          sec->set_sel(false);
        }
        else {
          sec->set_sel(true,act_color);
          selected.to_rose();
          selected.insert(lnr,snr,sec);
        }
        sec->draw_sect(lnr,snr);
        bool stacc;
        sec->get(act_color,0,&stacc);
        if (stacc) break;
        sec=score->get_section(lnr,++snr);
        if (!sec->occ(act_color) || snr>=score->len-1) break;
      }
    }
    else {
      state=eCollectSel1;
      SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
      sel_column(snr,act_color);
    }
  }
  else if (but==SDL_BUTTON_LEFT) {
    int scol;
    if (sec->occ(&scol)) {        // any color occupied?
      bool b=sec->is_sel();  // if b, then at least 1 color selected
      while (true) {
        if (b) {
          if (sec->is_sel())      // any color selected?
            selected.remove(lnr,snr);
          sec->set_sel(false);        // all colors unselected  
        }
        else {
          sec->set_sel(true);        // all colors selected
          selected.to_rose();
          selected.insert(lnr,snr,sec);
        }
        sec->draw_sect(lnr,snr);
        bool stacc;
        sec->get(scol,0,&stacc);  // stacc of first occupied color
        if (stacc) break;
        sec=score->get_section(lnr,++snr);
        if (!sec->occ() ||        // no color occupied?
            snr>=score->len-1) break;
      }
    }
    else {
      state=eCollectSel;
      SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
      sel_column(snr);
    }
  }
}

void ScoreView::mouse_down(int x,int y,int but) {
  const int snr=sectnr(x),
        lnr=linenr(y);
  state=eIdle;
  if (lnr<0 || lnr>=sclin_max || snr<0) return;
  ScSection *sec=score->get_section(lnr,snr);  // may increase score->len
  prev_snr=snr;
  cur_lnr=lnr;
  cur_snr=snr;
  cur_leftside=leftside;
  cur_point=prev_point=Point(x,y);
  if (keystate[SDL_SCANCODE_LCTRL]) {
    state=eScroll;
    SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
    return;
  }
  if (keystate[SDL_SCANCODE_M] || keystate[SDL_SCANCODE_C]) {
    if (!selected.sd_list.lis) return;
    state= keystate[SDL_SCANCODE_C] ? eCopying : eMoving;
    delta_lnr=delta_snr=left_snr=0;
    SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
  }
  else if (keystate[SDL_SCANCODE_I]) {
    char buf[60];
    snprintf(buf,60,"line %d note-unit %d",lnr,snr);
    messages->add_text(buf,false);
    for (int c=0;c<voice_max;++c) {
      bool play,stacc;
      sec->get(c,&play,&stacc);
      if (!play) continue;
      snprintf(buf,60,"  voice %d (%s): stacc=%d sel=%d",c,col_name[c],stacc,sec->is_sel(c));
      messages->add_text(buf,false);
    }
    messages->draw_blit_recur(); messages->upd();
  }
  else if (keystate[SDL_SCANCODE_P]) { // paste sections from selected
    SLList_elem<SectData> *sd=selected.sd_list.lis;
    if (!sd) return;
    ScSection *sec1;
    for (;sd;sd=sd->nxt) {
      ScSection *from=score->get_section(sd->d.lnr,sd->d.snr);
      from->set_sel(false); // unselect source section
      from->draw_sect(sd->d.lnr,sd->d.snr);
    }
    int min_snr=selected.min_snr(); // leftmost selected section
    for (sd=selected.sd_list.lis;sd;sd=sd->nxt) {
      sec1=score->get_section(sd->d.lnr,sd->d.snr);
      int snr1=sd->d.snr + snr - min_snr;
      if (score->lst_sect<snr1) score->lst_sect=snr1;
      sec1=score->get_section(sd->d.lnr,snr1);
      for (int col=0;col<voice_max;++col)
        if (sd->d.sect.is_sel(col)) sec1->copy(&sd->d.sect,col);
      sec1->draw_sect(sd->d.lnr,snr1);
      sd->d.snr=snr1;
    }
  }
  else if (scv_mode==eEdit) edit_down(sec,lnr,snr,but);
  else if (scv_mode==eSelect) select_down(sec,lnr,snr,but);
  else alert("ScV: mouse_down?");
}

void ScoreView::mouse_moved(int x,int y,int but) {
  if (keystate[SDL_SCANCODE_LCTRL] && state!=eScroll) // ctrl key pressed too late?
    state=eScroll;
  switch (state) {
    case eIdle:
      break;
    case eScroll: {
        int dx= x-cur_point.x,
            new_left=max(0,(cur_leftside*sect_len-dx)/sect_len);
        if (leftside!=new_left) {
          leftside=new_left;
          infoview->draw_blit_upd();
          scoreview->draw_blit_upd();
        }
      }
      break;
    case eCollectSel:
    case eCollectSel1: {
        int snr=sectnr(x);
        if (snr<0 || snr>=score->len) { state=eIdle; return; }
        if (snr>prev_snr)
          for (++prev_snr;;++prev_snr) {
            if (state==eCollectSel) sel_column(prev_snr); else sel_column(prev_snr,act_color);
            if (prev_snr==snr) break;
          }
        else if (snr<prev_snr)
          for (--prev_snr;;--prev_snr) {
            if (state==eCollectSel) sel_column(prev_snr); else sel_column(prev_snr,act_color);
            if (prev_snr==snr) break;
          }
      }     
      break;
    case eMoving:
    case eCopying: {
        SLList_elem<SectData> *sd;
        int lnr1, snr1,
            dx=(x-prev_point.x)/sect_len,
            dy=(y-prev_point.y)/sclin_dist;
        if (dy || dx) {
          prev_point.set(x,y);
          for (sd=selected.sd_list.lis;sd;sd=sd->nxt) { // erase old ghost notes
            lnr1=sd->d.lnr + delta_lnr;
            snr1=sd->d.snr + delta_snr + left_snr;
            if (lnr1>=0 && lnr1<sclin_max && snr1>=0 && snr1<score->len)
              score->get_section(lnr1,snr1)->draw_ghost(lnr1,snr1,true);
          }
          delta_lnr=(y-cur_point.y)/sclin_dist;
          delta_snr=(x-cur_point.x)/sect_len;
          for (sd=selected.sd_list.lis;sd;sd=sd->nxt) { // draw new ghost notes
            lnr1=sd->d.lnr + delta_lnr;
            snr1=sd->d.snr + delta_snr + left_snr;
            if (lnr1>=0 && lnr1<sclin_max && snr1>=0) {
              score->get_section(lnr1,snr1)->draw_ghost(lnr1,snr1,false);
            }
          }
        }
      }
      break;
    default: {
        int snr,
            new_snr=sectnr(x);
        if (new_snr<=prev_snr) return; // only tracking to the right
        for (snr=prev_snr+1;;++snr) {
          if (snr<0 || snr>=score->len) {
            state=eIdle;
            return;
          }
          ScSection *const sect=score->get_section(cur_lnr,snr);
          bool play;
          if (state==eTrackingCol || state==eErasingCol) {
            sect->get(act_color,&play,0);
            if (state==eTrackingCol) { 
              if (!play) {
                sect->set(act_color,true,false);
                sect->draw_sect(cur_lnr,snr);
              }
              else { state=eIdle; break; }
            }
            else {
              if (play) {
                sect->reset(act_color);
                sect->draw_sect(cur_lnr,snr);
              }
              else { state=eIdle; break; }
            }
          }
          else {
            if (state==eTracking) {
              if (!sect->occ()) {
                sect->set(act_color,true,false);
                sect->draw_sect(cur_lnr,snr);
              }
              else { state=eIdle; break; }
            }
            else {
              if (sect->occ()) {
                if (state==eErasing)
                  sect->reset();
                else
                  sect->reset(act_color);
                sect->draw_sect(cur_lnr,snr);
              }
              else { state=eIdle; break; }
            }
          }
          if (snr==new_snr) {
            if (score->lst_sect<snr) score->lst_sect=snr;
            break;
          }
        }
        prev_snr=new_snr;
      }
  }
}

void ScoreView::mouse_up(int x,int y,int but) {
  switch (state) {
    case eIdle:
    case eErasing:
    case eErasingCol:
    case eCollectSel:
    case eCollectSel1:
      break;
    case eScroll:
      sv_scroll->set_xpos(leftside*sect_len);
      break;
    case eTracking:
    case eTrackingCol:
      if (but==SDL_BUTTON_MIDDLE) {
        ScSection *const sect=score->get_section(cur_lnr,prev_snr);
        sect->set(act_color,true,true);
        sect->draw_sect(cur_lnr,prev_snr);
      }
      break;
    case eMoving:
    case eCopying: {
        SLList_elem<SectData> *sd;
        ScSection *from,*to;
        int lnr1, snr1,
            to_lnr, to_snr;
        for (sd=selected.sd_list.lis;sd;sd=sd->nxt) { // erase ghost notes
          lnr1=sd->d.lnr + delta_lnr;
          snr1=sd->d.snr + delta_snr + left_snr;
          if (lnr1>=0 && lnr1<sclin_max && snr1>=0 && snr1<score->len)
            score->get_section(lnr1,snr1)->draw_ghost(lnr1,snr1,true);
        }
        if (!keystate[SDL_SCANCODE_K]) {   // not keep last score
          if (delta_lnr || delta_snr) {
            bool warn=false;
                       for (sd=selected.sd_list.lis;sd;sd=sd->nxt) { // check
              to_lnr=sd->d.lnr + delta_lnr;
              to_snr=sd->d.snr + delta_snr + left_snr;
              if (to_lnr<0 || to_lnr>=sclin_max || to_snr<0) {
                warn=true;
                alert("notes in measure %d will disappear",sd->d.snr/meter);
              }
            }
            if (warn) {
              alert("moving/deleting undone");
              break;
            }
            for (sd=selected.sd_list.lis;sd;sd=sd->nxt) { // update source sections
              from=score->get_section(sd->d.lnr,sd->d.snr);
              for (int col=0;col<voice_max;++col) {
                if (from->is_sel(col)) {
                  if (state==eMoving) from->reset(col);
                  else from->set_sel(false,col);
                }
              }
              from->draw_sect(sd->d.lnr,sd->d.snr);
            }
            for (sd=selected.sd_list.lis;sd;) { // update destination sections
              to_lnr=sd->d.lnr + delta_lnr;
              to_snr=sd->d.snr + delta_snr + left_snr;
              to=score->get_section(to_lnr,to_snr);
              for (int col=0;col<voice_max;++col) {
                if (sd->d.sect.is_sel(col)) to->copy(&sd->d.sect,col);
              }
              if (score->lst_sect<to_snr) score->lst_sect=to_snr;
              to->draw_sect(to_lnr,to_snr);
              sd->d.lnr=to_lnr; // re-assign lnr,snr of copied section in Selected
              sd->d.snr=to_snr;
              sd=sd->nxt;
            }
          }
        }
      }
      break;
    default:
      alert("mouse-up state %d?",state);
  }
  state=eIdle;
  SDL_EventState(SDL_MOUSEMOTION,SDL_IGNORE);
}

void sv_scroll_cmd(HScrollbar* scb) {
  if (leftside!=scb->value/sect_len) {
    leftside=scb->value/sect_len;
    scoreview->draw_blit_upd();
    infoview->draw_blit_upd();
  }
}

Panel::Panel() {
  for (int i=0;i<ctrl_max;++i)
    SET[i].i=SET0[i].i=0;
  SET[eAsym].i=SET0[eAsym].i=4;
  SET[eAmpl].i=SET0[eAmpl].i=5;
  SET[eExl].i=SET0[eExl].i=2;
  SET[eFric].i=SET0[eFric].i=5;
  SET[eLstr].i=SET0[eLstr].i=2;
}

void fill_panel(Point top) {
  pan_view=new BgrWin(top_win,Rect(top.x,top.y,170,400),0,
    [](BgrWin *bgr) {
      bgr->draw_raised(0,cForeground,true);
      pan_view->border(tfer_curve,1);
    },
    0,0,0,cForeground);
  int ypos=4;
  voice_col=new RButtons(pan_view,0,Rect(116,ypos+16,30,2*TDIST),"channel",false,[](RButtons*,int nr,int fire) {
    act_color=2*act_instr+nr;
    if (one_col) { draw_notes(); scoreview->blit_upd(); }
  });
  voice_col->add_rbut("  X"); 
  voice_col->add_rbut("  Y");
  CONT[ePizz]=new CheckBox(pan_view,0,Rect(10,ypos,0,14),"pizzicato",0);
  CONT[eNonl]=new CheckBox(pan_view,0,Rect(10,ypos+16,0,14),"non-linear",0);
  CONT[ePnoi]=new CheckBox(pan_view,0,Rect(10,ypos+32,0,14),"pink noise",0);
  RButtons *pu_phase;
  CONT[ePaph]=pu_phase=new RButtons(pan_view,Style(1,1),Rect(10,ypos+50,120,2*TDIST),0,true,0);
  pu_phase->add_rbut("pickups in phase");
  pu_phase->add_rbut("pickups antiphase");
  ypos+=98;
  CONT[eTun]=new HSlider(pan_view,Style(1,1),Rect(10,ypos,150,0),60,"tuning",
    [](HSlider *sl,int fire,bool rel){
        float tun=powf(1.012,float(sl->value()+30));
        act_phm()->tuning=tun;
        if (fire) set_text(sl->text,"%.2f",tun);
    });
  CONT[eAsym]=new HSlider(pan_view,1,Rect(10,ypos+38,80,0),8,"x/y asymmetry",[](HSlider *sl,int fire,bool rel){
        static const float as[9]={ 0.51, 0.97, 0.98, 0.99, 1., 1.01, 1.02, 1.03, 1.98 };
        act_bnc()->asym=act_phm()->asym=as[sl->value()];
        if (fire) set_text(sl->text,"%.2f",as[sl->value()]);
    });
  CONT[eAmpl]=new VSlider(pan_view,1,Rect(110,ypos+38,0,70),7,"volume",[](VSlider *sl,int fire,bool rel){
        static const float amp[8]={ 0, 0.15, 0.2, 0.3, 0.5, 0.7, 1., 1.4 };
        INST[act_instr]->ampl=amp[sl->value()];
        if (fire) set_text(sl->text,"%.2f",amp[sl->value()]);
    });
  CONT[eFric]=new HSlider(pan_view,1,Rect(10,ypos+76,80,0),7,"friction",[](HSlider *sl,int fire,bool rel){
        static const float fric[8]={ 4,2,1,0.4,0.2,0.1,0.04,0.02 };
        float fr=fric[sl->value()]/2200.; // 44100/20 = 2205
        act_phm()->friction=1.-fr;  // bouncy not affected
        if (fire) set_text(sl->text,"%.2fe-3",fr*1000);
    });
  CONT[eLstr]=new HSlider(pan_view,1,Rect(10,ypos+114,80,0),6,"low strength",[](HSlider *sl,int fire,bool rel){
        static const float ls[7]={ 1,0.4,0.2,0.1,0.05,0.03,0.01 };
        act_bnc()->low_sprc=act_phm()->low_sprc=ls[sl->value()];
        if (fire) set_text(sl->text,"%.3f",ls[sl->value()]);
    });
  CONT[eExl]=new HSlider(pan_view,1,Rect(10,ypos+152,80,0),5,"excitation",[](HSlider *sl,int fire,bool rel){
        static const float exc[7]={ 10,20,30,50,70,100 };
        INST[act_instr]->excit=exc[sl->value()];
        if (fire) set_text(sl->text,"%d",INST[act_instr]->excit);
    });
  ypos+=192;
  CONT[eTfer]=tfer_mode=new RButtons(pan_view,1,Rect(10,ypos,100,4*TDIST),"transfer curve",false,[](RButtons *rb,int nr,int fire) {
    INST[act_instr]->tf_mode=rb->value();
    if (fire) // maybe from audio thread
      send_uev([](int){ draw_tfer_curve(tfer_curve); },0); 
  });
  tfer_mode->add_rbut("straight");
  tfer_mode->add_rbut("clip positive");
  tfer_mode->add_rbut("clip symmetric");
  tfer_mode->add_rbut("abs value");
  tfer_curve=new BgrWin(pan_view,Rect(110,ypos,tfer_w*2,tfer_h*2),0,draw_tfer_curve,0,0,0,cWhite);
  new Button(pan_view,1,Rect(10,ypos+62,0,0),"default = current nodes",[](Button *){
      for (int i=0;i<=INST[act_instr]->Nend;++i) {
        Node *n=INST[act_instr]->bouncy->nodes+i;
        n->nom_x+=n->d_x; n->d_x=0;
        n->nom_y+=n->d_y; n->d_y=0;
        n=INST[act_instr]->phys_model->nodes+i;
        n->nom_x+=n->d_x; n->d_x=0;
        n->nom_y+=n->d_y; n->d_y=0;
      }
    });
  new Button(pan_view,1,Rect(10,ypos+82,0,0),"default = current settings",[](Button *){
    for (int i=0;i<ctrl_max;++i) 
      PAN[act_instr]->SET0[i]=PAN[act_instr]->SET[i];
  });
}

void Selected::restore_sel() {
  SLList_elem<SectData> *sd;
  ScSection *sec;
  for (sd=sd_list.lis;sd;sd=sd->nxt) {
    sec=score->get_section(sd->d.lnr,sd->d.snr);
    sec->set_sel(false);
    sec->draw_sect(sd->d.lnr,sd->d.snr);
  }
  reset();
}

void Selected::modify_sel(int mes) {
  SLList_elem<SectData> *sd;
  ScSection *sec;
  for (sd=sd_list.lis;sd;) {
    sec=score->get_section(sd->d.lnr,sd->d.snr);
    switch (mes) {
      case 'uns':
        sec->set_sel(false);
        sec->draw_sect(sd->d.lnr,sd->d.snr);
        break;
      case 'rcol':
        for (int c=0;c<voice_max;++c)
          if (sec->is_sel(c)) sec->reset(c);
        sec->set(act_color,true,false);
        sec->set_sel(true,act_color);
        sd->d.sect.copy(sec); // re-assign copied section in Selected
        sec->draw_sect(sd->d.lnr,sd->d.snr);
        break;
      case 'dels':
        for (int c=0;c<voice_max;++c) 
          if (sec->is_sel(c)) sec->reset(c);
        sec->draw_sect(sd->d.lnr,sd->d.snr);
        break;
      default: alert("modify_sel?");
    }
    sd=sd->nxt;
  }
  switch (mes) {
    case 'uns':
    case 'dels':
      selected.reset();
      selected.to_blue();
      break;
  }
}

int main(int argc,char** argv) {
  for (int an=1;an<argc;an++) {
    if (!strcmp(argv[an],"-h")) {
      puts("This is bouncy-tune");
      puts("Usage:");
      puts("  bouncy-tune [<config-file>]");
      puts("  (the config-file can be created by the application)");
      exit(1);
    }
    if (!strcmp(argv[an],"-db")) debug=true;
    else config_file=argv[an];
  }
  alert_position.set(2,300);
  top_win=new TopWin("Bouncy Tune",Rect(100,100,550,760),SDL_INIT_AUDIO,0,false,topw_disp);
  top_win->bgcol=cBackground;
  handle_kev=handle_key_event;

  bview=new BgrWin(top_win,Rect(190,20,bg_w,bg_h),"physical model",0,bouncy_down,bouncy_moved,bouncy_up,sdl_col(0xe4e4ffff));
  messages=new TextWin(top_win,0,bview->tw_area,12,"messages");
  messages->bgcol=cMesBg;
  messages->hidden=true;
  new Button(messages,Style(0,1),Rect(messages->tw_area.w-20,3,14,14),"X",
    [](Button *b) { messages->hide(); bview->show(); });
  for (int i=0;i<voice_max/2;++i) INST[i]=new Instrument();
  score=new Score();
  mode_ctr=new ExtRButCtrl(Style(2,int_col(cGrey)),extb_cmd);
  mode_ctr->maybe_z=false;
  int ypos=10;
  eb_mmas=mode_ctr->add_extrbut(top_win,Rect(10,ypos,130,3*TDIST+2),Label(extrb_label,3),'mmas');
  mode_ctr->act_lbut=eb_mmas;
  eb_edma=mode_ctr->add_extrbut(top_win,Rect(10,ypos+4+3*TDIST,130,3*TDIST+2),Label(extrb_label,4),'edma');
  eb_edsp=mode_ctr->add_extrbut(top_win,Rect(10,ypos+8+7*TDIST,130,3*TDIST+2),Label(extrb_label,5),'edsp');
  ypos+=11*TDIST;
  new Button(top_win,1,Rect(10,ypos+20,0,0),"reset nodes to default",[](Button *){
      for (int i=0;i<=INST[act_instr]->Nend;++i) {
        Node *n=INST[act_instr]->bouncy->nodes+i;
        n->d_x=n->d_y=n->vx=n->vy=0;
        n=INST[act_instr]->phys_model->nodes+i;
        n->d_x=n->d_y=n->vx=n->vy=0;
      }
    });
  new Button(top_win,1,Rect(10,ypos+40,0,0),"load project ...",[](Button *){
      dialog->dialog_label("load project:",cAlert);
      dialog->dialog_def(config_file ? config_file : "x.bcy",dial_cmd,'r_cf');
    });

  new Button(top_win,1,Rect(10,ypos+60,0,0),"save project ...",[](Button*) {
      dialog->dialog_label("save:",cAlert);
      dialog->dialog_def(config_file ? config_file : "x.bcy",dial_cmd,'w_cf');
    });

  ypos+=80;
  dialog=new DialogWin(top_win,Rect(10,ypos,140,0));
  new Button(top_win,0,Rect(156,ypos+14,20,0),"ok",[](Button*) { dialog->dok(); });

  ypos+=36;
  freeze=new CheckBox(top_win,0,Rect(10,ypos,0,14),"freeze",[](CheckBox *chb){
      if (!chb->value())
        for (int i=0;i<=INST[act_instr]->Nend;++i) {
          INST[act_instr]->phys_model->nodes[i].d_x=INST[act_instr]->bouncy->nodes[i].d_x;
          INST[act_instr]->phys_model->nodes[i].d_y=INST[act_instr]->bouncy->nodes[i].d_y;
        }
    });
  new CheckBox(top_win,0,Rect(10,ypos+16,0,14),"low friction",[](CheckBox *chb){
      INST[act_instr]->bouncy->friction= chb->value() ? INST[act_instr]->phys_model->friction : nominal_friction;
  });

  write_wav=new CheckBox(top_win,0,Rect(10,ypos+32,0,14),"start writing out.wav",[](CheckBox *chb){
      if (chb->value()) {
        if (!i_am_playing && !play_tune) {
          alert("if 'play tune' not enabled,\n   then 'play' must have been started");
          write_wav->set_cbval(false,0);
          return;
        }
        if (!init_dump_wav("out.wav",2,SAMPLE_RATE)) return;
        wav_mode= play_tune ? eWavOutNoAudio : eWavOutStart;
        wav_indic->set_color(0xff00ff); // green
      }
      else { 
        if (!wav_mode) return;
        close_dump_wav();
        wav_indic->set_color(0xffffffff);
        wav_mode=0;
      }
    });
  wav_indic=new Lamp(top_win,Rect(140,ypos+34,0,0));
  ypos+=56;
  tab_ctr=new ExtRButCtrl(Style(0,int_col(cForeground)),[](RExtButton* rb,bool is_act){
    if (is_act) {
      act_instr=rb->id;
      PAN[act_instr]->connect_sliders(true);
      pan_view->draw_blit_recur();
      pan_view->upd();
      act_color= 2 * act_instr + voice_col->value();
      if (one_col) { draw_notes(); scoreview->blit_upd(); }
    }
  });
  tab_ctr->maybe_z=false;
  RExtButton *panel_tabs[voice_max/2];

  fill_panel(Point(10,ypos+16));

  for (int i=0;i<voice_max/2;++i) {
    PAN[i]=new Panel();
    INST[i]->inst_panel=PAN[i];
    const char* lab[]={ "black","blue","green" };
    panel_tabs[i]=tab_ctr->add_extrbut(top_win,Rect(10+i*43,ypos,40,17),lab[i],i);
  }
  tab_ctr->act_lbut=panel_tabs[0];
  ypos=26+bg_h;
  txtmes1=new Message(top_win,Style(1,int_col(cWhite)),"leftside:",Point(190,ypos));
  txtmes2=new Message(top_win,Style(1,int_col(cWhite)),"now:",Point(190+80,ypos));
  txtmes3=new Message(top_win,2,"project:",Point(190+160,ypos));
  ypos+=32;
  infoview=new BgrWin(top_win,Rect(190,ypos,scv_w,17),"modifications",draw_info,mouse_down,0,0,cMesBg,'infv');
  (new CheckBox(top_win,0,Rect(190+scv_w-120,ypos+20,0,0),"current instr only",[](CheckBox *chb){
    draw_notes();
    scoreview->blit_upd();
  }))->d=&one_col;
  scoreview=new BgrWin(top_win,Rect(190,ypos+38,scv_w,scv_h),"score",draw_score,mouse_down,mouse_moved,mouse_up,cWhite,'scv');
  sv_scroll=new HScrollbar(top_win,Style(1,0,sect_len),Rect(190,ypos+scv_h+39,scv_w,0),scv_w+30,sv_scroll_cmd);
  ypos+=scv_h+58;
  scv_edit_ctr=new ExtRButCtrl(Style(2,int_col(cGrey)),extb_cmd);
  scv_edit_ctr->maybe_z=false;
  eb_sved=scv_edit_ctr->add_extrbut(top_win,Rect(190,ypos,130,3*TDIST+2),Label(extrb_label,1),'sved');
  scv_edit_ctr->act_lbut=eb_sved;
  eb_ssel=scv_edit_ctr->add_extrbut(top_win,Rect(330,ypos,130,3*TDIST+2),Label(extrb_label,2),'ssel');
  unselect=new Button(top_win,0,Rect(470,ypos,64,0),"unselect",[](Button *){
    selected.modify_sel('uns');
  });
  del_sel=new Button(top_win,0,Rect(470,ypos+20,64,0),"delete sel",[](Button *){
    selected.modify_sel('dels');
  });
  recol_sel=new Button(top_win,0,Rect(470,ypos+40,64,0),"recolor sel",[](Button *){
    selected.modify_sel('rcol');
  });
  ypos+=80;
  tempo=new HSlider(top_win,1,Rect(190,ypos,90,0),10,"tempo",[](HSlider *sl,int fire,bool rel) {
      set_text(sl->text,"%d",10*(sl->value()+tempo_offset));
   });
  tempo->set_hsval(6,1,false);
  (new CheckBox(top_win,0,Rect(190,ypos+28,0,14),"play tune",[](CheckBox *chb){
    if (i_am_playing) {
      stop_req=eEndReached;
      SDL_WaitThread(audio_thread,0);
    }
  }))->d=&play_tune;
  ignore=new CheckBox(top_win,0,Rect(190,ypos+48,0,14),"ignore modifications",0);
  play_but=new Button(top_win,0,Rect(350,ypos,24,20),right_arrow,play_cmd);
  scope_win=new BgrWin(top_win,Rect(400,ypos,scope_dim,scope_h*2),"oscilloscope",
                       bgr_clear,0,0,0,cMesBg);
  if (config_file) {
    FILE *conf=fopen(config_file,"r");
    if (conf) {
      if (!set_mass_spring(conf)) alert("error in %s",config_file);
      fclose(conf);
    }
    else alert("project file %s not opened",config_file);
  }
  PAN[0]->connect_sliders(true);
  bouncy_thread=SDL_CreateThread(bouncy_threadfun,"bouncy_thread",0);
  get_events();
  return 0;
}  
