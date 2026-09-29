/*
 * Room transition (overlay 0, 0x0207F4D0-0x020802E0).
 *
 * Changing rooms: the fade, the load of the next room's data and the placement
 * of the party at the arrival point.
 */

#include <game/field_area.h>
#include <game/field_party_manager.h>
#include <game/field_system.h>
#include <game/field_timer.h>
#include <game/field_variable_entity.h>
#include <game/field_room_transition.h>
extern "C" {
#include <game/audio.h>
#include <game/save_data.h>
#include <game/rumble.h>
void func_ov000_0209ef28(FieldPartyManager *);
extern u8 data_0205a00c;
void func_ov000_0208b208(FieldPartyController *, int);

void FieldArea_QueuePartyRoomChange(FieldAreaContext *field, int side, int room, s16 x, s16 y, s16 z,
                                    int direction, u8 lock_control, s16 script, int bgm, u8 fade,
                                    u8 synchronize, s16 paired_script, u8 paired_fade)
{
    if (!field->party->parties[side].leader->bits.movement_mode) {
        if (field->party->parties[side].leader->entity.locomotion_state == 52 ||
            field->party->parties[side].follower->entity.locomotion_state == 52) {
            FieldParty_ResetActionState(&field->party->parties[0], 1, 0);
            FieldParty_ResetActionState(&field->party->parties[1], 1, 0);
            FieldPartyManager_BeginReunion(field->party, 1, 1);
            side = 0;
        } else if (field->party->parties[side].leader->entity.locomotion_state >= 24 &&
                   field->party->parties[side].leader->entity.locomotion_state <= 27) {
            FieldParty_EndHammerSwing(&field->party->parties[side], 0);
        }
    }
    u16 state = field->party->parties[side].leader->entity.locomotion_state;
    if (state >= 81 && state <= 87) {
        if (data_0205a00c)
            GameRumble_Stop();
        field->party->parties[1].leader->state_record->resources.flags.unknown_20_21 = 0;
        field->party->parties[1].follower->state_record->resources.flags.unknown_20_21 = 0;
        field->party->parties[1].leader->state_record->resources.flags.unknown_01_14 = 0x1a4;
        field->party->parties[1].follower->state_record->resources.flags.unknown_01_14 = 0x1a4;
        func_ov000_0208b208(&field->party->parties[1], 0);
        FieldParty_RestoreBrosBallRoll(&field->party->parties[0]);
    } else if (state >= 89 && state <= 90) {
        FieldParty_ResetActionState(&field->party->parties[0], 1, 0);
        FieldParty_ResetActionState(&field->party->parties[1], 1, 0);
    }
    int mode = field->party->parties[side].leader->bits.movement_mode;
    if (mode == 6) {
        field->transition.flags.pending = 1;
        field->transition.flags.fading = 0;
        field->transition.flags.party_mask = 3;
        field->transition.flags.control_locked = lock_control;
        field->transition.room = room;
        field->transition.script = script;
        field->transition.flags.bgm = bgm;
        /* Piggyback mode moves both groups to the same arrival position. */
        field->transition.flags.movement_modes = 0x66;
        field->transition.flags.direction_0 = direction;
        field->transition.x[0] = x;
        field->transition.y[0] = y;
        field->transition.z[0] = z;
        field->transition.flags.direction_1 = direction;
        field->transition.x[1] = x;
        field->transition.y[1] = y;
        field->transition.z[1] = z;
    } else {
        field->transition.flags.pending = 1;
        field->transition.flags.fading = 0;
        field->transition.flags.party_mask = 1 << side;
        field->transition.flags.control_locked = lock_control;
        field->transition.room = room;
        field->transition.script = script;
        field->transition.flags.bgm = bgm;
        field->transition.flags.movement_modes = mode << (4 * side);
        if (!side) {
            field->transition.flags.direction_0 = direction;
            field->transition.x[0] = x;
            field->transition.y[0] = y;
            field->transition.z[0] = z;
        } else {
            field->transition.flags.direction_1 = direction;
            field->transition.x[1] = x;
            field->transition.y[1] = y;
            field->transition.z[1] = z;
        }
        FieldPartyManager_CheckRegion71Bounds(field->party, side ^ 1);
    }
    if (fade) {
        field->transition.flags.fading = 1;
        FieldArea_StartBrightness(field, 255, -16, 16);
        FieldParty_AlignMovementToFacing(&field->party->parties[side]);
    }
    /* A visible paired room receives its own request before either screen reloads. */
    int paired_room = data_02048f18[room].paired_room;
    if (synchronize && !field->flags.screen && paired_room) {
        FieldAreaContext *paired = field->paired_area;
        if (paired) {
            int current_room = paired->room_id;
            if (data_02048f18[current_room].type == 2 && current_room != paired_room &&
                paired->unknown_23f0 == 71 && !paired->transition.flags.fading) {
                bool visible = !paired->unknown_2402.unknown_00 && paired->brightness > -4096 &&
                               paired->brightness < 4096;
                FieldArea_QueueRoomChange(paired, paired_room, paired_script, -3,
                                          (paired_fade & visible) != 0);
            }
        }
    }
}

void FieldArea_QueueBothPartyRoomChange(FieldAreaContext *field, int room, s16 x0, s16 y0, s16 z0,
                                        int direction0, s16 x1, s16 y1, s16 z1, int direction1, s16 script,
                                        int bgm, u8 fade, u8 synchronize, s16 paired_script, u8 paired_fade)
{
    field->transition.flags.pending = 1;
    field->transition.flags.fading = 0;
    field->transition.flags.party_mask = 3;
    field->transition.flags.control_locked = 0;
    field->transition.room = room;
    field->transition.script = script;
    field->transition.flags.bgm = bgm;
    field->transition.flags.movement_modes = field->party->parties[0].leader->bits.movement_mode |
                                             (field->party->parties[1].leader->bits.movement_mode << 4);
    field->transition.flags.direction_0 = direction0;
    field->transition.x[0] = x0;
    field->transition.y[0] = y0;
    field->transition.z[0] = z0;
    field->transition.flags.direction_1 = direction1;
    field->transition.x[1] = x1;
    field->transition.y[1] = y1;
    field->transition.z[1] = z1;
    if (fade) {
        field->transition.flags.fading = 1;
        FieldArea_StartBrightness(field, 255, -16, 16);
    }
    /* A visible paired room receives its own request before either screen reloads. */
    int paired_room = data_02048f18[room].paired_room;
    if (synchronize && !field->flags.screen && paired_room) {
        FieldAreaContext *paired = field->paired_area;
        if (paired) {
            int current_room = paired->room_id;
            if (data_02048f18[current_room].type == 2 && current_room != paired_room &&
                paired->unknown_23f0 == 71 && !paired->transition.flags.fading) {
                bool visible = !paired->unknown_2402.unknown_00 && paired->brightness > -4096 &&
                               paired->brightness < 4096;
                FieldArea_QueueRoomChange(paired, paired_room, paired_script, -3,
                                          (paired_fade & visible) != 0);
            }
        }
    }
}

void FieldArea_QueueRoomChange(FieldAreaContext *field, int room, s16 script, int bgm, u8 fade)
{
    field->transition.flags.pending = 1;
    field->transition.flags.fading = 0;
    field->transition.flags.party_mask = 0;
    field->transition.flags.control_locked = 0;
    field->transition.room = room;
    field->transition.script = script;
    field->transition.flags.bgm = bgm;
    if (fade) {
        field->transition.flags.fading = 1;
        FieldArea_StartBrightness(field, 255, -16, 16);
    }
}

void FieldArea_PrepareRoomDeparture(FieldAreaContext *field)
{
    FieldTimer *timer;
    int i;
    field->unknown_23f0 = 72;
    timer = field->timer;
    if (timer && timer->flags.screen == field->flags.screen && (timer->flags.allocated || timer->step) &&
        !timer->flags.unknown_3) {
        FieldTimer_SetVisible(field->timer, 0);
        timer->step = 0;
    }
    if (!field->flags.screen) {
        FieldSystem_ReleaseMaskErase(field->system);
        FieldSystem *system = field->system;
        system->touch.enabled = 0;
        system->touch.state = 0;
        system->touch.x = -1;
        system->touch.y = -1;
    }
    if (field->flags.screen == 1 && field->party->pending_bounds.active) {
        func_ov000_0209ef28(field->party);
        field->party->pending_bounds.active = 0;
    }
    field->transition.flags.fading = 0;
    for (i = 0; i < 2; ++i) {
        if (field->room_id == field->party->parties[i].flags.movement_state &&
            field->flags.screen == field->party->parties[i].flags.field_screen) {
            if (!(field->transition.flags.party_mask & (1 << i))) {
                if (!field->party->parties[i].backup.active) {
                    field->party->parties[i].backup.active = 1;
                    field->party->parties[i].backup.movement_state = field->room_id;
                    field->party->parties[i].backup.unknown_11 = 0;
                    field->party->parties[i].backup.snapshot_backed = 0;
                    FieldPartyEntity_CopyState(&field->party->parties[i].backup_entities[0],
                                               field->party->parties[i].leader);
                    FieldPartyEntity_CopyState(&field->party->parties[i].backup_entities[1],
                                               field->party->parties[i].follower);
                }
            }
            field->party->parties[i].leader = 0;
            field->party->parties[i].follower = 0;
        }
    }
    for (i = field->entity_count - field->variable_count; i < field->entity_count; ++i) {
        FieldVariableEntity *entity = (FieldVariableEntity *)field->entities[i];
        if (entity->state.flags.mode && entity->state.motion.countdown)
            VM_WriteVariable(entity->entity.unknown_3d4, 1, 0, 0);
    }
    for (i = 0; i < 4; ++i) {
        u16 sound = field->party->tracked_sounds[i];
        if (field->flags.screen == ((sound >> 15) & 1)) {
            GameAudio_StopEffect((s16)(sound & 0x7fff));
            field->party->tracked_sounds[i] = 0;
        }
    }
    if (field->unknown_2b90_bits.state) {
        field->unknown_2b90_bits.state = 0;
        if (field->transition.flags.party_mask & (1 << field->party->bits.active_party)) {
            field->flags.unknown_15 = 1;
            field->area_value = 2 * field->party->bits.active_party;
        }
    }
    GameAudio_StopEffect(312);
    GameAudio_StopEffect(216);
    GameAudio_StopEffect(216);
    FieldArea_ReleaseRoomResources(field);
    field->state_bits.unknown_06 = 0;
    field->state_bits.unknown_05 = 0;
    field->room_id = field->transition.room;
    if (!field->flags.screen && !((FieldMusicActivationFlags *)(gSaveData + 1300))->disabled) {
        int stop = 0;
        int bgm = field->transition.flags.bgm;
        switch (bgm) {
        case 0:
            stop = 1;
            break;
        case -1:
            stop = 1;
            break;
        case -3: {
            int music = data_02048f18[field->room_id].music;
            if (!music || music != field->party->bits.bgm)
                stop = 1;
            break;
        }
        case -2:
            break;
        default:
            if (bgm != field->party->bits.bgm)
                stop = 1;
            break;
        }
        if (stop)
            GameAudio_StopMusic(90);
    }
    field->unknown_23f0 = 0;
}
}
