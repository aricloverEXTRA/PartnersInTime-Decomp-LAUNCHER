/*
 * Pause page open and close (overlay 7, 0x0206E8DC-0x0206F0E4).
 *
 * Opening and closing a pause page, choosing which page the scene enters on, and
 * the delayed rumble that accompanies the entry. CanSelectMember is the
 * eligibility test the member grid applies.
 */

#include "pause_scene_internal.h"
#include <game/pause_menu_cursor.h>
#include <game/pause_selection_sprites.h>
#include <game/pause_navigation.h>
#include <game/pause_transition.h>
#include <game/pause_menu_label.h>
#include <game/menu_equipment.h>
#include <game/scene_menu_motion.h>
#include <game/overlay007_party.h>
#include <game/pause_background.h>

extern "C" {
extern const int data_ov007_0208d964[2][2];
extern u8 data_ov007_02090706[4];
extern HighlightItemView data_ov007_020907f0;
u8 func_ov007_02075400(Overlay7Party *);
int func_ov007_02074b44(Overlay7Party *, int, int, u32);
void func_ov007_02080ae4(void);
void func_ov007_0206e594(PauseMenuElement *);
void func_ov005_0206650c(void *);
void func_ov005_02069bcc(int, u16, u16, u16);
void func_ov005_020663d8(int);
void func_02036988(vu32 *, int, int, int, int);
void func_ov007_02078478(void);
void func_ov007_02078294(void);
void func_ov007_0207f868(PausePageTask *, int);
void func_ov007_0207fe90(int);
void func_ov005_02066358(PauseMenuElement *, void (*)(PauseMenuElement *), int);
void func_ov007_0206faf4(int, int, int);
void func_ov007_0206f754(int);
void func_ov007_0206f8f8(int, int);
void func_ov007_0207b2dc(int, int);
void func_ov007_0207ae0c(void);
void func_ov007_020784a0(void);
void func_ov007_020782a4(void);
void func_ov007_0206c6a0(PauseMenuElement *);
void func_ov007_0206c0e4(PauseMenuElement *);
}

extern "C" int PauseMenu_CanSelectMember(int x, int y)
{
    int member = (u8)data_ov007_0208d964[y][x];
    if (!data_ov007_02090706[member]) return 0;
    if (WORK.selected_menu == 1) {
        int kind = func_ov007_02075400((Overlay7Party *)data_ov007_0208e1e4);
        if (!func_ov007_02074b44((Overlay7Party *)data_ov007_0208e1e4, kind,
                                data_ov007_020907f0.item, member)) return 0;
    }
    return 1;
}

extern "C" void PausePage_OpenTask(PausePageTask *task)
{
    switch (task->phase) {
    case 0:
        func_ov005_02069bcc(234, 0, 0, 128);
        Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_MAIN, 1, 1);
        Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_MAIN, 2, 0);
        data_ov007_020906f0.x = 8;
        data_ov007_020906f0.y = 0;
        func_ov007_0207f868(task, 0);
        switch (WORK.selected_menu) {
        case 0:
            if (!WORK.unknown127[0]) func_ov007_0206faf4(0, 0, 0);
            else func_ov007_0206faf4(0, 1, 0);
            break;
        case 1:
            func_ov007_0206f754(0);
            break;
        case 2:
            WORK.selected[0] = WORK.selected[3];
            func_ov007_0206f8f8(0, (u8)WORK.selected[0]);
            break;
        case 3:
            func_ov007_0206faf4(0, 4, 0);
            break;
        case 4:
            break;
        }
        task->delay = 12;
        ++task->phase;
        break;
    case 1:
        Overlay5Display_EnablePlanes(DISPLAY_ENGINE_MAIN, 4);
        PauseMenuCursor_Close();
        ++task->phase;
        break;
    case 2:
        if (!task->pending) {
            func_02036988((vu32 *)0x04000050, 4, 18, 16, 0);
            Overlay5Display_EnablePlanes(DISPLAY_ENGINE_MAIN, 2);
            Overlay5DisplayBg_SetOffset(DISPLAY_ENGINE_MAIN, 1,
                -data_ov007_020906f0.x, -data_ov007_020906f0.y);
            switch (WORK.selected_menu) {
            case 0:
                PauseList_Show((Overlay7Party *)data_ov007_0208e1e4);
                if (!((u8 *)gSaveData)[0x515]) func_ov007_0207b2dc(0, 0);
                else func_ov007_0207b2dc(0, 1);
                if (!WORK.unknown127[0])
                    PauseBackground_StartAlternateLoad((PauseBackgroundTask *)task, 1, 0);
                else
                    PauseBackground_StartAlternateLoad((PauseBackgroundTask *)task, 0, 0);
                break;
            case 1:
                Overlay5Display_LoadObjTiles(DISPLAY_ENGINE_MAIN, WORK.number_glyph_tiles,
                    data_ov005_0206a180.allocations[0].offset + 4224, 2304);
                Overlay5Display_LoadObjTiles(DISPLAY_ENGINE_MAIN, (u8 *)WORK.number_glyph_tiles + 2048,
                    data_ov005_0206a180.allocations[0].offset + 6784, 256);
                func_ov007_0207ae0c();
                func_ov007_0207b2dc(0, 0);
                break;
            case 2:
                Overlay5Display_LoadObjTiles(DISPLAY_ENGINE_MAIN, WORK.number_glyph_tiles,
                    data_ov005_0206a180.allocations[0].offset + 4224, 2304);
                MenuEquipment_CreateHeading();
                func_ov007_020784a0();
                break;
            case 3:
                PauseList_Show((Overlay7Party *)data_ov007_0208e1e4);
                func_ov007_0207b2dc(0, 1);
                break;
            case 4:
                PauseMenuLabel_DrawPage(4);
                func_ov007_020782a4();
                func_ov007_0207b2dc(1, 0);
                break;
            }
            Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_MAIN, 1, 2);
            Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_MAIN, 2, 1);
            task->queued_direction = 0;
            task->delay = 8;
            ++task->phase;
        }
        break;
    case 3: {
        if (((u16 *)WORK.unknown00)[2] & 0x802) task->queued_direction = -1;
        if (((u16 *)WORK.unknown00)[2] & 0x401) task->queued_direction = 1;
        --task->delay;
        int blend = 2 * task->delay;
        REG16(0x04000052) = blend | ((16 - blend) << 8);
        if (!task->delay) {
            Overlay5Display_DisablePlanes(DISPLAY_ENGINE_MAIN, 4);
            int menu = WORK.selected_menu;
            switch (menu) {
            case 0:
            case 3:
                PauseList_CreateSelectionSprites(task, 0);
                func_ov005_02066358((PauseMenuElement *)task, func_ov007_0206c6a0, 0);
                break;
            case 1:
                PauseEquipment_CreateCategoryCursor();
                func_ov005_02066358((PauseMenuElement *)task, func_ov007_0206c0e4, 0);
                break;
            case 2:
                func_ov005_02066358((PauseMenuElement *)task, (void (*)(PauseMenuElement *))PauseStatusPage_UpdateTask, 0);
                break;
            case 4:
                func_ov005_02066358((PauseMenuElement *)task, (void (*)(PauseMenuElement *))PauseStarPage_UpdateTask, 0);
                break;
            }
        }
        break;
    }
    }
}

extern "C" void PausePage_CloseTask(PausePageTask *task)
{
    switch (task->phase) {
    case 0:
        func_ov005_02069bcc(235, 0, 0, 128);
        func_ov005_020663d8(3);
        func_ov005_020663d8(4);
        func_02036988((vu32 *)0x04000050, 4, 18, 0, 16);
        Overlay5Display_EnablePlanes(DISPLAY_ENGINE_MAIN, 4);
        task->delay = 4;
        ++task->phase;
        break;
    case 1: {
        --task->delay;
        int blend = 4 * task->delay;
        REG16(0x04000052) = (16 - blend) | (blend << 8);
        if (!task->delay) {
            switch (WORK.selected_menu) {
            case 0:
            case 3:
                PauseList_Hide((Overlay7Party *)data_ov007_0208e1e4);
                break;
            case 1: PauseMenuLabel_ReleaseRows(); break;
            case 2: func_ov007_02078478(); break;
            case 4: func_ov007_02078294(); break;
            }
            Overlay5Display_DisablePlanes(DISPLAY_ENGINE_MAIN, 2);
            func_ov007_0207f868(task, 1);
            Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_MAIN, 1, 1);
            Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_MAIN, 2, 0);
            ++task->phase;
        }
        break;
    }
    case 2:
        if (!task->pending) {
            func_ov007_0207fe90(0);
            Overlay5Display_DisablePlanes(DISPLAY_ENGINE_MAIN, 4);
            ++task->phase;
        }
        break;
    case 3:
        Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_MAIN, 1, 2);
        Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_MAIN, 2, 1);
        data_ov007_0208e1e0->phase = 2;
        func_ov005_02066358((PauseMenuElement *)task, (void (*)(PauseMenuElement *))PauseMenu_UpdateTask, 0);
        break;
    }
}

extern "C" void PauseScene_ChooseEntryTask(PauseMenuElement *task)
{
    func_ov007_02080ae4();
    u8 requested_menu = ((u8 *)gSaveData)[0x55d];
    if (requested_menu) {
        WORK.selected_menu = requested_menu - 1;
        func_ov005_02066358(task,
            (void (*)(PauseMenuElement *))PauseScene_FadeInTask, 0);
    } else {
        func_ov005_02066358(task, func_ov007_0206e594, 0);
    }
    data_ov007_0208e1e0->phase = 1;
}

extern "C" void PauseScene_DelayedRumbleTask(PauseRumbleDelayTask *task)
{
    if (task->delay) {
        --task->delay;
    } else {
        /* Fire on an update that starts at zero, then request deferred removal. */
        GameRumble_PlayRepeated(0, 2);
        func_ov005_0206650c(task);
    }
}
