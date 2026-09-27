#ifndef PIT_GAME_FIELD_NAVIGATION_H
#define PIT_GAME_FIELD_NAVIGATION_H
#include <game/field_entity.h>
#include <game/field_bounds_records.h>
typedef struct FieldAreaContext FieldAreaContext;

typedef struct FieldNavigationRecord {
    struct {
        u32 last : 1, unknown_01 : 1, type : 4, unknown_06_21 : 16, index : 10;
    } bits;
    u32 attributes;
    struct {
        s16 x, y, z, lower_z;
    } vertices[4];
} FieldNavigationRecord;
typedef char FieldNavigationRecord_SizeCheck[sizeof(FieldNavigationRecord) == 40 ? 1 : -1];
/* The packed boundary records follow surface_count navigation records. */
typedef struct FieldNavigationResource {
    u32 surface_count, boundary_count;
    FieldNavigationRecord records[1];
} FieldNavigationResource;
typedef char FieldNavigationResource_SizeCheck[sizeof(FieldNavigationResource) == 48 ? 1 : -1];
typedef struct FieldNavigationAdjustment {
    struct {
        u32 last : 1, index : 10, unknown_11_31 : 21;
    } bits;
    s16 delta_z[3], height;
    u32 attributes;
} FieldNavigationAdjustment;
typedef char FieldNavigationAdjustment_SizeCheck[sizeof(FieldNavigationAdjustment) == 16 ? 1 : -1];
typedef struct FieldNavigationChangeManager FieldNavigationChangeManager;
typedef struct FieldNavigationChangeData {
    struct {
        u32 unknown_00_01 : 2, mode : 2, unknown_04_31 : 28;
    } bits;
} FieldNavigationChangeData;
typedef char FieldNavigationChangeData_SizeCheck[sizeof(FieldNavigationChangeData) == 4 ? 1 : -1];
typedef struct FieldNavigationChangeEntry {
    const FieldNavigationChangeData *data;
    u32 unknown_04;
    u8 state, unknown_09[3];
} FieldNavigationChangeEntry;
typedef char FieldNavigationChangeEntry_SizeCheck[sizeof(FieldNavigationChangeEntry) == 12 ? 1 : -1];
typedef struct FieldNavigationChangeVTable {
    void *unknown_00[11];
    const FieldNavigationResource *(*get_navigation)(FieldNavigationChangeManager *);
    const FieldPairedBoundsRecord *(*get_paired_bounds)(FieldNavigationChangeManager *);
    void *unknown_34;
    const FieldNavigationAdjustment *(*get_adjustments)(FieldNavigationChangeManager *, u16);
    void *unknown_3c[2];
    void (*restore_change)(FieldNavigationChangeManager *, u16);
} FieldNavigationChangeVTable;
typedef char FieldNavigationChangeVTable_SizeCheck[sizeof(FieldNavigationChangeVTable) == 72 ? 1 : -1];
struct FieldNavigationChangeManager {
#ifdef __cplusplus
    virtual void unknown_00();
    virtual void unknown_04();
    virtual void unknown_08();
    virtual void unknown_0c();
    virtual void unknown_10();
    virtual void unknown_14();
    virtual void unknown_18();
    virtual void unknown_1c();
    virtual void unknown_20();
    virtual void unknown_24();
    virtual void unknown_28();
    virtual const FieldNavigationResource *get_navigation();
    virtual const FieldPairedBoundsRecord *get_paired_bounds();
    virtual void unknown_34();
    virtual const FieldNavigationAdjustment *get_adjustments(u16);
    virtual void unknown_3c();
    virtual void unknown_40();
    virtual void restore_change(u16);
#else
    const FieldNavigationChangeVTable *vtable;
#endif
    u8 unknown_004[1636];
    FieldNavigationChangeEntry entries[24];
    u8 count, unknown_789[3];
};
typedef char FieldNavigationChangeManager_SizeCheck[sizeof(FieldNavigationChangeManager) == 1932 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void FieldNavigation_ResolveCornerOverlap(FieldRuntimeEntity *entity, fx32 *corner_x,
                                          fx32 *corner_y, int iterations);
/* Native query combines blocked corners with corners outside all surfaces. */
int func_ov000_020aa5b0(FieldRuntimeEntity *entity, fx32 *corner_x, fx32 *corner_y);
fx32 FieldNavigation_GetMaximumHeight(const FieldRuntimeEntity *entity, const FieldNavigationSurface *surface);
/* Native height sampler: only the low byte of the attribute-check flag is read. */
void func_ov000_020bdc88(const FieldNavigationSurface *surface, fx32 x, fx32 y,
                       fx32 *height, fx32 *lower_height, int check_attributes);
void FieldArea_ApplyNavigationChange(FieldAreaContext *area, int index);
void FieldArea_RestoreNavigationChanges(FieldAreaContext *area);
void FieldArea_SaveNavigationChanges(FieldAreaContext *area);
void FieldArea_UpdateNavigationBounds(FieldAreaContext *area, FieldNavigationSurface *surface);
void FieldArea_ExpandNavigationRecords(FieldAreaContext *area, const FieldNavigationRecord *record,
                                       FieldNavigationSurface *surface);
void FieldArea_AllocateNavigationRecords(FieldAreaContext *area);
#ifdef __cplusplus
}
#endif
#endif
