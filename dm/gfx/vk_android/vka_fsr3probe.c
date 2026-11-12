#include "vka.h"
// FSR 3.1 depth feasibility probe (DM_VK_FSR3_DEPTH=1, off by default): at each render pass end that had a depth
// attachment, record what vka saw (format, size, layers) and log a few samples. This works out whether a regular
// game's scene depth is reachable to feed vka_fsr3.c (the temporal upscaler needs depth + motion); it changes
// nothing else -- no copies, no image ownership, no rendering. 0 = the probe is off, the game runs exactly as before.
// The result: "FSR3 depth probe: pass N had a depth attachment fmt=X WxH layers=L (capturable)" lines in the log.
static _Atomic int g_fsr3_probe_seen;   // a pass with a depth attachment was seen (any)
static uint32_t g_fsr3_probe_w, g_fsr3_probe_h;   // last seen depth size
static _Atomic uint64_t g_fsr3_probe_n;
int vka_fsr3_probe_note(VkFormat df, uint32_t w, uint32_t h, uint32_t nl) {
    if (!VKA_ENV("DM_VK_FSR3_DEPTH", 0)) return 0;
    atomic_store(&g_fsr3_probe_seen, 1);
    g_fsr3_probe_w = w; g_fsr3_probe_h = h;
    uint64_t n = atomic_fetch_add(&g_fsr3_probe_n, 1);
    if (n < 6) VLOG("FSR3 depth probe: pass %llu had a depth attachment fmt=%d %ux%u layers=%u (capturable)", (unsigned long long)n, (int)df, w, h, (unsigned)nl);
    if (n == 6) VLOG("FSR3 depth probe: more passes with depth follow (log throttled)");
    return 1;
}
// NDSR mode 5 consults the probe: depth reachable or not (vka_draw.c). 0 = the probe never saw depth
int vka_fsr3_probe_depth_ok(void) { return atomic_load_explicit(&g_fsr3_probe_seen, memory_order_relaxed); }
uint32_t vka_fsr3_probe_depth_w(void) { return g_fsr3_probe_w; }
uint32_t vka_fsr3_probe_depth_h(void) { return g_fsr3_probe_h; }
