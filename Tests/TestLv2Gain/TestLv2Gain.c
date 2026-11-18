// A small LV2 effect for the LV2 hosting tests: a stereo gain with one control, the gain as a
// factor (0 to 2, 1 by default). It is built into TestBundles/TorqwaveTestGain.lv2.
#include <stdint.h>
#include <stdlib.h>
#include <lv2/core/lv2.h>

#define TEST_GAIN_URI "urn:torqwave:test:gain"

enum { portGain, portInLeft, portInRight, portOutLeft, portOutRight, portCount };

typedef struct
{
    const float* ports[portCount];
} Gain;

static LV2_Handle instantiate (const LV2_Descriptor* d, double rate, const char* path, const LV2_Feature* const* features)
{
    (void) d; (void) rate; (void) path; (void) features;
    return calloc (1, sizeof (Gain));
}

static void connect_port (LV2_Handle h, uint32_t port, void* data)
{
    if (port < portCount)
        ((Gain*) h)->ports[port] = (const float*) data;
}

static void run (LV2_Handle h, uint32_t frames)
{
    Gain* g = (Gain*) h;
    const float gain = g->ports[portGain] != NULL ? *g->ports[portGain] : 1.0f;

    for (int c = 0; c < 2; ++c)
    {
        const float* in = g->ports[portInLeft + c];
        float* out = (float*) g->ports[portOutLeft + c];

        if (in == NULL || out == NULL)
            continue;

        for (uint32_t i = 0; i < frames; ++i)
            out[i] = in[i] * gain;
    }
}

static void cleanup (LV2_Handle h)
{
    free (h);
}

static const LV2_Descriptor descriptor = { TEST_GAIN_URI, instantiate, connect_port, NULL, run, NULL, cleanup, NULL };

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor (uint32_t index)
{
    return index == 0 ? &descriptor : NULL;
}
