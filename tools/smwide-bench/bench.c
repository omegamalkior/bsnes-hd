// headless libretro benchmark / verification harness
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#define RTLD_NOW 0
static void* dlopen(const char*p,int f){(void)f;return (void*)LoadLibraryA(p);}
static void* dlsym(void*h,const char*n){return (void*)GetProcAddress((HMODULE)h,n);}
static const char* dlerror(void){static char b[64];snprintf(b,sizeof b,"LoadLibrary failed, error %lu",(unsigned long)GetLastError());return b;}
#else
#include <dlfcn.h>
#endif
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
struct retro_variable{const char*key;const char*value;};
struct retro_game_info{const char*path;const void*data;size_t size;const char*meta;};
static const char *ar="21:9",*scale="2x",*wsmode="Mode 7";
static unsigned W,H; static uint64_t hash=1469598103934665603ULL; static int dohash=0; static int pad=0;
static FILE* dump=0;
static const char* lastpath=0; static int cur=-1, total=0, idle=0;  // last:file = keep only the final frame; idle = no input
static int docheck=0; static unsigned long long unrec=0, rec=0, unrecFrames=0;
static bool env(unsigned cmd,void*d){
  cmd&=0xffff;
  if(cmd==15){struct retro_variable*v=d;
    if(!strcmp(v->key,"bsnes_mode7_widescreen")){v->value=ar;return true;}
    if(!strcmp(v->key,"bsnes_mode7_scale")){v->value=scale;return true;}
    if(!strcmp(v->key,"bsnes_mode7_wsMode")){v->value=wsmode;return true;}
    if(!strcmp(v->key,"bsnes_entropy")){v->value="None";return true;}  // repeatable runs from power-on
    return false;}
  if(cmd==17){*(bool*)d=false;return true;}
  if(cmd==10)return true;
  if(cmd==9||cmd==31){*(const char**)d="/tmp/lr";return true;}
  if(cmd==37||cmd==32)return true;
  return false;}
static void video(const void*d,unsigned w,unsigned h,size_t p){W=w;H=h;
  if(dohash&&d){const uint8_t*b=d;for(unsigned y=0;y<h;y++){const uint32_t*r=(const uint32_t*)(b+y*p);for(unsigned x=0;x<w;x++){hash^=r[x]&0xffffff;hash*=1099511628211ULL;}}}
  if(dump&&d){const uint8_t*b=d;for(unsigned y=0;y<h;y++)fwrite(b+y*p,4,w,dump);}
  if(lastpath&&d&&cur==total-1){FILE*f=fopen(lastpath,"wb");const uint8_t*b=d;for(unsigned y=0;y<h;y++)fwrite(b+y*p,4,w,f);fclose(f);}}
static void audio(int16_t l,int16_t r){} static size_t audiob(const int16_t*d,size_t f){return f;}
static void poll(void){}
static int16_t state(unsigned port,unsigned dev,unsigned idx,unsigned id){ if(port||dev!=1)return 0; if(id==256)return pad; return (pad>>id)&1; }
enum{B=1<<0,Y=1<<1,SEL=1<<2,ST=1<<3,UP=1<<4,DN=1<<5,LT=1<<6,RT=1<<7,A=1<<8,X=1<<9};
static int boot=0;
static int script(int f){ int c=f%600; int p=0;
  if(idle) return 0;
  if(boot){ return (f%45)<4 ? ST : ((f%90)>60 ? A : 0); }
  if(c<200)p=LT|B; else if(c<400)p=RT|B; else if(c<500)p=LT; else p=RT;
  if(c%40<12)p|=A; if(c%25<4)p|=X; return p; }
static double now(){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
int main(int c,char**v){
  if(c==3&&!strcmp(v[2],"--smoke")){
    // no game needed: check that the core loads, exports the libretro API and starts up
    void*h=dlopen(v[1],RTLD_NOW);if(!h){puts(dlerror());return 1;}
    const char*need[]={"retro_api_version","retro_init","retro_deinit","retro_run","retro_load_game","retro_unload_game","retro_get_system_info","retro_get_system_av_info","retro_serialize","retro_unserialize","retro_set_environment","retro_get_memory_data"};
    for(unsigned i=0;i<sizeof need/sizeof*need;i++) if(!dlsym(h,need[i])){printf("missing export %s\n",need[i]);return 1;}
    struct{const char*name,*version,*ext;bool fp,be;}si={0};
    ((void(*)(void*))dlsym(h,"retro_get_system_info"))(&si);
    unsigned api=((unsigned(*)(void))dlsym(h,"retro_api_version"))();
    ((void(*)(void*))dlsym(h,"retro_set_environment"))(env);
    ((void(*)(void))dlsym(h,"retro_init"))();
    ((void(*)(void))dlsym(h,"retro_deinit"))();
    printf("smoke test OK: %s %s, libretro API %u, extensions %s\n",si.name,si.version,api,si.ext);
    return 0;}
  if(c<6){fprintf(stderr,"bench so rom state frames mode(time|hash|check|dump:file|last:file) [ar scale wsmode [boot|idle]]\n");return 2;}
  int frames=atoi(v[4]); if(!strcmp(v[5],"hash"))dohash=1; if(!strncmp(v[5],"dump:",5))dump=fopen(v[5]+5,"wb"); if(!strcmp(v[5],"check"))docheck=dohash=1; if(!strncmp(v[5],"last:",5))lastpath=v[5]+5; total=frames;
  if(c>6)ar=v[6]; if(c>7)scale=v[7]; if(c>8)wsmode=v[8]; if(c>9&&!strcmp(v[9],"boot"))boot=1; if(c>9&&!strcmp(v[9],"idle"))idle=1;
  void*h=dlopen(v[1],RTLD_NOW);if(!h){puts(dlerror());return 1;}
  #define S(n) dlsym(h,n)
  ((void(*)(void*))S("retro_set_environment"))(env);
  ((void(*)(void*))S("retro_set_video_refresh"))(video);
  ((void(*)(void*))S("retro_set_audio_sample"))(audio);
  ((void(*)(void*))S("retro_set_audio_sample_batch"))(audiob);
  ((void(*)(void*))S("retro_set_input_poll"))(poll);
  ((void(*)(void*))S("retro_set_input_state"))(state);
  ((void(*)(void))S("retro_init"))();
  struct retro_game_info gi={v[2],0,0,0};
  if(!((bool(*)(void*))S("retro_load_game"))(&gi)){puts("load failed");return 1;}
  void(*run)(void)=S("retro_run");
  for(int i=0;i<5;i++)run();
  if(strcmp(v[3],"-")){FILE*f=fopen(v[3],"rb");fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);void*b=malloc(n);fread(b,1,n,f);fclose(f);
    if(!((bool(*)(const void*,size_t))S("retro_unserialize"))(b,n)){puts("state load failed");return 1;} free(b);}
  int save_hash=dohash; FILE*sd=dump; dohash=0; dump=0;
  for(int i=0;i<60;i++){pad=0;run();}           // warm-up
  dohash=save_hash; dump=sd;
  double t0=now();
  void*(*memdata)(unsigned)=S("retro_get_memory_data");
  for(int i=0;i<frames;i++){cur=i;pad=script(i);run();
    if(docheck){unsigned*st=memdata(0x534d57); if(st){unrec+=st[0];rec+=st[1];unrecFrames+=st[0]!=0;}}}
  double t=now()-t0;
  printf("%ux%u frames=%d ms/frame=%.4f fps=%.1f hash=%016llx\n",W,H,frames,t*1000/frames,frames/t,(unsigned long long)hash);
  if(docheck)printf("selfcheck: side-area sprite tiles without a recorded position=%llu (in %llu frames), with=%llu\n",unrec,unrecFrames,rec);
  if(dump)fclose(dump);
  return 0;}
