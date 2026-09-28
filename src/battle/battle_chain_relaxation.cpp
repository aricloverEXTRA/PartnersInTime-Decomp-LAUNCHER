/*
 * Chain relaxation (overlay 2, 0x020B710C-0x020B7448).
 *
 * Smooths each segment toward its predecessor, redistributes cumulative drift,
 * and normalizes its Q8 vector to the requested length. Signed divisions
 * truncate toward zero; only the final vector and displacement stores narrow.
 */

#include <game/overlay025_effect_task.h>
#include <hardware.h>

extern "C" void BattleChain_RelaxSegments(Overlay25ChainJoint *joint,
    int count, int segment_length, int smoothing_q12)
{
    int previous_x = joint->x;
    int previous_y = joint->y;
    int previous_z = joint->z;
    int input_x = 0, input_y = 0, input_z = 0;
    int output_x = 0, output_y = 0, output_z = 0;

    for (int i = 0; i < count; ++i, ++joint) {
        int x = joint->x;
        int y = joint->y;
        int z = joint->z;
        int delta_x = previous_x - x;
        int delta_y = previous_y - y;
        int delta_z = previous_z - z;
        input_x += x;
        input_y += y;
        input_z += z;
        int squared_distance = delta_x * delta_x + delta_y * delta_y + delta_z * delta_z;
        // The allowed bend decreases along the remaining chain.
        int radius = (segment_length * (count - i)) * 4 / count;
        int correction_x, correction_y, correction_z;
        if (radius * radius < squared_distance) {
            *rSQRTCNT = SQRTCNT_MODE_32;
            *rSQRT_PARAM_L = squared_distance;
            while (*rSQRTCNT & SQRTCNTF_BUSY) {}
            int distance = *rSQRT_RESULT;
            int excess = distance - radius;
            correction_x = delta_x * excess / distance;
            correction_y = delta_y * excess / distance;
            correction_z = delta_z * excess / distance;
        } else {
            correction_x = 0;
            correction_y = 0;
            correction_z = 0;
        }
        // Preserve this grouping: correction precedes cumulative drift.
        previous_x = (input_x - (output_x + x)) / (i + 1) + (x + correction_x * smoothing_q12 / 4096);
        previous_y = (input_y - (output_y + y)) / (i + 1) + (y + correction_y * smoothing_q12 / 4096);
        previous_z = (input_z - (output_z + z)) / (i + 1) + (z + correction_z * smoothing_q12 / 4096);
        *rSQRTCNT = SQRTCNT_MODE_32;
        *rSQRT_PARAM_L = previous_x * previous_x + previous_y * previous_y + previous_z * previous_z;
        while (*rSQRTCNT & SQRTCNTF_BUSY) {}
        int length = *rSQRT_RESULT;
        if (length) {
            previous_x = (previous_x * segment_length << 8) / length;
            previous_y = (previous_y * segment_length << 8) / length;
            previous_z = (previous_z * segment_length << 8) / length;
        }
        // Keep full-width values for the next segment and running totals.
        joint->x = previous_x;
        output_x += previous_x;
        joint->y = previous_y;
        output_y += previous_y;
        output_z += previous_z;
        joint->z = previous_z;
        joint->displacement_x = (input_x - output_x) / 256;
        joint->displacement_y = (input_y - output_y) / 256;
        joint->displacement_z = (input_z - output_z) / 256;
    }
}
