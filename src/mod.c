#include "foxhollow_mod_api.h"
#include "console.h"
#include "platform/platform.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static const FhModHost* H;
static FhMod* M;
static DevConsole C;

typedef struct GameTextBoxCompat {
    uint16_t unk00, maxWidth, unk04, maxHeight, width, height;
    float scale;
    uint8_t alignH, alignV, alignment, style;
    int16_t x, y, cursorX, cursorY;
    uint16_t flags;
    uint8_t alpha, unk1F;
} GameTextBoxCompat;

static void (*origSubtitleUpdateAndDraw)(int);
static void* render_hook_target;
static void (*drawHudBox_)(short,short,short,short,unsigned char,unsigned char);
static void (*gameTextShowStr_)(char*,int,int,int);
static void (*gameTextSetColor_)(unsigned char,unsigned char,unsigned char,unsigned char);
static void (*gameTextRun_)(void);
static GameTextBoxCompat* gTextBoxes_;
static int render_ok;
static int render_seen;

static void (*origPadUpdate)(void);
static void* pad_hook_target;
static void (*setJoypadDisabled_)(void);
static int input_gate_ok;

/* Standalone native debug-cheat + DEV244 navigation bindings. */
typedef struct GameObject GameObject;
static GameObject* (*Obj_GetPlayerObject_)(void);
static int (*playerGetCurMagic_)(GameObject*), (*playerGetMaxMagic_)(GameObject*);
static void (*playerAddRemoveMagic_)(GameObject*,int);
static int (*playerGetCurHealth_)(GameObject*), (*playerGetMaxHealth_)(GameObject*);
static void (*playerAddHealth_)(GameObject*,int);
static GameObject* (*getArwing_)(void);
static int (*arwarwing_getHealth_)(GameObject*), (*arwarwing_getMaxHealth_)(GameObject*);
static void (*arwarwing_addHealth_)(GameObject*,int);
static unsigned char** gameBitSaveData_;
static void (*warpToMap_)(int,int), (*setMapAct_)(int,int);
static int (*loadMapAndParent_)(int), (*mapGetDirIdx_)(int);
static void (*mapLoadByCoords_)(float,float,float,int);
static void (*loadMapForCameraPos_)(float,float,float), (*doPendingMapLoads_)(void);
typedef struct SaveGameCharacterPositionCompat {
 float x,y,z;
 int8_t angle;
 int8_t mapLayer;
 int8_t mapDataFileId;
 uint8_t padF;
} SaveGameCharacterPositionCompat;
static void* (*SaveGame_getCurCharPos_)(void);
static int *gameLoopPendingMapId_, *gameLoopPendingMapDataFileId_;
static unsigned char *gameLoopMapLoadPending_, *gameLoopFullMapUnloadPending_, *gameLoopReloadRequested_;

static int (*lockLevel_)(int,int), (*unlockLevel_)(int,int,int); static void (*mainSetBits_)(int,int);
static int (*mainGetBit_)(int);
static void (*SaveGame_gplaySetObjGroupStatus_)(int,int,int), (*SaveGame_mapUpdateObjGroups_)(int);
static unsigned short (*SaveGame_getMapObjGroupBit_)(int);
static void (*clearLoadedFileFlags_blocks1_)(void);
static volatile int* assetInFlight_;
static unsigned char *pauseMenuState_, *cMenuOpen_;
static int (*getCurSeqNo_)(void);
typedef struct SfaPendingWarpDestination { float x,y,z; int16_t layer,angle; } SfaPendingWarpDestination;
static SfaPendingWarpDestination* rcpPendingWarpDest_;
static unsigned char* warpRequested_;
static void (*Pause_SetDisabled_)(int);
static int16_t *pendingWarpIndex_, *arrivedWarpIndex_;
static unsigned char* warpArrivalTimer_;
static int infinite_health, infinite_mana, infinite_tricky;

static void logi(const char* s);



typedef struct MapCellEntryCompat {
 int16_t mapId;
 int16_t adjacentMapId1;
 int16_t adjacentMapId2;
 int16_t blockId;
 int8_t cellIndex;
 int8_t romListIndex;
 int16_t unkA;
} MapCellEntryCompat;

typedef struct MapCellSeen {
 int gx,gz,layer,map,a1,a2,block,rom;
} MapCellSeen;
static MapCellSeen mapCellSeen[256];
static int mapCellSeenCount;

static int mapcell_seen(int gx,int gz,int layer,const MapCellEntryCompat* e){
 int i;
 for(i=0;i<mapCellSeenCount;i++){
  MapCellSeen *q=&mapCellSeen[i];
  if(q->gx==gx&&q->gz==gz&&q->layer==layer&&q->map==e->mapId&&
     q->a1==e->adjacentMapId1&&q->a2==e->adjacentMapId2&&
     q->block==e->blockId&&q->rom==e->romListIndex)return 1;
 }
 if(mapCellSeenCount<256){
  MapCellSeen *q=&mapCellSeen[mapCellSeenCount++];
  q->gx=gx;q->gz=gz;q->layer=layer;q->map=e->mapId;q->a1=e->adjacentMapId1;
  q->a2=e->adjacentMapId2;q->block=e->blockId;q->rom=e->romListIndex;
 }
 return 0;
}
static void logi(const char* s);
static void logw(const char* s);

#define GAMEBIT_WM_OBJGROUPS 0x405
#define GAMEBIT_CC_OBJGROUPS 0x3B7
#define GAMEBIT_WC_OBJGROUPS 0x36A
#define GAMEBIT_CF_OBJGROUPS 0x458
#define GAMEBIT_CD_OBJGROUPS 0x47C
#define GAMEBIT_CF_OBJGROUPS2 0x4A3
#define GAMEBIT_DR_OBJGROUPS 0x5DB
#define GAMEBIT_NW_OBJGROUPS 0x4AE
#define GAMEBIT_NW_MAMMOTH_TUMBLEWEED_COUNT 0x48B
#define GAMEBIT_NW_GEYSER_COMPLETE 0x398
#define GAMEBIT_NW_ARTIFACT_STARTED 0x19D
#define GAMEBIT_NW_ARTIFACT_COMPLETE 0x19F
#define GAMEBIT_NW_RESCUE_SEQUENCE_ACTIVE 0xECD
#define GAMEBIT_NW_MAMMOTH_BUSH1 0xF22
#define GAMEBIT_NW_MAMMOTH_BUSH2 0xF23
#define GAMEBIT_NW_MAMMOTH_BUSH3 0xF24
#define GAMEBIT_NW_MAMMOTH_BUSH4 0xF25
#define GAMEBIT_OFP_PUZZLE_SHOW 0x5E4
#define GAMEBIT_OFP_ZAPPED 0x5E5
#define GAMEBIT_OFP_PUZZLE_PAD 0x635
#define GAMEBIT_OFP_LOAD_BLOCK_SLIDE2 0x7A1
#define GAMEBIT_OFP_ELECTRIC_ACT1_COMPLETE 0xE57
#define GAMEBIT_OFP_ELECTRIC_ACT2_COMPLETE 0xE58
#define GAMEBIT_WC_MAGICCAVE_RELATED_0E05 0xE05
#define GAMEBIT_CF_RELATED_0D73 0xD73
#define GAMEBIT_DR_ARWING_RELATED_0E7B 0xE7B
#define GAMEBIT_DR_FLEW_TO 0x9E9
#define GAMEBIT_WM_RELATED_0164 0x164
#define GAMEBIT_WM_RELATED_0D1B 0xD1B
#define GAMEBIT_WM_RELATED_0D1C 0xD1C
#define GAMEBIT_WM_RELATED_0D1D 0xD1D
#define GAMEBIT_WM_RELATED_0D1E 0xD1E
#define GAMEBIT_WM_RELATED_0D1F 0xD1F
#define GAMEBIT_WM_WARP3_ENABLED 0xF43
#define GAMEBIT_WM_WARP4_ENABLED 0xF44

typedef struct NamedWarp { const char* name; const char* display_name; int map,warp,min_act,max_act; } NamedWarp;
static const NamedWarp named_warps[]={
 {"tth","ThornTail Hollow",0x07,0x6C,1,8},{"lfv","LightFoot Village",0x0E,0x50,1,1},{"crf","CloudRunner Fortress",0x0C,0x63,1,1},
 {"dim","DarkIce Mines",0x13,0x77,1,2},{"cc","Cape Claw",0x1D,0x35,1,2},{"wc","Walled City",0x0D,0x78,1,2},
 {"dr","Dragon Rock",0x02,0x79,1,2},{"mmp","Moon Mountain Pass",0x12,0x10,1,2},{"ofp","Ocean Force Point Temple",0x32,0x68,1,2},
 {"vfp","Volcano Force Point Temple",0x04,0x7C,1,3},
 {"im","Ice Mountain",0x17,0x02,1,4},{"shw","SnowHorn Wastes",0x0A,0x67,1,1}
};
typedef struct PendingRetailGoto { int active,wait,map,act,warp,dir; } PendingRetailGoto;
static PendingRetailGoto retail_goto;
typedef struct PendingKp { int active,wait,act,warp,groupA,groupB,dir; } PendingKp;
static PendingKp kp_goto;
static int andross_after_kp6, andross_wait;

typedef struct PendingSnowHornGoto {
    int active;
    int phase;
    int wait;
    int linkb_dir;
    int shw_dir;
} PendingSnowHornGoto;
static PendingSnowHornGoto shw_goto;

static void logi(const char* s){if(H&&H->log)H->log(M,FH_LOG_INFO,s);} static void logw(const char* s){if(H&&H->log)H->log(M,FH_LOG_WARN,s);}

#define DC_DRAW_CHARS DC_WRAP_CHARS
static void make_display_line(char* out,size_t out_size,const char* src){size_t n;if(!out||!out_size)return;if(!src)src="";n=strlen(src);if(n<out_size&&n<=DC_DRAW_CHARS){memcpy(out,src,n+1);return;}n=DC_DRAW_CHARS;if(n>out_size-1)n=out_size-1;memcpy(out,src,n);out[n]=0;}
static void make_prompt(char* out,size_t out_size,const char* input){size_t len,keep,start;if(!input)input="";len=strlen(input);keep=DC_DRAW_CHARS>3?DC_DRAW_CHARS-3:0;start=len>keep?len-keep:0;snprintf(out,out_size,"> %s_",input+start);}
static int nav_is_safe(void){if(Obj_GetPlayerObject_&&!Obj_GetPlayerObject_())return 0;if(pauseMenuState_&&*pauseMenuState_!=0)return 0;if(cMenuOpen_&&*cMenuOpen_!=0)return 0;if(getCurSeqNo_&&getCurSeqNo_()!=0)return 0;return 1;}

static void seed_kp_late_state(int act){if(!mainSetBits_||(act!=5&&act!=6))return;mainSetBits_(GAMEBIT_WM_RELATED_0D1B,1);mainSetBits_(GAMEBIT_WM_RELATED_0D1C,1);mainSetBits_(GAMEBIT_WM_RELATED_0D1D,1);mainSetBits_(GAMEBIT_WM_RELATED_0D1E,1);if(act==6){mainSetBits_(GAMEBIT_WM_RELATED_0D1F,1);mainSetBits_(GAMEBIT_WM_RELATED_0164,1);mainSetBits_(GAMEBIT_WM_WARP3_ENABLED,0);mainSetBits_(GAMEBIT_WM_WARP4_ENABLED,0);}else{mainSetBits_(GAMEBIT_WM_WARP3_ENABLED,0);mainSetBits_(GAMEBIT_WM_WARP4_ENABLED,1);}}
static void prepare_arwing_destination(int warp){int map=-1;if(!mainSetBits_||!SaveGame_gplaySetObjGroupStatus_)return;switch(warp){case 0x77:map=0x13;SaveGame_gplaySetObjGroupStatus_(map,0,1);SaveGame_gplaySetObjGroupStatus_(map,0x16,1);break;case 0x78:map=0x0D;mainSetBits_(GAMEBIT_WC_OBJGROUPS,0);SaveGame_gplaySetObjGroupStatus_(map,0,1);SaveGame_gplaySetObjGroupStatus_(map,1,1);SaveGame_gplaySetObjGroupStatus_(map,5,1);SaveGame_gplaySetObjGroupStatus_(map,0x0A,1);SaveGame_gplaySetObjGroupStatus_(map,0x0B,1);mainSetBits_(GAMEBIT_WC_MAGICCAVE_RELATED_0E05,0);break;case 0x63:map=0x0C;mainSetBits_(GAMEBIT_CF_OBJGROUPS,0);mainSetBits_(GAMEBIT_CD_OBJGROUPS,0);mainSetBits_(GAMEBIT_CF_OBJGROUPS2,0);SaveGame_gplaySetObjGroupStatus_(map,0,1);mainSetBits_(GAMEBIT_CF_RELATED_0D73,0);break;case 0x79:map=0x02;mainSetBits_(GAMEBIT_DR_OBJGROUPS,0);SaveGame_gplaySetObjGroupStatus_(map,0x0F,1);SaveGame_gplaySetObjGroupStatus_(map,0x10,1);mainSetBits_(GAMEBIT_DR_ARWING_RELATED_0E7B,0);mainSetBits_(GAMEBIT_DR_FLEW_TO,0);break;default:break;}if(map>=0&&SaveGame_mapUpdateObjGroups_)SaveGame_mapUpdateObjGroups_(map);}
static void prepare_forcepoint_groups(int map,int act){
 /* OFP is a special case. 0x32 is the map ID used by loadMapAndParent,
  * while its DFP controller objects can use the shared KrazTest/dfptop
  * map-event state. Do not treat the load-map ID as the only state key.
  * Seed both aliases, then explicitly reopen the electric-floor puzzle. */
 if(map==0x32&&(act==1||act==2)){
  int slots[2]={0x32,0x15};
  if(setMapAct_){setMapAct_(0x32,act);setMapAct_(0x15,act);}
  if(mainSetBits_){
   mainSetBits_(GAMEBIT_OFP_PUZZLE_SHOW,0);
   mainSetBits_(GAMEBIT_OFP_ZAPPED,0);
   mainSetBits_(GAMEBIT_OFP_PUZZLE_PAD,0);
   mainSetBits_(GAMEBIT_OFP_ELECTRIC_ACT1_COMPLETE,0);
   mainSetBits_(GAMEBIT_OFP_ELECTRIC_ACT2_COMPLETE,0);
   /* Act 2's controller only adds group 6 after this bit is set. */
   mainSetBits_(GAMEBIT_OFP_LOAD_BLOCK_SLIDE2,act==2?1:0);
  }
  for(int i=0;i<2;i++){
   int slot=slots[i];
   if(SaveGame_getMapObjGroupBit_&&mainSetBits_){int gb=SaveGame_getMapObjGroupBit_(slot);if(gb>0)mainSetBits_(gb,0xFFFFFFFFu);}
   if(SaveGame_mapUpdateObjGroups_)SaveGame_mapUpdateObjGroups_(slot);
  }
 }else if(map==0x04&&act>=1&&act<=3){
  if(SaveGame_getMapObjGroupBit_&&mainSetBits_){int gb=SaveGame_getMapObjGroupBit_(map);if(gb>0)mainSetBits_(gb,0x00400006);}
  if(SaveGame_mapUpdateObjGroups_)SaveGame_mapUpdateObjGroups_(map);
 }
}

static int full_transition_from_warp(int warp,int map,int act){
 SfaPendingWarpDestination d;SaveGameCharacterPositionCompat*pos;int dir;
 if(!warpToMap_||!rcpPendingWarpDest_||!warpRequested_||!mapLoadByCoords_||!SaveGame_getCurCharPos_||!setMapAct_||!mapGetDirIdx_)return 0;
 setMapAct_(map,act);
 if(SaveGame_mapUpdateObjGroups_)SaveGame_mapUpdateObjGroups_(map);
 warpToMap_(warp,0);d=*rcpPendingWarpDest_;
 /* Keep the verified Cape Claw developer landing point. */
 if(map==0x1D){d.x=1419.07f;d.y=-1206.83f;d.z=-4205.01f;d.layer=0;d.angle=0;}
 *warpRequested_=0;
 if(pendingWarpIndex_)*pendingWarpIndex_=-1;
 if(Pause_SetDisabled_)Pause_SetDisabled_(0);
 pos=(SaveGameCharacterPositionCompat*)SaveGame_getCurCharPos_();if(!pos)return 0;
 dir=mapGetDirIdx_(map);if(dir<0)return 0;
 pos->x=d.x;pos->y=d.y;pos->z=d.z;pos->mapLayer=(int8_t)d.layer;pos->angle=(int8_t)d.angle;
 /* mapSetup() takes the pending data-file id from this field.  Keep it in
    sync with the destination before asking for the full native transition. */
 pos->mapDataFileId=(int8_t)dir;
 /* Vanilla route capture: CC <-> LFV-side connector swaps 0x2F <-> 0x15.
    VFP's MMP-side approach uses 0x44 immediately before the VFP interior.
    Schedule the adjacent set after the destination's full unload/load settles. */
 mapLoadByCoords_(d.x,d.y,d.z,d.layer);
 return 1;
}

static int queue_loaded_named(DevConsole* c,const char* name,int act){size_t i;char out[DC_LINE_LEN];int dir;if(!strcmp(name,"kp")){if(act<2||act>6){console_push(c,"Usage: teleport kp <2-6>");return 1;}if(!loadMapAndParent_||!mapGetDirIdx_||!lockLevel_||!warpToMap_||!setMapAct_){console_push(c,"KP loaded teleport unavailable.");return 1;}dir=mapGetDirIdx_(0x0B);if(dir<0){console_push(c,"KP resource directory unavailable.");return 1;}if(act>=5){setMapAct_(0x0B,act);seed_kp_late_state(act);}loadMapAndParent_(0x0B);lockLevel_(dir,0);if(act<=4){kp_goto.active=1;kp_goto.wait=0;kp_goto.act=act;kp_goto.dir=dir;if(act==2){kp_goto.warp=0x20;kp_goto.groupA=5;kp_goto.groupB=6;}else{kp_goto.warp=0x22;kp_goto.groupA=8;kp_goto.groupB=9;}}else{retail_goto.active=1;retail_goto.wait=0;retail_goto.map=0x0B;retail_goto.act=act;retail_goto.warp=0x4E;retail_goto.dir=dir;}snprintf(out,sizeof(out),"Loading Krazoa Palace Act %d...",act);console_push(c,out);return 1;}
 if(!strcmp(name,"shw")){
  int shw_dir;
  SaveGameCharacterPositionCompat* pos;
  if(act==0)act=1;
  if(act!=1){console_push(c,"shw currently supports Act 1.");return 1;}
  if(!mapGetDirIdx_||!setMapAct_||!SaveGame_getCurCharPos_||
     !gameLoopPendingMapId_||!gameLoopPendingMapDataFileId_||
     !gameLoopMapLoadPending_||!gameLoopFullMapUnloadPending_||!gameLoopReloadRequested_){
   console_push(c,"SnowHorn direct full reload unavailable: native symbols missing.");return 1;
  }

  shw_dir=mapGetDirIdx_(0x0A);
  if(shw_dir<0){console_push(c,"SnowHorn resource directory unavailable.");return 1;}
  pos=(SaveGameCharacterPositionCompat*)SaveGame_getCurCharPos_();
  if(!pos){console_push(c,"SnowHorn reload failed: no character position.");return 1;}

  /* Stable SnowHorn Act 1 entry state required by its native controllers. */
  setMapAct_(0x0A,1);
  if(mainSetBits_){
   mainSetBits_(0x19D,0);
   mainSetBits_(0x19F,0);
   mainSetBits_(0xECD,0);
   mainSetBits_(0x398,0);
   mainSetBits_(0x4AE,(int)0x7FFFFFFEu);
  }
  if(SaveGame_mapUpdateObjGroups_)SaveGame_mapUpdateObjGroups_(0x0A);

  pos->x=-2634.34f;
  pos->y=-131.00f;
  pos->z=2167.80f;
  pos->mapLayer=0;
  pos->angle=0;
  pos->mapDataFileId=(int8_t)shw_dir;

  *gameLoopPendingMapId_=0x0A;
  *gameLoopPendingMapDataFileId_=shw_dir;
  *gameLoopFullMapUnloadPending_=1;
  *gameLoopMapLoadPending_=1;
  *gameLoopReloadRequested_=1;

  console_push(c,"Loading SnowHorn Wastes Act 1.");
  return 1;
 }
 if(!strcmp(name,"mmp")){
  if(act==0)act=1;
  if(act<1||act>2){console_push(c,"mmp supports Acts 1-2.");return 1;}
  if(!warpToMap_||!setMapAct_||!mainSetBits_){console_push(c,"Moon Mountain Pass teleport unavailable.");return 1;}
  setMapAct_(0x12,act);
  /* Captured from a naturally loaded MMP Act 1 state reached from VFP. */
  mainSetBits_(0x42E,0x00000806);
  if(SaveGame_mapUpdateObjGroups_)SaveGame_mapUpdateObjGroups_(0x12);
  /* Retain the verified non-crashing retail MMP load, but replace only its
     pending destination with a position captured inside the accessible
     VFP-side MMP route. */
  warpToMap_(0x10,0);
  if(rcpPendingWarpDest_){
   rcpPendingWarpDest_->x=-12830.08f;
   rcpPendingWarpDest_->y=-112.00f;
   rcpPendingWarpDest_->z=-237.88f;
   rcpPendingWarpDest_->layer=0;
   rcpPendingWarpDest_->angle=0;
  }
  snprintf(out,sizeof(out),"Loading Moon Mountain Pass Act %d...",act);
  console_push(c,out);
  return 1;
}
for(i=0;i<sizeof(named_warps)/sizeof(named_warps[0]);++i)if(!strcmp(name,named_warps[i].name)){const NamedWarp*n=&named_warps[i];if(act==0)act=n->min_act;if(act<n->min_act||act>n->max_act){snprintf(out,sizeof(out),"%s supports Acts %d-%d.",name,n->min_act,n->max_act);console_push(c,out);return 1;}if(n->map==0x07||n->map==0x1D||n->map==0x04){
 if(n->map==0x1D&&mainSetBits_&&SaveGame_mapUpdateObjGroups_){mainSetBits_(GAMEBIT_CC_OBJGROUPS,(int)0x80000013u);SaveGame_mapUpdateObjGroups_(0x1D);}
 else if(n->map==0x04)prepare_forcepoint_groups(0x04,act);
 if(!full_transition_from_warp(n->warp,n->map,act)){console_push(c,"Full native area transition unavailable.");return 1;}
 snprintf(out,sizeof(out),"Loading %s Act %d...",n->display_name,act);console_push(c,out);return 1;
}if(!loadMapAndParent_||!mapGetDirIdx_||!lockLevel_||!warpToMap_){console_push(c,"Loaded teleport unavailable: native symbols missing.");return 1;}dir=mapGetDirIdx_(n->map);if(dir<0){console_push(c,"Destination resource directory unavailable.");return 1;}if((n->map==0x32||n->map==0x04||n->map==0x12||n->map==0x17||n->map==0x07||n->map==0x0A)&&act>0&&setMapAct_){setMapAct_(n->map,act);if(n->map==0x32||n->map==0x04)prepare_forcepoint_groups(n->map,act);else if(SaveGame_mapUpdateObjGroups_)SaveGame_mapUpdateObjGroups_(n->map);}loadMapAndParent_(n->map);lockLevel_(dir,0);retail_goto.active=1;retail_goto.wait=0;retail_goto.map=n->map;retail_goto.act=act;retail_goto.warp=n->warp;retail_goto.dir=dir;snprintf(out,sizeof(out),"Loading %s Act %d...",n->display_name,act);console_push(c,out);return 1;}
 console_push(c,"Unknown: tth im shw lfv crf dim cc wc dr mmp ofp vfp kp");return 1;}

static void process_loaded_teleports(void){

if(kp_goto.active&&nav_is_safe()){if(kp_goto.wait++<4)return;if(assetInFlight_&&*assetInFlight_)return;if(mainSetBits_&&setMapAct_&&SaveGame_gplaySetObjGroupStatus_&&warpToMap_){mainSetBits_(GAMEBIT_WM_OBJGROUPS,0);setMapAct_(0x0B,kp_goto.act);SaveGame_gplaySetObjGroupStatus_(0x0B,kp_goto.groupA,1);SaveGame_gplaySetObjGroupStatus_(0x0B,kp_goto.groupB,1);if(SaveGame_mapUpdateObjGroups_)SaveGame_mapUpdateObjGroups_(0x0B);warpToMap_(kp_goto.warp,0);}kp_goto.active=0;}
 if(retail_goto.active&&nav_is_safe()){if(retail_goto.wait++<4)return;if(assetInFlight_&&*assetInFlight_)return;if(retail_goto.act>0&&setMapAct_)setMapAct_(retail_goto.map,retail_goto.act);if(mainSetBits_&&SaveGame_mapUpdateObjGroups_){if(retail_goto.map==0x0B&&retail_goto.act==5){mainSetBits_(GAMEBIT_WM_OBJGROUPS,0x00000C33);SaveGame_mapUpdateObjGroups_(0x0B);}else if(retail_goto.map==0x0B&&retail_goto.act==6){mainSetBits_(GAMEBIT_WM_OBJGROUPS,(int)0xE0000C33u);SaveGame_mapUpdateObjGroups_(0x0B);}else if(retail_goto.map==0x1D){mainSetBits_(GAMEBIT_CC_OBJGROUPS,(int)0x80000013u);SaveGame_mapUpdateObjGroups_(0x1D);}}
  if(retail_goto.warp==0x6C||retail_goto.warp==0x77||retail_goto.warp==0x78||retail_goto.warp==0x63||retail_goto.warp==0x79){prepare_arwing_destination(retail_goto.warp);if(clearLoadedFileFlags_blocks1_)clearLoadedFileFlags_blocks1_();}else if(retail_goto.map==0x32||retail_goto.map==0x04)prepare_forcepoint_groups(retail_goto.map,retail_goto.act);else if(SaveGame_mapUpdateObjGroups_)SaveGame_mapUpdateObjGroups_(retail_goto.map);
  if(retail_goto.map==0x0B&&(retail_goto.act==5||retail_goto.act==6)&&retail_goto.warp==0x4E&&rcpPendingWarpDest_){SfaPendingWarpDestination roof;warpToMap_(0x4E,0);roof=*rcpPendingWarpDest_;warpToMap_(0x22,0);*rcpPendingWarpDest_=roof;if(pendingWarpIndex_)*pendingWarpIndex_=-3;if(arrivedWarpIndex_)*arrivedWarpIndex_=-3;if(warpArrivalTimer_)*warpArrivalTimer_=0;}else warpToMap_(retail_goto.warp,0);
  if(retail_goto.map==0x1D&&retail_goto.warp==0x35&&rcpPendingWarpDest_){rcpPendingWarpDest_->x=1419.07f;rcpPendingWarpDest_->y=-1206.83f;rcpPendingWarpDest_->z=-4205.01f;rcpPendingWarpDest_->layer=0;rcpPendingWarpDest_->angle=0;}
  retail_goto.active=0;}
 if(andross_after_kp6){if(retail_goto.active||kp_goto.active||shw_goto.active){andross_wait=0;return;}if(andross_wait++>=180){if(warpToMap_)warpToMap_(0x32,0);andross_after_kp6=0;andross_wait=0;}}}

static int teleport_boss(DevConsole*c,const char*boss){int warp=-1;char out[DC_LINE_LEN];if(!strcmp(boss,"galdon"))warp=0x1D;else if(!strcmp(boss,"race"))warp=0x49;else if(!strcmp(boss,"redeye")||!strcmp(boss,"redeye_king"))warp=0x6D;else if(!strcmp(boss,"drakor"))warp=0x54;else if(!strcmp(boss,"andross")){queue_loaded_named(c,"kp",6);andross_after_kp6=1;andross_wait=0;console_push(c,"Andross queued after loaded KP6 settles.");return 1;}else{console_push(c,"Bosses: galdon, race, redeye, drakor, andross");return 1;}if(!warpToMap_){console_push(c,"Boss teleport unavailable.");return 1;}warpToMap_(warp,0);snprintf(out,sizeof(out),"Boss %s via retail WARPTAB 0x%X.",boss,warp);console_push(c,out);return 1;}

static int native_command(DevConsole*c,const char*cmd){char mout[DC_LINE_LEN],state[16];int mg,ms;
 if(!strcmp(cmd,"version")){console_push(c,"SFA Developer Console 0.1.22");return 1;}
 if(!strcmp(cmd,"position")){
  SaveGameCharacterPositionCompat* p=SaveGame_getCurCharPos_?(SaveGameCharacterPositionCompat*)SaveGame_getCurCharPos_():NULL;
  if(!p){console_push(c,"Player position unavailable.");return 1;}
  snprintf(mout,sizeof(mout),"Position: %.2f %.2f %.2f",p->x,p->y,p->z);
  console_push(c,mout);return 1;
 }

char what[32],out[DC_LINE_LEN],dest[32],boss[32];int warp,act=0;if(sscanf(cmd,"teleport boss %31s",boss)==1)return teleport_boss(c,boss);if(sscanf(cmd,"teleport %31s %i",dest,&act)>=1){char*end=0;long raw=strtol(dest,&end,0);if(end&&*end==0){if(!warpToMap_){console_push(c,"Raw teleport unavailable.");return 1;}warp=(int)raw;warpToMap_(warp,0);snprintf(out,sizeof(out),"Raw WARPTAB 0x%X queued.",warp);console_push(c,out);return 1;}return queue_loaded_named(c,dest,act);}if(!strcmp(cmd,"teleport boss")){console_push(c,"Usage: teleport boss <galdon|race|redeye|drakor|andross>");return 1;}if(!strcmp(cmd,"infinite status")){snprintf(out,sizeof(out),"Infinite: health %s, mana %s, tricky %s",infinite_health?"on":"off",infinite_mana?"on":"off",infinite_tricky?"on":"off");console_push(c,out);return 1;}if(sscanf(cmd,"infinite %31s",what)==1){int*f=0;if(!strcmp(cmd,"infinite health"))f=&infinite_health;else if(!strcmp(cmd,"infinite mana"))f=&infinite_mana;else if(!strcmp(cmd,"infinite tricky"))f=&infinite_tricky;if(f){*f=!(*f);snprintf(out,sizeof(out),"Infinite %s: %s",what,*f?"ON":"OFF");console_push(c,out);return 1;}console_push(c,"Usage: infinite <health|mana|tricky>");return 1;}return 0;}
static void apply_debug_resources(void){GameObject*player;GameObject*arwing;process_loaded_teleports();
player=Obj_GetPlayerObject_?Obj_GetPlayerObject_():0;arwing=getArwing_?getArwing_():0;if(infinite_mana&&player&&playerGetCurMagic_&&playerGetMaxMagic_&&playerAddRemoveMagic_){int cur=playerGetCurMagic_(player),max=playerGetMaxMagic_(player);if(max>cur)playerAddRemoveMagic_(player,max-cur);}if(infinite_health&&player&&playerGetCurHealth_&&playerGetMaxHealth_&&playerAddHealth_){int cur=playerGetCurHealth_(player),max=playerGetMaxHealth_(player);if(max>cur)playerAddHealth_(player,max-cur);}if(infinite_health&&arwing&&arwarwing_getHealth_&&arwarwing_getMaxHealth_&&arwarwing_addHealth_){int cur=arwarwing_getHealth_(arwing),max=arwarwing_getMaxHealth_(arwing);if(max>cur)arwarwing_addHealth_(arwing,max-cur);}if(infinite_tricky&&gameBitSaveData_&&*gameBitSaveData_){unsigned char*stats=(*gameBitSaveData_)+0x18;if(stats[1]>stats[0])stats[0]=stats[1];}}

static void draw_console(void) {
    int i, first, last, y;
    char prompt[DC_INPUT_LEN + 8];
    char display_line[DC_LINE_LEN];
    char scroll_hint[48];
    GameTextBoxCompat saved;

    if (!C.open || !render_ok) return;

    /* 0.1.22: keep the proven compact width/top position and add exactly one
       output row downward so the complete help listing remains visible. */
    drawHudBox_(188, 10, 244, 92, 155, 1);

    /* Box 0 is temporarily converted into a small, left-aligned console text
       box. gameTextRun() is called before restoring it, so queued strings use
       this compact configuration without permanently altering SFA's UI. */
    if (gTextBoxes_) {
        saved = gTextBoxes_[0];
        gTextBoxes_[0].x = 198;
        gTextBoxes_[0].y = 16;
        /* GameText width is measured before the 0.50 text scale.
           The panel has 224 screen-space units of usable interior width,
           so use 448 here to reach the right inset without resizing the HUD box. */
        gTextBoxes_[0].maxWidth = 448;
        gTextBoxes_[0].width = 448;
        gTextBoxes_[0].maxHeight = 88;
        gTextBoxes_[0].height = 88;
        gTextBoxes_[0].scale = 0.50f;
        gTextBoxes_[0].alignH = 0;
        gTextBoxes_[0].alignV = 0;
        gTextBoxes_[0].alignment = 0;
    }

    gameTextSetColor_(255,255,255,255);

    last = C.line_count - C.scroll_offset;
    if (last < 0) last = 0;
    first = last - DC_VISIBLE_LINES;
    if (first < 0) first = 0;
    y = 14;
    for (i=first; i<last; ++i) {
        make_display_line(display_line,sizeof(display_line),C.lines[i]);
        gameTextShowStr_(display_line,0,0,y);
        y += 10;
    }

    if (C.scroll_offset > 0) {
        snprintf(scroll_hint,sizeof(scroll_hint),"[scrollback: %d line%s above newest]",C.scroll_offset,C.scroll_offset==1?"":"s");
        gameTextSetColor_(190,190,190,255);
        gameTextShowStr_(scroll_hint,0,105,0);
        gameTextSetColor_(255,255,255,255);
    }

    make_prompt(prompt,sizeof(prompt),C.input);
    gameTextShowStr_(prompt,0,0,78);
    gameTextRun_();

    if (gTextBoxes_) gTextBoxes_[0] = saved;
}

static void pad_update_hook(void) {
    if (origPadUpdate) origPadUpdate();
    if (C.open && setJoypadDisabled_) setJoypadDisabled_();
}

static void subtitle_update_and_draw_hook(int mode) {
    if (origSubtitleUpdateAndDraw) origSubtitleUpdateAndDraw(mode);
    if (!render_seen) { render_seen=1; logi("0.1.22 subtitle-stage render callback is live."); }
    draw_console();
}

FH_MOD_EXPORT int fh_mod_initialize(FhMod* mod,const FhModHost* host) {
    void* target; void* pad_target; char msg[256];
    M=mod; H=host; console_init(&C); platform_input_init(&C);
    if(!host || host->abiVersion!=FH_MOD_ABI_VERSION) return FH_MOD_ERROR;

    drawHudBox_=(void(*)(short,short,short,short,unsigned char,unsigned char))host->symbolAddress(mod,"drawHudBox");
    gameTextShowStr_=(void(*)(char*,int,int,int))host->symbolAddress(mod,"gameTextShowStr");
    gameTextSetColor_=(void(*)(unsigned char,unsigned char,unsigned char,unsigned char))host->symbolAddress(mod,"gameTextSetColor");
    gameTextRun_=(void(*)(void))host->symbolAddress(mod,"gameTextRun");
    gTextBoxes_=(GameTextBoxCompat*)host->symbolAddress(mod,"gTextBoxes");
    target=host->symbolAddress(mod,"subtitleUpdateAndDraw"); render_hook_target=target;
    setJoypadDisabled_=(void(*)(void))host->symbolAddress(mod,"setJoypadDisabled");
    Obj_GetPlayerObject_=(GameObject*(*)(void))host->symbolAddress(mod,"Obj_GetPlayerObject");
    playerGetCurMagic_=(int(*)(GameObject*))host->symbolAddress(mod,"playerGetCurMagic");
    playerGetMaxMagic_=(int(*)(GameObject*))host->symbolAddress(mod,"playerGetMaxMagic");
    playerAddRemoveMagic_=(void(*)(GameObject*,int))host->symbolAddress(mod,"playerAddRemoveMagic");
    playerGetCurHealth_=(int(*)(GameObject*))host->symbolAddress(mod,"playerGetCurHealth");
    playerGetMaxHealth_=(int(*)(GameObject*))host->symbolAddress(mod,"playerGetMaxHealth");
    playerAddHealth_=(void(*)(GameObject*,int))host->symbolAddress(mod,"playerAddHealth");
    getArwing_=(GameObject*(*)(void))host->symbolAddress(mod,"getArwing");
    arwarwing_getHealth_=(int(*)(GameObject*))host->symbolAddress(mod,"arwarwing_getHealth");
    arwarwing_getMaxHealth_=(int(*)(GameObject*))host->symbolAddress(mod,"arwarwing_getMaxHealth");
    arwarwing_addHealth_=(void(*)(GameObject*,int))host->symbolAddress(mod,"arwarwing_addHealth");
    gameBitSaveData_=(unsigned char**)host->symbolAddress(mod,"gGameBitSaveData");
    warpToMap_=(void(*)(int,int))host->symbolAddress(mod,"warpToMap");
    setMapAct_=(void(*)(int,int))host->symbolAddress(mod,"SaveGame_gplaySetAct");
    loadMapAndParent_=(int(*)(int))host->symbolAddress(mod,"loadMapAndParent");
    mapGetDirIdx_=(int(*)(int))host->symbolAddress(mod,"mapGetDirIdx");
    mapLoadByCoords_=(void(*)(float,float,float,int))host->symbolAddress(mod,"mapLoadByCoords");
    loadMapForCameraPos_=(void(*)(float,float,float))host->symbolAddress(mod,"loadMapForCameraPos");
    doPendingMapLoads_=(void(*)(void))host->symbolAddress(mod,"doPendingMapLoads");
    SaveGame_getCurCharPos_=(void*(*)(void))host->symbolAddress(mod,"SaveGame_getCurCharPos");
    gameLoopPendingMapId_=(int*)host->symbolAddress(mod,"gGameLoopPendingMapId");
    gameLoopPendingMapDataFileId_=(int*)host->symbolAddress(mod,"gGameLoopPendingMapDataFileId");
    gameLoopMapLoadPending_=(unsigned char*)host->symbolAddress(mod,"gGameLoopMapLoadPending");
    gameLoopFullMapUnloadPending_=(unsigned char*)host->symbolAddress(mod,"gGameLoopFullMapUnloadPending");
    gameLoopReloadRequested_=(unsigned char*)host->symbolAddress(mod,"gGameLoopReloadRequested");
    lockLevel_=(int(*)(int,int))host->symbolAddress(mod,"lockLevel");
    unlockLevel_=(int(*)(int,int,int))host->symbolAddress(mod,"unlockLevel");
    mainSetBits_=(void(*)(int,int))host->symbolAddress(mod,"mainSetBits");
    mainGetBit_=(int(*)(int))host->symbolAddress(mod,"mainGetBit");
    SaveGame_gplaySetObjGroupStatus_=(void(*)(int,int,int))host->symbolAddress(mod,"SaveGame_gplaySetObjGroupStatus");
    SaveGame_mapUpdateObjGroups_=(void(*)(int))host->symbolAddress(mod,"SaveGame_mapUpdateObjGroups");
    SaveGame_getMapObjGroupBit_=(unsigned short(*)(int))host->symbolAddress(mod,"SaveGame_getMapObjGroupBit");
    clearLoadedFileFlags_blocks1_=(void(*)(void))host->symbolAddress(mod,"clearLoadedFileFlags_blocks1");
    assetInFlight_=(volatile int*)host->symbolAddress(mod,"gAssetLoadInFlightFlags");
    pauseMenuState_=(unsigned char*)host->symbolAddress(mod,"pauseMenuState");
    cMenuOpen_=(unsigned char*)host->symbolAddress(mod,"cMenuOpen");
    getCurSeqNo_=(int(*)(void))host->symbolAddress(mod,"getCurSeqNo");
    rcpPendingWarpDest_=(SfaPendingWarpDestination*)host->symbolAddress(mod,"gRcpPendingWarpDest");
    warpRequested_=(unsigned char*)host->symbolAddress(mod,"gWarpRequested");
    Pause_SetDisabled_=(void(*)(int))host->symbolAddress(mod,"Pause_SetDisabled");
    pendingWarpIndex_=(int16_t*)host->symbolAddress(mod,"gPendingWarpIndex");
    arrivedWarpIndex_=(int16_t*)host->symbolAddress(mod,"gArrivedWarpIndex");
    warpArrivalTimer_=(unsigned char*)host->symbolAddress(mod,"gWarpArrivalTimer");

    /* map streaming diagnostics retired from normal build */
    console_set_command_handler(native_command);
    pad_target=host->symbolAddress(mod,"padUpdate"); pad_hook_target=pad_target;

    if(target&&drawHudBox_&&gameTextShowStr_&&gameTextSetColor_&&gameTextRun_&&gTextBoxes_&&host->hookInstall&&
       host->hookInstall(mod,target,(void*)subtitle_update_and_draw_hook,(void**)&origSubtitleUpdateAndDraw)==FH_MOD_OK){
        render_ok=1; logi("0.1.22 compact full-width-text scrollable HUD/GameText console installed.");
    } else logw("0.1.22 overlay unavailable: missing native draw/text symbol or subtitle-stage hook.");

    if(pad_target&&setJoypadDisabled_&&host->hookInstall&&
       host->hookInstall(mod,pad_target,(void*)pad_update_hook,(void**)&origPadUpdate)==FH_MOD_OK){
        input_gate_ok=1; logi("0.1.22 game input gate installed; gameplay controls are suppressed while console is open.");
    } else logw("0.1.22 input gate unavailable: console typing may also reach gameplay.");

    snprintf(msg,sizeof(msg),"SFA Developer Console 0.1.22 loaded. input=%s. F1 toggle; PgUp/PgDn scroll.",platform_input_backend_name());
    logi(msg); return FH_MOD_OK;
}

FH_MOD_EXPORT void fh_mod_update(FhMod* mod){
 static int was=-1;(void)mod;
 platform_input_tick(&C);apply_debug_resources();

 if(C.open!=was){logi(C.open?"Developer console opened.":"Developer console closed.");was=C.open;}
}
FH_MOD_EXPORT void fh_mod_shutdown(FhMod* mod){(void)mod;
 if(H&&H->hookRemove&&render_hook_target)H->hookRemove(M,render_hook_target);
 if(H&&H->hookRemove&&pad_hook_target)H->hookRemove(M,pad_hook_target);
 platform_input_shutdown();
}
