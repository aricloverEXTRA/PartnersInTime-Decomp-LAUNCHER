/*
 * Rumble driver (ARM9 resident, 0x0202DE54-0x0202E384).
 *
 * The Rumble Pak driver: detection, the raw register write, and the alarm-driven
 * playback of a pulse pattern. Removal is detected and reported to whoever
 * registered a callback.
 */

#include <game/rumble.h>

extern int data_02060e08;
extern OsAlarm data_02060e0c;
extern GameRumbleState data_02060e38;
extern u32 data_02060e44[6];
extern u32 data_02060e5c[6];
extern u16 data_02060e84;
extern const volatile char data_0204b7ec[19];
extern void MIi_CpuClearFast(u32 value, void *destination, u32 size);

static inline u32 GameRumble_ToTicks(u32 milliseconds)
{
    return ((33514 * milliseconds) >> 6) / 10 - 19;
}

int GameRumble_InitCartridge(void)
{
    unsigned int i;
    const volatile char *marker = data_0204b7ec;
    int lock;
    for (i = 0; i < 19; ++i) *marker++;
    CTRDG_Init();
    lock = OS_GetLockID();
    if (lock == -3) return 0;
    data_02060e84 = lock;
    return CTRDG_IsOptionCartridge(2);
}

void GameRumble_Write(u16 value)
{
    OS_LockCartridge(data_02060e84);
    *(vu16 *)0x08001000 = value;
    OS_UnlockCartridge(data_02060e84);
}

int GameRumble_IsPresent(void)
{
    return CTRDG_IsOptionCartridge(2);
}

int GameRumble_Init(void)
{
    if (data_02060e08) return GameRumble_IsPresent();
    data_02060e08 = 1;
    if (OS_IsAlarmAvailable() && GameRumble_InitCartridge()) {
        volatile u32 zero;
        OS_CreateAlarm(&data_02060e0c);
        zero = 0;
        MIi_CpuClearFast(zero, &data_02060e38, sizeof(GameRumbleState));
        CTRDG_SetPulledOutCallback(GameRumble_Removed);
        return 1;
    }
    return 0;
}

void GameRumble_Start(const GameRumblePattern *pattern)
{
    u32 i;
    if (!GameRumble_IsPresent()) return;
    for (i = 0; i < pattern->count; ++i) {
        if (pattern->on[i] > 15) return;
    }
    for (i = 0; i < pattern->count - 1; ++i) {
        if (pattern->on[i] > pattern->off[i]) return;
    }
    if (pattern->rest < 15) return;
    GameRumble_Stop();
    data_02060e38.rest = GameRumble_ToTicks(pattern->rest);
    data_02060e38.repeats = pattern->repeats;
    data_02060e38.completed = 0;
    data_02060e38.step = 1;
    for (i = 0; i < 6; ++i) {
        data_02060e38.on[i] = GameRumble_ToTicks(pattern->on[i]);
        data_02060e38.off[i] = GameRumble_ToTicks(pattern->off[i]);
    }
    data_02060e38.last_step = 2 * pattern->count - 1;
    GameRumble_Write(2);
    OS_SetAlarm(&data_02060e0c, data_02060e44[data_02060e38.step >> 1], GameRumble_Alarm, 0);
    data_02060e38.active = 1;
}

void GameRumble_Stop(void)
{
    if (!data_02060e38.active) return;
    OS_CancelAlarm(&data_02060e0c);
    GameRumble_Write(0);
    data_02060e38.active = 0;
}

int GameRumble_IsActive(void)
{
    return data_02060e38.active;
}

void GameRumble_SkipPulse(void)
{
    if (!data_02060e38.active) return;
    if (!(data_02060e38.step & 1)) return;
    OS_CancelAlarm(&data_02060e0c);
    GameRumble_Alarm(0);
}

void GameRumble_SetRemovedCallback(void (*callback)(void))
{
    data_02060e38.removed = callback;
}

void GameRumble_Alarm(void *argument)
{
    if (!data_02060e38.step) {
        ++data_02060e38.completed;
        if (data_02060e38.repeats && data_02060e38.completed >= data_02060e38.repeats) {
            data_02060e38.active = 0;
            return;
        }
    }
    if (data_02060e38.step == data_02060e38.last_step) {
        GameRumble_Write(0);
        OS_SetAlarm(&data_02060e0c, data_02060e38.rest, GameRumble_Alarm, 0);
        data_02060e38.step = 0;
    } else if (data_02060e38.step & 1) {
        GameRumble_Write(0);
        OS_SetAlarm(&data_02060e0c, data_02060e5c[data_02060e38.step >> 1], GameRumble_Alarm, 0);
        ++data_02060e38.step;
    } else {
        GameRumble_Write(2);
        OS_SetAlarm(&data_02060e0c, data_02060e44[data_02060e38.step >> 1], GameRumble_Alarm, 0);
        ++data_02060e38.step;
    }
}

int GameRumble_Removed(int event)
{
    GameRumble_Stop();
    if (data_02060e38.removed) data_02060e38.removed();
    return 0;
}
