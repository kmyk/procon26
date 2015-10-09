/*  
    Demo program for SDL-widgets-2.1
    To be included in make-waves.cpp
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

#include <math.h>
const int TESTSAMPLES=SAMPLE_RATE/10; // so 20Hz will be tested 2 times

bool change_param;

struct FilterBase {
  int mode;
  FilterBase():mode(0) {
  }
  virtual void init(float cutoff,int slider_val,char **txt)=0;
  virtual float getSample(float input)=0;
  virtual const char *get_mode(int nr)=0;
};

struct Filter2:FilterBase {
  float freqcut,fc1,fc2,
        d1,d2,d3,d4,
        qres;
  enum { Lopass,Bandpass,Hipass };
  const char *get_mode(int nr) {
    if (nr>=3) return 0;
    static const char *mm[7]={ "lowpass","bandpass","highpass" };
    return mm[nr];
  }
  void init(float cutoff,int sl_val,char **txt) {
    freqcut=2 * M_PI * cutoff / SAMPLE_RATE;
    fc1=fc2=freqcut;
    //fc1=freqcut/1.1; fc2=freqcut*1.1; // <-- flatter peak
    d1=d2=d3=d4=0;
    static float filter_q[8]={ 1.4,1.0,0.6,0.4,0.3,0.2,0.14,0.1 };
    qres=filter_q[sl_val];
    set_text(*txt,"%.1f",1./qres);
  }
/*
  float getSample(float input) {  // with high-freq correction
    float highpass,out1;
    if (change_param) in1=0.5*(input+in1);
    else in1=input;
    d2+=fc1*d1;                // d2 = lowpass
    highpass=in1-d2-qres*d1;
    d1+=fc1*(in1-d2-qres*d1); // d1 = bandpass
    out1= mode==Lopass ? d2 : mode==Bandpass ? d1 : mode==Hipass ? highpass : 0;

    if (change_param) in2=0.5*(out1+in2);
    else in2=out1;
    d4+=fc2*d3;                // d4 = lowpass
    highpass=in2-d4-qres*d3;
    d3+=fc2*highpass;    // d3 = bandpass
    return mode==Lopass ? d4 : mode==Bandpass ? d3 : highpass;
  }
*/
/*
  float getSample(float input) { // with soft clipping at 2nd input
    float hp,out1,out2;
    d2+=fc1*d1;                // d2 = lowpass
    hp=input-d2-qres*d1;
    d1+=fc1*hp; // d1 = bandpass
    out1= mode==Lopass ? d2 : mode==Bandpass ? d1 : mode==Hipass ? hp : 0;

    d4+=fc2*d3;                // d4 = lowpass
    if (change_param) {
      float in=out1-d4-qres*d3;
      hp=in/(1+in*in*0.1);
    }
    else
      hp=out1-d4-qres*d3;
    d3+=fc2*hp;    // d3 = bandpass
    out2=mode==Lopass ? d4 : mode==Bandpass ? d3 : hp;
    return out2;
  }
*/
  float getSample(float input) { // with high-Q damping
    float highpass,output;
    d2+=fc1*d1;                // d2 = lowpass
    highpass=input-d2-qres*d1;
    d1+=fc1*highpass;          // d1 = bandpass
    output= mode==Lopass ? d2 : mode==Bandpass ? d1 : mode==Hipass ? highpass : 0;

    d4+=fc2*d3;                // d4 = lowpass
    highpass=output-d4-qres*d3;
    if (change_param)
      d3=(d3+fc2*highpass)/(1+d3*d3*0.2);
    //  d3=(d3+fc2*highpass)/(1-fminf(0,d1*d3)); // alternative?
    else d3+=fc2*highpass;    // d3 = bandpass
    output= mode==Lopass ? d4 : mode==Bandpass ? d3 : highpass;
    return output;
  }
};

struct Biquad:FilterBase {
  float a0, a1, a2, a3, a4,
        x1, x2, y1, y2,
        qres,
        result;
  const char *get_mode(int nr) {
    if (nr>=6) return 0;
    static const char *mm[7]={  "LP", "BP", "HP", "notch", "EQ-H", "EQ-L" };
    return mm[nr];
  }
  void init(float cutoff,int sl_val,char **txt) {
    set_text(*txt,0);
    enum {  LPF, BPF, HPF, NOTCH, PEQ_H, PEQ_L };
    float omega, sn, cs, alpha,
          A0, A1, A2, B0, B1, B2;
    /* setup variables */
    omega = 2 * M_PI * cutoff / SAMPLE_RATE;
    sn = sin(omega);
    cs = cos(omega);
    static float filter_q[8]={ 1.4,1.0,0.6,0.4,0.3,0.2,0.14,0.1 };
    qres=filter_q[sl_val];
    set_text(*txt,"%.1f",1./qres);

    alpha = sn * qres / 2.;

    switch (mode) {
    case LPF:
        B0 = (1 - cs) /2;
        B1 = 1 - cs;
        B2 = (1 - cs) /2;
        A0 = 1 + alpha;
        A1 = -2 * cs;
        A2 = 1 - alpha;
        break;
    case HPF:
        B0 = (1 + cs) /2;
        B1 = -(1 + cs);
        B2 = (1 + cs) /2;
        A0 = 1 + alpha;
        A1 = -2 * cs;
        A2 = 1 - alpha;
        break;
    case BPF:
        B0 = alpha;
        B1 = 0;
        B2 = -alpha;
        A0 = 1 + alpha;
        A1 = -2 * cs;
        A2 = 1 - alpha;
        break;
    case NOTCH:
        B0 = 1;
        B1 = -2 * cs;
        B2 = 1;
        A0 = 1 + alpha;
        A1 = -2 * cs;
        A2 = 1 - alpha;
        break;
    case PEQ_H:
    case PEQ_L: {
        float A= mode==PEQ_H ? 2. : 0.5;
        B0 = 1 + (alpha * A);
        B1 = -2 * cs;
        B2 = 1 - (alpha * A);
        A0 = 1 + (alpha /A);
        A1 = -2 * cs;
        A2 = 1 - (alpha /A);
      }
      break;
/*
    case LSH:
        B0 = A * ((A + 1) - (A - 1) * cs + beta * sn);
        B1 = 2 * A * ((A - 1) - (A + 1) * cs);
        B2 = A * ((A + 1) - (A - 1) * cs - beta * sn);
        A0 = (A + 1) + (A - 1) * cs + beta * sn;
        A1 = -2 * ((A - 1) + (A + 1) * cs);
        A2 = (A + 1) + (A - 1) * cs - beta * sn;
        break;
    case HSH:
        B0 = A * ((A + 1) + (A - 1) * cs + beta * sn);
        B1 = -2 * A * ((A - 1) + (A + 1) * cs);
        B2 = A * ((A + 1) + (A - 1) * cs - beta * sn);
        A0 = (A + 1) - (A - 1) * cs + beta * sn;
        A1 = 2 * ((A - 1) - (A + 1) * cs);
        A2 = (A + 1) - (A - 1) * cs - beta * sn;
        break;
*/
     default:
        alert("mode?");
        B0=B1=B2=A0=A1=A2=0; // make compiler happy
    }

    /* precompute the coefficients */
    a0 = B0 /A0;
    a1 = B1 /A0;
    a2 = B2 /A0;
    a3 = A1 /A0;
    a4 = A2 /A0; //printf("a3=%.3f a4=%.3f\n",a3,a4);

    /* zero initial samples */
    x1 = x2 = 0;
    y1 = y2 = 0;
  }
  
  float getSample(float sample) {
    float change= change_param ? 0.4 : 0;
    result= a0 * sample + a1 * x1 + a2 * x2 - (a3+change) * y1 - a4 * y2;
    x2 = x1;
    x1 = sample;
    y2 = y1;
    y1 = result;
    return result;
  }
};

struct PinkNoise:FilterBase { // oversampled
  float b0,b1,b2,
        prev,out;
  const char *get_mode(int nr) { 
    if (nr>=3) return 0;
    static const char *mm[7]={ "2 sections","3 sections","oversampled" };
    return mm[nr];
  }
  void init(float,int,char **txt) {
    b0=b1=b2=prev=out=0.;
  }
  float getSample(float input) {
    input= input/4. - out/20.;  // scaling and DC blocker
    if (mode==0) {  // 2 sections
      b1 = 0.98 * b1 + input * 0.4;
      b2 = 0.6 * b2 + input;
      out = b1 + b2;
    }
    else {          // 3 sections
      if (mode==2) {             // oversampling
        float pr=(input+prev)/2; // interpolation
        prev=input;
        b0 = 0.998 * b0 + pr * 0.1;
        b1 = 0.963 * b1 + pr * 0.3;
        b2 = 0.57 * b2 + pr;
      }
      b0 = 0.998 * b0 + input * 0.1;
      b1 = 0.963 * b1 + input * 0.3;
      b2 = 0.57 * b2 + input;
      out= b0 + b1 + b2;
    }
    return out;
  }
};  

/* from sinc.c:
3 points: 1.00 0.64 (0.44 0.28)
4 points: 1.00 0.50 (0.33 0.17)
5 points: 1.00 0.83 0.41 (0.29 0.24 0.12)
6 points: 1.00 0.72 0.35 (0.24 0.17 0.08)
*/

struct ThreePoint:FilterBase {
  float d1,d2,d3,d4,out,
        f_q;
  const char *get_mode(int nr) { 
    if (nr>=3) return 0;
    static const char *name[3]={ "2 points","3 points","4 points" };
    return name[nr];
  }
  void init(float,int sl_val,char **txt) {
    d1=d2=d3=d4=out=0;
    const float fq_arr[6]={ 0,0.4,0.7,1.,1.4,1.7 };
    f_q=fq_arr[sl_val];
    set_text(*txt,"%.1f",f_q);
  }
  float getSample(float input) {
    switch (mode) {
      case 0:
        out=((d1=d2)+(d2=input-f_q*out))*0.5;
/*
        d1=d2; d2=input-f_q*out;
        out=(d1+d2)*0.5;
*/
        break;
      case 1:
        d1=d2; d2=d3; d3=input-f_q*out;
        out=0.28*(d1+d3) + 0.44*d2;
        break;
      case 2:
        d1=d2; d2=d3; d3=d4; d4=input-f_q*out;
        out=0.17*(d1+d4) + 0.33*(d2 + d3);
        break;
    }
    return 2*out;
  }
};

float exp2ap (float x)
{
    int i;

    i = (int)(floorf (x));
    x -= i;
    return ldexpf (1 + x * (0.6930f + x * (0.2416f + x * (0.0517f + x * 0.0137f))), i);
}

struct MoogVCF:FilterBase { // from: http://www.musicdsp.org/showone.php?id=26
  float in,in1,in2,in3,in4,
        out1,out2,out3,out4,
        qres,
        fc,fc_4,
        fb;
  const char *get_mode(int nr) {
    if (nr>=1) return 0;
    static const char *mm[]={ "lowpass" };
    return mm[nr];
  }
  void init(float cutoff,int sl_val,char **txt) {
    fc=2 * M_PI * cutoff / SAMPLE_RATE * 1.16;
    fc_4=fc*fc*fc*fc;
    in1=in2=in3=in4=out1=out2=out3=out4=0;
    static float filter_q[8]={ 0.2,0.3,0.4,0.6,1,1.4 };
    qres=filter_q[sl_val];
    fb = qres * (1.0 - 0.15 * fc * fc);
    set_text(*txt,"%.1f",qres);
  }
  float getSample(const float input) {
    if (change_param) in=(input - out4 * fb)/(1+out4*out4*0.1) * fc_4;
    else in=(input - out4 * fb) * fc_4;
    out1 = in + 0.3 * in1 + (1 - fc) * out1;    // Pole 1
    in1  = in;
    out2 = out1 + 0.3 * in2 + (1 - fc) * out2;  // Pole 2
    in2  = out1;
    out3 = out2 + 0.3 * in3 + (1 - fc) * out3;  // Pole 3
    in3  = out2;
    out4 = out3 + 0.3 * in4 + (1 - fc) * out4;  // Pole 4
    //out4 = (out3 + 0.3 * in4 + (1 - fc) * out4)/(change_param ? 1+out4*out4*0.1 : 1);  // Pole 4, with soft clipping
    in4  = out3;
    return out4;
  }
/* ORIGINAL
  float run(float input, float fc, float res) {
    float f = fc * 1.16;
    float fb = res * (1.0 - 0.15 * f * f);
    input -= out4 * fb;
    input *= 0.35013 * (f*f)*(f*f);
    out1 = input + 0.3 * in1 + (1 - f) * out1; // Pole 1
    in1  = input;
    out2 = out1 + 0.3 * in2 + (1 - f) * out2;  // Pole 2
    in2  = out1;
    out3 = out2 + 0.3 * in3 + (1 - f) * out3;  // Pole 3
    in3  = out2;
    out4 = out3 + 0.3 * in4 + (1 - f) * out4;  // Pole 4
    in4  = out3;
    return out4;
  }
*/
};

/*
struct MoogVcf2:FilterBase { // from: MCP-plugins-0.4.0/mvclpf24.cc
  float freqcut,
        qres,
        g0, g1,
        p1, p4,
        c1, c2, c3, c4, c5,
        r, // feedback factor
        w, // cutoff frequency
        x, // input to filter sections
        ipgain,
        opgain;

  const char *get_mode(int nr) {
    if (nr>=1) return 0;
    static const char *mm[]={ "lowpass" };
    return mm[nr];
  }
  void init(float cutoff,int sl_val,char **txt) { printf("cutoff=%.0f sl_val=%d\n",cutoff,sl_val);
    ipgain=1.;
    opgain=1.;
    c1 = c2 = c3 = c4 = c5 = w = r = 0;
    c1 = 1e-6f;
    freqcut=2 * M_PI * cutoff;
    g0 = exp2ap (0.1661f * ipgain) / 4;
    g1 = exp2ap (0.1661f * opgain) * 4;
    static float filter_q[8]={ 1.4,1.0,0.6,0.4,0.3,0.2,0.14,0.1 };
    p4=qres=filter_q[sl_val];
    set_text(*txt,"%.1f",1./qres);
  }
  float getSample(float p0) {
    float t;
    t = exp2ap (freqcut + 10.71f) / SAMPLE_RATE;
    if (t < 0.8f) t *= 1 - 0.4f * t - 0.125f * t * t;
    else {
        t *= 0.6f; 
        if (t > 0.92f) t = 0.92f;
    }
    w=t;

    t = qres * p4;// + qres;
    if (t > 1) t = 1;
    if (t < 0) t = 0;
    r=t;

    x = -4.5f * r * c5 + p0 * g0 + 1e-10f;
    c1 += w * (x  - c1);
    c2 += w * (c1 - c2);
    c3 += w * (c2 - c3);
    c4 += w * (c3 - c4);

    p1 = g1 * c4;
    c5 += 0.5f * (c4 - c5);

    return p1;
  }
};
*/
