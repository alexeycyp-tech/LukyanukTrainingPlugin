/* Integration test of the real frame controller with synthetic memory/keys.
 * This executable never starts RBR or calls the real reset functions.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
static int fake_keys[256];
static DWORD fake_ticks=10000;
static unsigned reset_calls,pose_calls;
static unsigned seq_keydowns,seq_keyups;
static int pose_ignores_rotation;
static float mock_surface_z=-0.28048f;
static unsigned char code_pose[0x129],code_ngp_pose[201];
static void *mapped(uintptr_t p);
static void *fake_movement;
static HMODULE WINAPI mock_module(LPCWSTR name) {
    if(name && !lstrcmpiW(name,L"NGP.dll"))return (HMODULE)0x5a310000;
    return GetModuleHandleW(name);
}
static int reset_misses;
static DWORD WINAPI mock_ticks(void) { return fake_ticks; }
static UINT WINAPI mock_send_input(UINT count,LPINPUT inputs,int size) {
    if(count!=1 || size!=(int)sizeof(INPUT) || !inputs || inputs[0].type!=INPUT_KEYBOARD || inputs[0].ki.wVk!=VK_F12) return 0;
    if(inputs[0].ki.dwFlags&KEYEVENTF_KEYUP) seq_keyups++;
    else {
        seq_keydowns++;
        int gear=0;
        memcpy(&gear,(char*)fake_movement+0x1100,sizeof(gear)); gear++;
        memcpy((char*)fake_movement+0x1100,&gear,sizeof(gear));
    }
    return 1;
}
static void mock_reset_sequence(uintptr_t movement,uintptr_t notes,float index);
static void mock_pose_sequence(uintptr_t movement,const float position[4],const float q[4]);
static unsigned char globals_a[0x10000],globals_b[0x20000],globals_c[0x10000];
static unsigned char code_reset[32],code_ctor[32],code_notes[32];
static void *mapped(uintptr_t p) {
    if(p==0x5a3fe650)return code_ngp_pose;
    if(p==0x4e9660)return code_pose;
    if(p==0x4e8df0)return code_reset;
    if(p==0x52fea0)return code_ctor;
    if(p==0x4dd600)return code_notes;
    if(p>=0x7e0000 && p<0x7f0000)return globals_a+(p-0x7e0000);
    if(p>=0x8d0000 && p<0x8f0000)return globals_b+(p-0x8d0000);
    if(p>=0x1650000 && p<0x1660000)return globals_c+(p-0x1650000);
    return (void*)p;
}
static BOOL WINAPI mock_read(HANDLE h,LPCVOID p,LPVOID out,SIZE_T n,SIZE_T *got) {return ReadProcessMemory(h,mapped((uintptr_t)p),out,n,got);}
static SHORT WINAPI mock_key(int k) { return k>=0 && k<256 && fake_keys[k] ? (SHORT)0x8000 : 0; }
static DWORD WINAPI mock_window_process(HWND w,LPDWORD p) { (void)w;*p=GetCurrentProcessId();return GetCurrentThreadId(); }
#define GetAsyncKeyState mock_key
#define GetWindowThreadProcessId mock_window_process
#define ReadProcessMemory mock_read
#define GetTickCount mock_ticks
#define GetModuleHandleW mock_module
#define SendInput mock_send_input
#define BOOKMARK_RESET_SEQUENCE mock_reset_sequence
#define BOOKMARK_APPLY_POSE mock_pose_sequence
#define BOOKMARK_TEST_NO_DIRECTINPUT
#include "bookmark.c"
#include "test_rsf_fixture.h"
#include "test_ngp_pose_fixture.h"
static unsigned checks;
#define CHECK(x) do {checks++;if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
static void putptr(uintptr_t p,void *v) { memcpy(mapped(p),&v,4); }
static void puti(void *p,unsigned o,int v) { memcpy((char*)p+o,&v,4); }
static void putf(void *p,unsigned o,float v) { memcpy((char*)p+o,&v,4); }
static void *alloc(size_t n) {return VirtualAlloc(0,n,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);}
static void mock_reset_sequence(uintptr_t movement,uintptr_t notes,float index) {
    (void)notes; reset_calls++;
    if(!reset_misses) {
        void *car=(void*)pointer_at(0x165fc68);
        putf(car,0x20,index*100.0f+.01f);putf(car,0xc,0);
        /* The game reset changes the raw gear field; the plugin must restore it. */
        puti((void*)movement,0x1100,1);
        /* Wrong heading after the mock reset must be corrected by the pose call. */
        putf((void*)movement,0x100,0);putf((void*)movement,0x104,0);
        putf((void*)movement,0x108,.70710678f);putf((void*)movement,0x10c,.70710678f);
        for(int j=0;j<3;j++)putf((void*)movement,0x1c0+4*j,0);
        putf((void*)movement,0x140,11);putf((void*)movement,0x144,22);putf((void*)movement,0x148,33);
    }
}
static void mock_pose_sequence(uintptr_t movement,const float position[4],const float q[4]) {
    pose_calls++;
    putf((void*)movement,0x140,position[0]);
    putf((void*)movement,0x144,position[1]);
    putf((void*)movement,0x148,2.0f*position[2]-mock_surface_z);
    if(!pose_ignores_rotation) memcpy((void*)(movement+0x100),q,16);
    for(int j=0;j<3;j++)putf((void*)movement,0x1c0+4*j,0);
}
int main(void) {
    CHECK(countdown_segments(3)==0x4f && countdown_segments(2)==0x5b && countdown_segments(1)==0x06);
    CHECK(countdown_segments(-1)==0 && countdown_segments(10)==0);
    const unsigned char *digit8=font5x7('8'),*digit7=font5x7('7');
    CHECK(digit8 && digit8[0]==14 && digit8[3]==14 && digit8[6]==14);
    CHECK(digit7 && digit7[0]==31 && digit7[6]==8);
    overlay_batch_begin((IDirect3DDevice9*)1);
    CHECK(SUCCEEDED(overlay_rect((IDirect3DDevice9*)1,1,2,3,4,0xffffffff)) && overlay_vertex_count==6);
    overlay_batch_cancel();
    D3DVIEWPORT9 marker_viewport; memset(&marker_viewport,0,sizeof(marker_viewport));
    marker_viewport.X=100; marker_viewport.Y=50; marker_viewport.Width=640; marker_viewport.Height=480;
    CHECK(finish_marker_fully_visible(200,150,&marker_viewport,48));
    CHECK(!finish_marker_fully_visible(-34,290,&marker_viewport,48));
    CHECK(!finish_marker_fully_visible(700,150,&marker_viewport,48));
    CHECK(TIMER_SAMPLE_SECONDS>0.0332f && TIMER_SAMPLE_SECONDS<0.0334f && TIMER_DISPLAY_INTERVAL_MS==33);
    CHECK(countdown_seconds_remaining(0)==3 && countdown_seconds_remaining(999)==3);
    CHECK(countdown_seconds_remaining(1000)==2);
    CHECK(countdown_seconds_remaining(2000)==1);
    CHECK(countdown_seconds_remaining(3000)==0);
    void *cfg=alloc(0x1000),*car=alloc(0x1000),*move=alloc(0x4000),*line=alloc(0x75400),*damage_root=alloc(0x1000),*part=alloc(0x4000);
    CHECK(cfg && car && move && line && damage_root && part);
    fake_movement=move;
    putptr(0x7eac48,cfg);putptr(0x165fc68,car);putptr(0x8ef660,move);putptr(0x1659184,line);putptr(0x165fa90,damage_root);
    putptr((uintptr_t)damage_root+0xc98,part);
    puti(cfg,0x728,1);putf(car,0x140,10);putf(car,0x244,-10);putf(car,0x20,150);putf(car,0x13c,1.5f);putf(car,0xc,20);
    puti(part,0x10+0x3b20,1);putf(part,0x10+0x3aa0,1);
    for(int j=0;j<4;j++)puti(part,0x10+0x3a30+12*j,1);
    putf(move,0x10c,1);
    putf(move,0x140,123);putf(move,0x144,456);putf(move,0x148,789);putf(move,0x14c,1);
    puti(move,0x1100,5);
    puti(line,0x75310,5);for(int j=0;j<5;j++)putf(line,0x30+0x30*j,(float)j*100);
    GetFullPathNameW(L"work\\frame-test.log",1024,folder,0);
    log_file=CreateFileW(folder,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);CHECK(log_file!=INVALID_HANDLE_VALUE);
    const unsigned char reset[]={0x55,0x8b,0xec,0x83,0xe4,0xf0,0x83,0xec,0x3c};
    const unsigned char ctor[]={0x0f,0x57,0xc0,0x8b,0xc1,0xc7,0x40,0x04};
    const unsigned char notes[]={0x56,0x57,0xc7,0x41,0x5c,0,0,0,0};
    memcpy(code_reset,reset,sizeof(reset));memcpy(code_ctor,ctor,sizeof(ctor));memcpy(code_notes,notes,sizeof(notes));
    CHECK(runtime_matches(1));
    /* Simulate a reset hook. Diagnostics must notice it; never follow/call it. */
    code_reset[0]=0xe9;int32_t rel=0x52fea0-(0x4e8df0+5);memcpy(code_reset+1,&rel,4);
    CHECK(!runtime_matches(1));
    unsigned char hooked_entry[32]={0},hooked_body[sizeof(rsf_body_expected)];
    uintptr_t rsf_base=0x58ec0000;
    hooked_entry[0]=0xe9;hooked_entry[5]=0x90;
    memcpy(hooked_entry+6,reset_tail_expected,sizeof(reset_tail_expected));
    rel=(int32_t)(rsf_base+RSF_HOOK_RVA-0x4e8df5);memcpy(hooked_entry+1,&rel,4);
    memcpy(hooked_body,rsf_test_body,sizeof(hooked_body));
    CHECK(rsf_reset_layout(hooked_entry,rsf_base,hooked_body,0x4e8df6));
    CHECK(!rsf_reset_layout(hooked_entry,rsf_base,hooked_body,0x4e8df7));
    CHECK(!rsf_reset_layout(hooked_entry,rsf_base+0x10000,hooked_body,0x4e8df6));
    hooked_entry[1]^=1;CHECK(!rsf_reset_layout(hooked_entry,rsf_base,hooked_body,0x4e8df6));hooked_entry[1]^=1;
    hooked_entry[5]=0xcc;CHECK(!rsf_reset_layout(hooked_entry,rsf_base,hooked_body,0x4e8df6));hooked_entry[5]=0x90;
    hooked_entry[10]^=1;CHECK(!rsf_reset_layout(hooked_entry,rsf_base,hooked_body,0x4e8df6));hooked_entry[10]^=1;
    hooked_body[30]^=1;CHECK(!rsf_reset_layout(hooked_entry,rsf_base,hooked_body,0x4e8df6));hooked_body[30]^=1;
    hooked_body[rsf_relocations[0]]^=1;CHECK(!rsf_reset_layout(hooked_entry,rsf_base,hooked_body,0x4e8df6));
    void *manager=alloc(0x11000),*notes_state=alloc(0x1000);
    CHECK(manager && notes_state);putptr(0x8d5338,manager);putptr((uintptr_t)manager+0x105b0,notes_state);
    void *movement_table=alloc(0x200);CHECK(movement_table);
    putptr((uintptr_t)move,movement_table);putptr((uintptr_t)movement_table+0x180,(void*)0x4e9660);
    memcpy(code_pose,fixture_patched_pose,sizeof(code_pose));
    memcpy(code_ngp_pose,fixture_ngp_pose,sizeof(code_ngp_pose));
    CHECK(pose_ngp_layout(code_pose,0x5a310000,code_ngp_pose));
    CHECK(!pose_ngp_layout(code_pose,0x5a320000,code_ngp_pose));
    CHECK(!pose_ngp_layout(code_pose,0,code_ngp_pose));
    CHECK(!pose_ngp_layout(pose_body_expected,0x5a310000,code_ngp_pose));
    code_pose[POSE_CALL_OPERAND_OFFSET]^=1;CHECK(!pose_ngp_layout(code_pose,0x5a310000,code_ngp_pose));code_pose[POSE_CALL_OPERAND_OFFSET]^=1;
    code_pose[30]^=1;CHECK(!pose_ngp_layout(code_pose,0x5a310000,code_ngp_pose));code_pose[30]^=1;
    code_ngp_pose[40]^=1;CHECK(!pose_ngp_layout(code_pose,0x5a310000,code_ngp_pose));code_ngp_pose[40]^=1;
    code_ngp_pose[ngp_pose_relocations[0]]^=1;CHECK(!pose_ngp_layout(code_pose,0x5a310000,code_ngp_pose));code_ngp_pose[ngp_pose_relocations[0]]^=1;
    restore_orientation=1;restore_speed=1;
    executable_ok=ngp_ok=checked_ngp=config_ok=1;
    CHECK(pose_runtime_matches((uintptr_t)move,1));
    ngp_ok=0;CHECK(!pose_runtime_matches((uintptr_t)move,1));ngp_ok=1;
    frame_tick(); CHECK(was_in_run && generation==1 && geometry_count==5);
    CHECK(previous_sample.x==123 && previous_sample.y==456 && previous_sample.z==789);
    fake_keys[VK_RETURN]=1;frame_tick();CHECK(mark.valid && mark.sample.distance==150 && mark.target_index==1.5f);
    putf(car,0x20,154);putf(car,0x140,10.2f);frame_tick();CHECK(mark.sample.distance==150);
    fake_keys[VK_RETURN]=0;frame_tick();
    putf(car,0x20,156);putf(car,0x140,10.3f);frame_tick();CHECK(timer_state==TIMER_RUNNING && fabsf(timer_start_line-155)<.001f);
    for(int i=0;i<14;i++) { putf(car,0x20,166.0f+10.0f*i);putf(car,0x140,10.7f+.4f*i);frame_tick(); }
    putf(car,0x20,300);putf(car,0x140,16.5f);frame_tick();
    CHECK(timer_run_count>=10 && timer_state==TIMER_RUNNING);
    putf(car,0xc,0);frame_tick();stopped_since=GetTickCount()-700;stopped_tracking=1;
    fake_keys[VK_SPACE]=1;frame_tick(); CHECK(strstr(message,"DRY RUN")!=0 && mark.valid && !verifying);
    CHECK(timer_finish_set && timer_reference_valid && fabsf(timer_finish_line-300)<.01f && timer_best_count>=2);
    timer_prepare_trial();frame_tick();
    putf(car,0x20,150);putf(car,0x140,16.6f);frame_tick();
    putf(car,0x20,154);putf(car,0x140,16.7f);frame_tick();
    putf(car,0x20,156);putf(car,0x140,16.8f);frame_tick();
    for(int i=0;i<=14;i++) { putf(car,0x20,166.0f+10.0f*i);putf(car,0x140,17.1f+.3f*i);frame_tick(); }
    CHECK(timer_state==TIMER_FINISHED && timer_last_result_delta<-.1f && timer_best_total>0 && timer_display_mode==4 && timer_last_result_valid);
    float faster_best=timer_best_total;
    timer_prepare_trial();frame_tick();
    CHECK(timer_display_mode==4 && timer_last_result_valid); /* Keep results visible after return until the next start line. */
    putf(car,0x20,150);putf(car,0x140,22.4f);frame_tick();
    putf(car,0x20,154);putf(car,0x140,22.5f);frame_tick();
    putf(car,0x20,156);putf(car,0x140,22.6f);frame_tick();
    for(int i=0;i<=14;i++) { putf(car,0x20,166.0f+10.0f*i);putf(car,0x140,23.1f+.5f*i);frame_tick(); }
    CHECK(timer_state==TIMER_FINISHED && timer_last_result_delta>.1f && fabsf(timer_best_total-faster_best)<.001f);
    frame_tick();CHECK(!verifying);
    fake_keys[VK_SPACE]=0;frame_tick();
    puti(cfg,0x728,2);frame_tick();CHECK(mark.valid && was_paused && generation==1);
    fake_keys[VK_SPACE]=1;frame_tick();CHECK(strstr(message,"not_driving")!=0 && mark.valid && !verifying);
    fake_keys[VK_SPACE]=0;frame_tick();puti(cfg,0x728,1);frame_tick();
    CHECK(mark.valid && !was_paused && generation==1);
    stopped_since=GetTickCount()-700;stopped_tracking=1;live_enabled=1;
    fake_keys[VK_SPACE]=1;frame_tick();CHECK(strstr(message,"runtime_code_changed")!=0 && !verifying);
    fake_keys[VK_SPACE]=0;frame_tick();
    putf(car,0xc,601);putf(move,0x1c0,170);
    fake_keys[VK_SPACE]=1;frame_tick();CHECK(strstr(message,"runtime_code_changed")!=0 && reset_calls==0);
    fake_keys[VK_SPACE]=0;frame_tick();
    putf(car,0xc,50);putf(move,0x1c0,14);live_enabled=0;
    fake_keys[VK_SPACE]=1;frame_tick();CHECK(strstr(message,"DRY RUN")!=0 && reset_calls==0);
    fake_keys[VK_SPACE]=0;frame_tick();live_enabled=1;
    fake_keys[VK_SPACE]=1;frame_tick();CHECK(strstr(message,"runtime_code_changed")!=0 && reset_calls==0);
    fake_keys[VK_SPACE]=0;frame_tick();
    memcpy(code_reset,reset,sizeof(reset));
    code_pose[0]^=1;fake_keys[VK_SPACE]=1;frame_tick();
    CHECK(reset_calls==0 && strstr(message,"saved pose or route unverified")!=0);
    fake_keys[VK_SPACE]=0;frame_tick();code_pose[0]^=1;
    putptr((uintptr_t)movement_table+0x180,(void*)0x4e9661);fake_keys[VK_SPACE]=1;frame_tick();
    CHECK(reset_calls==0 && strstr(message,"saved pose or route unverified")!=0);
    fake_keys[VK_SPACE]=0;frame_tick();putptr((uintptr_t)movement_table+0x180,(void*)0x4e9660);
    /* One Space press at nonzero speed must call only the mock sequence once. */
    fake_keys[VK_SPACE]=1;frame_tick();
    Sample corrected;
    CHECK(reset_calls==1 && pose_calls>=2 && pose_restored && verifying && target_observed && mark.valid && countdown_active);
    CHECK(gear_neutral_active && !gear_restored && seq_keydowns==0 && get_sample(&corrected) && corrected.gear==GEAR_HOLD_RAW_CANDIDATE);
    CHECK(get_sample(&corrected) && rotation_error_degrees(corrected.q,mark.sample.q)<.1f);
    CHECK(corrected.x==mark.sample.x && corrected.y==mark.sample.y && fabsf(corrected.z-mark.sample.z)<=0.05f);
    CHECK(corrected.velocity[0]==0 && corrected.velocity[1]==0 && corrected.velocity[2]==0);
    CHECK(load_before.speed==50 && load_before.vector_speed==14 && stopped_tracking==0);
    frame_tick();frame_tick();CHECK(reset_calls==1 && verifying);
    fake_keys[VK_SPACE]=0;frame_tick();fake_keys[VK_SPACE]=1;frame_tick();
    CHECK(reset_calls==1 && countdown_active && strstr(message,"Moving in 3")!=0);
    fake_keys[VK_SPACE]=0;frame_tick();
    /* Countdown hold raw value stays selected; saved gear engages before saved velocity is released. */
    putf(car,0x20,154);putf(car,0xc,20);putf(move,0x140,999);putf(move,0x1c0,77);puti(move,0x1100,1);fake_ticks+=999;frame_tick();
    CHECK(countdown_active && !disabled && strstr(message,"Moving in 3")!=0);
    CHECK(previous_sample.x==999); /* frame sample precedes the hold write */
    CHECK(get_sample(&corrected) && corrected.x==mark.sample.x && corrected.y==mark.sample.y && fabsf(corrected.z-mark.sample.z)<=0.05f);
    CHECK(corrected.velocity[0]==0 && corrected.gear==GEAR_HOLD_RAW_CANDIDATE && !gear_restored && seq_keydowns==0);
    putf(move,0x140,888);fake_ticks+=1;frame_tick();CHECK(countdown_active && strstr(message,"Moving in 2")!=0 && get_sample(&corrected) && corrected.gear==GEAR_HOLD_RAW_CANDIDATE);
    putf(move,0x140,777);fake_ticks+=1000;frame_tick();CHECK(countdown_active && strstr(message,"Moving in 1")!=0 && !gear_restored && seq_keydowns==0 && get_sample(&corrected) && corrected.gear==GEAR_HOLD_RAW_CANDIDATE);
    putf(move,0x140,666);fake_ticks+=1000;frame_tick();CHECK(countdown_active && !gear_restored && seq_state==1 && seq_keydowns==0 && strstr(message,"Engaging saved gear")!=0 && get_sample(&corrected) && corrected.gear==4 && corrected.velocity[0]==0);
    fake_ticks+=1;frame_tick();CHECK(countdown_active && seq_state==2 && seq_keydowns==1 && get_sample(&corrected) && corrected.gear==mark.sample.gear && corrected.velocity[0]==0);
    fake_ticks+=80;frame_tick();CHECK(countdown_active && seq_state==3 && seq_keyups==1 && corrected.velocity[0]==0);
    fake_ticks+=150;frame_tick();CHECK(!countdown_active && gear_restored && verifying && !disabled && strstr(message,"GO")!=0);
    CHECK(get_sample(&corrected) && corrected.x==mark.sample.x && fabsf(corrected.velocity[0]-mark.sample.velocity[0])<.001f);
    fake_ticks+=500;frame_tick();CHECK(!verifying && !disabled && strstr(message,"Returned;")!=0);
    /* Unsupported saved raw gear falls back to a direct field write. A later
     * mismatch remains nonfatal and must not disable reset or timer. */
    mark.sample.gear=2;
    putf(car,0x20,220);putf(car,0xc,31);fake_keys[VK_SPACE]=1;frame_tick();
    CHECK(reset_calls==2 && verifying && countdown_active && gear_neutral_active && !gear_restored && get_sample(&corrected) && corrected.gear==GEAR_HOLD_RAW_CANDIDATE);
    fake_keys[VK_SPACE]=0;fake_ticks+=3000;frame_tick();CHECK(!countdown_active && verifying && !disabled && gear_restored && get_sample(&corrected) && corrected.gear==2);
    /* The +5 m start is crossed during the 500 ms verification window. The
     * timer must use the release baseline and keep sampling while verifying. */
    puti(move,0x1100,1);putf(car,0x20,156);putf(car,0xc,31.5f);pose_position_ok=0;speed_restored=0;fake_ticks+=500;frame_tick();
    CHECK(!verifying && !disabled && strstr(message,"position or speed differs")!=0);
    CHECK(timer_state==TIMER_RUNNING && timer_run_count==1 && timer_start_time>30.0f && timer_start_time<30.2f);
    putf(car,0x20,154);putf(car,0xc,31.6f);frame_tick();
    putf(car,0x20,156);putf(car,0xc,31.7f);frame_tick();
    CHECK(timer_state==TIMER_RUNNING && !disabled);
    /* Metres alone must not verify a return when the heading remains wrong. */
    gear_restore_mode=1;
    unsigned pose_calls_before_wrong_heading=pose_calls;
    putf(car,0x20,220);pose_ignores_rotation=1;fake_keys[VK_SPACE]=1;frame_tick();
    CHECK(reset_calls==3 && pose_calls>pose_calls_before_wrong_heading && verifying && target_observed);
    fake_keys[VK_SPACE]=0;fake_ticks+=3500;frame_tick();CHECK(!countdown_active && verifying && !disabled);
    fake_ticks+=500;frame_tick();
    CHECK(disabled && !verifying && rotation_error_degrees(previous_sample.q,mark.sample.q)>80);
    disabled=0;pose_ignores_rotation=0;
    /* A second distinct press may initiate the next return without an arm key. */
    putf(car,0x20,220);reset_misses=1;fake_keys[VK_SPACE]=1;frame_tick();
    CHECK(reset_calls==4 && verifying && !target_observed);
    fake_keys[VK_SPACE]=0;fake_ticks+=500;frame_tick();
    CHECK(disabled && !verifying && strstr(message,"Load context or orientation mismatch")!=0);
    fake_keys[VK_SPACE]=1;frame_tick();CHECK(reset_calls==4);
    disabled=0;fake_keys[VK_SPACE]=0;frame_tick();
    /* Context loss while verification is pending must be visible and disable calls. */
    reset_misses=0;fake_keys[VK_SPACE]=1;frame_tick();CHECK(reset_calls==5 && verifying);
    fake_keys[VK_SPACE]=0;putf(car,0x140,1);frame_tick();
    CHECK(disabled && !mark.valid && !verifying && generation==2);
    disabled=0;putf(car,0x20,NAN);fake_keys[VK_RETURN]=1;frame_tick();CHECK(!mark.valid);
    fake_keys[VK_RETURN]=0;putf(car,0x20,250);frame_tick();
    timer_finish_set=1;timer_reference_valid=1;timer_best_count=2;timer_best_total=8.5f;timer_last_result_valid=1;timer_last_result_total=8.7f;timer_finish_world_valid=1;
    fake_keys[VK_RETURN]=1;frame_tick();
    CHECK(mark.valid && timer_display_mode==3 && !timer_finish_set && !timer_reference_valid && timer_best_count==0 && !timer_last_result_valid && !timer_finish_world_valid);
    fake_keys[VK_RETURN]=0;frame_tick();
    timer_finish_set=1;timer_reference_valid=1;timer_best_count=2;timer_best_total=8.5f;timer_last_result_valid=1;timer_last_result_total=8.7f;timer_finish_world_valid=1;
    Sample queue_sample;CHECK(get_sample(&queue_sample));queue_sample.generation=generation;previous_sample=queue_sample;was_in_run=1;was_paused=0;
    freeze_pose=mark.sample;freeze_pose_input_z=mark.sample.z;countdown_active=1;countdown_started_at=GetTickCount();countdown_released_at=0;countdown_last=-1;
    verifying=1;target_observed=1;load_observed=mark.sample;verification_at=GetTickCount();pose_restored=1;pose_position_ok=1;pose_rotation_error=0;speed_restored=0;gear_attempted=1;gear_restored=1;gear_neutral_active=0;seq_state=0;
    fake_keys[VK_RETURN]=1;frame_tick();
    CHECK(pending_fresh_session && !pending_fresh_sample_valid && countdown_active && verifying);
    fake_keys[VK_RETURN]=0;frame_tick();countdown_started_at=GetTickCount()-(DWORD)(RESUME_DELAY_SECONDS*1000);frame_tick();
    CHECK(!countdown_active && verifying && pending_fresh_session && pending_fresh_sample_valid);
    fake_ticks+=500;frame_tick();
    CHECK(!pending_fresh_session && timer_display_mode==3 && !timer_finish_set && !timer_reference_valid && timer_best_count==0 && !timer_last_result_valid && !timer_finish_world_valid);
    render_thread=GetCurrentThreadId()+1;frame_tick();CHECK(disabled);
    FlushFileBuffers(log_file);CloseHandle(log_file);log_file=INVALID_HANDLE_VALUE;
    printf("PASS: %u integration assertions; single Space press while moving, held-key suppression, exact finish at first Space, fresh Enter session, nonfatal position/speed/gear mismatches, neutral during fixed three-second countdown, sequential gear restore before saved velocity release, direct fallback, saved XYZ and heading restore, missed/aborted verification, RSF layout and dry run; reset and pose sequences MOCKED; wrong-heading rejection; verified NGP pose dispatch\n",checks);
    return 0;
}
