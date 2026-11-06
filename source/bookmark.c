/* LukyanukTrainingPlugin 0.5.11-rbrcontrols-ignition-test. No RPM code; hash-gated RBRControls ignition test.
 * Native x86 test plugin, no PaceNote dependency.
 * ABI declarations are written in C deliberately: GNU C++ virtual destructors
 * have a different layout from RBR's MSVC IPlugin ABI.
 * Address provenance: supplied binary analysis and NGPCarMenu RBRAPI sources.
 */
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <bcrypt.h>
#include <d3d9.h>
#include <dinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include "core.h"
#include "jump_back.h"
#include "rsf_layout.h"
#include "pose_layout.h"
#include "ngp_pose_layout.h"
#define TC __attribute__((thiscall))
#define VERSION "0.5.11-rbrcontrols-ignition-test"
#define DISPLAY_VERSION "0.5.11"
#define DIAGNOSTIC_DISABLE_GAMEPLAY_OVERLAYS 0
#define RESUME_DELAY_SECONDS 3
#define POSITION_SAVED_DISPLAY_MS 3000
#define RESULTS_DISPLAY_DEFAULT_SECONDS 5
#define RESULTS_DISPLAY_MIN_SECONDS 1
#define RESULTS_DISPLAY_MAX_SECONDS 30
#define GEAR_HOLD_RAW_CANDIDATE 1
#define CLUTCH_HELP_VALUE_ADDRESS ((uintptr_t)0x1660EA4u)
#define CLUTCH_HELP_ROOT_ADDRESS ((uintptr_t)0x7ECD00u)
#define CLUTCH_HELP_WRAPPER_OFFSET ((uintptr_t)0x6DCu)
#define EXE_HASH "5bb1b0c8f6719faecad63b0aafd7541089d1d1ff7a4c9bb68ed49bb53dc6f63b"
#define NGP_HASH "cf351cdf99f880631ed2cb1d9b6b5c9db7d7c233ad905740393c37df967801ec"
typedef struct { void **v; } Object;
typedef HRESULT (WINAPI *EndSceneFn)(IDirect3DDevice9 *);
static HMODULE self_module;
static Object plugin, *game;
static HANDLE log_file=INVALID_HANDLE_VALUE;
static wchar_t folder[1024], ini_path[1024];
static UINT_PTR timer_id;
static void **hook_slot;
static EndSceneFn previous_end;
static volatile LONG busy, log_busy, inside_load, disabled;
static DWORD factory_thread, render_thread, last_heartbeat, stopped_since, verification_at;
static DWORD last_config, message_until;
static LONG sequence;
static unsigned generation;
static int runtime_reported;
static HMODULE rsf_checked_module;
static int rsf_hash_ok;
static int executable_ok, checked_ngp, ngp_ok, was_in_run, was_paused, was_focus=-1, keys[4];
static int run_context_suspended, suspended_context_valid;
static Sample suspended_context_sample;
static int vk_save=VK_RETURN, vk_load=VK_SPACE, vk_jump_back=0, vk_reload=VK_F10;
static int vk_seq_up=VK_F12;
enum { BIND_KEYBOARD=0, BIND_CONTROLLER=1, JOY_MAX_DEVICES=16, JOY_BUTTON_COUNT=128 };
typedef struct { GUID guid; IDirectInputDevice8A *device; DIJOYSTATE2 state,previous; BYTE button_edges[JOY_BUTTON_COUNT]; unsigned button_count; int ready,have_sample; } JoyDevice;
static HMODULE dinput_module;
static IDirectInput8A *direct_input;
static JoyDevice joy_devices[JOY_MAX_DEVICES];
static unsigned joy_device_count;
static int joy_reported;
static int save_input_kind=BIND_KEYBOARD,load_input_kind=BIND_KEYBOARD,jump_back_input_kind=BIND_KEYBOARD;
static GUID save_joy_guid,load_joy_guid,jump_back_joy_guid;
static int save_joy_button=-1,load_joy_button=-1,jump_back_joy_button=-1;
static int menu_selection,menu_binding_target,menu_binding_wait_release;
static DWORD plugin_menu_draw_tick;
static int menu_overlay_reported=-1;
static int live_enabled, stopped_tracking, verifying;
static int restore_position=1, restore_orientation, restore_speed=1, restore_gear=1;
static int clutch_help_during_countdown=1, clutch_help_temp_active;
static uintptr_t clutch_help_saved_root, clutch_help_saved_wrapper;
static int clutch_help_saved_global, clutch_help_saved_status, clutch_help_saved_value;
static DWORD clutch_help_restore_retry_at;
static int pose_attempted, pose_restored, pose_position_ok, speed_attempted, speed_restored, gear_attempted, gear_restored, gear_neutral_active, gear_neutral_last_reported=-1;
static int seq_state, seq_key_down, seq_saved_gear, seq_stage_gear, seq_target_gear, seq_keyup_ok;
static uintptr_t seq_movement;
static DWORD seq_release_at, seq_check_at;
static int countdown_active, countdown_last=-1;
static int overlay_reported_second=-1;
static DWORD countdown_started_at, countdown_released_at;
static int rbr_ignition_hold_active, rbr_ignition_bridge_checked, rbr_ignition_bridge_available;
static DWORD rbr_ignition_release_at;
static int pending_fresh_session,pending_fresh_sample_valid;
static Sample pending_fresh_sample;
static Sample freeze_pose, freeze_last;
static float freeze_q[4], freeze_pose_input_z, pose_position_error, pose_rotation_error;
static int config_ok=1, shutdown_done;
static Sample previous_sample;
static Mark mark;
static int pending_first_finish_valid;
static Sample pending_first_finish_sample;
static Sample load_before, load_observed;
static int target_observed;
static unsigned first_observed_ms, motion_bucket;
static char message[192]="LukyanukTrainingPlugin: initializing";
static DWORD render_heartbeat_at;
static unsigned render_heartbeat_frames;
#if DIAGNOSTIC_DISABLE_GAMEPLAY_OVERLAYS
static int render_diagnostic_reported;
#endif
static float distances[10000];
static uintptr_t geometry_line;
static unsigned geometry_count;
#define TIMER_MAX_POINTS 12000
#define TIMER_SAMPLE_SECONDS (1.0f/30.0f)
#define TIMER_DISPLAY_INTERVAL_MS 33
enum { TIMER_OFF=0, TIMER_WAIT_START=1, TIMER_RUNNING=2, TIMER_FINISHED=3 };
static TimerPoint timer_run[TIMER_MAX_POINTS], timer_best[TIMER_MAX_POINTS];
static unsigned timer_run_count, timer_best_count;
static int timer_armed, timer_state, timer_finish_set, timer_reference_valid, timer_previous_valid;
static float timer_start_line, timer_finish_line, timer_start_time, timer_last_record_time;
static float timer_best_total, timer_last_result_delta, timer_display_value, timer_last_result_total;
static int timer_last_result_valid;
static int timer_display_mode; /* 0 hidden, 1 elapsed, 2 delta to best, 4 finished summary */
static DWORD timer_display_updated, position_saved_until;
static int timer_regression_reported;
static int results_display_seconds=RESULTS_DISPLAY_DEFAULT_SECONDS;
static DWORD timer_result_display_started_at;
static int timer_result_display_active;
static int timer_overlay_reported=-1;
static int timer_overlay_suspended_reported;
static float timer_previous_distance, timer_previous_race_time;
static float timer_finish_world[3];
static int timer_finish_world_valid,finish_marker_reported,finish_marker_candidate;
static DWORD finish_marker_candidate_at;
static JumpBackState jump_back_state;
static int jump_back_pending;
static unsigned jump_back_pending_point;
static float jump_back_pending_distance,jump_back_pending_index;
static Sample jump_back_before;
static DWORD jump_back_started_at,jump_back_last_success_at;
#define DAMAGE_BUFFER_SIZE 0x3EB0u
static uintptr_t saved_damage_address;
static char saved_damage_hash[65];
static int saved_damage_valid;

static void log_event(const char *event,const char *fmt,...) {
    if (log_file==INVALID_HANDLE_VALUE) return;
    if (InterlockedCompareExchange(&log_busy,1,0)) return;
    SYSTEMTIME t; GetLocalTime(&t);
    char detail[2000], line[2300]; va_list args; va_start(args,fmt);
    vsnprintf(detail,sizeof(detail),fmt,args); va_end(args);
    int n=snprintf(line,sizeof(line),"%04u-%02u-%02uT%02u:%02u:%02u.%03u seq=%ld tick=%lu thread=%lu event=%s %s\r\n",
        t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds,
        InterlockedIncrement(&sequence),(unsigned long)GetTickCount(),(unsigned long)GetCurrentThreadId(),event,detail);
    if(n>0) { DWORD written; if(!WriteFile(log_file,line,(DWORD)strlen(line),&written,0) || written!=strlen(line)) InterlockedExchange(&disabled,1); }
    /* Keep the 30 Hz render-thread sample stream buffered. Control events
     * still flush immediately so Save/Load diagnostics survive. */
    if(strcmp(event,"SAMPLE") && strcmp(event,"TIMER_SAMPLE")) FlushFileBuffers(log_file);
    InterlockedExchange(&log_busy,0);
}
static void notify(const char *text) {
    snprintf(message,sizeof(message),"LukyanukTrainingPlugin: %s",text); message_until=GetTickCount()+6000;
    log_event("MESSAGE","text=%s",text);
}
typedef HRESULT (WINAPI *DirectInput8CreateFn)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
static const IID iid_direct_input8a={0xBF798030,0x483A,0x4DA2,{0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00}};
static const GUID guid_button_object={0xA36D02F0,0xC9F3,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
static DIOBJECTDATAFORMAT joy_object_format[JOY_BUTTON_COUNT];
static DIDATAFORMAT joy_data_format;
static DWORD joy_refresh_at;
static void make_joy_data_format(unsigned button_count) {
    if(button_count>JOY_BUTTON_COUNT) button_count=JOY_BUTTON_COUNT;
    for(unsigned i=0;i<button_count;i++) {
        joy_object_format[i].pguid=&guid_button_object;
        joy_object_format[i].dwOfs=(DWORD)(offsetof(DIJOYSTATE2,rgbButtons)+i);
        joy_object_format[i].dwType=DIDFT_BUTTON|DIDFT_MAKEINSTANCE(i);
        joy_object_format[i].dwFlags=0;
    }
    joy_data_format.dwSize=sizeof(joy_data_format);
    joy_data_format.dwObjSize=sizeof(DIOBJECTDATAFORMAT);
    joy_data_format.dwFlags=0;
    joy_data_format.dwDataSize=sizeof(DIJOYSTATE2);
    joy_data_format.dwNumObjs=button_count;
    joy_data_format.rgodf=joy_object_format;
}
static void guid_to_text(const GUID *guid,wchar_t out[33]) {
    swprintf(out,33,L"%08lX%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
      (unsigned long)guid->Data1,guid->Data2,guid->Data3,guid->Data4[0],guid->Data4[1],guid->Data4[2],guid->Data4[3],
      guid->Data4[4],guid->Data4[5],guid->Data4[6],guid->Data4[7]);
}
static int parse_guid(const wchar_t *text,GUID *guid) {
    if(!text || !guid || wcslen(text)!=32) return 0;
    unsigned long values[11]={0};
    const unsigned widths[11]={8,4,4,2,2,2,2,2,2,2,2};
    unsigned offset=0;
    for(unsigned i=0;i<11;i++) {
        for(unsigned j=0;j<widths[i];j++) {
            wchar_t c=text[offset++]; unsigned digit;
            if(c>=L'0' && c<=L'9') digit=(unsigned)(c-L'0');
            else if(c>=L'A' && c<=L'F') digit=(unsigned)(c-L'A'+10);
            else if(c>=L'a' && c<=L'f') digit=(unsigned)(c-L'a'+10);
            else return 0;
            values[i]=(values[i]<<4)|digit;
        }
    }
    guid->Data1=(DWORD)values[0]; guid->Data2=(WORD)values[1]; guid->Data3=(WORD)values[2];
    for(unsigned i=0;i<8;i++) guid->Data4[i]=(BYTE)values[3+i];
    return 1;
}
static int valid_vk(int vk) { return vk>=8 && vk<=254; }
static int guid_equal(const GUID *a,const GUID *b) { return !memcmp(a,b,sizeof(*a)); }
static BOOL CALLBACK enum_joystick(const DIDEVICEINSTANCEA *instance,void *context) {
    (void)context;
    if(joy_device_count>=JOY_MAX_DEVICES || !direct_input) return DIENUM_STOP;
    JoyDevice *entry=&joy_devices[joy_device_count]; memset(entry,0,sizeof(*entry));
    HRESULT hr=IDirectInput8_CreateDevice(direct_input,&instance->guidInstance,&entry->device,0);
    if(FAILED(hr) || !entry->device) { log_event("CONTROLLER_OPEN_FAILED","product=%s hr=%08lx",instance->tszProductName,(unsigned long)hr); return DIENUM_CONTINUE; }
    DIDEVCAPS capabilities; memset(&capabilities,0,sizeof(capabilities)); capabilities.dwSize=sizeof(capabilities);
    hr=IDirectInputDevice8_GetCapabilities(entry->device,&capabilities);
    unsigned buttons=capabilities.dwButtons>JOY_BUTTON_COUNT?JOY_BUTTON_COUNT:capabilities.dwButtons;
    if(FAILED(hr) || !buttons) {
        log_event("CONTROLLER_INIT_FAILED","product=%s stage=button_capabilities hr=%08lx reported_buttons=%lu",instance->tszProductName,(unsigned long)hr,(unsigned long)capabilities.dwButtons);
        IDirectInputDevice8_Release(entry->device); entry->device=0; return DIENUM_CONTINUE;
    }
    make_joy_data_format(buttons);
    hr=IDirectInputDevice8_SetDataFormat(entry->device,&joy_data_format);
    HWND window=GetForegroundWindow();
    if(SUCCEEDED(hr)) hr=IDirectInputDevice8_SetCooperativeLevel(entry->device,window,DISCL_BACKGROUND|DISCL_NONEXCLUSIVE);
    if(FAILED(hr)) {
        log_event("CONTROLLER_INIT_FAILED","product=%s stage=data_or_coop hr=%08lx hwnd=%p",instance->tszProductName,(unsigned long)hr,window);
        IDirectInputDevice8_Release(entry->device); entry->device=0; return DIENUM_CONTINUE;
    }
    entry->guid=instance->guidInstance; entry->button_count=buttons; entry->ready=1; joy_device_count++;
    log_event("CONTROLLER_READY","index=%u product=%s buttons=%u guid_data1=%08lx guid_data2=%04x guid_data3=%04x",joy_device_count-1,instance->tszProductName,buttons,(unsigned long)entry->guid.Data1,entry->guid.Data2,entry->guid.Data3);
    return DIENUM_CONTINUE;
}
static void ensure_controller_input(void) {
#ifdef BOOKMARK_TEST_NO_DIRECTINPUT
    return;
#endif
    if(!direct_input) {
        if(!dinput_module) dinput_module=LoadLibraryW(L"dinput8.dll");
        if(!dinput_module) { if(!joy_reported) log_event("CONTROLLER_INPUT_UNAVAILABLE","stage=load_dinput8 winerror=%lu",GetLastError()); joy_reported=1; return; }
        DirectInput8CreateFn create=(DirectInput8CreateFn)GetProcAddress(dinput_module,"DirectInput8Create");
        if(!create) { if(!joy_reported) log_event("CONTROLLER_INPUT_UNAVAILABLE","stage=resolve_DirectInput8Create winerror=%lu",GetLastError()); joy_reported=1; return; }
        HRESULT hr=create((HINSTANCE)GetModuleHandleW(0),DIRECTINPUT_VERSION,&iid_direct_input8a,(void**)&direct_input,0);
        if(FAILED(hr) || !direct_input) { if(!joy_reported) log_event("CONTROLLER_INPUT_UNAVAILABLE","stage=create_interface hr=%08lx",(unsigned long)hr); joy_reported=1; direct_input=0; return; }
        joy_reported=0;
        log_event("CONTROLLER_INPUT_READY","backend=DirectInput8 enumeration=game_controllers exclusive=0");
    }
    if(!joy_device_count && (DWORD)(GetTickCount()-joy_refresh_at)>=1000) {
        joy_refresh_at=GetTickCount();
        HRESULT hr=IDirectInput8_EnumDevices(direct_input,DI8DEVCLASS_GAMECTRL,enum_joystick,0,DIEDFL_ATTACHEDONLY);
        if(FAILED(hr) && !joy_reported) { log_event("CONTROLLER_ENUM_FAILED","hr=%08lx",(unsigned long)hr); joy_reported=1; }
    }
}
static void poll_controllers(void) {
    ensure_controller_input();
    for(unsigned i=0;i<joy_device_count;i++) {
        JoyDevice *entry=&joy_devices[i];
        if(!entry->device) continue;
        DIJOYSTATE2 sample; memset(&sample,0,sizeof(sample));
        HRESULT hr=IDirectInputDevice8_Poll(entry->device);
        if(FAILED(hr)) { IDirectInputDevice8_Acquire(entry->device); hr=IDirectInputDevice8_Poll(entry->device); }
        if(SUCCEEDED(hr)) hr=IDirectInputDevice8_GetDeviceState(entry->device,sizeof(sample),&sample);
        if(SUCCEEDED(hr)) {
            if(entry->have_sample) {
                entry->previous=entry->state;
                for(unsigned b=0;b<JOY_BUTTON_COUNT;b++) if((sample.rgbButtons[b]&0x80) && !(entry->previous.rgbButtons[b]&0x80)) entry->button_edges[b]=1;
            }
            else { entry->previous=sample; entry->have_sample=1; }
            entry->state=sample; entry->ready=1;
        } else {
            entry->previous=entry->state; memset(&entry->state,0,sizeof(entry->state));
            if((DWORD)(GetTickCount()-last_heartbeat)>=1000) log_event("CONTROLLER_POLL_FAILED","index=%u hr=%08lx",i,(unsigned long)hr);
        }
    }
}
static int controller_button_down(const GUID *guid,int button) {
    if(button<0 || button>=JOY_BUTTON_COUNT) return 0;
    for(unsigned i=0;i<joy_device_count;i++) if(joy_devices[i].ready && guid_equal(&joy_devices[i].guid,guid))
        return (unsigned)button<joy_devices[i].button_count && (joy_devices[i].state.rgbButtons[button]&0x80)!=0;
    return 0;
}
static int controller_button_edge(const GUID *guid,int button) {
    if(button<0 || button>=JOY_BUTTON_COUNT) return 0;
    for(unsigned i=0;i<joy_device_count;i++) if(joy_devices[i].ready && guid_equal(&joy_devices[i].guid,guid))
        return (unsigned)button<joy_devices[i].button_count && (joy_devices[i].state.rgbButtons[button]&0x80)!=0 && (joy_devices[i].previous.rgbButtons[button]&0x80)==0;
    return 0;
}
static int any_controller_button_down(void) {
    for(unsigned i=0;i<joy_device_count;i++) for(unsigned b=0;b<joy_devices[i].button_count;b++)
        if(joy_devices[i].state.rgbButtons[b]&0x80) return 1;
    return 0;
}
static int capture_controller_button(GUID *guid,int *button) {
    for(unsigned i=0;i<joy_device_count;i++) for(unsigned b=0;b<joy_devices[i].button_count;b++)
        if(joy_devices[i].button_edges[b]) {
            *guid=joy_devices[i].guid; *button=(int)b;
            for(unsigned j=0;j<joy_device_count;j++) memset(joy_devices[j].button_edges,0,sizeof(joy_devices[j].button_edges));
            return 1;
        }
    return 0;
}
static int countdown_seconds_remaining(DWORD elapsed_ms) {
    DWORD duration=(DWORD)RESUME_DELAY_SECONDS*1000;
    return elapsed_ms>=duration?0:(int)((duration-elapsed_ms+999)/1000);
}
typedef struct { float x,y,z,rhw; DWORD color; } ScreenVertex;
#define OVERLAY_MAX_RECTS 32768
static ScreenVertex overlay_vertices[OVERLAY_MAX_RECTS*6];
static unsigned overlay_vertex_count;
static IDirect3DDevice9 *overlay_batch_device;
static int overlay_batch_active;
static void overlay_batch_begin(IDirect3DDevice9 *device) {
    overlay_vertex_count=0; overlay_batch_device=device; overlay_batch_active=1;
}
static void overlay_batch_cancel(void) {
    overlay_vertex_count=0; overlay_batch_device=0; overlay_batch_active=0;
}
static HRESULT overlay_batch_flush(IDirect3DDevice9 *device) {
    if(!overlay_batch_active || device!=overlay_batch_device) { overlay_batch_cancel(); return E_INVALIDARG; }
    overlay_batch_active=0; overlay_batch_device=0;
    unsigned vertices=overlay_vertex_count; overlay_vertex_count=0;
    if(!vertices) return D3D_OK;
    return IDirect3DDevice9_DrawPrimitiveUP(device,D3DPT_TRIANGLELIST,vertices/3,overlay_vertices,sizeof(overlay_vertices[0]));
}
static HRESULT overlay_rect(IDirect3DDevice9 *device,float left,float top,float right,float bottom,DWORD color) {
    ScreenVertex v[6]={
        {left,top,0,1,color},{right,top,0,1,color},{right,bottom,0,1,color},
        {left,top,0,1,color},{right,bottom,0,1,color},{left,bottom,0,1,color}
    };
    if(overlay_batch_active) {
        if(!device || device!=overlay_batch_device) return E_INVALIDARG;
        if(overlay_vertex_count+6u>OVERLAY_MAX_RECTS*6u) return E_OUTOFMEMORY;
        memcpy(overlay_vertices+overlay_vertex_count,v,sizeof(v)); overlay_vertex_count+=6; return D3D_OK;
    }
    return IDirect3DDevice9_DrawPrimitiveUP(device,D3DPT_TRIANGLELIST,2,v,sizeof(v[0]));
}
static unsigned countdown_segments(int digit) {
    static const unsigned masks[10]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
    return digit>=0 && digit<=9 ? masks[digit] : 0;
}
static const unsigned char *font5x7(char ch);
static HRESULT draw_countdown_digit(IDirect3DDevice9 *device,int digit,float x,float y,float w,float h,float t,DWORD color,int outlined) {
    (void)t; (void)outlined;
    if(digit<0 || digit>9) return D3D_OK;
    const unsigned char *rows=font5x7((char)('0'+digit));
    if(!rows) return D3D_OK;
    float cell=fminf(w/5.0f,h/7.0f),glyph_width=cell*5.0f;
    float left=x+(w-glyph_width)*0.5f,stroke=fmaxf(1.0f,cell*0.14f),inset=cell*0.10f;
    for(unsigned row=0;row<7;row++) for(unsigned col=0;col<5;col++) if(rows[row]&(1u<<(4-col))) {
        float px=left+(float)col*cell,py=y+(float)row*cell;
        HRESULT hr=overlay_rect(device,px-stroke,py-stroke,px+cell+stroke,py+cell+stroke,D3DCOLOR_ARGB(255,0,0,0));
        if(SUCCEEDED(hr)) hr=overlay_rect(device,px+inset,py+inset,px+cell-inset,py+cell-inset,color);
        if(FAILED(hr)) return hr;
    }
    return D3D_OK;
}
/* Render the countdown after the game scene. The state block restores the game's
 * D3D state before the original EndScene handler runs. */
static int overlay_reported_state=-1;
static void draw_countdown_overlay(IDirect3DDevice9 *device) {
    if(!countdown_active || countdown_last<1 || countdown_last>10) { overlay_reported_second=-1; overlay_reported_state=0; return; }
    if(overlay_reported_second!=countdown_last) { overlay_reported_second=countdown_last; overlay_reported_state=0; }
    D3DVIEWPORT9 viewport; memset(&viewport,0,sizeof(viewport));
    HRESULT hr=IDirect3DDevice9_GetViewport(device,&viewport);
    if(FAILED(hr) || viewport.Width<160 || viewport.Height<120) {
        if(!(overlay_reported_state&2)) {
            log_event("COUNTDOWN_OVERLAY_FAILED","seconds=%d stage=viewport hr=%08lx width=%lu height=%lu",
                countdown_last,(unsigned long)hr,(unsigned long)viewport.Width,(unsigned long)viewport.Height);
            overlay_reported_state|=2;
        }
        return;
    }
    IDirect3DStateBlock9 *state=0;
    hr=IDirect3DDevice9_CreateStateBlock(device,D3DSBT_ALL,&state);
    if(SUCCEEDED(hr) && state) hr=IDirect3DStateBlock9_Capture(state);
    else if(SUCCEEDED(hr)) hr=E_FAIL;
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetVertexShader(device,0);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetPixelShader(device,0);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetTexture(device,0,0);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetFVF(device,D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ZENABLE,FALSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_LIGHTING,FALSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_CULLMODE,D3DCULL_NONE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ALPHATESTENABLE,FALSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ALPHABLENDENABLE,TRUE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    if(SUCCEEDED(hr)) overlay_batch_begin(device);
    float scale=fminf((float)viewport.Width*0.14f,108.0f);
    float width=scale,height=scale*1.45f,thickness=scale*0.19f;
    float center_x=(float)viewport.X+(float)viewport.Width*0.5f;
    float center_y=(float)viewport.Y+(float)viewport.Height*0.34f;
    int digits=countdown_last>=10?2:1;
    float gap=digits==2?thickness*1.3f:0;
    float total=(float)digits*width+gap;
    float left=center_x-total*0.5f,top=center_y-height*0.5f;
    if(SUCCEEDED(hr) && digits==2) hr=draw_countdown_digit(device,1,left,top,width,height,thickness,D3DCOLOR_ARGB(255,255,218,76),1);
    if(SUCCEEDED(hr)) hr=draw_countdown_digit(device,countdown_last%10,left+(digits==2?width+gap:0),top,width,height,thickness,D3DCOLOR_ARGB(255,255,218,76),1);
    if(SUCCEEDED(hr)) hr=overlay_batch_flush(device); else overlay_batch_cancel();
    HRESULT restore=state?IDirect3DStateBlock9_Apply(state):E_FAIL;
    if(state) IDirect3DStateBlock9_Release(state);
    if(SUCCEEDED(hr) && SUCCEEDED(restore)) {
        if(!(overlay_reported_state&1)) {
            log_event("COUNTDOWN_OVERLAY_DRAWN","seconds=%d method=d3d9_batched_5x7 viewport=%lux%lu center=%.1f,%.1f digit_count=%d digit=%.1fx%.1f style=yellow_pixel_glyph_black_outline_no_backplate state_restored=1",
                countdown_last,(unsigned long)viewport.Width,(unsigned long)viewport.Height,center_x,center_y,digits,width,height);
            overlay_reported_state|=1;
        }
    } else if(!(overlay_reported_state&4)) {
        log_event("COUNTDOWN_OVERLAY_FAILED","seconds=%d stage=draw_or_restore hr=%08lx restore_hr=%08lx state_restored=%d",
            countdown_last,(unsigned long)hr,(unsigned long)restore,state?1:0);
        overlay_reported_state|=4;
    }
}
static const unsigned char *font5x7(char ch) {
    static const unsigned char glyphs[36][7]={
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
        {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
        {14,17,17,15,1,1,14}
    };
    if(ch>='A' && ch<='Z') return glyphs[(unsigned)(ch-'A')];
    if(ch>='0' && ch<='9') return glyphs[26u+(unsigned)(ch-'0')];
    return 0;
}
static HRESULT draw_pixel_text(IDirect3DDevice9 *device,const char *text,float x,float y,float height,DWORD color) {
    if(!text || height<7.0f) return D3D_OK;
    float cell=height/7.0f,step=cell*6.0f,stroke=fmaxf(1.0f,cell*0.20f);
    for(size_t i=0;text[i];i++) {
        char ch=text[i];
        if(ch==' ') continue;
        float left=x+(float)i*step;
        if(ch=='.' || ch=='+') {
            HRESULT hr=overlay_rect(device,left+cell*3.2f,y+cell*5.8f,left+cell*4.5f,y+cell*6.8f,D3DCOLOR_ARGB(255,0,0,0));
            if(FAILED(hr)) return hr;
            if(ch=='.') hr=overlay_rect(device,left+cell*3.4f,y+cell*6.0f,left+cell*4.3f,y+cell*6.7f,color);
            else {
                if(SUCCEEDED(hr)) hr=overlay_rect(device,left+cell*0.8f,y+cell*2.8f,left+cell*4.8f,y+cell*4.2f,D3DCOLOR_ARGB(255,0,0,0));
                if(SUCCEEDED(hr)) hr=overlay_rect(device,left+cell*2.2f,y+cell*1.5f,left+cell*3.5f,y+cell*5.5f,D3DCOLOR_ARGB(255,0,0,0));
                if(SUCCEEDED(hr)) hr=overlay_rect(device,left+cell*1.0f,y+cell*3.0f,left+cell*4.6f,y+cell*4.0f,color);
                if(SUCCEEDED(hr)) hr=overlay_rect(device,left+cell*2.4f,y+cell*1.7f,left+cell*3.3f,y+cell*5.3f,color);
            }
            if(FAILED(hr)) return hr;
            continue;
        }
        if(ch=='-') {
            HRESULT hr=overlay_rect(device,left+cell*0.8f,y+cell*2.8f,left+cell*4.8f,y+cell*4.2f,D3DCOLOR_ARGB(255,0,0,0));
            if(SUCCEEDED(hr)) hr=overlay_rect(device,left+cell,y+cell*3.0f,left+cell*4.6f,y+cell*4.0f,color);
            if(FAILED(hr)) return hr;
            continue;
        }
        const unsigned char *rows=font5x7(ch); if(!rows) continue;
        for(unsigned row=0;row<7;row++) for(unsigned col=0;col<5;col++) if(rows[row]&(1u<<(4-col))) {
            float px=left+(float)col*cell,py=y+(float)row*cell;
            HRESULT hr=overlay_rect(device,px-stroke,py-stroke,px+cell+stroke,py+cell+stroke,D3DCOLOR_ARGB(255,0,0,0));
            if(SUCCEEDED(hr)) hr=overlay_rect(device,px,py,px+cell*0.82f,py+cell*0.82f,color);
            if(FAILED(hr)) return hr;
        }
    }
    return D3D_OK;
}
static void menu_binding_overlay_text(int target,char *out,size_t size) {
    int kind=target==1?save_input_kind:(target==2?load_input_kind:jump_back_input_kind);
    int vk=target==1?vk_save:(target==2?vk_load:vk_jump_back);
    int button=target==1?save_joy_button:(target==2?load_joy_button:jump_back_joy_button);
    if(menu_binding_target==target) snprintf(out,size,"WAIT FOR INPUT");
    else if(kind==BIND_CONTROLLER) snprintf(out,size,"PAD BUTTON %d",button+1);
    else if(vk==0) snprintf(out,size,"UNBOUND");
    else if(vk==VK_RETURN) snprintf(out,size,"ENTER");
    else if(vk==VK_SPACE) snprintf(out,size,"SPACE");
    else if(vk==VK_ESCAPE) snprintf(out,size,"ESC");
    else if(vk>=VK_F1 && vk<=VK_F24) snprintf(out,size,"F%d",vk-VK_F1+1);
    else snprintf(out,size,"KEY %02X",vk);
}
static void draw_plugin_menu_overlay(IDirect3DDevice9 *device) {
    DWORD now=GetTickCount();
    if(!plugin_menu_draw_tick || (DWORD)(now-plugin_menu_draw_tick)>250u) { menu_overlay_reported=-1; return; }
    D3DVIEWPORT9 viewport; memset(&viewport,0,sizeof(viewport));
    HRESULT hr=IDirect3DDevice9_GetViewport(device,&viewport);
    if(FAILED(hr) || viewport.Width<160 || viewport.Height<120) return;
    char lines[5][80],save[32],load[32],jump[32];
    menu_binding_overlay_text(1,save,sizeof(save)); menu_binding_overlay_text(2,load,sizeof(load)); menu_binding_overlay_text(3,jump,sizeof(jump));
    snprintf(lines[0],sizeof(lines[0]),"LUKYANUK TRAINING %s",DISPLAY_VERSION);
    snprintf(lines[1],sizeof(lines[1]),"1 SAVE POSITION %s",save);
    snprintf(lines[2],sizeof(lines[2]),"2 RESTART RESET %s",load);
    snprintf(lines[3],sizeof(lines[3]),"3 JUMP BACK %s",jump);
    snprintf(lines[4],sizeof(lines[4]),"4 RESULTS DISPLAY %d SEC",results_display_seconds);
    float max_chars=0;
    for(unsigned i=0;i<5;i++) { float n=(float)strlen(lines[i]); if(n>max_chars) max_chars=n; }
    float margin_x=fmaxf(14.0f,(float)viewport.Width*0.025f);
    float margin_y=fmaxf(12.0f,(float)viewport.Height*0.025f);
    float text_height=fminf(30.0f,(float)viewport.Height*0.040f);
    float max_text_width=(float)viewport.Width-margin_x*2.0f;
    float max_width_height=max_text_width*7.0f/(max_chars*6.0f);
    if(text_height>max_width_height) text_height=max_width_height;
    if(text_height<8.0f) return;
    float cell=text_height/7.0f,content_width=max_chars*cell*6.0f;
    float panel_width=fminf((float)viewport.Width,fmaxf(content_width+margin_x*2.0f,(float)viewport.Width*0.65f));
    float row_step=text_height*1.42f;
    float panel_height=fminf((float)viewport.Height,margin_y*2.0f+row_step*5.0f+12.0f);
    float panel_left=(float)viewport.X+((float)viewport.Width-panel_width)*0.5f;
    float panel_top=(float)viewport.Y+((float)viewport.Height-panel_height)*0.5f;
    IDirect3DStateBlock9 *state=0;
    hr=IDirect3DDevice9_CreateStateBlock(device,D3DSBT_ALL,&state);
    if(SUCCEEDED(hr) && state) hr=IDirect3DStateBlock9_Capture(state);
    else if(SUCCEEDED(hr)) hr=E_FAIL;
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetVertexShader(device,0);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetPixelShader(device,0);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetTexture(device,0,0);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetFVF(device,D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ZENABLE,FALSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_LIGHTING,FALSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_CULLMODE,D3DCULL_NONE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ALPHATESTENABLE,FALSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ALPHABLENDENABLE,TRUE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    if(SUCCEEDED(hr)) overlay_batch_begin(device);
    DWORD black=D3DCOLOR_ARGB(255,0,0,0),white=D3DCOLOR_ARGB(255,255,255,255),select=D3DCOLOR_ARGB(255,255,218,76);
    if(SUCCEEDED(hr)) hr=overlay_rect(device,panel_left,panel_top,panel_left+panel_width,panel_top+panel_height,black);
    for(unsigned i=0;SUCCEEDED(hr) && i<5;i++) {
        float y=panel_top+margin_y+(float)i*row_step;
        if(i>=1 && i<=4 && menu_selection==(int)i-1) hr=overlay_rect(device,panel_left+margin_x*0.35f,y+text_height*0.20f,panel_left+margin_x*0.75f,y+text_height*0.80f,select);
        if(SUCCEEDED(hr)) hr=draw_pixel_text(device,lines[i],panel_left+margin_x,y,text_height,white);
    }
    if(SUCCEEDED(hr)) hr=overlay_batch_flush(device); else overlay_batch_cancel();
    HRESULT restore=state?IDirect3DStateBlock9_Apply(state):E_FAIL;
    if(state) IDirect3DStateBlock9_Release(state);
    if(SUCCEEDED(hr) && SUCCEEDED(restore)) {
        if(menu_overlay_reported!=1) { log_event("MENU_OVERLAY_DRAWN","method=d3d9_batched_black_panel_white_5x7 viewport=%lux%lu anchor=center state_restored=1",(unsigned long)viewport.Width,(unsigned long)viewport.Height); menu_overlay_reported=1; }
    } else if(menu_overlay_reported!=2) {
        log_event("MENU_OVERLAY_FAILED","hr=%08lx restore_hr=%08lx state_restored=%d",(unsigned long)hr,(unsigned long)restore,state?1:0); menu_overlay_reported=2;
    }
}
static int timer_result_display_visible(DWORD now) {
    return timer_result_display_active &&
      (DWORD)(now-timer_result_display_started_at)<(DWORD)results_display_seconds*1000u;
}
static void timer_expire_result_display(DWORD now) {
    if(timer_display_mode!=4 || !timer_result_display_active) return;
    DWORD elapsed=(DWORD)(now-timer_result_display_started_at);
    DWORD duration=(DWORD)results_display_seconds*1000u;
    if(elapsed<duration) return;
    timer_display_mode=0; timer_result_display_active=0; timer_overlay_reported=-1;
    log_event("TIMER_RESULT_DISPLAY_EXPIRED","duration_seconds=%d elapsed_ms=%lu",results_display_seconds,(unsigned long)elapsed);
}
/* Screen-space digital display. The glyphs have a black stroke and no panel. */
static void draw_timer_overlay(IDirect3DDevice9 *device) {
    DWORD now=GetTickCount();
    timer_expire_result_display(now);
    if(run_context_suspended) {
        if(!timer_overlay_suspended_reported) log_event("TIMER_OVERLAY_HIDDEN","reason=run_context_suspended mode=%d",timer_display_mode);
        timer_overlay_suspended_reported=1; timer_overlay_reported=-1; return;
    }
    if(timer_overlay_suspended_reported) {
        log_event("TIMER_OVERLAY_RESUMED","mode=%d",timer_display_mode);
        timer_overlay_suspended_reported=0; timer_overlay_reported=-1;
    }
    int saved_visible=position_saved_until && (LONG)(position_saved_until-now)>0;
    int mode=timer_display_mode;
    int timer_visible=mode==1 || mode==2 || mode==4;
    if((!timer_visible && !saved_visible) || countdown_active) { timer_overlay_reported=-1; return; }
    char text[32],best[48],last[48];
    if(!timer_visible) text[0]=0;
    else if(mode==4) snprintf(text,sizeof(text),"%.2f",fmaxf(0,timer_last_result_total));
    else if(mode==1) snprintf(text,sizeof(text),"%.2f",fmaxf(0,timer_display_value));
    else snprintf(text,sizeof(text),"%+.2f",timer_display_value);
    size_t length=strlen(text); if(timer_visible && (!length || length>=sizeof(text)-1)) return;
    D3DVIEWPORT9 viewport; memset(&viewport,0,sizeof(viewport));
    HRESULT hr=IDirect3DDevice9_GetViewport(device,&viewport);
    if(FAILED(hr) || viewport.Width<160 || viewport.Height<120) {
        if(timer_overlay_reported!=mode) {
            log_event("TIMER_OVERLAY_FAILED","mode=%d stage=viewport hr=%08lx width=%lu height=%lu",mode,(unsigned long)hr,(unsigned long)viewport.Width,(unsigned long)viewport.Height);
            timer_overlay_reported=mode;
        }
        return;
    }
    IDirect3DStateBlock9 *state=0;
    hr=IDirect3DDevice9_CreateStateBlock(device,D3DSBT_ALL,&state);
    if(SUCCEEDED(hr) && state) hr=IDirect3DStateBlock9_Capture(state);
    else if(SUCCEEDED(hr)) hr=E_FAIL;
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetVertexShader(device,0);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetPixelShader(device,0);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetTexture(device,0,0);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetFVF(device,D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ZENABLE,FALSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_LIGHTING,FALSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_CULLMODE,D3DCULL_NONE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ALPHATESTENABLE,FALSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ALPHABLENDENABLE,TRUE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    float height=fminf((float)viewport.Height*0.080f,84.0f);
    if(mode==4) {
        float combined_per_height=((float)length*6.0f+8.0f*0.58f*6.0f+2.0f)/7.0f;
        float combined_limit=(float)viewport.Width*0.44f;
        if(combined_per_height*height>combined_limit) height=combined_limit/combined_per_height;
    }
    float cell=height/7.0f,total=(float)length*cell*6.0f;
    if(total>(float)viewport.Width*0.86f) { height*=((float)viewport.Width*0.86f)/total; cell=height/7.0f; total=(float)length*cell*6.0f; }
    float left=mode==4?(float)viewport.X+(float)viewport.Width*0.05f:
      (float)viewport.X+((float)viewport.Width-total)*0.5f;
    float top=(float)viewport.Y+fmaxf(8.0f,(float)viewport.Height*0.012f);
    DWORD yellow=D3DCOLOR_ARGB(255,255,218,76);
    DWORD color=mode==1?yellow:
      (mode==4?D3DCOLOR_ARGB(255,238,244,255):
       (timer_display_value<-.005f?D3DCOLOR_ARGB(255,85,255,105):
        (timer_display_value>.005f?D3DCOLOR_ARGB(255,255,86,74):D3DCOLOR_ARGB(255,238,244,255))));
    if(SUCCEEDED(hr)) overlay_batch_begin(device);
    if(SUCCEEDED(hr) && timer_visible) hr=draw_pixel_text(device,text,left,top,height,color);
    if(SUCCEEDED(hr) && mode==4) {
        float details_x=(float)viewport.X+(float)viewport.Width*0.53f;
        float detail_height=height*0.55f;
        snprintf(best,sizeof(best),"BEST TIME %.2f",timer_best_total);
        if(timer_last_result_valid) snprintf(last,sizeof(last),"LAST %.2f",timer_last_result_total);
        else snprintf(last,sizeof(last),"LAST --");
        size_t detail_length=strlen(best)>strlen(last)?strlen(best):strlen(last);
        float detail_limit=(float)viewport.Width*0.42f;
        float detail_per_height=(float)detail_length*6.0f/7.0f;
        if(detail_per_height*detail_height>detail_limit) detail_height=detail_limit/detail_per_height;
        hr=draw_pixel_text(device,"FINISHED",left+total+cell*2.0f,top,height*0.58f,yellow);
        if(SUCCEEDED(hr)) hr=draw_pixel_text(device,best,details_x,top,detail_height,yellow);
        if(SUCCEEDED(hr)) hr=draw_pixel_text(device,last,details_x,top+detail_height*1.35f,detail_height,yellow);
    }
    if(SUCCEEDED(hr) && saved_visible) {
        const char *saved_text="POSITION SAVED";
        float saved_height=height*0.36f;
        float saved_limit=(float)viewport.Width*0.86f;
        float saved_width=(float)strlen(saved_text)*saved_height/7.0f*6.0f;
        if(saved_width>saved_limit) saved_height*=saved_limit/saved_width;
        float saved_width_final=(float)strlen(saved_text)*saved_height/7.0f*6.0f;
        float saved_left=(float)viewport.X+((float)viewport.Width-saved_width_final)*0.5f;
        float saved_top=top+height+fmaxf(4.0f,height*0.08f);
        hr=draw_pixel_text(device,saved_text,saved_left,saved_top,saved_height,yellow);
    }
    if(SUCCEEDED(hr)) hr=overlay_batch_flush(device); else overlay_batch_cancel();
    HRESULT restore=state?IDirect3DStateBlock9_Apply(state):E_FAIL;
    if(state) IDirect3DStateBlock9_Release(state);
    if(SUCCEEDED(hr) && SUCCEEDED(restore)) {
        if(timer_overlay_reported!=mode) {
            log_event("TIMER_OVERLAY_DRAWN","mode=%d saved_message=%d method=d3d9_outlined_5x7 position=top viewport=%lux%lu size_scale=2x state_restored=1",mode,saved_visible,(unsigned long)viewport.Width,(unsigned long)viewport.Height);
            timer_overlay_reported=mode;
        }
    } else if(timer_overlay_reported!=mode) {
        log_event("TIMER_OVERLAY_FAILED","mode=%d stage=draw_or_restore hr=%08lx restore_hr=%08lx state_restored=%d",mode,(unsigned long)hr,(unsigned long)restore,state?1:0);
        timer_overlay_reported=mode;
    }
}
static void transform_row_point(const D3DMATRIX *matrix,const float in[4],float out[4]) {
    for(unsigned col=0;col<4;col++) out[col]=in[0]*matrix->m[0][col]+in[1]*matrix->m[1][col]+in[2]*matrix->m[2][col]+in[3]*matrix->m[3][col];
}
static int project_finish_marker(IDirect3DDevice9 *device,const D3DVIEWPORT9 *viewport,float *screen_x,float *screen_y) {
    D3DMATRIX view,projection;
    if(FAILED(IDirect3DDevice9_GetTransform(device,D3DTS_VIEW,&view)) || FAILED(IDirect3DDevice9_GetTransform(device,D3DTS_PROJECTION,&projection))) return 0;
    float point[4]={timer_finish_world[0],timer_finish_world[1],timer_finish_world[2],1.0f},camera[4],clip[4];
    transform_row_point(&view,point,camera); transform_row_point(&projection,camera,clip);
    if(!isfinite(clip[3]) || clip[3]<=0.01f) return 0;
    float nx=clip[0]/clip[3],ny=clip[1]/clip[3],nz=clip[2]/clip[3];
    if(!isfinite(nx) || !isfinite(ny) || !isfinite(nz) || nx < -1.15f || nx>1.15f || ny < -1.15f || ny>1.15f || nz<0 || nz>1) return 0;
    *screen_x=(float)viewport->X+(nx+1.0f)*0.5f*(float)viewport->Width;
    *screen_y=(float)viewport->Y+(1.0f-ny)*0.5f*(float)viewport->Height;
    return isfinite(*screen_x) && isfinite(*screen_y);
}
static int finish_marker_fully_visible(float x,float y,const D3DVIEWPORT9 *viewport,float span) {
    const float label_width=108.0f; /* FINISH at 21 px in the 5x7 renderer. */
    return x-span-2.0f>=(float)viewport->X && x+8.0f+label_width<=(float)viewport->X+(float)viewport->Width &&
      y-32.0f>=(float)viewport->Y && y+3.0f<=(float)viewport->Y+(float)viewport->Height;
}
static void report_finish_marker_state(int state,const char *reason,float x,float y) {
    DWORD now=GetTickCount();
    if(finish_marker_candidate!=state) { finish_marker_candidate=state; finish_marker_candidate_at=now; return; }
    if((DWORD)(now-finish_marker_candidate_at)<250u || finish_marker_reported==state) return;
    if(state==1) log_event("FINISH_MARKER_DRAWN","method=projected_world_billboard x=%.1f y=%.1f depth_test=off world_xyz=%.4f,%.4f,%.4f state_restored=1",x,y,timer_finish_world[0],timer_finish_world[1],timer_finish_world[2]);
    else if(state==2) log_event("FINISH_MARKER_NOT_VISIBLE","reason=%s projected=%.1f,%.1f world_xyz=%.4f,%.4f,%.4f",reason?reason:"outside_clip",x,y,timer_finish_world[0],timer_finish_world[1],timer_finish_world[2]);
    else log_event("FINISH_MARKER_DRAW_FAILED","reason=%s world_xyz=%.4f,%.4f,%.4f",reason?reason:"d3d_error",timer_finish_world[0],timer_finish_world[1],timer_finish_world[2]);
    finish_marker_reported=state;
}
/* Experimental billboard pinned to the saved world-space finish point using
 * the game's current view and projection matrices. */
static void draw_finish_marker(IDirect3DDevice9 *device) {
    if(!timer_finish_set || !timer_finish_world_valid || countdown_active) return;
    D3DVIEWPORT9 viewport; memset(&viewport,0,sizeof(viewport));
    HRESULT hr=IDirect3DDevice9_GetViewport(device,&viewport);
    float x=0,y=0;
    if(FAILED(hr) || !project_finish_marker(device,&viewport,&x,&y)) {
        report_finish_marker_state(2,"outside_clip_or_projection_failed",x,y);
        return;
    }
    float span=fminf(48.0f,(float)viewport.Width*0.035f);
    if(!finish_marker_fully_visible(x,y,&viewport,span)) {
        report_finish_marker_state(2,"outside_viewport",x,y);
        return;
    }
    IDirect3DStateBlock9 *state=0;
    hr=IDirect3DDevice9_CreateStateBlock(device,D3DSBT_ALL,&state);
    if(SUCCEEDED(hr) && state) hr=IDirect3DStateBlock9_Capture(state);
    else if(SUCCEEDED(hr)) hr=E_FAIL;
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetVertexShader(device,0);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetPixelShader(device,0);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetTexture(device,0,0);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetFVF(device,D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ZENABLE,FALSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_LIGHTING,FALSE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_CULLMODE,D3DCULL_NONE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ALPHABLENDENABLE,TRUE);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    if(SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    DWORD black=D3DCOLOR_ARGB(255,0,0,0),yellow=D3DCOLOR_ARGB(255,255,218,76);
    if(SUCCEEDED(hr)) overlay_batch_begin(device);
    if(SUCCEEDED(hr)) hr=overlay_rect(device,x-span-2,y-3,x+span+2,y+3,black);
    if(SUCCEEDED(hr)) hr=overlay_rect(device,x-span,y-1,x+span,y+1,yellow);
    if(SUCCEEDED(hr)) hr=overlay_rect(device,x-3,y-25,x+3,y-4,black);
    if(SUCCEEDED(hr)) hr=overlay_rect(device,x-1,y-23,x+1,y-4,yellow);
    if(SUCCEEDED(hr)) hr=draw_pixel_text(device,"FINISH",x+8,y-32,21,yellow);
    if(SUCCEEDED(hr)) hr=overlay_batch_flush(device); else overlay_batch_cancel();
    HRESULT restore=state?IDirect3DStateBlock9_Apply(state):E_FAIL;
    if(state) IDirect3DStateBlock9_Release(state);
    if(SUCCEEDED(hr) && SUCCEEDED(restore)) {
        report_finish_marker_state(1,0,x,y);
    } else report_finish_marker_state(3,"d3d_error_or_state_restore",x,y);
}
static int read_mem(uintptr_t p,void *out,size_t n) {
    SIZE_T got=0;
    return p>=0x10000 && ReadProcessMemory(GetCurrentProcess(),(void*)p,out,n,&got) && got==n;
}
static int memory_hash(const void *data,ULONG size,char hex[65]) {
    BCRYPT_ALG_HANDLE alg=0; BCRYPT_HASH_HANDLE hash=0;
    unsigned char digest[32]; int ok=0;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,0,0)<0) goto done;
    if(BCryptCreateHash(alg,&hash,0,0,0,0,0)<0) goto done;
    if(BCryptHashData(hash,(PUCHAR)data,size,0)<0) goto done;
    if(BCryptFinishHash(hash,digest,sizeof(digest),0)<0) goto done;
    for(unsigned i=0;i<sizeof(digest);i++) sprintf(hex+2*i,"%02x",digest[i]);
    hex[64]=0; ok=1;
done:
    if(hash) BCryptDestroyHash(hash);
    if(alg) BCryptCloseAlgorithmProvider(alg,0);
    return ok;
}
static uintptr_t damage_buffer_address(void) {
    uintptr_t root=0,part=0;
    if(!read_mem(0x165fa90,&root,sizeof(root))) return 0;
    if(root<0x10000 || root>UINTPTR_MAX-0xc98) return 0;
    if(!read_mem(root+0xc98,&part,sizeof(part))) return 0;
    if(part<0x10000 || part>UINTPTR_MAX-0x10-DAMAGE_BUFFER_SIZE) return 0;
    return part+0x10;
}
/* Diagnostic only: read and hash the candidate RBR damage buffer; never writes it. */
static void log_damage_probe(const char *phase,int set_baseline) {
    unsigned char buffer[DAMAGE_BUFFER_SIZE];
    uintptr_t address=damage_buffer_address();
    char hash[65]="unavailable";
    int readable=address && read_mem(address,buffer,sizeof(buffer)) && memory_hash(buffer,sizeof(buffer),hash);
    int present=0,missing=0,flags[4]={0,0,0,0}; float check=NAN;
    if(readable) {
        memcpy(&present,buffer+0x3b20,sizeof(present));
        memcpy(&check,buffer+0x3aa0,sizeof(check));
        for(int i=0;i<4;i++) {
            memcpy(&flags[i],buffer+0x3a30+12*i,sizeof(flags[i]));
            if(!flags[i]) missing++;
        }
    }
    if(set_baseline) {
        saved_damage_valid=readable;
        saved_damage_address=readable?address:0;
        if(readable) memcpy(saved_damage_hash,hash,sizeof(saved_damage_hash));
        else saved_damage_hash[0]=0;
    }
    int same_address=!set_baseline && readable && saved_damage_valid && address==saved_damage_address;
    int same_hash=!set_baseline && readable && saved_damage_valid && !strcmp(hash,saved_damage_hash);
    log_event("DAMAGE_BUFFER_PROBE","phase=%s source=RBR_damage_handler_candidate readable=%d bytes=0x%lx address=%08lx sha256=%s baseline_valid=%d same_address=%d same_hash=%d present=%d check=%.6f flags=%d,%d,%d,%d missing_flags=%d raw_bytes_logged=0 restore_attempted=0",
        phase,readable,(unsigned long)DAMAGE_BUFFER_SIZE,(unsigned long)address,readable?hash:"unavailable",saved_damage_valid,same_address,same_hash,present,check,flags[0],flags[1],flags[2],flags[3],missing);
}
static int write_velocity(uintptr_t movement,const float velocity[3],float actual[3]) {
    uintptr_t p=movement+0x1c0;
    MEMORY_BASIC_INFORMATION region;
    if(!VirtualQuery((void*)p,&region,sizeof(region)) || region.State!=MEM_COMMIT || p+sizeof(float)*3>(uintptr_t)region.BaseAddress+region.RegionSize) return 0;
    DWORD protect=region.Protect&0xff;
    if(protect!=PAGE_READWRITE && protect!=PAGE_WRITECOPY && protect!=PAGE_EXECUTE_READWRITE && protect!=PAGE_EXECUTE_WRITECOPY) return 0;
    SIZE_T written=0;
    if(!WriteProcessMemory(GetCurrentProcess(),(void*)p,velocity,sizeof(float)*3,&written) || written!=sizeof(float)*3) return 0;
    return read_mem(p,actual,sizeof(float)*3) && !memcmp(actual,velocity,sizeof(float)*3);
}
static int write_gear(uintptr_t movement,int gear,int *actual) {
    uintptr_t p=movement+0x1100;
    if(!valid_gear(gear) || !actual) return 0;
    MEMORY_BASIC_INFORMATION region;
    if(!VirtualQuery((void*)p,&region,sizeof(region)) || region.State!=MEM_COMMIT || p+sizeof(int)>(uintptr_t)region.BaseAddress+region.RegionSize) return 0;
    DWORD protect=region.Protect&0xff;
    if(protect!=PAGE_READWRITE && protect!=PAGE_WRITECOPY && protect!=PAGE_EXECUTE_READWRITE && protect!=PAGE_EXECUTE_WRITECOPY) return 0;
    SIZE_T written=0;
    if(!WriteProcessMemory(GetCurrentProcess(),(void*)p,&gear,sizeof(gear),&written) || written!=sizeof(gear)) return 0;
    return read_mem(p,actual,sizeof(*actual)) && *actual==gear;
}
static int write_checked_int(uintptr_t p,int value,int *actual) {
    MEMORY_BASIC_INFORMATION region;
    if(!actual || p<0x10000 || p>UINTPTR_MAX-sizeof(value) || !VirtualQuery((void*)p,&region,sizeof(region)) || region.State!=MEM_COMMIT || (region.Protect&PAGE_GUARD) || (region.Protect&0xff)==PAGE_NOACCESS || p+sizeof(value)>(uintptr_t)region.BaseAddress+region.RegionSize) return 0;
    DWORD protect=region.Protect&0xff;
    if(protect!=PAGE_READWRITE && protect!=PAGE_WRITECOPY && protect!=PAGE_EXECUTE_READWRITE && protect!=PAGE_EXECUTE_WRITECOPY) return 0;
    SIZE_T written=0;
    if(!WriteProcessMemory(GetCurrentProcess(),(void*)p,&value,sizeof(value),&written) || written!=sizeof(value)) return 0;
    return read_mem(p,actual,sizeof(*actual)) && *actual==value;
}
static int read_clutch_help_state(uintptr_t *root_out,uintptr_t *wrapper_out,int *global_out,int *status_out,int *value_out) {
    uintptr_t root=0;
    if(!root_out || !wrapper_out || !global_out || !status_out || !value_out || !read_mem(CLUTCH_HELP_ROOT_ADDRESS,&root,sizeof(root)) || root<0x10000 || root>UINTPTR_MAX-CLUTCH_HELP_WRAPPER_OFFSET-0x10) return 0;
    uintptr_t wrapper=root+CLUTCH_HELP_WRAPPER_OFFSET;
    int global=0,status=0,value=0;
    if(!read_mem(CLUTCH_HELP_VALUE_ADDRESS,&global,sizeof(global)) || !read_mem(wrapper+8,&status,sizeof(status)) || !read_mem(wrapper+0xC,&value,sizeof(value))) return 0;
    *root_out=root; *wrapper_out=wrapper; *global_out=global; *status_out=status; *value_out=value;
    return 1;
}
static void log_clutch_help_state(const char *phase) {
    uintptr_t root=0,wrapper=0; int global=0,status=0,value=0;
    int readable=read_clutch_help_state(&root,&wrapper,&global,&status,&value);
    log_event("CLUTCH_HELP_STATE","phase=%s readable=%d root=%08lx wrapper=%08lx global=%d status=%d value=%d",phase,readable,(unsigned long)root,(unsigned long)wrapper,global,status,value);
}
static void restore_clutch_help(const char *reason) {
    if(!clutch_help_temp_active) return;
    int global=0,status=0,value=0,global_ok=0,status_ok=0,value_ok=0;
    uintptr_t current_root=0,current_wrapper=0;
    if(!read_clutch_help_state(&current_root,&current_wrapper,&global,&status,&value)) {
        if(!clutch_help_restore_retry_at) log_event("CLUTCH_HELP_RESTORE_FAILED","reason=%s stage=state_unreadable",reason);
        clutch_help_restore_retry_at=GetTickCount(); return;
    }
    if(current_root!=clutch_help_saved_root || current_wrapper!=clutch_help_saved_wrapper) {
        log_event("CLUTCH_HELP_RESTORE_CONTEXT_CHANGED","reason=%s saved_root=%08lx current_root=%08lx saved_wrapper=%08lx current_wrapper=%08lx no_write=1",reason,(unsigned long)clutch_help_saved_root,(unsigned long)current_root,(unsigned long)clutch_help_saved_wrapper,(unsigned long)current_wrapper);
        clutch_help_temp_active=0; clutch_help_restore_retry_at=0; return;
    }
    global_ok=(global==clutch_help_saved_global) || (global==1 && write_checked_int(CLUTCH_HELP_VALUE_ADDRESS,clutch_help_saved_global,&global));
    status_ok=(status==clutch_help_saved_status) || (status==1 && write_checked_int(clutch_help_saved_wrapper+8,clutch_help_saved_status,&status));
    value_ok=(value==clutch_help_saved_value) || (value==1 && write_checked_int(clutch_help_saved_wrapper+0xC,clutch_help_saved_value,&value));
    int complete=global_ok && status_ok && value_ok;
    if(complete) {
        log_event("CLUTCH_HELP_TEMP_RESTORE","reason=%s root_match=1 global=%d status=%d value=%d restored_global=%d restored_status=%d restored_value=%d",reason,global,status,value,clutch_help_saved_global,clutch_help_saved_status,clutch_help_saved_value);
        clutch_help_temp_active=0; clutch_help_restore_retry_at=0;
    } else {
        if(!clutch_help_restore_retry_at) log_event("CLUTCH_HELP_RESTORE_FAILED","reason=%s root_match=1 global_ok=%d status_ok=%d value_ok=%d global_now=%d status_now=%d value_now=%d",reason,global_ok,status_ok,value_ok,global,status,value);
        clutch_help_restore_retry_at=GetTickCount();
    }
}
static void enable_clutch_help_for_countdown(void) {
    if(!clutch_help_during_countdown) { log_event("CLUTCH_HELP_TEMP_SKIP","reason=config_disabled"); return; }
    if(!executable_ok) { log_event("CLUTCH_HELP_TEMP_SKIP","reason=unsupported_executable"); return; }
    if(clutch_help_temp_active) { log_event("CLUTCH_HELP_TEMP_SKIP","reason=previous_restore_pending"); return; }
    uintptr_t root=0,wrapper=0; int global=0,status=0,value=0;
    if(!read_clutch_help_state(&root,&wrapper,&global,&status,&value)) {
        log_event("CLUTCH_HELP_TEMP_SKIP","reason=state_unreadable"); return;
    }
    if((global!=0 && global!=1) || (status!=0 && status!=1) || (value!=0 && value!=1)) {
        log_event("CLUTCH_HELP_TEMP_SKIP","reason=unexpected_value root=%08lx wrapper=%08lx global=%d status=%d value=%d",(unsigned long)root,(unsigned long)wrapper,global,status,value); return;
    }
    clutch_help_saved_root=root; clutch_help_saved_wrapper=wrapper;
    clutch_help_saved_global=global; clutch_help_saved_status=status; clutch_help_saved_value=value;
    clutch_help_temp_active=1;
    int actual_global=global,actual_status=status,actual_value=value;
    int global_ok=global==1 || write_checked_int(CLUTCH_HELP_VALUE_ADDRESS,1,&actual_global);
    int status_ok=status==1 || write_checked_int(wrapper+8,1,&actual_status);
    int value_ok=value==1 || write_checked_int(wrapper+0xC,1,&actual_value);
    uintptr_t check_root=0,check_wrapper=0; int check_global=0,check_status=0,check_value=0;
    int readback=read_clutch_help_state(&check_root,&check_wrapper,&check_global,&check_status,&check_value);
    int verified=global_ok && status_ok && value_ok && readback && check_root==root && check_wrapper==wrapper && check_global==1 && check_status==1 && check_value==1;
    log_event(verified?"CLUTCH_HELP_TEMP_ENABLE":"CLUTCH_HELP_TEMP_ENABLE_FAILED","root=%08lx wrapper=%08lx original_global=%d original_status=%d original_value=%d write_global=%d write_status=%d write_value=%d readback=%d actual_global=%d actual_status=%d actual_value=%d",(unsigned long)root,(unsigned long)wrapper,global,status,value,global_ok,status_ok,value_ok,readback,check_global,check_status,check_value);
    if(!verified) restore_clutch_help("enable_failed");
}
static int read_int(uintptr_t p,int *out);
static int send_shift_key(int key,int up) {
    INPUT input; memset(&input,0,sizeof(input));
    input.type=INPUT_KEYBOARD; input.ki.wVk=(WORD)key;
    input.ki.dwFlags=up?KEYEVENTF_KEYUP:0;
    return SendInput(1,&input,sizeof(input))==1;
}
static void sequential_shift_abort(const char *reason) {
    int actual=-1, restored=write_gear(seq_movement,seq_saved_gear,&actual);
    log_event("SEQ_SHIFT_ABORT","reason=%s saved_gear_raw=%d stage_gear_raw=%d actual_gear_raw=%d fallback_write=%d",
      reason,seq_saved_gear,seq_stage_gear,actual,restored);
    seq_state=0; seq_key_down=0; seq_check_at=0; gear_restored=restored && actual==seq_saved_gear;
}
static void release_sequential_shift_key(void) {
    DWORD now=GetTickCount();
    if(seq_state==2 && seq_key_down && (LONG)(now-seq_release_at)>=0) {
        if(!send_shift_key(vk_seq_up,1)) {
            log_event("SEQ_UP_KEYUP","sendinput_accepted=0 key=%d winerror=%lu retry=1",vk_seq_up,GetLastError()); return;
        }
        seq_key_down=0; seq_keyup_ok=1; seq_state=3; seq_check_at=now+150;
        log_event("SEQ_UP_KEYUP","sendinput_accepted=1 key=%d input=SendInput verify_delay_ms=150",vk_seq_up);
    }
}
static void service_sequential_shift(const Sample *sample,int sample_valid) {
    DWORD now=GetTickCount(); int actual=-1;
    if(seq_state==1) {
        if(!sample_valid || !sample->active || !sample->foreground || !mark.valid || !same_context(sample,&mark.sample)) {
            log_event("SEQ_SHIFT_ABORT","reason=context_lost_before_input saved_gear_raw=%d staged_gear_raw=%d no_stale_write=1",seq_saved_gear,seq_stage_gear);
            seq_state=0; gear_restored=0; return;
        }
        if(!read_int(seq_movement+0x1100,&actual) || actual!=seq_stage_gear) {
            sequential_shift_abort("staged_gear_not_stable_before_input"); return;
        }
        if(!send_shift_key(vk_seq_up,0)) {
            log_event("SEQ_UP_KEYDOWN","sendinput_accepted=0 key=%d winerror=%lu",vk_seq_up,GetLastError());
            sequential_shift_abort("SendInput_keydown_failed"); return;
        }
        seq_key_down=1; seq_state=2; seq_release_at=now+80;
        log_event("SEQ_UP_KEYDOWN","sendinput_accepted=1 key=%d stage_gear_raw=%d target_gear_raw=%d hold_ms=80 input=SendInput",
          vk_seq_up,seq_stage_gear,seq_target_gear);
    } else if(seq_state==3 && (LONG)(now-seq_check_at)>=0) {
        if(!sample_valid || !sample->active || !sample->foreground || !mark.valid || !same_context(sample,&mark.sample)) {
            log_event("SEQ_UP_RESULT","input_sent=1 context_valid=0 saved_gear_raw=%d target_gear_raw=%d engagement_status=not_directly_observable",seq_saved_gear,seq_target_gear);
            seq_state=0; gear_restored=0; return;
        }
        int readable=read_int(seq_movement+0x1100,&actual);
        gear_restored=readable && seq_keyup_ok && actual==seq_target_gear;
        log_event("SEQ_UP_RESULT","input_sent=1 keyup_ok=%d readable=%d saved_gear_raw=%d staged_gear_raw=%d target_gear_raw=%d observed_gear_raw=%d gear_field_match=%d engagement_status=not_directly_observable",
          seq_keyup_ok,readable,seq_saved_gear,seq_stage_gear,seq_target_gear,actual,gear_restored);
        if(!gear_restored) {
            int fallback=-1; int fallback_ok=write_gear(seq_movement,seq_saved_gear,&fallback);
            gear_restored=fallback_ok && fallback==seq_saved_gear;
            log_event("SEQ_GEAR_FALLBACK","write_saved_raw=%d success=%d actual_gear_raw=%d gear_field_match=%d engagement_status=not_directly_observable",
              seq_saved_gear,fallback_ok,fallback,gear_restored);
        }
        seq_state=0; seq_check_at=0;
    }
}
static void begin_saved_gear_restore(const Sample *s) {
    if(!s || gear_attempted) return;
    gear_attempted=1; gear_neutral_active=0;
    int actual=-1;
    gear_restored=write_gear(s->movement,mark.sample.gear,&actual);
    log_event("GEAR_DIRECT_FIELD_WRITE","phase=countdown_end success=%d address=%08lx saved_gear_raw=%d actual_gear_raw=%d engagement_status=not_directly_observable",gear_restored,(unsigned long)(s->movement+0x1100),mark.sample.gear,actual);
}
static uintptr_t pointer_at(uintptr_t p) { uintptr_t v=0; read_mem(p,&v,sizeof(v)); return v; }
static int read_int(uintptr_t p,int *out) { return read_mem(p,out,4); }
static int read_float(uintptr_t p,float *out) { return read_mem(p,out,4); }
static void snapshot_log(const char *ev,const Sample *s) {
    log_event(ev,"run=%u mode=%d active=%d focus=%d info=%08lx movement=%08lx line=%08lx nodes=%u distance=%.5f progress=%.5f speed_raw=%.5f velocity_norm=%.5f velocity_xyz=%.5f,%.5f,%.5f gear_raw=%d gear_valid=%d time=%.5f countdown=%.5f state138=%d drivable=%d xyz=%.5f,%.5f,%.5f xyz_source=movement_140 quaternion=%.6f,%.6f,%.6f,%.6f q_valid=%d",
      s->generation,s->mode,s->active,s->foreground,(unsigned long)s->car,(unsigned long)s->movement,(unsigned long)s->line,s->count,
      s->distance,s->progress,s->speed,s->vector_speed,s->velocity[0],s->velocity[1],s->velocity[2],s->gear,s->gear_valid,s->time,s->countdown,s->state,s->drivable,s->x,s->y,s->z,s->q[0],s->q[1],s->q[2],s->q[3],quaternion_valid(s->q));
}
static int file_hash(const wchar_t *path,char hex[65]) {
    BCRYPT_ALG_HANDLE alg=0; BCRYPT_HASH_HANDLE h=0; HANDLE f=INVALID_HANDLE_VALUE;
    unsigned char digest[32], buffer[32768]; DWORD n=0; int ok=0;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,0,0)<0) goto done;
    if(BCryptCreateHash(alg,&h,0,0,0,0,0)<0) goto done;
    f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,0,OPEN_EXISTING,0,0);
    if(f==INVALID_HANDLE_VALUE) goto done;
    for(;;) { if(!ReadFile(f,buffer,sizeof(buffer),&n,0)) goto done; if(!n) break; if(BCryptHashData(h,buffer,n,0)<0) goto done; }
    if(BCryptFinishHash(h,digest,32,0)<0) goto done;
    for(unsigned i=0;i<32;i++) sprintf(hex+2*i,"%02x",digest[i]); hex[64]=0; ok=1;
done:
    if(f!=INVALID_HANDLE_VALUE) CloseHandle(f); if(h) BCryptDestroyHash(h); if(alg) BCryptCloseAlgorithmProvider(alg,0); return ok;
}
static int hash_module(HMODULE module,const char *expected,const char *name) {
    wchar_t path[1024]; char hash[65]="unavailable";
    int ok=GetModuleFileNameW(module,path,1024) && file_hash(path,hash) && !strcmp(hash,expected);
    log_event("BINARY_CHECK","module=%s sha256=%s match=%d base=%p",name,hash,ok,module); return ok;
}
#define RBR_CONTROLS_IGNITION_SHA256 "ac6c1858c4a7abf22fa30df0d9e943c37b3e68caa7b1a86ce8c09547a78918dc"
#define RBR_CONTROLS_IGNITION_RVA 0x2e4c0u
#define RBR_CONTROLS_IGNITION_ACTION 6
#define RBR_CONTROLS_IGNITION_HOLD_MS 650u
typedef int (__stdcall *RbrControlsActionFn)(int action,int state);
static RbrControlsActionFn rbr_controls_action;
static int rbr_ignition_release_attempts;
static void check_rbrcontrols_ignition_bridge(void) {
    if(rbr_ignition_bridge_checked) return;
    rbr_ignition_bridge_checked=1;
    HMODULE module=GetModuleHandleW(L"RBRControls.dll");
    if(!module) { log_event("RBR_CONTROLS_IGNITION_BRIDGE","available=0 reason=module_missing"); return; }
    if(!hash_module(module,RBR_CONTROLS_IGNITION_SHA256,"RBRControls")) {
        log_event("RBR_CONTROLS_IGNITION_BRIDGE","available=0 reason=hash_mismatch expected_sha256=%s",RBR_CONTROLS_IGNITION_SHA256); return;
    }
    uintptr_t address=(uintptr_t)module+RBR_CONTROLS_IGNITION_RVA;
    MEMORY_BASIC_INFORMATION region; memset(&region,0,sizeof(region)); unsigned char signature[5];
    const unsigned char expected[5]={0x83,0xec,0x14,0x83,0x3d};
    DWORD protection=0;
    if(VirtualQuery((const void*)address,&region,sizeof(region))) protection=region.Protect&0xffu;
    if(!region.BaseAddress || region.State!=MEM_COMMIT ||
       (protection!=PAGE_EXECUTE && protection!=PAGE_EXECUTE_READ && protection!=PAGE_EXECUTE_READWRITE && protection!=PAGE_EXECUTE_WRITECOPY) ||
       !read_mem(address,signature,sizeof(signature)) || memcmp(signature,expected,sizeof(expected))) {
        log_event("RBR_CONTROLS_IGNITION_BRIDGE","available=0 reason=code_signature_mismatch address=%08lx",(unsigned long)address); return;
    }
    rbr_controls_action=(RbrControlsActionFn)address;
    rbr_ignition_bridge_available=1;
    log_event("RBR_CONTROLS_IGNITION_BRIDGE","available=1 rva=%08lx action=%d call_convention=stdcall verified_sha256=1 signature=1 hold_ms=%lu",
      (unsigned long)RBR_CONTROLS_IGNITION_RVA,RBR_CONTROLS_IGNITION_ACTION,(unsigned long)RBR_CONTROLS_IGNITION_HOLD_MS);
}
static int call_rbrcontrols_ignition(int state,const char *phase) {
    check_rbrcontrols_ignition_bridge();
    if(!rbr_ignition_bridge_available || !rbr_controls_action) return 0;
    int route_result=rbr_controls_action(RBR_CONTROLS_IGNITION_ACTION,state);
    log_event("RBR_CONTROLS_IGNITION_STATE","phase=%s action=%d state=%d route_result=%d rva=%08lx",
      phase,RBR_CONTROLS_IGNITION_ACTION,state,route_result,(unsigned long)RBR_CONTROLS_IGNITION_RVA);
    return route_result!=0;
}
static void begin_rbrcontrols_ignition_hold(void) {
    if(rbr_ignition_hold_active) { log_event("RBR_CONTROLS_IGNITION_SKIP","reason=hold_already_active"); return; }
    check_rbrcontrols_ignition_bridge();
    if(!rbr_ignition_bridge_available) { log_event("RBR_CONTROLS_IGNITION_SKIP","reason=bridge_unavailable hash_gated=1"); return; }
    if(call_rbrcontrols_ignition(1,"countdown_begin")) {
        rbr_ignition_hold_active=1; rbr_ignition_release_at=GetTickCount()+RBR_CONTROLS_IGNITION_HOLD_MS; rbr_ignition_release_attempts=0;
        log_event("RBR_CONTROLS_IGNITION_HOLD_BEGIN","hold_ms=%lu timing=countdown_frozen clutch_help_active=%d route_result=1",
          (unsigned long)RBR_CONTROLS_IGNITION_HOLD_MS,clutch_help_temp_active);
    } else log_event("RBR_CONTROLS_IGNITION_HOLD_FAILED","phase=countdown_begin reason=internal_route_returned_zero");
}
static void release_rbrcontrols_ignition(const char *reason) {
    if(!rbr_ignition_hold_active) return;
    if(call_rbrcontrols_ignition(0,reason)) {
        rbr_ignition_hold_active=0; rbr_ignition_release_at=0; rbr_ignition_release_attempts=0;
        log_event("RBR_CONTROLS_IGNITION_HOLD_END","reason=%s release_route_result=1",reason);
    } else {
        rbr_ignition_release_attempts++;
        rbr_ignition_release_at=GetTickCount()+250u;
        log_event("RBR_CONTROLS_IGNITION_RELEASE_RETRY","reason=%s internal_route_zero_attempt=%d retry_ms=250",reason,rbr_ignition_release_attempts);
    }
}
static void service_rbrcontrols_ignition(DWORD now) {
    if(rbr_ignition_hold_active && (LONG)(now-rbr_ignition_release_at)>=0) release_rbrcontrols_ignition("hold_elapsed");
}static int guid_is_zero(const GUID *guid) {
    static const GUID zero={0}; return guid_equal(guid,&zero);
}
static void profile_write_int(const wchar_t *section,const wchar_t *key,int value) {
    wchar_t text[32]; swprintf(text,32,L"%d",value);
    if(!WritePrivateProfileStringW(section,key,text,ini_path)) log_event("CONFIG_WRITE_FAILED","section=%ls key=%ls winerror=%lu",section,key,GetLastError());
}
static const char *binding_action_name(int target) { return target==1?"save":(target==2?"load":"jump_back"); }
static int action_key_conflict(int target,int vk) {
    if(vk==vk_reload) return 1;
    if(target!=1 && save_input_kind==BIND_KEYBOARD && vk==vk_save) return 1;
    if(target!=2 && load_input_kind==BIND_KEYBOARD && vk==vk_load) return 1;
    if(target!=3 && jump_back_input_kind==BIND_KEYBOARD && vk==vk_jump_back) return 1;
    return 0;
}
static void config_read(void);
static void save_binding_config(int target,int kind,int vk,const GUID *guid,int button) {
    const wchar_t *key=target==1?L"Save":(target==2?L"Load":L"JumpBack");
    const wchar_t *type_key=target==1?L"SaveType":(target==2?L"LoadType":L"JumpBackType");
    const wchar_t *guid_key=target==1?L"SaveJoyGuid":(target==2?L"LoadJoyGuid":L"JumpBackJoyGuid");
    const wchar_t *button_key=target==1?L"SaveJoyButton":(target==2?L"LoadJoyButton":L"JumpBackJoyButton");
    if(kind==BIND_KEYBOARD) {
        profile_write_int(L"Keys",key,vk);
        profile_write_int(L"Bindings",type_key,BIND_KEYBOARD);
        profile_write_int(L"Bindings",button_key,-1);
        WritePrivateProfileStringW(L"Bindings",guid_key,L"",ini_path);
    } else {
        wchar_t guid_text[33]; guid_to_text(guid,guid_text);
        profile_write_int(L"Keys",key,0);
        profile_write_int(L"Bindings",type_key,BIND_CONTROLLER);
        profile_write_int(L"Bindings",button_key,button);
        WritePrivateProfileStringW(L"Bindings",guid_key,guid_text,ini_path);
    }
    config_read();
    keys[target-1]=1;
    menu_binding_target=0; menu_binding_wait_release=0;
    const char *action=binding_action_name(target);
    log_event("BINDING_SAVED","action=%s kind=%s vk=%d controller_button=%d",action,kind==BIND_KEYBOARD?"keyboard":"controller",kind==BIND_KEYBOARD?vk:0,kind==BIND_CONTROLLER?button:-1);
    notify(target==1?"Save binding saved":(target==2?"Restart binding saved":"Jump Back binding saved"));
}
static void change_results_display_seconds(int delta) {
    int old=results_display_seconds, next=old+delta;
    if(next<RESULTS_DISPLAY_MIN_SECONDS) next=RESULTS_DISPLAY_MIN_SECONDS;
    if(next>RESULTS_DISPLAY_MAX_SECONDS) next=RESULTS_DISPLAY_MAX_SECONDS;
    if(next==old) return;
    results_display_seconds=next;
    profile_write_int(L"Display",L"ResultsSeconds",results_display_seconds);
    log_event("RESULTS_DISPLAY_CHANGED","previous_seconds=%d seconds=%d min=%d max=%d persisted=1",old,results_display_seconds,RESULTS_DISPLAY_MIN_SECONDS,RESULTS_DISPLAY_MAX_SECONDS);
    char text[64]; snprintf(text,sizeof(text),"Result display: %d sec",results_display_seconds); notify(text);
}
static void config_read(void) {
    int old_live=live_enabled;
    wchar_t save_guid_text[64]=L"",load_guid_text[64]=L"",jump_back_guid_text[64]=L"";
    vk_save=GetPrivateProfileIntW(L"Keys",L"Save",VK_RETURN,ini_path);
    vk_load=GetPrivateProfileIntW(L"Keys",L"Load",VK_SPACE,ini_path);
    vk_jump_back=GetPrivateProfileIntW(L"Keys",L"JumpBack",0,ini_path);
    vk_reload=GetPrivateProfileIntW(L"Keys",L"Reload",VK_F10,ini_path);
    save_input_kind=GetPrivateProfileIntW(L"Bindings",L"SaveType",BIND_KEYBOARD,ini_path);
    load_input_kind=GetPrivateProfileIntW(L"Bindings",L"LoadType",BIND_KEYBOARD,ini_path);
    jump_back_input_kind=GetPrivateProfileIntW(L"Bindings",L"JumpBackType",BIND_KEYBOARD,ini_path);
    save_joy_button=GetPrivateProfileIntW(L"Bindings",L"SaveJoyButton",-1,ini_path);
    load_joy_button=GetPrivateProfileIntW(L"Bindings",L"LoadJoyButton",-1,ini_path);
    jump_back_joy_button=GetPrivateProfileIntW(L"Bindings",L"JumpBackJoyButton",-1,ini_path);
    GetPrivateProfileStringW(L"Bindings",L"SaveJoyGuid",L"",save_guid_text,64,ini_path);
    GetPrivateProfileStringW(L"Bindings",L"LoadJoyGuid",L"",load_guid_text,64,ini_path);
    GetPrivateProfileStringW(L"Bindings",L"JumpBackJoyGuid",L"",jump_back_guid_text,64,ini_path);
    int save_guid_ok=parse_guid(save_guid_text,&save_joy_guid),load_guid_ok=parse_guid(load_guid_text,&load_joy_guid);
    int jump_back_guid_ok=parse_guid(jump_back_guid_text,&jump_back_joy_guid);
    if(save_input_kind==BIND_CONTROLLER && save_guid_ok && save_joy_button>=0 && save_joy_button<JOY_BUTTON_COUNT) vk_save=0;
    else save_input_kind=BIND_KEYBOARD;
    if(load_input_kind==BIND_CONTROLLER && load_guid_ok && load_joy_button>=0 && load_joy_button<JOY_BUTTON_COUNT) vk_load=0;
    else load_input_kind=BIND_KEYBOARD;
    if(jump_back_input_kind==BIND_CONTROLLER && jump_back_guid_ok && jump_back_joy_button>=0 && jump_back_joy_button<JOY_BUTTON_COUNT) vk_jump_back=0;
    else jump_back_input_kind=BIND_KEYBOARD;
    live_enabled=GetPrivateProfileIntW(L"Test",L"LiveLoad",0,ini_path)==1;
    restore_position=GetPrivateProfileIntW(L"Test",L"RestorePosition",1,ini_path)==1;
    restore_orientation=GetPrivateProfileIntW(L"Test",L"RestoreOrientation",1,ini_path)==1;
    restore_speed=GetPrivateProfileIntW(L"Test",L"RestoreSpeed",1,ini_path)==1;
    restore_gear=GetPrivateProfileIntW(L"Test",L"RestoreGear",1,ini_path)==1;
    clutch_help_during_countdown=GetPrivateProfileIntW(L"Test",L"ClutchHelpDuringCountdown",1,ini_path)==1;
    results_display_seconds=GetPrivateProfileIntW(L"Display",L"ResultsSeconds",RESULTS_DISPLAY_DEFAULT_SECONDS,ini_path);
    if(results_display_seconds<RESULTS_DISPLAY_MIN_SECONDS) results_display_seconds=RESULTS_DISPLAY_MIN_SECONDS;
    if(results_display_seconds>RESULTS_DISPLAY_MAX_SECONDS) results_display_seconds=RESULTS_DISPLAY_MAX_SECONDS;
    config_ok=valid_vk(vk_reload);
    if(save_input_kind==BIND_KEYBOARD && !valid_vk(vk_save)) config_ok=0;
    if(load_input_kind==BIND_KEYBOARD && !valid_vk(vk_load)) config_ok=0;
    if(jump_back_input_kind==BIND_KEYBOARD && vk_jump_back && !valid_vk(vk_jump_back)) config_ok=0;
    if(save_input_kind==BIND_KEYBOARD && load_input_kind==BIND_KEYBOARD && vk_save==vk_load) config_ok=0;
    if(save_input_kind==BIND_KEYBOARD && jump_back_input_kind==BIND_KEYBOARD && vk_save==vk_jump_back) config_ok=0;
    if(load_input_kind==BIND_KEYBOARD && jump_back_input_kind==BIND_KEYBOARD && vk_load==vk_jump_back) config_ok=0;
    if(save_input_kind==BIND_KEYBOARD && vk_save==vk_reload) config_ok=0;
    if(load_input_kind==BIND_KEYBOARD && vk_load==vk_reload) config_ok=0;
    if(jump_back_input_kind==BIND_KEYBOARD && vk_jump_back==vk_reload) config_ok=0;
    if(save_input_kind==BIND_CONTROLLER && (save_joy_button<0 || save_joy_button>=JOY_BUTTON_COUNT || guid_is_zero(&save_joy_guid))) config_ok=0;
    if(load_input_kind==BIND_CONTROLLER && (load_joy_button<0 || load_joy_button>=JOY_BUTTON_COUNT || guid_is_zero(&load_joy_guid))) config_ok=0;
    if(jump_back_input_kind==BIND_CONTROLLER && (jump_back_joy_button<0 || jump_back_joy_button>=JOY_BUTTON_COUNT || guid_is_zero(&jump_back_joy_guid))) config_ok=0;
    if(save_input_kind==BIND_CONTROLLER && load_input_kind==BIND_CONTROLLER && save_joy_button==load_joy_button && guid_equal(&save_joy_guid,&load_joy_guid)) config_ok=0;
    if(save_input_kind==BIND_CONTROLLER && jump_back_input_kind==BIND_CONTROLLER && save_joy_button==jump_back_joy_button && guid_equal(&save_joy_guid,&jump_back_joy_guid)) config_ok=0;
    if(load_input_kind==BIND_CONTROLLER && jump_back_input_kind==BIND_CONTROLLER && load_joy_button==jump_back_joy_button && guid_equal(&load_joy_guid,&jump_back_joy_guid)) config_ok=0;
    last_config=GetTickCount();
    int key_vks[]={vk_save,vk_load,vk_jump_back,vk_reload};
    for(int i=0;i<4;i++) keys[i]=key_vks[i]>0 && (GetAsyncKeyState(key_vks[i])&0x8000)!=0;
    log_event("CONFIG","valid=%d save_kind=%d save_vk=%d save_joy_button=%d load_kind=%d load_vk=%d load_joy_button=%d jump_back_kind=%d jump_back_vk=%d jump_back_joy_button=%d jump_back_spacing_m=%.0f reload=%d live=%d speed_limit=none restore_position=%d restore_orientation=%d restore_speed=%d restore_gear=%d resume_delay_seconds=%d sample_hz=30 gear_strategy=direct_field_write results_display_seconds=%d single_press=1 previous_live=%d",
      config_ok,save_input_kind,vk_save,save_joy_button,load_input_kind,vk_load,load_joy_button,jump_back_input_kind,vk_jump_back,jump_back_joy_button,JUMP_BACK_SPACING_METERS,vk_reload,live_enabled,restore_position,restore_orientation,restore_speed,restore_gear,RESUME_DELAY_SECONDS,results_display_seconds,old_live);
    notify(config_ok?(live_enabled?"LIVE: Load key returns once per press":"DRY RUN; Load will only log"):"Invalid settings or duplicate bindings; actions disabled");
}
static void begin_menu_binding(int target) {
    menu_binding_target=target; menu_binding_wait_release=1;
    for(unsigned i=0;i<joy_device_count;i++) memset(joy_devices[i].button_edges,0,sizeof(joy_devices[i].button_edges));
    log_event("BINDING_CAPTURE_BEGIN","action=%s wait_for_release=1 controller_input=deferred_until_next_frame",binding_action_name(target));
    notify("Release controls, then press a key or controller button");
}
static int keyboard_any_down(void) {
    for(int vk=8;vk<=254;vk++) if(GetAsyncKeyState(vk)&0x8000) return 1;
    return 0;
}
static void menu_binding_tick(void) {
    if(!menu_binding_target) return;
    if(menu_binding_wait_release) {
        if(!keyboard_any_down() && !any_controller_button_down()) {
            menu_binding_wait_release=0;
            log_event("BINDING_CAPTURE_ARMED","action=%s",binding_action_name(menu_binding_target));
            notify("Press a key or controller button");
        }
        return;
    }
    GUID guid; int button=-1;
    if(capture_controller_button(&guid,&button)) {
        int conflict=(menu_binding_target!=1 && save_input_kind==BIND_CONTROLLER && button==save_joy_button && guid_equal(&guid,&save_joy_guid)) ||
          (menu_binding_target!=2 && load_input_kind==BIND_CONTROLLER && button==load_joy_button && guid_equal(&guid,&load_joy_guid)) ||
          (menu_binding_target!=3 && jump_back_input_kind==BIND_CONTROLLER && button==jump_back_joy_button && guid_equal(&guid,&jump_back_joy_guid));
        if(conflict) { log_event("BINDING_CAPTURE_REJECTED","action=%s reason=already_assigned controller_button=%d",binding_action_name(menu_binding_target),button); menu_binding_wait_release=1; notify("That controller button is already assigned"); return; }
        save_binding_config(menu_binding_target,BIND_CONTROLLER,0,&guid,button); return;
    }
    for(int vk=8;vk<=254;vk++) if(GetAsyncKeyState(vk)&0x8000) {
        if(vk==VK_ESCAPE) { int target=menu_binding_target; menu_binding_target=0; menu_binding_wait_release=0; log_event("BINDING_CAPTURE_CANCEL","action=%s",binding_action_name(target)); notify("Button assignment cancelled"); return; }
        if(!valid_vk(vk) || action_key_conflict(menu_binding_target,vk)) {
            log_event("BINDING_CAPTURE_REJECTED","action=%s reason=reserved_key vk=%d",binding_action_name(menu_binding_target),vk);
            menu_binding_wait_release=1; notify("That key is reserved; press another"); return;
        }
        save_binding_config(menu_binding_target,BIND_KEYBOARD,vk,0,-1); return;
    }
}
static void menu_binding_text(int target,char *out,size_t size) {
    int kind=target==1?save_input_kind:(target==2?load_input_kind:jump_back_input_kind);
    int vk=target==1?vk_save:(target==2?vk_load:vk_jump_back);
    int button=target==1?save_joy_button:(target==2?load_joy_button:jump_back_joy_button);
    if(menu_binding_target==target) snprintf(out,size,"Press a key or controller button...");
    else if(kind==BIND_CONTROLLER) snprintf(out,size,"Controller button %d",button+1);
    else if(vk==0) snprintf(out,size,"Unassigned");
    else if(vk==VK_RETURN) snprintf(out,size,"ENTER");
    else if(vk==VK_SPACE) snprintf(out,size,"SPACE");
    else if(vk==VK_ESCAPE) snprintf(out,size,"ESC");
    else if(vk>=VK_F1 && vk<=VK_F24) snprintf(out,size,"F%d",vk-VK_F1+1);
    else snprintf(out,size,"VK %02X",vk);
}
static int geometry(uintptr_t line,unsigned count) {
    if(line==geometry_line && count==geometry_count) return 1;
    geometry_line=0; geometry_count=0;
    if(count<2 || count>10000) return 0;
    size_t bytes=(size_t)count*0x30; unsigned char *buf=(unsigned char*)malloc(bytes+0x30);
    if(!buf) return 0;
    int ok=read_mem(line,buf,bytes+4);
    for(unsigned i=0;ok && i<count;i++) {
        memcpy(&distances[i],buf+0x30+i*0x30,4);
        if(!isfinite(distances[i]) || fabsf(distances[i])>1000000 || (i && distances[i]<distances[i-1])) ok=0;
    }
    free(buf);
    if(ok && distances[count-1]>distances[0]+10) {
        geometry_line=line; geometry_count=count;
        log_event("GEOMETRY","line=%08lx nodes=%u first=%.4f last=%.4f",(unsigned long)line,count,distances[0],distances[count-1]); return 1;
    }
    return 0;
}
static int get_sample(Sample *s) {
    memset(s,0,sizeof(*s)); s->generation=generation; s->mode=-1; s->state=-1;
    DWORD process=0; GetWindowThreadProcessId(GetForegroundWindow(),&process); s->foreground=process==GetCurrentProcessId();
    uintptr_t config=pointer_at(0x7eac48), damage_root=pointer_at(0x165fa90);
    s->car=pointer_at(0x165fc68); s->movement=pointer_at(0x8ef660); s->line=pointer_at(0x1659184);
    if(!read_int(config+0x728,&s->mode) || !s->car || !s->movement || !s->line) return 0;
    unsigned char info[0x260]; float velocity[3],xyz[3]; int finished;
    if(!read_mem(s->car,info,sizeof(info)) || !read_mem(s->movement+0x100,s->q,sizeof(s->q)) || !read_mem(s->movement+0x1c0,velocity,sizeof(velocity)) || !read_mem(s->movement+0x140,xyz,sizeof(xyz)) || !read_mem(s->line+0x75310,&s->count,4)) return 0;
    s->gear_valid=read_int(s->movement+0x1100,&s->gear) && valid_gear(s->gear);
    memcpy(s->velocity,velocity,sizeof(s->velocity));
    memcpy(&s->speed,info+0xc,4); memcpy(&s->distance,info+0x20,4); memcpy(&s->state,info+0x138,4);
    memcpy(&s->progress,info+0x13c,4); memcpy(&s->time,info+0x140,4); memcpy(&finished,info+0x144,4); memcpy(&s->countdown,info+0x244,4);
    s->x=xyz[0]; s->y=xyz[1]; s->z=xyz[2]; s->vector_speed=sqrtf(velocity[0]*velocity[0]+velocity[1]*velocity[1]+velocity[2]*velocity[2]);
    s->active=s->mode==1 && finished==0 && s->time>0 && s->countdown<=0;
    uintptr_t part=pointer_at(damage_root+0xc98), damage=part?part+0x10:0;
    int present=0, count_missing=0, flag; float check=0;
    if(damage && read_int(damage+0x3b20,&present) && read_float(damage+0x3aa0,&check) && isfinite(check) && present && check!=0) {
        int ok=1;
        for(int i=0;i<4;i++) { if(!read_int(damage+0x3a30+12*i,&flag)) ok=0; else if(!flag) count_missing++; }
        s->drivable=ok && count_missing<=2;
    }
    return finite_sample(s) && geometry(s->line,s->count) && s->distance>=distances[0] && s->distance<=distances[s->count-1];
}
static void timer_disarm(const char *reason) {
    if(timer_armed) log_event("TIMER_DISARMED","reason=%s state=%d start=%.5f finish_set=%d finish=%.5f",reason,timer_state,timer_start_line,timer_finish_set,timer_finish_line);
    timer_armed=0; timer_state=TIMER_OFF; timer_previous_valid=0; timer_display_mode=0; position_saved_until=0;
    timer_run_count=0; timer_best_count=0; timer_finish_set=0; timer_reference_valid=0;
    timer_last_result_valid=0; timer_finish_world_valid=0; timer_result_display_active=0; timer_result_display_started_at=0;
}
static void timer_reset_for_new_session(const Sample *s) {
    DWORD now=GetTickCount();
    timer_armed=0; timer_state=TIMER_OFF; timer_finish_set=0; timer_reference_valid=0; timer_previous_valid=0;
    timer_start_line=s->distance+5.0f; timer_finish_line=0; timer_start_time=0; timer_last_record_time=0;
    timer_run_count=0; timer_best_count=0; timer_best_total=0; timer_last_result_delta=0;
    timer_last_result_total=0; timer_last_result_valid=0; timer_finish_world_valid=0; finish_marker_reported=0; finish_marker_candidate=0; finish_marker_candidate_at=now;
    timer_display_mode=0; timer_display_value=0; timer_display_updated=now; position_saved_until=now+3000u; timer_overlay_reported=-1;
    timer_result_display_active=0; timer_result_display_started_at=0; timer_regression_reported=0;
    pending_first_finish_valid=0;
    log_event("SESSION_RESET","run=%u previous_results_cleared=1 finish_line_cleared=1",generation);
    log_event("TIMER_WAIT_FINISH_POINT","saved_mark=%.5f timer_start_line=%.5f timer_armed=0",s->distance,timer_start_line);
    notify("Position saved; press Space at the finish point");
}
static int timer_queue_first_finish(const Sample *s) {
    if(!s || !mark.valid || !s->active || !same_context(s,&mark.sample)) {
        log_event("TIMER_FINISH_POINT_REJECTED","reason=invalid_or_different_context active=%d mark_valid=%d",s?s->active:0,mark.valid);
        notify("Finish point requires the saved stage context"); return 0;
    }
    if(!isfinite(s->distance) || s->distance<=timer_start_line+1.0f) {
        log_event("TIMER_FINISH_POINT_REJECTED","reason=too_close_to_start start=%.5f minimum_finish=%.5f candidate=%.5f",timer_start_line,timer_start_line+1.0f,s->distance);
        notify("Drive past the +5 m start, then press Space at the finish"); return 0;
    }
    pending_first_finish_sample=*s; pending_first_finish_valid=1;
    log_event("TIMER_FIRST_FINISH_PENDING","start=%.5f finish=%.5f segment=%.5f timer_armed=0 reset_pending=1",timer_start_line,s->distance,s->distance-timer_start_line);
    notify("Finish marked; first Space return starts the timer");
    return 1;
}
static int timer_arm_after_first_restart(void) {
    if(!pending_first_finish_valid || !mark.valid || !same_context_objects(&pending_first_finish_sample,&mark.sample)) return 0;
    DWORD now=GetTickCount();
    timer_armed=1; timer_state=TIMER_WAIT_START; timer_finish_set=1; timer_reference_valid=0; timer_previous_valid=0;
    timer_start_line=mark.sample.distance+5.0f; timer_finish_line=pending_first_finish_sample.distance;
    timer_start_time=0; timer_last_record_time=0; timer_run_count=0; timer_best_count=0; timer_best_total=0;
    timer_last_result_delta=0; timer_last_result_total=0; timer_last_result_valid=0; timer_regression_reported=0;
    timer_finish_world[0]=pending_first_finish_sample.x; timer_finish_world[1]=pending_first_finish_sample.y; timer_finish_world[2]=pending_first_finish_sample.z;
    timer_finish_world_valid=1; finish_marker_reported=0; finish_marker_candidate=0; finish_marker_candidate_at=now;
    timer_display_mode=0; timer_display_value=0; timer_display_updated=now; timer_overlay_reported=-1; position_saved_until=0;
    timer_result_display_active=0; timer_result_display_started_at=0;
    log_event("TIMER_FINISH_SET","source=first_restart start=%.5f finish=%.5f segment=%.5f",timer_start_line,timer_finish_line,timer_finish_line-timer_start_line);
    log_event("TIMER_ARMED","source=first_restart_confirmed start_line=%.5f finish_line=%.5f timer_starts_on_crossing=1 sample_hz=30",timer_start_line,timer_finish_line);
    pending_first_finish_valid=0;
    notify("First reset confirmed; timer starts at +5 m");
    return 1;
}
static void timer_prepare_trial(void) {
    if(!timer_armed) return;
    DWORD now=GetTickCount();
    timer_expire_result_display(now);

    timer_state=TIMER_WAIT_START; timer_previous_valid=0; timer_run_count=0;
    timer_start_time=0; timer_last_record_time=0; timer_display_mode=timer_last_result_valid && timer_result_display_visible(now)?4:0; timer_display_value=timer_last_result_total; timer_display_updated=now; position_saved_until=0; timer_overlay_reported=-1;
    log_event("TIMER_TRIAL_ARMED","start_line=%.5f finish_set=%d finish_line=%.5f reference_points=%u",timer_start_line,timer_finish_set,timer_finish_line,timer_best_count);
}
static void timer_set_display(int mode,float value,int force) {
    DWORD now=GetTickCount();
    if(!force && (DWORD)(now-timer_display_updated)<TIMER_DISPLAY_INTERVAL_MS) return;
    timer_display_mode=mode; timer_display_value=value; timer_display_updated=now;
    if(mode==4) { timer_result_display_started_at=now; timer_result_display_active=1; }
    else timer_result_display_active=0;
}
static int timer_record_sample(float distance,float race_time,const char *source,const char **failure_reason) {
    float elapsed=race_time-timer_start_time;
    if(failure_reason) *failure_reason=0;
    if(source && !strcmp(source,"periodic") && timer_run_count && timer_run_count<=TIMER_MAX_POINTS) {
        TimerPoint last=timer_run[timer_run_count-1];
        float distance_regression=last.distance-distance;
        float elapsed_regression=last.elapsed-elapsed;
        if((distance_regression>0.001f && distance_regression<=TIMER_BACKWARD_DISTANCE_JUMP_METERS) ||
           (elapsed_regression>0.001f && elapsed_regression<=TIMER_BACKWARD_TIME_JUMP_SECONDS)) {
            if(!timer_regression_reported)
                log_event("TIMER_SAMPLE_JITTER_ACCEPTED","distance_regression_m=%.5f time_regression_s=%.5f tolerance_m=%.2f tolerance_s=%.2f action=keep_timer_and_clamp_profile",distance_regression,elapsed_regression,TIMER_BACKWARD_DISTANCE_JUMP_METERS,TIMER_BACKWARD_TIME_JUMP_SECONDS);
            timer_regression_reported=1;
        } else if(distance_regression<=0.001f && elapsed_regression<=0.001f) timer_regression_reported=0;
    }
    unsigned old_count=timer_run_count;
    const char *reason=0;
    if(!timer_profile_append_detailed(timer_run,&timer_run_count,TIMER_MAX_POINTS,distance,elapsed,&reason)) {
        if(!reason) reason="append_rejected_after_precheck";
        float previous_distance=NAN,previous_elapsed=NAN;
        if(timer_run_count && timer_run_count<=TIMER_MAX_POINTS) {
            previous_distance=timer_run[timer_run_count-1].distance;
            previous_elapsed=timer_run[timer_run_count-1].elapsed;
        }
        log_event("TIMER_SAMPLE_REJECTED","source=%s reason=%s candidate_distance=%.5f candidate_race_time=%.5f start_time=%.5f candidate_elapsed=%.5f previous_distance=%.5f previous_elapsed=%.5f distance_delta=%.5f elapsed_delta=%.5f count=%u capacity=%u",
            source?source:"unknown",reason,distance,race_time,timer_start_time,elapsed,previous_distance,previous_elapsed,distance-previous_distance,elapsed-previous_elapsed,timer_run_count,TIMER_MAX_POINTS);
        if(failure_reason) *failure_reason=reason;
        return 0;
    }
    if(timer_run_count>old_count) {
        timer_regression_reported=0;
        timer_last_record_time=race_time;
        float reference=NAN,delta=NAN;
        if(timer_reference_valid && timer_profile_time_at(timer_best,timer_best_count,distance,&reference)) delta=elapsed-reference;
        log_event("TIMER_SAMPLE","distance=%.3f elapsed=%.3f reference_elapsed=%.3f delta=%.3f count=%u",distance,elapsed,reference,delta,timer_run_count);
    }
    return 1;
}
static void timer_finish_trial(float elapsed) {
    if(!timer_reference_valid) {
        if(timer_run_count<2) {
            log_event("TIMER_FINISH_REJECTED","reason=baseline_needs_two_points elapsed=%.3f points=%u",elapsed,timer_run_count);
            timer_state=TIMER_FINISHED; timer_last_result_total=elapsed; timer_last_result_valid=1; timer_set_display(4,elapsed,1); return;
        }
        unsigned copied=timer_run_count;
        if(copied>TIMER_MAX_POINTS) copied=TIMER_MAX_POINTS;
        memcpy(timer_best,timer_run,copied*sizeof(timer_best[0])); timer_best_count=copied;
        timer_best_total=elapsed; timer_reference_valid=1; timer_last_result_total=elapsed;
        timer_last_result_delta=0; timer_last_result_valid=1; timer_state=TIMER_FINISHED; timer_set_display(4,elapsed,1);
        log_event("TIMER_BEST_ESTABLISHED","first_timed_run=1 elapsed=%.3f finish=%.3f samples=%u",elapsed,timer_finish_line,timer_best_count);
        notify("Finished; first result saved as best"); return;
    }
    float reference=NAN;
    if(!timer_profile_time_at(timer_best,timer_best_count,timer_finish_line,&reference)) {
        log_event("TIMER_FINISH_REJECTED","reason=reference_unavailable elapsed=%.3f points=%u",elapsed,timer_best_count);
        timer_state=TIMER_FINISHED; timer_last_result_total=elapsed; timer_last_result_valid=1; timer_set_display(4,elapsed,1); return;
    }
    float delta=elapsed-reference;
    timer_last_result_delta=delta; timer_last_result_total=elapsed; timer_last_result_valid=1;
    timer_state=TIMER_FINISHED; timer_set_display(4,elapsed,1);
    log_event("TIMER_FINISH","finish=%.3f elapsed=%.3f previous_best=%.3f delta=%+.3f improved=%d samples=%u",timer_finish_line,elapsed,reference,delta,elapsed<timer_best_total,timer_run_count);
    if(elapsed<timer_best_total && timer_run_count>=2) {
        unsigned copied=timer_run_count;
        if(copied>TIMER_MAX_POINTS) copied=TIMER_MAX_POINTS;
        memcpy(timer_best,timer_run,copied*sizeof(timer_best[0])); timer_best_count=copied; timer_best_total=elapsed;
        log_event("TIMER_BEST_UPDATED","elapsed=%.3f finish=%.3f samples=%u improvement=%+.3f",timer_best_total,timer_finish_line,timer_best_count,delta);
        notify("Finished; new best time");
    } else notify(delta<-.005f?"Finished; faster than best":"Finished; result saved");
}
static void timer_step(const Sample *s,int valid,int suppressed) {
    if(!timer_armed) return;
    /* Keep the release baseline across the last frozen frame. Invalid samples
     * clear it; a deliberate freeze only pauses sampling. */
    if(suppressed) return;
    if(!valid || !s->active || !s->foreground || !same_context(s,&mark.sample)) {
        timer_previous_valid=0;
        return;
    }
    if(!timer_previous_valid) {
        timer_previous_distance=s->distance; timer_previous_race_time=s->time; timer_previous_valid=1;
        return;
    }
    float before_distance=timer_previous_distance,before_time=timer_previous_race_time;
    float distance_delta=s->distance-before_distance,time_delta=s->time-before_time;
    if(timer_sample_discontinuous(distance_delta,time_delta)) {
        log_event("TIMER_DISCONTINUITY","before_distance=%.3f distance=%.3f before_time=%.3f time=%.3f state=%d action=discard_attempt_and_rearm",before_distance,s->distance,before_time,s->time,timer_state);
        timer_state=TIMER_WAIT_START; timer_run_count=0; timer_display_mode=0;
        timer_previous_distance=s->distance; timer_previous_race_time=s->time; timer_previous_valid=1;
        return;
    }
    if(timer_state==TIMER_WAIT_START && distance_delta>0 && before_distance<timer_start_line && s->distance>=timer_start_line) {
        float start_time=0;
        if(timer_crossing_time(before_distance,before_time,s->distance,s->time,timer_start_line,&start_time)) {
            timer_start_time=start_time; timer_last_record_time=start_time; timer_run_count=0; timer_state=TIMER_RUNNING;
            if(timer_profile_append(timer_run,&timer_run_count,TIMER_MAX_POINTS,timer_start_line,0)) {
                log_event("TIMER_START","line=%.3f race_time=%.5f distance_before=%.3f distance_after=%.3f target_sample_hz=30",timer_start_line,start_time,before_distance,s->distance);
                timer_set_display(timer_reference_valid?2:1,0,1);
            }
        }
    }
    if(timer_state==TIMER_RUNNING) {
        float elapsed=s->time-timer_start_time;
        int finish_crossed=timer_finish_set && before_distance<timer_finish_line && s->distance>=timer_finish_line && distance_delta>0;
        if(finish_crossed) {
            float finish_time=0;
            if(timer_crossing_time(before_distance,before_time,s->distance,s->time,timer_finish_line,&finish_time)) {
                float finish_elapsed=finish_time-timer_start_time;
                timer_record_sample(timer_finish_line,finish_time,"finish",0);
                timer_finish_trial(finish_elapsed);
            }
        } else {
            if(s->time-timer_last_record_time>=TIMER_SAMPLE_SECONDS && distance_delta>=0) {
                const char *sample_rejection=0;
                if(!timer_record_sample(s->distance,s->time,"periodic",&sample_rejection)) {
                    if(sample_rejection && (!strcmp(sample_rejection,"distance_regression_over_5m") || !strcmp(sample_rejection,"elapsed_regression_over_0_1s"))) {
                        log_event("TIMER_ATTEMPT_REARMED","reason=%s distance=%.5f race_time=%.5f finish=%.5f action=wait_for_start_line",sample_rejection,s->distance,s->time,timer_finish_line);
                        timer_state=TIMER_WAIT_START; timer_run_count=0; timer_display_mode=0; timer_regression_reported=0;
                        timer_previous_distance=s->distance; timer_previous_race_time=s->time; timer_previous_valid=1;
                        notify("Large rewind; timer waiting for +5 m start");
                        return;
                    }
                    log_event("TIMER_STOPPED","reason=%s count=%u capacity=%u distance=%.5f race_time=%.5f",sample_rejection?sample_rejection:"unknown",timer_run_count,TIMER_MAX_POINTS,s->distance,s->time);
                    timer_state=TIMER_FINISHED; timer_display_mode=0; notify("Timer stopped; see log");
                }
            }
            if(timer_state==TIMER_RUNNING) {
                float best_elapsed=0;
                if(timer_reference_valid && timer_finish_set && timer_profile_time_at(timer_best,timer_best_count,s->distance,&best_elapsed)) timer_set_display(2,elapsed-best_elapsed,0);
                else timer_set_display(timer_reference_valid?2:1,elapsed,0);
            }
        }
    }
    timer_previous_distance=s->distance; timer_previous_race_time=s->time; timer_previous_valid=1;
}
static void invalidate(const char *why) {
    timer_disarm(why);
    release_rbrcontrols_ignition("invalidate");
    restore_clutch_help("invalidate");
    if(countdown_active) { log_event("FREEZE_ABORTED","reason=%s no_stale_write=1",why); countdown_active=0; }
    if(jump_back_pending) log_event("JUMP_BACK_ABORTED","reason=context_invalidated target=%.3f",jump_back_pending_distance);
    jump_back_pending=0; jump_back_started_at=0; jump_back_last_success_at=0;
    jump_back_clear(&jump_back_state);
    pending_first_finish_valid=0;
    run_context_suspended=0; suspended_context_valid=0;
    if(pending_fresh_session) log_event("FRESH_SESSION_CANCELLED","reason=run_invalidated");
    pending_fresh_session=0; pending_fresh_sample_valid=0;
    if(mark.valid || verifying) log_event("INVALIDATE","reason=%s previous_run=%u",why,generation);
    if(verifying) { log_event("LOAD_VERIFICATION_ABORTED","reason=%s target=%.5f further_calls_disabled=1",why,mark.sample.distance); InterlockedExchange(&disabled,1); }
    mark.valid=0; verifying=0; stopped_tracking=0;
}
/* Read-only diagnostics. A mismatch never resolves to or calls a new target. */
static void hex_bytes(const unsigned char *bytes,size_t n,char *out) {
    for(size_t i=0;i<n;i++) sprintf(out+2*i,"%02x",bytes[i]);
    out[2*n]=0;
}
static void address_owner(uintptr_t address,char *out,size_t n) {
    MEMORY_BASIC_INFORMATION region;
    wchar_t path[1024]; char utf8[2048];
    if(VirtualQuery((void*)address,&region,sizeof(region)) && region.Type==MEM_IMAGE &&
       GetModuleFileNameW((HMODULE)region.AllocationBase,path,1024) &&
       WideCharToMultiByte(CP_UTF8,0,path,-1,utf8,sizeof(utf8),0,0)) {
        char *name=strrchr(utf8,'\\');
        snprintf(out,n,"%s+%08lx",name?name+1:utf8,(unsigned long)(address-(uintptr_t)region.AllocationBase));
    } else snprintf(out,n,"unresolved");
}
static int code_matches(const char *name,uintptr_t p,const unsigned char *expected,size_t n,int report) {
    unsigned char actual[32]; int readable=read_mem(p,actual,sizeof(actual));
    int match=readable && n<=sizeof(actual) && !memcmp(actual,expected,n);
    if(report) {
        char hex[65]="unreadable",want[65],owner[256],target_owner[256]="none";
        uintptr_t target=0; const char *kind="none";
        hex_bytes(expected,n,want); address_owner(p,owner,sizeof(owner));
        if(readable) {
            hex_bytes(actual,sizeof(actual),hex);
            if(actual[0]==0xe9) { int32_t rel; memcpy(&rel,actual+1,4); target=p+5+rel; kind="jmp_rel32"; }
            else if(actual[0]==0xeb) { target=p+2+(int8_t)actual[1]; kind="jmp_rel8"; }
            else if(actual[0]==0xff && actual[1]==0x25) { uintptr_t slot; memcpy(&slot,actual+2,4); target=pointer_at(slot); kind="jmp_indirect"; }
            if(target) address_owner(target,target_owner,sizeof(target_owner));
        }
        log_event("RUNTIME_CODE","name=%s address=%08lx readable=%d match=%d expected=%s actual32=%s owner=%s jump=%s target=%08lx target_owner=%s",name,(unsigned long)p,readable,match,want,hex,owner,kind,(unsigned long)target,target_owner);
    }
    return match;
}
/* Preserve the RSF adapter at the original entry. Its wrapper updates RSF
 * state then invokes the original body via its own trampoline. */
static int known_rsf_runtime(int report) {
    HMODULE module=GetModuleHandleW(L"Rallysimfans.hu.dll");
    if(!module) return 0;
    if(module!=rsf_checked_module) {
        rsf_hash_ok=hash_module(module,RSF_HASH,"RSF"); rsf_checked_module=module;
    }
    unsigned char entry[32],body[sizeof(rsf_body_expected)];
    uintptr_t base=(uintptr_t)module;
    uintptr_t continuation=pointer_at(base+RSF_CONTINUATION_RVA);
    int readable=read_mem(0x4e8df0,entry,sizeof(entry)) && read_mem(base+RSF_BODY_RVA,body,sizeof(body));
    int layout_ok=readable && rsf_reset_layout(entry,base,body,continuation);
    if(report) log_event("RSF_RESET_CHECK","hash_ok=%d readable=%d layout_ok=%d base=%08lx hook_rva=%08lx body_bytes=%u continuation=%08lx expected_continuation=004e8df6",rsf_hash_ok,readable,layout_ok,(unsigned long)base,(unsigned long)RSF_HOOK_RVA,(unsigned)sizeof(body),(unsigned long)continuation);
    return rsf_hash_ok && layout_ok;
}
static int runtime_matches(int report) {
    static const unsigned char reset[]={0x55,0x8b,0xec,0x83,0xe4,0xf0,0x83,0xec,0x3c};
    static const unsigned char ctor[]={0x0f,0x57,0xc0,0x8b,0xc1,0xc7,0x40,0x04};
    static const unsigned char notes[]={0x56,0x57,0xc7,0x41,0x5c,0,0,0,0};
    int a=code_matches("reset",0x4e8df0,reset,sizeof(reset),report);
    int b=code_matches("ctor",0x52fea0,ctor,sizeof(ctor),report);
    int c=code_matches("notes",0x4dd600,notes,sizeof(notes),report);
    const char *route="stock";
    if(!a) { a=known_rsf_runtime(report); route=a?"verified_rsf_adapter":"rejected_reset"; }
    if(report) log_event("RUNTIME_ROUTE","route=%s reset_ok=%d ctor_ok=%d notes_ok=%d supported=%d call_entry=004e8df0",route,a,b,c,a && b && c);
    return a && b && c;
}
static void position_report(const Sample *s) {
    float legacy[3]={0}, matrix[16]={0};
    int a=read_mem(s->car+0xef8,legacy,sizeof(legacy));
    int b=read_mem(s->movement+0x110,matrix,sizeof(matrix));
    log_event("POSITION_SOURCES","legacy_ok=%d legacy_xyz=%.5f,%.5f,%.5f matrix_ok=%d matrix_xyz=%.5f,%.5f,%.5f slot_14c_raw=%.5f selected=movement_140",a,legacy[0],legacy[1],legacy[2],b,matrix[12],matrix[13],matrix[14],matrix[15]);
}
/* The stock vtable slot 0x180 accepts position then quaternion (ret 8).
 * Refuse unknown overrides instead of bypassing them with a direct call. */
static int pose_runtime_matches(uintptr_t movement,int report) {
    uintptr_t table=pointer_at(movement), target=pointer_at(table+0x180);
    unsigned char actual[sizeof(pose_body_expected)]={0};
    int readable=target==0x4e9660 && read_mem(target,actual,sizeof(actual));
    int stock=readable && !memcmp(actual,pose_body_expected,sizeof(actual));
    uintptr_t ngp_base=(uintptr_t)GetModuleHandleW(L"NGP.dll");
    unsigned char handler[sizeof(ngp_pose_body_expected)]={0};
    int handler_readable=readable && !stock && ngp_base && read_mem(ngp_base+NGP_POSE_RVA,handler,sizeof(handler));
    int ngp_layout=handler_readable && pose_ngp_layout(actual,ngp_base,handler);
    int known_ngp=ngp_ok && ngp_layout;
    int supported=stock || known_ngp;
    if(report) {
        char owner[256]; address_owner(target,owner,sizeof(owner));
        int first=-1;unsigned differences=0;
        for(unsigned i=0;readable && i<sizeof(actual);i++) if(actual[i]!=pose_body_expected[i]) { if(first<0)first=(int)i;differences++; }
        int32_t rel=0;memcpy(&rel,actual+POSE_CALL_OPERAND_OFFSET,4);
        uintptr_t dispatch=readable && actual[POSE_CALL_OPERAND_OFFSET-1]==0xe8?(uint32_t)(0x4e9780u+rel):0;
        char dispatch_owner[256];address_owner(dispatch,dispatch_owner,sizeof(dispatch_owner));
        log_event("POSE_ROUTE","movement=%08lx vtable=%08lx target=%08lx owner=%s readable=%d body_match=%d ngp_hash_ok=%d ngp_layout=%d handler_readable=%d supported=%d route=%s bytes=%u first_difference=%d changed_bytes=%u dispatch_target=%08lx dispatch_owner=%s",(unsigned long)movement,(unsigned long)table,(unsigned long)target,owner,readable,stock,ngp_ok,ngp_layout,handler_readable,supported,stock?"stock":(known_ngp?"verified_ngp_pose_dispatch":"rejected"),(unsigned)sizeof(actual),first,differences,(unsigned long)dispatch,dispatch_owner);
        if(!supported && readable) {
            char actual_hex[2*sizeof(actual)+1];hex_bytes(actual,sizeof(actual),actual_hex);
            log_event("POSE_CODE_MISMATCH","address=004e9660 actual=%s",actual_hex);
            if(handler_readable) {
                char handler_hex[2*sizeof(handler)+1];hex_bytes(handler,sizeof(handler),handler_hex);
                log_event("POSE_NGP_HANDLER","address=%08lx actual=%s",(unsigned long)(ngp_base+NGP_POSE_RVA),handler_hex);
            }
        }
    }
    return supported;
}
static void call_game_pose(uintptr_t movement,const float position[4],const float quaternion[4]) {
    typedef void (TC *Pose)(void*,const float*,const float*);
    uintptr_t target=pointer_at(pointer_at(movement)+0x180);
    ((Pose)target)((void*)movement,position,quaternion);
}
#ifndef BOOKMARK_APPLY_POSE
#define BOOKMARK_APPLY_POSE call_game_pose
#endif
static float xyz_error(const Sample *a,const Sample *b) {
    float x=a->x-b->x,y=a->y-b->y,z=a->z-b->z;
    return sqrtf(x*x+y*y+z*z);
}
static int apply_sample_pose(uintptr_t movement,const Sample *pose,float setter_z) {
    float position[4]={pose->x,pose->y,setter_z,0},q[4];
    if(!normalized_quaternion(pose->q,q) || !pose_runtime_matches(movement,0)) return 0;
    InterlockedExchange(&inside_load,1);
    BOOKMARK_APPLY_POSE(movement,position,q);
    InterlockedExchange(&inside_load,0);
    return 1;
}
/* The stock pose setter transforms vertical position as output_z=2*input_z-reference_z.
 * Correct its input from readback error, using half the error per iteration. */
static int apply_pose_with_z_feedback(uintptr_t movement,const Sample *pose,Sample *actual) {
    float setter_z=pose->z;
    for(int iteration=0;iteration<3;iteration++) {
        if(!apply_sample_pose(movement,pose,setter_z)) return 0;
        if(!get_sample(actual)) return 0;
        actual->generation=generation;
        float error=pose->z-actual->z;
        log_event("POSE_Z_FEEDBACK","iteration=%d target_z=%.5f setter_z=%.5f actual_z=%.5f error=%.5f",
          iteration+1,pose->z,setter_z,actual->z,error);
        if(!isfinite(error)) return 0;
        if(fabsf(error)<=POSE_Z_FEEDBACK_TOLERANCE_METERS || iteration==2) {
            freeze_pose_input_z=setter_z;
            return 1;
        }
        setter_z+=error*0.5f;
        if(!isfinite(setter_z) || fabsf(setter_z)>1000.0f) return 0;
    }
    freeze_pose_input_z=setter_z;
    return 1;
}
/* Only the native reset sequence differs in the simulated test build. */
static void call_game_reset(uintptr_t movement,uintptr_t notes,float index) {
    typedef void *(TC *Ctor)(void*);
    typedef void (TC *Reset)(void*,int,float,float);
    typedef void (TC *ClearNotes)(void*);
    unsigned char block[16];
    ((Ctor)0x52fea0)(block); memcpy(block+12,&index,4);
    memcpy((void*)(movement+0x170),block,16);
    /* Preserve the verified RSF adapter in the original entry's chain. */
    ((Reset)0x4e8df0)((void*)movement,0,0.0f,1.0f);
    ((ClearNotes)0x4dd600)((void*)notes);
}
#ifndef BOOKMARK_RESET_SEQUENCE
#define BOOKMARK_RESET_SEQUENCE call_game_reset
#endif
static void observe_load(const Sample *s,int valid,unsigned elapsed) {
    if(!target_observed && valid && s->active && same_context(s,&mark.sample) && fabsf(s->distance-mark.sample.distance)<=2.0f) {
        target_observed=1; load_observed=*s; first_observed_ms=elapsed;
        snapshot_log("LOAD_TARGET_OBSERVED",s);
        log_event("LOAD_TARGET_TIMING","elapsed_ms=%u target=%.5f observed=%.5f delta=%.5f",elapsed,mark.sample.distance,s->distance,s->distance-mark.sample.distance);
        log_damage_probe("load_target",0);
        if(!pose_attempted) {
            pose_attempted=1;
            freeze_pose=*s;
            if(restore_position) { freeze_pose.x=mark.sample.x; freeze_pose.y=mark.sample.y; freeze_pose.z=mark.sample.z; }
            if(restore_orientation) memcpy(freeze_pose.q,mark.sample.q,sizeof(freeze_pose.q));
            if(!normalized_quaternion(freeze_pose.q,freeze_q) || !pose_runtime_matches(s->movement,1)) {
                log_event("POSE_RESTORE_REJECTED","reason=invalid_target_pose_or_changed_pose_route");
                pending_first_finish_valid=0;
                InterlockedExchange(&disabled,1); verifying=0;
                notify("Pose route changed; further calls disabled"); return;
            }
            snapshot_log("POSE_BEFORE_APPLY",s);
            log_event("POSE_RESTORE_BEGIN","position_source=%s orientation_source=%s saved_xyz=%.5f,%.5f,%.5f saved_q=%.6f,%.6f,%.6f,%.6f",restore_position?"saved_xyz":"native_reset_xyz",restore_orientation?"saved_quaternion":"native_reset_quaternion",freeze_pose.x,freeze_pose.y,freeze_pose.z,freeze_q[0],freeze_q[1],freeze_q[2],freeze_q[3]);
            Sample after;
            pose_restored=apply_pose_with_z_feedback(s->movement,&freeze_pose,&after);
            if(!pose_restored) {
                log_event("POSE_RESTORE_REJECTED","reason=pose_route_changed_before_apply");
                pending_first_finish_valid=0;
                InterlockedExchange(&disabled,1); verifying=0;
                notify("Pose route changed; further calls disabled"); return;
            }
            snapshot_log("POSE_AFTER_APPLY",&after);
            pose_position_error=xyz_error(&after,&freeze_pose);
            pose_rotation_error=rotation_error_degrees(after.q,freeze_pose.q);
            freeze_last=after;
            pose_position_ok=!restore_position || (isfinite(pose_position_error) && pose_position_error<=POSITION_TOLERANCE_METERS);
            log_event("POSE_RESTORE_RETURNED","sample_valid=1 position_ok=%d position_error=%.5f actual_xyz=%.5f,%.5f,%.5f setter_z=%.5f velocity_norm=%.5f rotation_error_deg=%.5f",pose_position_ok,pose_position_error,after.x,after.y,after.z,freeze_pose_input_z,after.vector_speed,pose_rotation_error);
            speed_attempted=0; speed_restored=0;
            countdown_active=1; countdown_last=-1; countdown_started_at=GetTickCount(); countdown_released_at=0;
            log_event("FREEZE_BEGIN","duration_ms=%d method=pose_reapply_native_zero_velocity target_xyz=%.5f,%.5f,%.5f setter_z=%.5f velocity_norm_after_pose=%.5f saved_velocity_xyz=%.5f,%.5f,%.5f",RESUME_DELAY_SECONDS*1000,freeze_pose.x,freeze_pose.y,freeze_pose.z,freeze_pose_input_z,after.vector_speed,mark.sample.velocity[0],mark.sample.velocity[1],mark.sample.velocity[2]);
            if(pending_first_finish_valid && !timer_arm_after_first_restart()) {
                log_event("TIMER_FIRST_RESTART_ARM_FAILED","reason=finish_candidate_context_mismatch");
                pending_first_finish_valid=0;
            }
            enable_clutch_help_for_countdown();
            if(restore_gear) {
                gear_neutral_active=1; gear_attempted=0; gear_restored=0; gear_neutral_last_reported=-1;
                int neutral_actual=-1;
                int neutral_ok=write_gear(s->movement,GEAR_HOLD_RAW_CANDIDATE,&neutral_actual);
                gear_neutral_last_reported=neutral_ok?1:0;
                log_event("GEAR_COUNTDOWN_GEAR_SET","phase=countdown_begin success=%d hold_raw_candidate=%d saved_gear_raw=%d actual_gear_raw=%d hold_during_countdown=1",neutral_ok,GEAR_HOLD_RAW_CANDIDATE,mark.sample.gear,neutral_actual);
            }
            begin_rbrcontrols_ignition_hold();
        }
    }
}
static void abort_return_countdown(const Sample *s,int valid,const char *reason) {
    int context_ok=valid && s->active && same_context(s,&mark.sample);
    if(context_ok && restore_gear && gear_neutral_active) {
        int key_released=1;
        if(seq_key_down) {
            key_released=send_shift_key(vk_seq_up,1);
            log_event("SEQ_UP_KEYUP_ABORT","success=%d key=%d winerror=%lu",key_released,vk_seq_up,key_released?0:GetLastError());
            if(key_released) seq_key_down=0;
        }
        if(key_released) {
            seq_state=0; seq_check_at=0; gear_attempted=1;
            int actual=-1; gear_restored=write_gear(s->movement,mark.sample.gear,&actual);
            log_event("GEAR_COUNTDOWN_GEAR_ABORT_RECOVERY","saved_gear_raw=%d success=%d actual_gear_raw=%d",mark.sample.gear,gear_restored,actual);
            gear_neutral_active=0;
        }
    }
    if(context_ok && restore_speed && !speed_attempted) {
        float actual[3]={0}; speed_attempted=1;
        speed_restored=write_velocity(s->movement,mark.sample.velocity,actual);
    }
    release_rbrcontrols_ignition("countdown_abort");
    restore_clutch_help("countdown_abort");
    countdown_active=0; verifying=0; InterlockedExchange(&disabled,1);
    if(pending_fresh_session) log_event("FRESH_SESSION_CANCELLED","reason=return_countdown_aborted");
    pending_fresh_session=0; pending_fresh_sample_valid=0;
    log_event("FREEZE_ABORT","reason=%s context_ok=%d speed_recovery=%d",reason,context_ok,speed_restored);
    notify("Freeze interrupted; further calls disabled");
}
static void service_return_countdown(const Sample *s,int valid) {
    service_rbrcontrols_ignition(GetTickCount());
    if(!countdown_active) {
        if(clutch_help_temp_active && (!clutch_help_restore_retry_at || (DWORD)(GetTickCount()-clutch_help_restore_retry_at)>=1000)) restore_clutch_help("deferred_retry");
        return;
    }
    if(!valid || !s->active || !same_context(s,&mark.sample)) {
        abort_return_countdown(s,valid,"run_context_changed"); return;
    }
    if(!apply_sample_pose(s->movement,&freeze_pose,freeze_pose_input_z)) {
        abort_return_countdown(s,valid,"pose_route_changed_during_freeze"); return;
    }
    float actual[3]={0};
    Sample held; int held_valid=get_sample(&held); held.generation=generation;
    if(!held_valid || !same_context(&held,&mark.sample)) {
        abort_return_countdown(&held,held_valid,"sample_lost_during_freeze"); return;
    }
    pose_position_error=xyz_error(&held,&freeze_pose);
    pose_rotation_error=rotation_error_degrees(held.q,freeze_pose.q);
    freeze_last=held;
    pose_position_ok=!restore_position || (isfinite(pose_position_error) && pose_position_error<=POSITION_TOLERANCE_METERS);
    DWORD now=GetTickCount(),elapsed=(DWORD)(now-countdown_started_at);
    int seconds=countdown_seconds_remaining(elapsed);
    if(seconds>0) {
        if(gear_neutral_active && held.gear!=GEAR_HOLD_RAW_CANDIDATE) {
            int neutral_actual=-1;
            int neutral_ok=write_gear(s->movement,GEAR_HOLD_RAW_CANDIDATE,&neutral_actual);
            int report_state=neutral_ok?2:3;
            if(report_state!=gear_neutral_last_reported) {
                if(neutral_ok) log_event("GEAR_COUNTDOWN_GEAR_REAPPLIED","hold_raw_candidate=%d previous_gear_raw=%d",GEAR_HOLD_RAW_CANDIDATE,held.gear);
                else log_event("GEAR_COUNTDOWN_GEAR_REAPPLY_FAILED","hold_raw_candidate=%d observed_gear_raw=%d actual_gear_raw=%d",GEAR_HOLD_RAW_CANDIDATE,held.gear,neutral_actual);
                gear_neutral_last_reported=report_state;
            }
        }
        if(seconds!=countdown_last) {
            countdown_last=seconds;
            log_event("COUNTDOWN_TICK","seconds=%d elapsed_ms=%lu xyz_error=%.5f rotation_error_deg=%.5f held_velocity_norm=%.5f",seconds,(unsigned long)elapsed,pose_position_error,pose_rotation_error,held.vector_speed);
            log_clutch_help_state("countdown_tick");
            char text[64]; snprintf(text,sizeof(text),"Moving in %d",seconds); notify(text);
        }
        return;
    }
    if(restore_gear && !gear_attempted) {
        countdown_last=0;
        log_event("COUNTDOWN_ELAPSED","elapsed_ms=%lu next=restore_saved_gear_then_velocity",(unsigned long)elapsed);
        notify("Engaging saved gear");
        begin_saved_gear_restore(s);
    }
    if(restore_gear && seq_state!=0) return;
    speed_attempted=1; speed_restored=0;
    if(restore_speed) speed_restored=write_velocity(s->movement,mark.sample.velocity,actual);
    release_rbrcontrols_ignition("countdown_release");
    restore_clutch_help("countdown_release");
    if(pending_fresh_session && valid) {
        Sample fresh; int fresh_valid=get_sample(&fresh); fresh.generation=generation;
        pending_fresh_sample=fresh;
        pending_fresh_sample_valid=fresh_valid && fresh.active && fresh.foreground && same_context(&fresh,&mark.sample);
        log_event("FRESH_SESSION_CAPTURED","source=return_release sample_valid=%d distance=%.5f race_time=%.5f speed_restored=%d",pending_fresh_sample_valid,fresh.distance,fresh.time,speed_restored);
    }
    countdown_active=0; countdown_released_at=now; gear_neutral_active=0;
    if(timer_armed && timer_finish_set && timer_state==TIMER_WAIT_START) {
        timer_previous_distance=s->distance; timer_previous_race_time=s->time; timer_previous_valid=1;
        log_event("TIMER_RETURN_BASELINE","distance=%.5f race_time=%.5f start_line=%.5f distance_to_start=%.5f",s->distance,s->time,timer_start_line,timer_start_line-s->distance);
    }
    log_event("COUNTDOWN_RELEASE","elapsed_ms=%lu gear_restore_completed=%d gear_field_match=%d xyz_error=%.5f position_ok=%d rotation_error_deg=%.5f restore_speed=%d speed_applied=%d saved_velocity_xyz=%.5f,%.5f,%.5f actual_velocity_xyz=%.5f,%.5f,%.5f",(unsigned long)elapsed,!restore_gear || gear_attempted,gear_restored,pose_position_error,pose_position_ok,pose_rotation_error,restore_speed,speed_restored,mark.sample.velocity[0],mark.sample.velocity[1],mark.sample.velocity[2],actual[0],actual[1],actual[2]);
    notify(restore_speed?(speed_restored?"GO":"GO; speed restore failed"):"Ready; speed restore is off");
}
static void do_load(const Sample *s) {
    if(verifying || disabled) {
        snapshot_log("LOAD_REJECTED_PENDING_OR_DISABLED",s);
        notify("Previous Load still pending or failed"); return;
    }
    Gates g={live_enabled,executable_ok && ngp_ok,log_file!=INVALID_HANDLE_VALUE && !disabled,GetCurrentThreadId()==render_thread,runtime_matches(1)};
    const char *reason=load_guard(s,&mark,&g);
    snapshot_log("LOAD_REQUEST",s);
    log_event("LOAD_GATES","reason=%s target=%.5f index=%.6f live=%d speed_limit=none runtime=%d",reason?reason:"passed",mark.sample.distance,mark.target_index,g.enabled,g.runtime_ok);
    if(reason) { notify(!strcmp(reason,"dry_run")?"DRY RUN: request recorded; car not moved":reason); return; }
    if((restore_position || restore_orientation) && ((restore_orientation && !quaternion_valid(mark.sample.q)) || !pose_runtime_matches(s->movement,1))) {
        log_event("LOAD_REJECTED_POSE","saved_q_valid=%d reason=position_or_orientation_route_unverified",quaternion_valid(mark.sample.q));
        notify("Load rejected: saved pose or route unverified"); return;
    }
    float index=0;
    if(!distance_index(distances,s->count,mark.sample.distance,&index) || fabsf(index-mark.target_index)>.001f) { notify("Saved geometry changed"); return; }
    /* Mirror the observed backward branch of PaceNote, using game-owned vtables.
     * Never copy the whole car or restore stale pointers from a save file.
     */
    uintptr_t manager=pointer_at(0x8d5338), notes=pointer_at(manager+0x105b0);
    unsigned char old_block[16];
    if(!notes || !read_mem(s->movement+0x170,old_block,16)) { notify("Reset prerequisites unavailable"); return; }
    MEMORY_BASIC_INFORMATION region;
    if(!VirtualQuery((void*)(s->movement+0x170),&region,sizeof(region)) || !(region.Protect&(PAGE_READWRITE|PAGE_EXECUTE_READWRITE))) { notify("Reset state not writable"); return; }
    log_event("LIVE_LOAD_BEGIN","target=%.5f index=%.6f movement=%08lx notes=%08lx original_block=%08lx,%08lx,%08lx,%08lx speed_limit=none speed_raw_before=%.5f velocity_norm_before=%.5f saved_speed_raw=%.5f saved_velocity_norm=%.5f saved_velocity_xyz=%.5f,%.5f,%.5f saved_gear_raw=%d restore_speed=%d restore_gear=%d",mark.sample.distance,index,(unsigned long)s->movement,(unsigned long)notes,(unsigned long)((DWORD*)old_block)[0],(unsigned long)((DWORD*)old_block)[1],(unsigned long)((DWORD*)old_block)[2],(unsigned long)((DWORD*)old_block)[3],s->speed,s->vector_speed,mark.sample.speed,mark.sample.vector_speed,mark.sample.velocity[0],mark.sample.velocity[1],mark.sample.velocity[2],mark.sample.gear,restore_speed,restore_gear);
    load_before=*s; target_observed=0; first_observed_ms=0; motion_bucket=0; pose_attempted=0; pose_restored=0; pose_position_ok=0; pose_position_error=NAN; pose_rotation_error=NAN; speed_attempted=0; speed_restored=0; gear_attempted=0; gear_restored=0; gear_neutral_active=0; gear_neutral_last_reported=-1; countdown_active=0; countdown_last=-1; countdown_released_at=0;
    timer_prepare_trial();
    verification_at=GetTickCount();
    InterlockedExchange(&inside_load,1);
    BOOKMARK_RESET_SEQUENCE(s->movement,notes,index);
    InterlockedExchange(&inside_load,0);
    verifying=1;
    Sample immediate; int valid=get_sample(&immediate); immediate.generation=generation;
    snapshot_log("LOAD_IMMEDIATE",&immediate);
    observe_load(&immediate,valid,(unsigned)(GetTickCount()-verification_at));
    log_event("LIVE_LOAD_RETURNED","verification_pending=%d",verifying);
    notify(disabled?"Pose restore failed; further calls disabled":"Load returned; holding for 3 seconds");
}
static int save_session_from_sample(const Sample *s) {
    if(!s) return 0;
    snapshot_log("SAVE_REQUEST",s); position_report(s); float index;
    if(!s->active || !s->foreground || verifying || (restore_orientation && !quaternion_valid(s->q)) ||
       (restore_gear && !s->gear_valid) || !distance_index(distances,s->count,s->distance,&index) ||
       s->distance<distances[0]+5 || s->distance>distances[s->count-1]-5) {
        notify("Save rejected: context, gear, geometry, orientation or boundary");
        log_event("SAVE_REJECTED","active=%d foreground=%d verifying=%d distance=%.5f",s->active,s->foreground,verifying,s->distance);
        return 0;
    }
    mark.valid=1; mark.sample=*s; mark.target_index=index; timer_reset_for_new_session(s);
    log_event("SAVE_OK","distance=%.5f index=%.6f run=%u xyz=%.5f,%.5f,%.5f velocity_xyz=%.5f,%.5f,%.5f quaternion=%.6f,%.6f,%.6f,%.6f gear_raw=%d gear_valid=%d gear_semantics=raw_field_only position_tolerance_m=%.2f session_reset=1",s->distance,index,generation,s->x,s->y,s->z,s->velocity[0],s->velocity[1],s->velocity[2],s->q[0],s->q[1],s->q[2],s->q[3],s->gear,s->gear_valid,POSITION_TOLERANCE_METERS);
    log_damage_probe("save",1);
    return 1;
}
static void update_jump_back_points(const Sample *s,int valid) {
    if(!valid || !s->active || !s->foreground || jump_back_pending) return;
    int result=jump_back_build(&jump_back_state,s,distances,s->count);
    if(result&JUMP_BACK_BUILD_CONTEXT_RESET) {
        log_event("JUMP_BACK_POINTS_READY","run=%u count=%u spacing_m=%.0f start=%.3f end=%.3f stored=route_distance_and_reset_index only=1",
            s->generation,jump_back_state.point_count,JUMP_BACK_SPACING_METERS,distances[0],distances[s->count-1]);
    }
}
static void observe_jump_back(const Sample *s,int valid) {
    if(!jump_back_pending) return;
    DWORD now=GetTickCount(),elapsed=(DWORD)(now-jump_back_started_at);
    if(valid && s->active && same_context(s,&jump_back_before)) {
        float error=fabsf(s->distance-jump_back_pending_distance);
        if(isfinite(error) && error<=2.0f) {
            jump_back_commit(&jump_back_state,jump_back_pending_point);
            jump_back_pending=0; jump_back_started_at=0; jump_back_last_success_at=now;
            log_event("JUMP_BACK_DONE","target=%.3f actual=%.3f error=%.3f index=%.3f elapsed_ms=%lu speed_before=%.3f speed_after=%.3f gear_before=%d gear_after=%d speed_write=0 gear_write=0",
                jump_back_pending_distance,s->distance,error,jump_back_pending_index,(unsigned long)elapsed,jump_back_before.vector_speed,s->vector_speed,jump_back_before.gear,s->gear);
            notify("Jump Back complete");
            return;
        }
    }
    if(elapsed>=1500u) {
        log_event("JUMP_BACK_FAILED","reason=target_not_observed elapsed_ms=%lu expected=%.3f sample_valid=%d actual=%.3f pending_state_preserved=0",
            (unsigned long)elapsed,jump_back_pending_distance,valid,valid?s->distance:NAN);
        jump_back_pending=0; jump_back_started_at=0;
        notify("Jump Back target not confirmed; see log");
    }
}
static void do_jump_back(const Sample *s,int valid) {
    if(jump_back_pending || verifying || countdown_active) {
        log_event("JUMP_BACK_REJECTED","reason=reset_or_countdown_pending verifying=%d countdown=%d jump_pending=%d",verifying,countdown_active,jump_back_pending);
        notify("Jump Back unavailable during reset"); return;
    }
    if(!valid || !s->active || !s->foreground) {
        log_event("JUMP_BACK_REJECTED","reason=invalid_or_inactive_sample valid=%d active=%d foreground=%d",valid,s?s->active:0,s?s->foreground:0);
        notify("Jump Back requires an active stage"); return;
    }
    if(disabled || !live_enabled || !executable_ok || !ngp_ok || !config_ok || GetCurrentThreadId()!=render_thread) {
        log_event("JUMP_BACK_REJECTED","reason=actions_unavailable disabled=%d live=%d exe=%d ngp=%d config=%d render_thread=%d",disabled,live_enabled,executable_ok,ngp_ok,config_ok,GetCurrentThreadId()==render_thread);
        notify("Jump Back unavailable; see log"); return;
    }
    unsigned point_index=0;
    if(!jump_back_choose(&jump_back_state,s->distance,&point_index)) {
        log_event("JUMP_BACK_REJECTED","reason=no_checkpoint_behind current=%.3f points=%u last_jump_valid=%d",s->distance,jump_back_state.point_count,jump_back_state.last_jump_valid);
        notify("No checkpoint behind the car"); return;
    }
    const JumpBackPoint *point=jump_back_get(&jump_back_state,point_index);
    float verified_index=0;
    if(!point || point->distance>=s->distance || !distance_index(distances,s->count,point->distance,&verified_index) || fabsf(verified_index-point->index)>.001f) {
        log_event("JUMP_BACK_REJECTED","reason=checkpoint_geometry_changed point=%u current=%.3f target=%.3f stored_index=%.3f",point_index,s->distance,point?point->distance:NAN,point?point->index:NAN);
        notify("Jump Back route data changed"); return;
    }
    if(!runtime_matches(1)) {
        log_event("JUMP_BACK_REJECTED","reason=unsupported_reset_route target=%.3f",point->distance);
        notify("Jump Back reset route unsupported"); return;
    }
    uintptr_t manager=pointer_at(0x8d5338),notes=0;
    if(manager && manager<=UINTPTR_MAX-0x105b0) notes=pointer_at(manager+0x105b0);
    unsigned char old_block[16];
    if(!notes || !read_mem(s->movement+0x170,old_block,sizeof(old_block))) {
        log_event("JUMP_BACK_REJECTED","reason=reset_prerequisites_unavailable movement=%08lx notes=%08lx",(unsigned long)s->movement,(unsigned long)notes);
        notify("Jump Back prerequisites unavailable"); return;
    }
    MEMORY_BASIC_INFORMATION region;
    if(!VirtualQuery((void*)(s->movement+0x170),&region,sizeof(region)) || region.State!=MEM_COMMIT ||
       !(region.Protect&(PAGE_READWRITE|PAGE_EXECUTE_READWRITE))) {
        log_event("JUMP_BACK_REJECTED","reason=reset_state_not_writable movement=%08lx",(unsigned long)s->movement);
        notify("Jump Back state not writable"); return;
    }
    jump_back_before=*s; jump_back_pending=1; jump_back_pending_point=point_index;
    jump_back_pending_distance=point->distance; jump_back_pending_index=point->index; jump_back_started_at=GetTickCount();
    log_event("JUMP_BACK_BEGIN","current=%.3f target=%.3f index=%.3f point=%u points=%u run=%u speed_before=%.3f gear_before=%d speed_write=0 gear_write=0 countdown=0",
        s->distance,point->distance,point->index,point_index,jump_back_state.point_count,s->generation,s->vector_speed,s->gear);
    InterlockedExchange(&inside_load,1);
    BOOKMARK_RESET_SEQUENCE(s->movement,notes,point->index);
    InterlockedExchange(&inside_load,0);
    Sample immediate; int immediate_valid=get_sample(&immediate); immediate.generation=generation;
    snapshot_log("JUMP_BACK_IMMEDIATE",&immediate);
    observe_jump_back(&immediate,immediate_valid);
    if(jump_back_pending) notify("Jump Back pending; checking target");
}
static void frame_tick(void) {
    DWORD now=GetTickCount();
    int countdown_was_active=countdown_active;
    if(!render_thread) { render_thread=GetCurrentThreadId(); log_event("RENDER_THREAD","id=%lu factory_thread=%lu",render_thread,factory_thread); }
    if(GetCurrentThreadId()!=render_thread) { log_event("RENDER_THREAD_MISMATCH","expected=%lu actual=%lu actions_disabled=1",render_thread,GetCurrentThreadId()); InterlockedExchange(&disabled,1); return; }
    release_sequential_shift_key();
    if(menu_binding_target || save_input_kind==BIND_CONTROLLER || load_input_kind==BIND_CONTROLLER || jump_back_input_kind==BIND_CONTROLLER) poll_controllers();
    menu_binding_tick();
    if(!checked_ngp) {
        HMODULE n=GetModuleHandleW(L"NGP.dll");
        if(n) { ngp_ok=hash_module(n,NGP_HASH,"NGP"); checked_ngp=1; if(!ngp_ok) notify("Unsupported NGP; actions disabled"); }
    }
    int vk[]={vk_save,vk_load,vk_jump_back,vk_reload}, edge[4], down[4];
    down[0]=save_input_kind==BIND_CONTROLLER?controller_button_down(&save_joy_guid,save_joy_button):(vk_save>0 && (GetAsyncKeyState(vk_save)&0x8000)!=0);
    down[1]=load_input_kind==BIND_CONTROLLER?controller_button_down(&load_joy_guid,load_joy_button):(vk_load>0 && (GetAsyncKeyState(vk_load)&0x8000)!=0);
    down[2]=jump_back_input_kind==BIND_CONTROLLER?controller_button_down(&jump_back_joy_guid,jump_back_joy_button):(vk_jump_back>0 && (GetAsyncKeyState(vk_jump_back)&0x8000)!=0);
    down[3]=vk_reload>0 && (GetAsyncKeyState(vk_reload)&0x8000)!=0;
    for(int i=0;i<4;i++) {
        int was_down=keys[i];
        edge[i]=key_edge(down[i],&keys[i]);
        int controller_action=(i==0 && save_input_kind==BIND_CONTROLLER) || (i==1 && load_input_kind==BIND_CONTROLLER) || (i==2 && jump_back_input_kind==BIND_CONTROLLER);
        if(!down[i] && was_down) log_event(controller_action?"CONTROLLER_ACTION_UP":"KEY_UP","action=%s vk=%d",i==0?"save":(i==1?"load":(i==2?"jump_back":"reload")),vk[i]);
        if(edge[i]) {
            if(controller_action)
                log_event("CONTROLLER_ACTION_DOWN","action=%s button=%d",i==0?"save":(i==1?"load":"jump_back"),i==0?save_joy_button:(i==1?load_joy_button:jump_back_joy_button));
            else log_event("KEY_DOWN","action=%s vk=%d",i==0?"save":(i==1?"load":(i==2?"jump_back":"reload")),vk[i]);
        }
    }
    if(edge[3] && !countdown_was_active) { config_read(); return; }
    if(edge[3] && countdown_was_active) log_event("RELOAD_IGNORED","reason=countdown_active");
    if(disabled || !executable_ok || !ngp_ok || !config_ok) { if(edge[0] || edge[1] || edge[2]) notify("Actions disabled; see log"); return; }
    Sample s; int valid=get_sample(&s);
    if(edge[0] && countdown_was_active && !edge[1] && !edge[2]) {
        pending_fresh_session=1; pending_fresh_sample_valid=0;
        log_event("FRESH_SESSION_QUEUED","source=return_countdown capture_at_release=1");
    } else if(edge[0] && verifying && !edge[1] && !edge[2]) {
        pending_fresh_session=1; pending_fresh_sample=s; pending_fresh_sample_valid=valid && s.active && s.foreground;
        log_event("FRESH_SESSION_QUEUED","source=verification_frame sample_valid=%d distance=%.5f race_time=%.5f",pending_fresh_sample_valid,s.distance,s.time);
    }
    if(was_focus!=s.foreground) { log_event("FOCUS","foreground=%d",s.foreground); was_focus=s.foreground; }
    /* Mode 2 is a known pause. Other temporary sample/mode losses suspend the
     * run and preserve the mark; a different active context is a hard boundary. */
    int paused=valid && was_in_run && s.mode==2 && s.time>0 && s.countdown<=0;
    int in_run=valid && (s.active || paused);
    int same_objects=same_context_objects(&s,&previous_sample);
    int recent_jump_back=jump_back_pending || (jump_back_last_success_at && (DWORD)(now-jump_back_last_success_at)<=1500u);
    int expected_jump_back_time_rewind=same_objects && recent_jump_back && s.time+0.25f<previous_sample.time;
    int changed=0;
    if(run_context_suspended && s.active) {
        int resumed_objects=suspended_context_valid && same_context_objects(&s,&suspended_context_sample);
        int resumed_time_ok=!suspended_context_valid || s.time+0.25f>=suspended_context_sample.time || recent_jump_back;
        int mark_objects_ok=!mark.valid || same_context_objects(&s,&mark.sample);
        if(resumed_objects && resumed_time_ok && mark_objects_ok) {
            log_event("RUN_CONTEXT_RESUMED","mode=%d mark_preserved=%d car=%08lx movement=%08lx line=%08lx nodes=%u",s.mode,mark.valid,(unsigned long)s.car,(unsigned long)s.movement,(unsigned long)s.line,s.count);
            run_context_suspended=0; suspended_context_valid=0;
        } else {
            log_event("RUN_CONTEXT_REPLACED","resume_objects_match=%d resume_time_ok=%d mark_objects_match=%d",resumed_objects,resumed_time_ok,mark_objects_ok);
            invalidate("mode_objects_or_race_time_changed"); changed=1;
        }
    } else if(was_in_run && in_run && s.active &&
        (!same_objects || (s.time+0.25f<previous_sample.time && !expected_jump_back_time_rewind))) {
        invalidate("mode_objects_or_race_time_changed"); changed=1;
    }
    int pause_context_transient=was_in_run && paused &&
        (!same_objects || (s.time+0.25f<previous_sample.time && !expected_jump_back_time_rewind));
    if(was_in_run && (!in_run || pause_context_transient) && !run_context_suspended) {
        suspended_context_sample=previous_sample;
        suspended_context_valid=previous_sample.car && previous_sample.movement && previous_sample.line && previous_sample.count>=2;
        run_context_suspended=1; timer_previous_valid=0; stopped_tracking=0;
        log_event("RUN_CONTEXT_SUSPENDED","reason=%s mode=%d sample_valid=%d mark_preserved=%d timer_preserved=%d reference=%08lx,%08lx,%08lx nodes=%u",!valid?"sample_unavailable":(pause_context_transient?"pause_context_changed":"drive_inactive"),valid?s.mode:-1,valid,mark.valid,timer_armed,(unsigned long)suspended_context_sample.car,(unsigned long)suspended_context_sample.movement,(unsigned long)suspended_context_sample.line,suspended_context_sample.count);
    }
    if(valid && s.active && (!was_in_run || changed)) { generation++; s.generation=generation; log_event("RUN_BEGIN","run=%u",generation); }
    if(paused && !changed && !was_paused) { log_event("PAUSE_BEGIN","run=%u bookmark_preserved=%d",generation,mark.valid); }
    if(was_paused && valid && s.active && !changed) log_event("PAUSE_END","run=%u bookmark_preserved=%d",generation,mark.valid);
    s.generation=generation;
    if(in_run || run_context_suspended) was_in_run=1;
    else was_in_run=0;
    if(valid && s.active) was_in_run=1;
    was_paused=paused && !changed;
    update_jump_back_points(&s,valid);
    observe_jump_back(&s,valid);
    service_sequential_shift(&s,valid);
    if(valid && s.active && !runtime_reported) { runtime_matches(1); position_report(&s); runtime_reported=1; }
    if(!valid || !s.active || !s.foreground || fabsf(s.speed)>.1f || s.vector_speed>.1f) stopped_tracking=0;
    else if(!stopped_tracking) { stopped_tracking=1; stopped_since=now; }
    if(now-last_heartbeat>=1000) { snapshot_log(valid?"SAMPLE":"SAMPLE_INVALID",&s); last_heartbeat=now; }
    if(verifying) {
        unsigned elapsed=(unsigned)(now-verification_at);
        observe_load(&s,valid,elapsed);
        service_return_countdown(&s,valid);
        if(elapsed/100>motion_bucket) { motion_bucket=elapsed/100; snapshot_log("LOAD_MOTION",&s); }
        DWORD verify_now=GetTickCount();
        int timed_out=!target_observed && elapsed>=500;
        int settled=target_observed && !countdown_active && countdown_released_at && (DWORD)(verify_now-countdown_released_at)>=500;
        if(timed_out || settled) {
            float rotation_error=rotation_error_degrees(s.q,mark.sample.q);
            int rotation_ok=!restore_orientation || (pose_restored && orientation_within_tolerance(pose_rotation_error) && orientation_within_tolerance(rotation_error));
            int position_ok=!restore_position || (pose_restored && pose_position_ok && isfinite(pose_position_error) && pose_position_error<=POSITION_TOLERANCE_METERS);
            int velocity_ok=!restore_speed || speed_restored;
            int gear_ok=!restore_gear || (gear_restored && s.gear_valid && s.gear==mark.sample.gear);
            int core_good=target_observed && valid && s.active && same_context(&s,&mark.sample) && rotation_ok;
            if(target_observed) log_damage_probe("load_after",0);
            snapshot_log("LOAD_AFTER",&s);
            const char *verify_event=!core_good?"LOAD_VERIFICATION_FAILED":(!gear_ok?"LOAD_VERIFIED_GEAR_MISMATCH":((!position_ok || !velocity_ok)?"LOAD_VERIFIED_POSITION_SPEED_MISMATCH":"LOAD_VERIFIED"));
            log_event(verify_event,"target=%.5f observed=%d first_observed_ms=%u actual=%.5f delta=%.5f final_distance=%.5f saved_xyz=%.5f,%.5f,%.5f held_xyz=%.5f,%.5f,%.5f xyz_error=%.5f position_ok=%d speed_raw_before=%.5f velocity_norm_before=%.5f speed_raw_after=%.5f velocity_norm_after=%.5f restore_orientation=%d restore_position=%d pose_applied=%d rotation_error_deg=%.5f rotation_ok=%d restore_speed=%d velocity_applied=%d velocity_ok=%d restore_gear=%d gear_strategy=direct_field_write gear_field_applied=%d gear_field_ok=%d gear_ok=%d engagement_status=not_directly_observable saved_gear_raw=%d actual_gear_raw=%d actions_disabled=%d",mark.sample.distance,target_observed,first_observed_ms,target_observed?load_observed.distance:s.distance,(target_observed?load_observed.distance:s.distance)-mark.sample.distance,s.distance,mark.sample.x,mark.sample.y,mark.sample.z,pose_restored?freeze_last.x:s.x,pose_restored?freeze_last.y:s.y,pose_restored?freeze_last.z:s.z,pose_position_error,position_ok,load_before.speed,load_before.vector_speed,s.speed,s.vector_speed,restore_orientation,restore_position,pose_restored,rotation_error,rotation_ok,restore_speed,speed_restored,velocity_ok,restore_gear,gear_restored,gear_ok,gear_ok,mark.sample.gear,s.gear,core_good?0:1);
            verifying=0; if(!core_good) { InterlockedExchange(&disabled,1); pending_first_finish_valid=0; }
            if(!core_good) notify("Load context or orientation mismatch; further calls disabled");
            else if(!position_ok || !velocity_ok) notify("Returned; position or speed differs; actions remain enabled");
            else if(!gear_ok) notify("Returned; gear restore mismatch, actions remain enabled");
            else notify("Returned; position, direction and speed checked");
            if(!core_good && pending_fresh_session) {
                log_event("FRESH_SESSION_CANCELLED","reason=load_verification_failed");
                pending_fresh_session=0; pending_fresh_sample_valid=0;
            }
        }
    }
    if(!verifying && pending_fresh_session) {
        if(!disabled && pending_fresh_sample_valid && valid && same_context(&pending_fresh_sample,&s)) {
            Sample fresh=pending_fresh_sample;
            pending_fresh_session=0; pending_fresh_sample_valid=0;
            log_event("FRESH_SESSION_COMMIT","distance=%.5f race_time=%.5f",fresh.distance,fresh.time);
            save_session_from_sample(&fresh);
        } else if(disabled || !pending_fresh_sample_valid) {
            log_event("FRESH_SESSION_CANCELLED","reason=%s",disabled?"plugin_disabled":"sample_invalid");
            pending_fresh_session=0; pending_fresh_sample_valid=0;
        }
    }
    timer_step(&s,valid,countdown_was_active || countdown_active || pending_fresh_session);
    if(countdown_was_active) {
        if(edge[1]) log_event("COUNTDOWN_INPUT_IGNORED","save=0 load=1");
        if(edge[2]) log_event("JUMP_BACK_REJECTED","reason=bookmark_countdown_active");
        previous_sample=s; return;
    }
    int actions=edge[0]+edge[1]+edge[2];
    if(actions>1) { log_event("ACTIONS_REJECTED","reason=simultaneous save=%d load=%d jump_back=%d",edge[0],edge[1],edge[2]); notify("Simultaneous action keys rejected"); previous_sample=s; return; }
    if(edge[0]) {
        if(!valid) { snapshot_log("SAVE_REQUEST_INVALID_SAMPLE",&s); notify("Save rejected: context, gear, geometry, orientation or boundary"); }
        else save_session_from_sample(&s);
    }
    if(edge[1]) {
        if(!valid) { snapshot_log("LOAD_REJECTED_INVALID_SAMPLE",&s); notify("Load rejected: invalid sample"); }
        else if(mark.valid && !timer_finish_set) {
            if(timer_queue_first_finish(&s)) do_load(&s);
        } else do_load(&s);
    }
    if(edge[2]) do_jump_back(&s,valid);
    previous_sample=s;
}
static HRESULT WINAPI end_scene(IDirect3DDevice9 *device) {
    DWORD heartbeat_now=GetTickCount();
    render_heartbeat_frames++;
    if(!render_heartbeat_at || (DWORD)(heartbeat_now-render_heartbeat_at)>=1000) {
        log_event("RENDER_HEARTBEAT","end_scene_calls=%u plugin_menu_overlay=enabled gameplay_overlays=%d",render_heartbeat_frames,DIAGNOSTIC_DISABLE_GAMEPLAY_OVERLAYS?0:1);
        render_heartbeat_frames=0; render_heartbeat_at=heartbeat_now;
    }
    if(!shutdown_done && !InterlockedCompareExchange(&busy,1,0)) {
        frame_tick();
        DWORD now=GetTickCount();
        int menu_visible=plugin_menu_draw_tick && (DWORD)(now-plugin_menu_draw_tick)<=250u;
        if(!menu_visible && game && (LONG)(message_until-now)>0) {
            typedef void (TC *WriteMessage)(Object*,const char*,float,float,float);
            /* SDK documents 0.0 as "until next call"; short timed messages can
             * expire before the game's message pass when sent from EndScene. */
            ((WriteMessage)game->v[1])(game,message,0.0f,20.0f,30.0f);
        }
#if DIAGNOSTIC_DISABLE_GAMEPLAY_OVERLAYS
        if(!render_diagnostic_reported) {
            log_event("RENDER_DIAGNOSTIC","plugin_menu_overlay=enabled gameplay_overlays=disabled frame_tick=enabled previous_end_scene=called");
            render_diagnostic_reported=1;
        }
        if(menu_visible) draw_plugin_menu_overlay(device);
#else
        if(menu_visible) draw_plugin_menu_overlay(device);
        else {
            menu_overlay_reported=-1;
            draw_countdown_overlay(device);
            draw_timer_overlay(device);
            /* The in-world FINISH marker is disabled; the timer summary remains on screen. */
        }
#endif
        InterlockedExchange(&busy,0);
    }
    return previous_end?previous_end(device):D3D_OK;
}
static void CALLBACK timer_tick(HWND w,UINT msg,UINT_PTR id,DWORD time) {
    (void)w;(void)msg;(void)id;(void)time;
    if(!executable_ok || disabled || shutdown_done || hook_slot) return;
    uintptr_t a=pointer_at(0x7ea990), b=pointer_at(a+0x28), device=pointer_at(b+0xf4), vt=pointer_at(device);
    uintptr_t fn=pointer_at(vt+42*sizeof(void*));
    if(!device || !vt || !fn) return;
    DWORD old,ignored;
    void **slot=(void**)(vt+42*sizeof(void*));
    if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old)) { log_event("HOOK_FAILED","winerror=%lu",GetLastError()); return; }
    previous_end=(EndSceneFn)InterlockedExchangePointer(slot,(void*)end_scene); hook_slot=slot;
    VirtualProtect(slot,sizeof(void*),old,&ignored);
    log_event("HOOK_READY","device=%08lx slot=%p previous=%p",(unsigned long)device,slot,previous_end);
}
static LONG WINAPI exception_trace(EXCEPTION_POINTERS *e) {
    uintptr_t address=(uintptr_t)e->ExceptionRecord->ExceptionAddress;
    if(inside_load || (address>=(uintptr_t)self_module && address<(uintptr_t)self_module+0x100000)) {
        log_event("EXCEPTION","code=%08lx address=%p inside_load=%ld recovery=none",e->ExceptionRecord->ExceptionCode,e->ExceptionRecord->ExceptionAddress,inside_load);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
static void shutdown_plugin(void) {
    if(shutdown_done) return; shutdown_done=1;
    release_rbrcontrols_ignition("plugin_shutdown");
    restore_clutch_help("plugin_shutdown");
    if(seq_key_down) { int released=send_shift_key(vk_seq_up,1); log_event("SEQ_UP_KEYUP_SHUTDOWN","success=%d key=%d winerror=%lu",released,vk_seq_up,released?0:GetLastError()); seq_key_down=0; }
    if(timer_id) KillTimer(0,timer_id);
    if(hook_slot && *hook_slot==(void*)end_scene) {
        DWORD old,ignored; if(VirtualProtect(hook_slot,4,PAGE_READWRITE,&old)) { InterlockedCompareExchangePointer(hook_slot,(void*)previous_end,(void*)end_scene); VirtualProtect(hook_slot,4,old,&ignored); }
    }
    for(unsigned i=0;i<joy_device_count;i++) if(joy_devices[i].device) {
        IDirectInputDevice8_Unacquire(joy_devices[i].device);
        IDirectInputDevice8_Release(joy_devices[i].device); joy_devices[i].device=0;
    }
    if(direct_input) { IDirectInput8_Release(direct_input); direct_input=0; }
    if(dinput_module) { FreeLibrary(dinput_module); dinput_module=0; }
    log_event("SHUTDOWN","run=%u",generation);
    /* DLL is pinned for process lifetime, including in another plugin's hook chain. */
}
static void *TC destroy(Object *o,unsigned flags) { (void)flags; shutdown_plugin(); return o; }
static const char *TC name(Object *o) { (void)o; return "LukyanukTraining"; }
static void TC draw(Object *o) {
    (void)o;
    /* EndScene paints the settings panel after the game's scenic menu pass. */
    plugin_menu_draw_tick=GetTickCount();
}
static void TC draw_results(Object *o) { (void)o; }
static void TC events(Object *o,char key,_Bool up,_Bool down,_Bool left,_Bool right,_Bool select) {
    (void)o;
    log_event("MENU_INPUT","key=%d up=%d down=%d left=%d right=%d select=%d row=%d capture=%d",(unsigned char)key,up,down,left,right,select,menu_selection,menu_binding_target);
    if(menu_binding_target) return;
    if(up) { int previous=menu_selection; menu_selection=(menu_selection+3)%4; log_event("MENU_NAV","direction=up from=%d to=%d",previous,menu_selection); return; }
    if(down) { int previous=menu_selection; menu_selection=(menu_selection+1)%4; log_event("MENU_NAV","direction=down from=%d to=%d",previous,menu_selection); return; }
    if(menu_selection==3 && (left || right)) { change_results_display_seconds(right?1:-1); return; }
    if(!select) return;
    if(menu_selection==0) begin_menu_binding(1);
    else if(menu_selection==1) begin_menu_binding(2);
    else if(menu_selection==2) begin_menu_binding(3);
    else change_results_display_seconds(1);
}
static void TC tick_frontend(Object *o,float dt) { (void)o;(void)dt; }
static void TC stage_started(Object *o,int map,const char *player,_Bool false_start) { (void)o;(void)player; invalidate("StageStarted_callback"); was_in_run=0; was_paused=0; log_event("STAGE_STARTED_CALLBACK","map=%d false_start=%d",map,false_start); }
static void TC results(Object *o,float a,float b,float end,const char *player) { (void)o;(void)player; invalidate("results_callback"); log_event("RESULTS_CALLBACK","split1=%.3f split2=%.3f finish=%.3f",a,b,end); }
static void TC checkpoint(Object *o,float t,int id,const char *player) { (void)o;(void)player; log_event("CHECKPOINT_CALLBACK","time=%.3f id=%d",t,id); }
static void *plugin_vtable[]={destroy,name,draw,draw_results,events,tick_frontend,stage_started,results,checkpoint};

__declspec(dllexport) Object *__cdecl RBR_CreatePlugin(Object *engine) {
    if(game) return &plugin;
    game=engine; plugin.v=plugin_vtable; factory_thread=GetCurrentThreadId();
    HMODULE pinned; GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,(LPCWSTR)(uintptr_t)RBR_CreatePlugin,&pinned);
    GetModuleFileNameW(self_module,folder,1024); wchar_t *slash=wcsrchr(folder,L'\\'); if(slash) *slash=0;
    size_t len=wcslen(folder); if(len>850) return &plugin;
    wcscat(folder,L"\\LukyanukTrainingPlugin"); CreateDirectoryW(folder,0);
    swprintf(ini_path,1024,L"%ls\\LukyanukTrainingPlugin.ini",folder);
    wchar_t logs[1024],file[1024]; swprintf(logs,1024,L"%ls\\logs",folder); CreateDirectoryW(logs,0);
    SYSTEMTIME t; GetLocalTime(&t);
    swprintf(file,1024,L"%ls\\session-%04u%02u%02u-%02u%02u%02u-%lu.log",logs,t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,GetCurrentProcessId());
    log_file=CreateFileW(file,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
    if(log_file==INVALID_HANDLE_VALUE) { InterlockedExchange(&disabled,1); snprintf(message,sizeof(message),"Logging unavailable; plugin disabled"); return &plugin; }
    log_event("START","version=%s bits=%u backend=direct-RBR factory_thread=%lu position_tolerance_m=%.2f orientation_tolerance_deg=%.1f pose_z_feedback_tolerance_m=%.2f timer_target_sample_hz=30 engine_rpm_code=absent rbrcontrols_ignition_test=hash_gated_rva_2e4c0 action=6 hold_ms=%lu",VERSION,(unsigned)(sizeof(void*)*8),factory_thread,POSITION_TOLERANCE_METERS,ORIENTATION_TOLERANCE_DEGREES,POSE_Z_FEEDBACK_TOLERANCE_METERS,(unsigned long)RBR_CONTROLS_IGNITION_HOLD_MS);
    executable_ok=(uintptr_t)GetModuleHandleW(0)==0x400000 && hash_module(GetModuleHandleW(0),EXE_HASH,"RBR_EXE");
    if(!executable_ok) log_event("DISABLED","reason=unsupported_executable");
    config_read(); AddVectoredExceptionHandler(0,exception_trace);
    timer_id=SetTimer(0,0,100,timer_tick);
    log_event("TIMER","id=%lu winerror=%lu",(unsigned long)timer_id,timer_id?0:GetLastError());
    return &plugin;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved) {
    (void)reserved; if(reason==DLL_PROCESS_ATTACH) { self_module=module; DisableThreadLibraryCalls(module); } return TRUE;
}



