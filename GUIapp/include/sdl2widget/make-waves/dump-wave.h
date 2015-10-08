bool init_dump_wav(const char *fname,int channels,int sample_rate);
bool dump_wav(char *buf, int size);
bool close_dump_wav();
extern void alert(const char *form,...);
