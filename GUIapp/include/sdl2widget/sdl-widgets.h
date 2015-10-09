/*  Copyright 2012 W.Boeke

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SDL.h>
#include <SDL_ttf.h>
#include "shapes.h"

extern const SDL_Color cNop, cWhite, cBlack, cGrey, cRed, cBlue;
extern SDL_Color cForeground, cBackground, cBorder, cPointer, cScrollbar;

struct WinBase;

const int
  TDIST=14;  // distance text lines

struct Int2 {
  short x,y;
  Int2(int x,int y);
  Int2();
  void set(int x,int y);
  bool operator!=(Int2 b);
};

struct Point {
  short x, y;
  Point();
  Point(short x,short y);
  void set(short x,short y);
  bool operator==(Point b);
  bool operator!=(Point b);
};

struct Rect:SDL_Rect {
  Rect();
  Rect(Sint16 x,Sint16 y,Uint16 dx,Uint16 dy);
  void set(Sint16 x,Sint16 y,Uint16 dx,Uint16 dy);
};

static Rect *rp(int x,int y,int w,int h) { // creates pointer to Rect. Each source file gets its own instance.
  static Rect rp_rect;
  rp_rect.set(x,y,w,h);
  return &rp_rect;
}

struct Label {
  struct RenderText *render_t;
  void (*draw_cmd)(Render,int id,int y_off);
  const char *str;  // the text
  int id;
  Label(const char* txt);
  Label(void (*draw_cmd)(Render,int id,int y_off),int id=0);
  Label(const char *txt,void (*dr)(Render,int id,int y_off),int id=0);
  void draw(Render,Point pnt);
};

struct Style {
  const int st;
  Uint32 param;
  int param2;
  Style(int st);
  Style(int st,int par);
  Style(int st,int par,int par2);
};

struct RenderText {
  static const int dim=128-' ';
  SDL_Surface *chars[dim+1];
  int ch_wid[dim];
  TTF_Font *ttf_font;
  SDL_Color text_col;
  RenderText(TTF_Font*,SDL_Color text_col);
  int draw_string(Render rend,const char *s,Point);
  int text_width(const char *s);
  void set_char(const int ind);
};

struct WinBase {
  Render render;      // rend.win and rend.render created by init_gui()
  SDL_Surface *title; // created by TTF_RenderText_Blended()
  WinBase *parent;
  WinBase **children;
  struct OnTopWin *ontopw;
  const char *title_str;
  Point topleft; // relative to parent window
  Rect tw_area,  // relative to top window
       title_area; // title surface
  Point title_top;
  int lst_child,end_child;
  SDL_Color bgcol;
  bool hidden;
  WinBase(WinBase *pw,const char *t,int x,int y,int dx,int dy,SDL_Color bgcol);
  ~WinBase();
  void set_parent(WinBase *pw);
  virtual void draw()=0;
  void reloc_title(int dx,int dy); // shift title area
  void set_title(const char* new_t,bool _upd=true);
  void add_child(WinBase* child);
  void remove_child(WinBase* child);
  void clear(Rect *rect=0);
  void clear(Rect*,SDL_Color col,bool _upd);
  void hide();
  void show();
  void draw_raised(Rect *rect,SDL_Color col,bool up);
  void draw_gradient(Rect rect,const struct Color5* col,bool vertical=false,bool hollow=false);
  void border(WinBase *child,int wid=1);
  WinBase *in_a_win(int x,int y);
  void draw_blit_recur();    // recursivily draw surface, blit own and children's surface plus title to parent
  void draw_blit_upd();      // draw surface, blit surface to parents
  void blit_upd(Rect *rect=0); // blit surface to parents
  void upd(Rect *rect=0);    // update top window
  void widen(int dx,int dy); // widen win
  void move(int dx,int dy);  // move win
  bool move_if_ok(int delta_x,int delta_y); // guarded move
  Point tit_os();            // get title offset
  void keep_on_top();        // make staying visible
  void init_gui();
};

struct TopWin:WinBase {
  SDL_Window *window;
  SDL_Texture *texture;
  SDL_Renderer *renderer;
  void (*display_cmd)();
  TopWin(const char* title,Rect rect,Uint32 init_flag,Uint32 video_flag,bool accel,void (*disp_cmd)(),void (*set_icon)(SDL_Window*)=0);
  void draw();
  void refresh(Rect *rect=0);
};

struct Button:WinBase {
  bool is_down;
  Style style;
  Label label;
  void (*cmd)(Button*);
  Button(WinBase *pw,Style,Rect,Label lab,void (*cmd)(Button*),const char *title=0);
  void draw();
};

struct BgrWin:WinBase {  // background window
  void (*display_cmd)(BgrWin*);
  void (*down_cmd)(BgrWin*,int x,int y,int but);
  void (*moved_cmd)(BgrWin*,int x,int y,int but);
  void (*up_cmd)(BgrWin*,int x,int y,int but);
  int id;
  BgrWin(WinBase *pw,
         Rect,
         const char* title,
         void (*display_cmd)(BgrWin*),
         void (*down_cmd)(BgrWin*,int,int,int),
         void (*moved_cmd)(BgrWin*,int,int,int),
         void (*up_cmd)(BgrWin*,int,int,int),
         SDL_Color bgcol,int id=0);
  //void move_contents_h(int delta);  // move contents horizontal
  void draw();
};

// non-editable text window
struct TextWin:WinBase {
  static const int SMAX=200;    // string length
  int linenr,
      lmax;
  Style style;
  char (*textbuf)[SMAX];
  
  TextWin(WinBase *pw,Style,Rect rect,int lmax,const char* title);
  void draw();
  void add_text(const char*,bool do_draw);
  void reset();
};

// radio buttons
struct RButton {
  Label label;
  RButton();
  RButton(Label lab);
};

struct RButtons:WinBase {
  int def_button,
      *d,
      butnr,
      rb_max,
      y_off;
  RButton *but;
  int next();
  const int b_dist; // button distance
  bool maybe_z;  // unselect with second click?
  void (*rb_cmd)(RButtons*,int nr,int fire);
  Style style;
  RButtons(WinBase *parent,Style,Rect,const char *title,bool mbz,void(*cmd)(RButtons*,int nr,int fire));
  ~RButtons();
  void draw_rbutton(RButton *rb);
  void draw();
  void set_rbutnr(int nr,int fire=1,bool do_draw=true);
  int& value();
  void reset();
  RButton *add_rbut(Label lab);
  RButton *is_in_rbutton(SDL_MouseButtonEvent *ev);
};

// extern radio button
struct RExtButton:WinBase {
  struct ExtRButCtrl *rxb_ctr;
  int id;
  Style style;
  Label label;
  RExtButton(WinBase *pw,Style,Rect,Label,int id);
  void draw();
};

struct ExtRButCtrl {
  int butnr;
  bool maybe_z;  // can all buttons be unselected?
  Style style;
  RExtButton *act_lbut;
  void (*reb_cmd)(RExtButton*,bool is_act);
  ExtRButCtrl(Style,void (*cmd)(RExtButton*,bool is_act));
  int next();
  void set_rbut(RExtButton*,int fire=1);
  void reset();
  RExtButton *add_extrbut(WinBase *pw,Rect,Label lab,int id);
};

// horizontal slider
struct HSlider:WinBase {
  const int
    sdx,
    maxv,
    x_sens; // sensitivity
  int def_data,  // default buffer for data
      *d,
      d_start;
  const char *lab_left,
             *lab_right;
  char *text;
  Style style;
  void (*cmd)(HSlider*,int fire,bool rel);
  bool rel_cmd;
  HSlider(WinBase *parent,Style,Rect rect,int maxval,const char* t,
          void (*cmd)(HSlider*,int fire,bool rel),bool rel_cmd=false);
  int &value();
  void calc_hslval(int x,bool init);
  void draw();
  void set_hsval(int val,int fire=1,bool do_draw=true,bool rel=false);
};

// vertical slider
struct VSlider:WinBase {
  const int
    sdy,
    maxv,
    y_sens;
  int def_data,  // default buffer for data
      *d,
      d_start;
  char *text;
  Style style;
  void (*cmd)(VSlider*,int fire,bool rel);
  bool rel_cmd;
  Rect txt_rect;
  VSlider(WinBase *parent,Style,Rect rect,int maxval,const char* t,
          void (*cmd)(VSlider*,int fire,bool rel),bool rel_cmd=false);
  int &value();
  void calc_vslval(int x,bool init);
  void draw();
  void set_vsval(int val,int fire=1,bool do_draw=true,bool rel=false);
  void hide(); // = WinBase::hide + hide text
  void show(); // = WinBase::show + draw text
};

// 2-dimensional slider
struct HVSlider:WinBase {
  const int sdx, sdy; // active area
  const Int2 maxv;
  Int2 def_data,  // default buffer for data
       *d;
  char *text_x,*text_y;
  Style style;
  void (*cmd)(HVSlider*,int fire,bool rel);
  bool rel_cmd;
  Rect txt_rect;
  HVSlider(WinBase *pw,Style,Rect rect,Int2 maxval,const char* t,
           void (*cmd)(HVSlider*,int fire,bool rel),bool rel_cmd=false);
  void calc_hvslval(Int2 i2);
  Int2& value();
  void draw();
  void set_hvsval(Int2,int fire=1,bool do_draw=true,bool rel=false);
  void hide(); // like VSlider::hide()
  void show();
};

// circular slider
struct Dial:WinBase {
  const int maxv;
  int def_data,  // default buffer for data
      *d;
  char *text;
  Style style;
  void (*cmd)(Dial*,int fire,bool rel);
  bool rel_cmd;
  const Point mid;
  static const int pnt_max=4,
                   sdx=60; // mouse sensitivity
  int d_start;
  const Point loc[pnt_max];
  Point aloc[pnt_max]; // actual location
  float radius[pnt_max],
        angle[pnt_max],
        ang;
  Dial(WinBase *parent,Style,Rect rect,int maxval,const char* t,
       void (*cmd)(Dial*,int fire,bool rel),bool rel_cmd=false);
  int &value();
  void calc_dialval(int x,bool init);
  void draw();
  void set_dialval(int val,int fire=1,bool do_draw=true,bool rel=false);
  void rotate();
  Point rotate(float r,float a);
};

// check box
struct CheckBox:WinBase {
  bool def_val,
       *d;
  Label label;
  Style style;
  void (*cmd)(CheckBox*);
  CheckBox(WinBase *pw,Style,Rect,Label lab,void (*cmd)(CheckBox*));
  bool &value();
  void draw();
  void set_cbval(bool,int fire=1,bool do_draw=true);
};

// horizontal scrollbar
struct HScrollbar:WinBase {
  int range, p0, xpos, wid,
      value;
  Style style;
  const int ssdim; // soft scroll area
  void (*cmd)(HScrollbar*);
  HScrollbar(WinBase *pw,Style,Rect,int r,void (*cmd)(HScrollbar*));
  void calc_params(int r);
  void draw();
  void set_range(int range,bool upd=true);
  void calc_xpos(int newx);
  void set_xpos(int newx,bool upd=true);
  void inc_value(bool incr);
  bool in_ss_area(SDL_MouseButtonEvent *ev,bool *dir);
};

// vertical scrollbar
struct VScrollbar:WinBase {
  int range, p0, ypos, height,
      value;
  Style style;
  const int ssdim; // soft scroll area
  void (*cmd)(VScrollbar*);
  VScrollbar(WinBase *pw,Style,Rect,int r,void (*cmd)(VScrollbar*));
  void calc_params(int r);
  void draw();
  void set_range(int range,bool upd=true);
  void calc_ypos(int newy);
  void set_ypos(int newy,bool upd=true);
  void inc_value(bool incr);
  bool in_ss_area(SDL_MouseButtonEvent *ev,bool *dir);
};

// editable text window
struct EditWin:WinBase {
  int linenr,
      lmax,
      y_off;
  struct Line **lines;
  void (*cmd)(int ctrl_key,int key);
  struct {
    int x,y;
  } cursor;
  EditWin(WinBase *pw,Rect,const char *title,void (*cmd)(int ctrl_key,int key));
  void draw();
  void draw_line(int vpos,bool upd);
  void set_offset(int yoff);
  void set_cursor(int x,int y);
  void unset_cursor();
  void set_line(int n,bool upd,const char *form,...);     // assign lines[n]
  void insert_line(int n,bool upd,const char *form,...);  // insert a line at lines[n]
  const char *get_line(int n);
  void reset();
  void get_info(bool *active,int* nr_of_lines,int* cursor_ypos,int *nr_chars,int *total_nr_chars);
  void handle_key(SDL_Keysym *key);
};

struct DialogWin:WinBase {
  Rect textr;  // text area
  int cursor,
      cmd_id;
  const char *label;
  struct Line *lin;
  void (*cmd)(const char* text,int cmd_id);
  DialogWin(WinBase *pw,Rect);
  void draw_line();
  void set_cursor(int pos);
  void unset_cursor();
  void draw();
  void dialog_label(const char *s,SDL_Color col=cNop);
  void dialog_def(const char *str,void(*cmd)(const char* text,int cmdid),int cmd_id);
  bool handle_key(SDL_Keysym *key);
  void dok();
};

struct Lamp {
  WinBase *pwin;
  Rect rect;
  Uint32 col;
  bool hidden;
  Lamp(WinBase *pw,Rect);
  void draw();
  void set_color(Uint32 col);
  void show();
};

struct Message {
  WinBase *pwin;
  Style style;
  const char *label;
  Point lab_pt; // label Point
  Rect mes_r;   // message Rect
  RenderText *custom_ttf;
  Message(WinBase *pw,Style,const char* lab,Point top);
  void draw_label(bool upd=false);
  void draw_mes(const char *form,...);
  void draw(const char *form,...);
private:
  void draw_message(const char *form,va_list ap);
};

struct CmdMenu {
  Rect mrect;
  int nr_buttons,
      def_val,
      *d;
  bool sticky;
  Button *src;
  RButtons *buttons;
  CmdMenu(Button *src);
  bool init(int wid,int nr_buttons,void (*menu_cmd)(RButtons*,int nr,int fire));
  RButton* add_mbut(Label lab);
  int& value();
  void mclose();
};

struct Lazy {
  bool pause,
       cmd_done;
  const int dur;
  void (*cmd)(int);
  SDL_mutex *mtx;
  SDL_cond *cond;
  Lazy(void (*_cmd)(int),int dur);
  void threadfun();
  void kick();
};

extern bool sdl_running;
extern RenderText *draw_ttf,
                  *draw_title_ttf,
                  *draw_mono_ttf,
                  *draw_blue_ttf;
extern Point alert_position;
extern const char *def_fontpath;
extern void (*handle_kev)(SDL_Keysym *key,bool down);    // keyboard event
extern void (*handle_rsev)(int dw,int dh);               // resize event

void send_uev(void (*cmd)(int),int par);
void set_text(char *&txt,const char *form,...);
void say(const char *form,...);
void err(const char *form,...);
void alert(const char *form,...);
const char *id2s(int id);
void get_events();
SDL_Surface *create_pixmap(const char* pm_data[]);
SDL_Cursor *init_system_cursor(const char *image[]);
void file_chooser(void (*cmd)(const char* f_name));
void working_dir(void (*cmd)(const char* dir_name));
void print_h();  // print widget hierarchy
namespace mwin { // moving BgrWin
  void down(BgrWin *bgw,int x,int y,int but);
  void move(BgrWin *bgw,int x,int y,int but);
  void up(BgrWin *bgw,int x,int y,int but);
}
void fill_rect(Render,Rect *rect,SDL_Color col);
