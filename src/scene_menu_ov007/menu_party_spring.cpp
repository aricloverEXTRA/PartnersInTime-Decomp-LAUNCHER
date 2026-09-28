/*
 * Party model spring update (overlay 7, 0x020769A4-0x02076E30).
 *
 * Selects the normal/low-HP animation, moves the pinned selection point,
 * applies requested impulses, then projects the relaxed chain onto the model.
 * Spring coordinates use Q12; the model angle wraps to sixteen bits.
 */

#include <game/battle_scene.h>
#include <game/menu_spring.h>
#include <game/pause_transition.h>
#include <game/pause_scene.h>
extern "C" {
#include <nitro/fx_atan.h>
#include <game/save_data.h>
#include <game/random.h>
}

struct SpringSaveView { u8 unknown_000[1016]; SavePartyMember members[4]; };
extern "C" {
extern MenuSpringChain data_ov007_020a67dc[4];
extern u8 data_ov007_020a67d8;
extern s8 data_ov007_020a6b8c[4];
extern PauseSceneTask *data_ov007_0208e1e0;
extern void *data_ov007_020a6b90;
BattleModel *Overlay5ResourceA_Get(void *);
int SceneController_IsObjectReady(void *);
void func_ov005_02069084(void *, u8);

void MenuSpring_UpdatePartyModel(MenuSpringTask *task)
{
    BattleModel *model = Overlay5ResourceA_Get(task);
    MenuSpringChain *chain = &data_ov007_020a67dc[task->chain_index];
    SavePartyMember *member = &((SpringSaveView *)gSaveData)->members[task->chain_index];
    if (100 * member->current_hp > 25 * member->max_hp)
        model->set_primary_animation((u8)task->chain_index, 0, 1);
    else
        model->set_primary_animation((u8)(task->chain_index + 4), 0, 1);
    if (!data_ov007_020a67d8)
        return;
    int x, y, width, height;
    int screen_x = task->x - 128;
    int screen_y = task->y - 192;
    int angle = PauseTransition_GetProgress(data_ov007_0208e1e0);
    PauseTransition_Project(screen_x, screen_y, angle, &x, &y, &width, &height);
    x += 128 << 12;
    y += 192 << 12;
    MenuSpringPoint *fixed = &chain->points[3];
    fixed->previous_y = y + 4096;
    fixed->y = fixed->previous_y;
    chain->minimum_y = y + 16384;
    MenuSpringPoint *selected = &chain->points[1];
    switch (chain->state) {
    case 0: // Resting or held selection; no transition step.
    case 3:
        break;
    case 1:
        selected->pinned = 1;
        if ((unsigned)task->chain_index >> 1) {
            chain->target_x = selected->previous_x - 32768;
            chain->target_y = selected->previous_y - 36864;
        } else {
            chain->target_x = selected->previous_x + 32768;
            chain->target_y = selected->previous_y - 32768;
        }
        task->counter = 8;
        chain->step_x = (chain->target_x - selected->previous_x) / task->counter;
        chain->step_y = (chain->target_y - selected->previous_y) / task->counter;
        ++chain->state;
        // The first motion step happens in the same update.
    case 2:
        selected->previous_x += chain->step_x;
        selected->previous_y += chain->step_y;
        if (!--task->counter) {
            selected->previous_x = chain->target_x;
            selected->previous_y = chain->target_y;
            ++chain->state;
        }
        break;
    case 4:
        selected->pinned = 0;
        chain->state = 0;
        break;
    }
    selected = &chain->points[0];
    // The byte wrap selects only active modes 1 and 2.
    if ((u8)(data_ov007_020a67d8 + 255) <= 1) {
        int amount;
        if (data_ov007_020a67d8 == 2)
            amount = (Random_NextModulo(4) + 4) << 12;
        else
            amount = (Random_NextModulo(20) << 12) / 40;
        if (Random_NextModulo(100) & 1)
            selected->x += amount;
        else
            selected->x -= amount;
    }
    int direction = data_ov007_020a6b8c[task->chain_index];
    if (direction) {
        if (direction == 2) {
            if (Random_NextModulo(100) & 1)
                direction = -1;
            else
                direction = 1;
        }
        int amount = ((Random_NextModulo(20) + 50) << 12) / 10;
        selected->x += direction * amount / 2;
        selected->y -= amount;
        data_ov007_020a6b8c[task->chain_index] = 0;
    }
    MenuSpring_Update(&data_ov007_020a67dc[task->chain_index], 10);
    screen_x = chain->points[0].x / 4096 - 128;
    screen_y = chain->points[0].y / 4096 - 192;
    angle = PauseTransition_GetProgress(data_ov007_0208e1e0);
    PauseTransition_Project(screen_x, screen_y, angle, &x, &y, &width, &height);
    x += 128 << 12;
    int chain_y = chain->points[0].y;
    model->animation_offset_x = x / 4096;
    model->animation_offset_y = chain_y / 4096;
    unsigned rotation = (FX_Atan2Idx(chain->points[1].y - chain->points[0].y,
        chain->points[1].x - chain->points[0].x) + 16384) & 0xFFFF;
    if (rotation < 640 || rotation > 64896)
        rotation = 0;
    task->angle = rotation;
    model->rotation_z = rotation;
    if (!SceneController_IsObjectReady(data_ov007_020a6b90))
        func_ov005_02069084(model, (u8)task->selector);
}
}
