/*
 * Party HP panel (overlay 2, 0x02097F58-0x020988B0).
 *
 * The HP and status readout for the party. UpdateMember recomputes one member's
 * row when its values change, Update walks the party and Draw uploads the panel;
 * the data cache is flushed before the upload because the panel is written with
 * the CPU and read by the display hardware.
 */

#include <nitro/mi_dma.h>
#include <game/battle_actor.h>
#include <game/battle_context.h>
#include <game/battle_effect.h>
#include <game/battle_object.h>
#include <game/battle_scene.h>
#include <game/save_data.h>

enum BattlePartyHpPanelConstant {
    BATTLE_PARTY_HP_PANEL_MARIO_OFFSET = 0x6578,
    BATTLE_PARTY_HP_PANEL_LUIGI_OFFSET = 0x6594,
    BATTLE_PARTY_HP_PANEL_BABY_MARIO_OFFSET = 0x65B0,
    BATTLE_PARTY_HP_PANEL_BABY_LUIGI_OFFSET = 0x65CC,
    BATTLE_PARTY_HP_PANEL_OBJECT_OFFSET = 0x65E8,
    BATTLE_PARTY_HP_PANEL_TEXTURE_FRAME_OFFSET = 0x68D0,
    BATTLE_VIEW_X_OFFSET = 0xCB9C,
    BATTLE_VIEW_Y_OFFSET = 0xCB9E,
    SAVE_PARTY_FORM_OFFSET = 0x558
};

typedef struct BattlePartyHpCounterEffect {
    u8 unknown_00[0x16];
    s16 y_offset;
    u8 unknown_18[4];
    s32 displayed_hp;
} BattlePartyHpCounterEffect;

typedef struct BattlePartyHpPanelState {
    BattleSceneObject *scene_object;
    s16 intensity;
    s16 requested_layout;
    u16 actor_id;
    s16 displayed_hp;
    s16 target_hp;
    s16 hp_transition_frames;
    s16 y;
    s16 active_layout;
    BattlePartyHpCounterEffect *hp_counter_effect;
    BattleEffect *low_hp_effect;
} BattlePartyHpPanelState;

typedef union BattlePartyHpPanelTransform {
    BattleSpriteTransform value;
    s32 words[16];
} BattlePartyHpPanelTransform;

typedef struct BattlePartyHpPanelDisplayList {
    u8 unknown_00[0x50];
    u8 *start;
    u8 *end;
} BattlePartyHpPanelDisplayList;

extern const BattlePartyHpPanelTransform gBattlePartyHpPanelTransformTemplate;

extern void func_0202cc58(const void *source, void *destination, u32 size);
extern void func_0202cd2c(const void *source, void *destination, u32 size);
extern void DC_FlushRange(const void *start, u32 size);

void BattlePartyHpPanel_UpdateMember(BattlePartyHpPanelState *state) {
    int requested_layout;
    int active_layout;
    int y;
    int current_hp;
    int displayed_hp;

    if (BattleRender_UpdateIntensity(
            state->requested_layout, &state->intensity) == 0) {
        goto close_panel;
    }

    requested_layout = state->requested_layout;
    active_layout = state->active_layout;
    if (active_layout != requested_layout) {
        if (active_layout == 0) {
            state->y = 44;
        }

        y = state->y;
        if (y > 44) {
            state->y = y - 4;
        } else if (y < 44) {
            state->y = y + 4;
        }

        if (state->y == 44) {
            state->active_layout = state->requested_layout;
            if (state->requested_layout == 2) {
                if (state == (BattlePartyHpPanelState *)(
                                 gBattleContext +
                                 BATTLE_PARTY_HP_PANEL_MARIO_OFFSET)) {
                    *(s16 *)(gBattleContext + 0x6500 + 0xB6) = 2;
                    *(s16 *)(gBattleContext + 0x6500 + 0xC0) = 44;
                }
                if (state == (BattlePartyHpPanelState *)(
                                 gBattleContext +
                                 BATTLE_PARTY_HP_PANEL_LUIGI_OFFSET)) {
                    *(s16 *)(gBattleContext + 0x6500 + 0xD2) = 2;
                    *(s16 *)(gBattleContext + 0x6500 + 0xDC) = 44;
                }
            }
        }
    } else {
        if (requested_layout == 3) {
            goto close_panel;
        }

        y = state->y;
        if (y > 0) {
            state->y = y - 4;
            state->displayed_hp = state->target_hp;
        } else if (y < 0) {
            state->y = y + 4;
        }
    }

    if (*(s16 *)(gSaveData + SAVE_PARTY_FORM_OFFSET) == 2 &&
        state->active_layout == 2) {
        if (((BattlePartyActor *)BattleActor_GetPartySlot(
                 BATTLE_ACTOR_MARIO))->formation_index ==
            PARTY_MEMBER_BABY_MARIO) {
            if (state == (BattlePartyHpPanelState *)(
                             gBattleContext +
                             BATTLE_PARTY_HP_PANEL_MARIO_OFFSET)) {
                state->actor_id = BATTLE_ACTOR_BABY_MARIO;
            }
            if (state == (BattlePartyHpPanelState *)(
                             gBattleContext +
                             BATTLE_PARTY_HP_PANEL_BABY_MARIO_OFFSET)) {
                state->actor_id = BATTLE_ACTOR_MARIO;
            }
        } else {
            if (state == (BattlePartyHpPanelState *)(
                             gBattleContext +
                             BATTLE_PARTY_HP_PANEL_MARIO_OFFSET)) {
                state->actor_id = BATTLE_ACTOR_MARIO;
            }
            if (state == (BattlePartyHpPanelState *)(
                             gBattleContext +
                             BATTLE_PARTY_HP_PANEL_BABY_MARIO_OFFSET)) {
                state->actor_id = BATTLE_ACTOR_BABY_MARIO;
            }
        }

        if (((BattlePartyActor *)BattleActor_GetPartySlot(
                 BATTLE_ACTOR_LUIGI))->formation_index ==
            PARTY_MEMBER_BABY_LUIGI) {
            if (state == (BattlePartyHpPanelState *)(
                             gBattleContext +
                             BATTLE_PARTY_HP_PANEL_LUIGI_OFFSET)) {
                state->actor_id = BATTLE_ACTOR_BABY_LUIGI;
            }
            if (state == (BattlePartyHpPanelState *)(
                             gBattleContext +
                             BATTLE_PARTY_HP_PANEL_BABY_LUIGI_OFFSET)) {
                state->actor_id = BATTLE_ACTOR_LUIGI;
            }
        } else {
            if (state == (BattlePartyHpPanelState *)(
                             gBattleContext +
                             BATTLE_PARTY_HP_PANEL_LUIGI_OFFSET)) {
                state->actor_id = BATTLE_ACTOR_LUIGI;
            }
            if (state == (BattlePartyHpPanelState *)(
                             gBattleContext +
                             BATTLE_PARTY_HP_PANEL_BABY_LUIGI_OFFSET)) {
                state->actor_id = BATTLE_ACTOR_BABY_LUIGI;
            }
        }
    } else if (state == (BattlePartyHpPanelState *)(
                            gBattleContext +
                            BATTLE_PARTY_HP_PANEL_MARIO_OFFSET)) {
        state->actor_id = BATTLE_ACTOR_MARIO;
    } else if (state == (BattlePartyHpPanelState *)(
                            gBattleContext +
                            BATTLE_PARTY_HP_PANEL_LUIGI_OFFSET)) {
        state->actor_id = BATTLE_ACTOR_LUIGI;
    } else if (state == (BattlePartyHpPanelState *)(
                            gBattleContext +
                            BATTLE_PARTY_HP_PANEL_BABY_MARIO_OFFSET)) {
        state->actor_id = 0;
    } else if (state == (BattlePartyHpPanelState *)(
                            gBattleContext +
                            BATTLE_PARTY_HP_PANEL_BABY_LUIGI_OFFSET)) {
        state->actor_id = 0;
    }

    if (state->actor_id == 0) {
        goto close_panel;
    }

    current_hp = BattleActor_GetPartySlot(state->actor_id)->current_hp;
    if (state->target_hp != current_hp) {
        state->target_hp = current_hp;
        state->hp_transition_frames = 20;
    }
    if (state->hp_transition_frames > 0) {
        displayed_hp = state->displayed_hp;
        state->displayed_hp = displayed_hp +
            (state->target_hp - displayed_hp) /
            state->hp_transition_frames;
        state->hp_transition_frames--;
    } else {
        state->displayed_hp = state->target_hp;
        state->hp_transition_frames = 0;
    }

    if (BattleActor_IsHpAtMostQuarter(
            BattleActor_GetById(state->actor_id)) &&
        BattleActor_GetPartySlot(state->actor_id)->current_hp > 0) {
        if (state->low_hp_effect == 0) {
            state->low_hp_effect = BattleSpriteEffect_Spawn(
                506, -100, -100, 0, 256);
            *(u16 *)((u8 *)state->low_hp_effect + 0x14) |= 0x4000;
            *(u32 *)((u8 *)state->low_hp_effect + 0x24) = 0xA00;
        }
        return;
    }

    if (state->low_hp_effect != 0) {
        state->low_hp_effect->update_callback = 0;
        state->low_hp_effect = 0;
    }
    if (state->hp_counter_effect != 0) {
        state->hp_counter_effect->displayed_hp = state->displayed_hp;
    }
    return;

close_panel:
    if (state->low_hp_effect != 0) {
        state->low_hp_effect->update_callback = 0;
        state->low_hp_effect = 0;
    }
    if (state->hp_counter_effect != 0) {
        *(void **)((u8 *)state->hp_counter_effect + 4) = 0;
        state->hp_counter_effect = 0;
    }
}

void BattlePartyHpPanel_Draw(BattlePartyHpPanelState *state) {
    BattlePartyHpPanelTransform transform;
    int intensity;
    BattlePartyActor *actor;
    int actor_frame;
    BattleObjectDataLoadState *load_state;
    u8 *destination;
    u8 *transform_destination;
    u32 display_list_size;
    int aligned_x;
    int aligned_y;

    intensity = state->intensity;
    if (intensity < 1 || state->actor_id == 0) {
        return;
    }

    actor = (BattlePartyActor *)BattleActor_GetPartySlot(state->actor_id);
    switch (actor->formation_index) {
    case 0:
        actor_frame = 10;
        break;
    case 1:
        actor_frame = 11;
        break;
    case 2:
        actor_frame = 28;
        break;
    case 3:
        actor_frame = 29;
        break;
    case 4:
        actor_frame = 10;
        break;
    case 5:
        actor_frame = 11;
        break;
    default:
        return;
    }

    transform = gBattlePartyHpPanelTransformTemplate;
    transform.value.y = state->y << 8;
    if (state->active_layout == 2) {
        int current_x;
        int current_y;

        if (*(s16 *)(gSaveData + SAVE_PARTY_FORM_OFFSET) == 2 &&
            (u16)(actor->formation_index + 0xFFFE) <= 1) {
            transform.value.x += 27136;
        }
        BattleSprite_DrawFrame(
            actor_frame, intensity, &transform.value,
            (int)state->scene_object, 8, 3, 0x7FFF);
        /* Preserve the original independent field-update schedule. */
        asm {
            ldr r2, [sp, #0x3C]
            ldr r0, [sp, #0x44]
            ldr r1, [sp, #0x40]
            add r2, r2, #0xC00
            sub r0, r0, #2
            sub r1, r1, #0x800
            str r2, [sp, #0x3C]
            str r0, [sp, #0x44]
            str r1, [sp, #0x40]
        }

        if (state->hp_counter_effect != 0) {
            state->hp_counter_effect->y_offset = -8;
        } else {
            BattleNumber_DrawDecimal(
                state->displayed_hp, intensity, &transform.value,
                0, 0x7FFF, 0, 0);
        }

        current_x = transform.value.x;
        current_y = transform.value.y;
        if (state->displayed_hp > 99) {
            current_x -= 2048;
        } else if (state->displayed_hp > 9) {
            current_x -= 1024;
        }

        transform.value.x += 2048;
        transform.value.y += 2816;
        BattleNumber_DrawDecimal(
            actor->actor.max_hp, intensity, &transform.value,
            0, 0x7FFF, 42, 0);
        if (actor->actor.max_hp > 99) {
            current_x -= 2048;
        } else if (actor->actor.max_hp > 9) {
            current_x -= 1024;
        }

        /* Keep MWCC from folding each center into its source register. */
        asm {
            ldr r3, [sp, #0x3C]
            ldr r0, [sp, #0x40]
            add r1, current_x, r3
            sub r4, r3, #0x1800
            add r2, current_y, r0
            add r3, r0, #0x200
            add r1, r1, r1, lsr #31
            add r2, r2, r2, lsr #31
            str r4, [sp, #0x3C]
            mov r0, #3
            str r3, [sp, #0x40]
            mov aligned_x, r1, asr #1
            mov aligned_y, r2, asr #1
            bl BattleObjectData_GetLoadState
            mov load_state, r0
        }
        destination =
            ((BattlePartyHpPanelDisplayList *)load_state->data)->start;
        transform_destination = destination + 12;
        if ((u8 *)&transform < transform_destination) {
            func_0202cd2c(
                &transform, transform_destination, sizeof(transform));
        } else {
            func_0202cc58(
                &transform, transform_destination, sizeof(transform));
        }
        *(u32 *)(destination + 0x4C) =
            (*(u32 *)(destination + 0x4C) & 0xC0E0FFFF) |
            (*(u16 *)(gBattleContext +
                      BATTLE_PARTY_HP_PANEL_TEXTURE_FRAME_OFFSET) << 24) |
            (intensity << 16);
        display_list_size =
            ((BattlePartyHpPanelDisplayList *)load_state->data)->end -
            ((BattlePartyHpPanelDisplayList *)load_state->data)->start;
        DC_FlushRange(destination, display_list_size);
        MI_SendGXCommand(1, destination, display_list_size);
    } else {
        BattleSprite_DrawFrame(
            actor_frame, intensity, &transform.value,
            (int)state->scene_object, 8, 3, 0x7FFF);
        transform.value.x += 3072;
        transform.value.y -= 1024;
        transform.value.z -= 2;
        if (state->hp_counter_effect == 0) {
            BattleNumber_DrawDecimal(
                state->displayed_hp, intensity, &transform.value,
                0, 0x7FFF, 0, 0);
        }

        aligned_x = transform.value.x;
        aligned_y = transform.value.y;
        if (state->displayed_hp > 99) {
            aligned_x -= 2048;
        } else if (state->displayed_hp > 9) {
            aligned_x -= 1024;
        }
    }

    if (state->low_hp_effect != 0) {
        state->low_hp_effect->x =
            aligned_x / 256 + 2 +
            *(s16 *)(gBattleContext + BATTLE_VIEW_X_OFFSET);
        state->low_hp_effect->y =
            aligned_y / 256 +
            *(s16 *)(gBattleContext + BATTLE_VIEW_Y_OFFSET);
        state->low_hp_effect->z = 8;
    }
}

void BattlePartyHpPanel_Update(BattlePartyHpPanelState *state) {
    int intensity;

    if (state->scene_object->primary_model == 0) {
        return;
    }

    intensity = state->intensity;
    if (state->requested_layout != 0) {
        if (state->scene_object->primary_model->
                flag_bits.panel_animation_trigger != 0 &&
            state->scene_object->primary_model->animation_id == 0) {
            BattleSceneObject_SetAnimation(
                *(BattleSceneObject **)(gBattleContext +
                                         BATTLE_PARTY_HP_PANEL_OBJECT_OFFSET),
                1, -1);
        }

        if (intensity > 22) {
            intensity++;
        } else {
            intensity += 2;
        }
        if (intensity > 31) {
            intensity = 31;
        }
        state->intensity = intensity;
        state->scene_object->primary_model->animation_state_bits.state = 31;
        return;
    }

    if (intensity <= 0) {
        return;
    }
    intensity -= 3;
    if (intensity <= 0) {
        intensity = 0;
        BattleSceneObject_SetAnimation(state->scene_object, -1, -1);
    }
    state->scene_object->primary_model->animation_state_bits.state = intensity;
    state->intensity = state->scene_object->primary_model->
        animation_state_bits.state;
}
