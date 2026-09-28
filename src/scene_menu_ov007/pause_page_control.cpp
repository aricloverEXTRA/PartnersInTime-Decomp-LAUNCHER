/*
 * Pause page tasks (overlay 7, 0x0206ABD0-0x0206B080).
 *
 * The per-frame tasks of the status page and the Cobalt Star page.
 */

#include "pause_scene_internal.h"
#include <game/pause_text_panel.h>
#include <game/menu_equipment.h>
#include <game/pause_navigation.h>
#include <game/pause_background.h>

extern "C" {
void func_ov005_020663d8(int);
void func_ov005_02066358(PauseMenuElement *, void (*)(PauseMenuElement *), int);
void func_ov005_02069bcc(int, u16, u16, u16);
void func_ov007_0206f8f8(int, int);
void func_ov007_02078478(void);
void func_ov007_020784a0(void);
void func_ov007_02077110(int, u8);
void GameResource_Move16(const void *, void *, u32);
void func_02036988(vu32 *, int, int, int, int);
}
#define INPUT_PRESS (((u16 *)WORK.unknown00)[2])
#define ACTIVE_MEMBER WORK.selected[0]

extern "C" void PauseStatusPage_UpdateTask(PauseStatusPageTask *task)
{
    switch (task->phase) {
    case 0:
        task->phase = 100;
        /* fall through */
    case 100: {
        int direction = 0;
        WORK.initialized = 0;
        if (INPUT_PRESS & 8) {
            func_ov005_020663d8(3);
            data_ov007_0208e1e0->phase = 5;
            func_ov005_02066358((PauseMenuElement *)task,
                (void (*)(PauseMenuElement *))PauseScene_PrepareExitTask, 0);
        } else if ((INPUT_PRESS & 0x802) || task->queued_direction == -1) {
            WORK.initialized = 1;
            MenuEquipment_RequestClose();
            WORK.selected[3] = ACTIVE_MEMBER;
            func_ov005_02066358((PauseMenuElement *)task,
                (void (*)(PauseMenuElement *))PausePage_CloseTask, 0);
        } else if (WORK.party_count > 1) {
            if (INPUT_PRESS & 0x220) --direction;
            if (INPUT_PRESS & 0x110) ++direction;
            if (direction) {
                task->previous_member = ACTIVE_MEMBER;
                do {
                    ACTIVE_MEMBER += direction;
                    if (ACTIVE_MEMBER < 0) ACTIVE_MEMBER = 3;
                    if (ACTIVE_MEMBER >= 4) ACTIVE_MEMBER = 0;
                } while (!WORK.available[ACTIVE_MEMBER]);
                WORK.initialized = 1;
                func_ov005_02069bcc(2, 0, 0, 128);
                WORK.equipment_switching = direction;
                task->phase = 500;
            }
        }
        break;
    }
    case 500:
        GameResource_Move16((void *)0x06001800, (void *)0x06002000, 2048);
        func_ov007_0206f8f8(0, (u8)ACTIVE_MEMBER);
        ++task->phase;
        break;
    case 501:
        func_ov005_020663d8(3);
        func_02036988((vu32 *)0x04000050, 4, 18, 0, 16);
        Overlay5Display_EnablePlanes(DISPLAY_ENGINE_MAIN, 4);
        Overlay5DisplayBg_SetScreenBase(DISPLAY_ENGINE_MAIN, 2, 4);
        Overlay5DisplayBg_SetCharacterBase(DISPLAY_ENGINE_MAIN, 2, 4);
        task->blend_step = 0;
        task->blend = 0;
        ++task->phase;
        break;
    case 502: {
        task->blend += task->blend_step;
        task->blend_step += 2048;
        int alpha = task->blend / 4096;
        if (alpha > 16) alpha = 16;
        REG16(0x04000052) = alpha | ((16 - alpha) << 8);
        if (alpha >= 16) {
            func_ov007_02078478();
            Overlay5DisplayBg_SetScreenBase(DISPLAY_ENGINE_MAIN, 1, 3);
            Overlay5DisplayBg_SetCharacterBase(DISPLAY_ENGINE_MAIN, 1, 4);
            task->blend = 6;
            ++task->phase;
        }
        break;
    }
    case 503:
        func_ov007_02077110(ACTIVE_MEMBER, 2);
        func_ov007_020784a0();
        PauseBackground_QueueBankSwap(1);
        task->blend_step = 0;
        task->blend = 0;
        ++task->phase;
        break;
    case 504: {
        task->blend += task->blend_step;
        task->blend_step += 2048;
        int alpha = task->blend / 4096;
        if (alpha > 16) alpha = 16;
        REG16(0x04000052) = (16 - alpha) | (alpha << 8);
        if (alpha >= 16) {
            Overlay5Display_DisablePlanes(DISPLAY_ENGINE_MAIN, 4);
            task->phase = 100;
        }
        break;
    }
    }
}

extern "C" void PauseStarPage_UpdateTask(PausePageTask *task)
{
    switch (task->phase) {
    case 0:
        task->phase = 100;
        /* fall through */
    case 100:
        WORK.initialized = 0;
        if (INPUT_PRESS & 8) {
            func_ov005_020663d8(3);
            data_ov007_0208e1e0->phase = 5;
            func_ov005_02066358((PauseMenuElement *)task,
                (void (*)(PauseMenuElement *))PauseScene_PrepareExitTask, 0);
        } else if ((INPUT_PRESS & 0x802) || task->queued_direction == -1) {
            WORK.initialized = 1;
            PauseTextPanel_RequestClose();
            func_ov005_02066358((PauseMenuElement *)task,
                (void (*)(PauseMenuElement *))PausePage_CloseTask, 0);
        }
        break;
    }
}
