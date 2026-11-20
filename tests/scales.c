/* The KEYS page's scales, on the core alone. Run by tests/run_tests.sh.
 *
 * Each scale's notes are written out here a second time, apart from synth.c's masks, so a wrong bit in a mask
 * shows. For every scale and every ROOT, over every key and octave: a key plays a note of the scale, the nearest
 * at or below it, and every note of the scale is reached. The page offers all of them by name, a saved scale
 * keeps its number, and one past the end loads clamped. */
#include <stdio.h>
#include <string.h>
#include "../core/internal.h"
#include "../core/h1.h"

static int fails, checks;
#define CHECK(c, what) do { checks++; if (!(c)) { fails++; printf("FAIL line %d: %s\n", __LINE__, what); } } while (0)

static const struct { const char *name; int n; int pc[12]; } WANT[] = {
    {"CHROM", 12, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
    {"MAJOR", 7, {0, 2, 4, 5, 7, 9, 11}},
    {"MINOR", 7, {0, 2, 3, 5, 7, 8, 10}},
    {"PENTA", 5, {0, 2, 4, 7, 9}},
    {"MPENT", 5, {0, 3, 5, 7, 10}},
    {"BLUES", 6, {0, 3, 5, 6, 7, 10}},
    {"DORIAN", 7, {0, 2, 3, 5, 7, 9, 10}},
    {"WHOLE", 6, {0, 2, 4, 6, 8, 10}},
    {"MIXO", 7, {0, 2, 4, 5, 7, 9, 10}},
    {"LYDIAN", 7, {0, 2, 4, 6, 7, 9, 11}},
    {"PHRYG", 7, {0, 1, 3, 5, 7, 8, 10}},
    {"LOCRIA", 7, {0, 1, 3, 5, 6, 8, 10}},
    {"ARABIC", 7, {0, 1, 4, 5, 7, 8, 11}},
    {"HARMIN", 7, {0, 2, 3, 5, 7, 8, 11}},
    {"MELMIN", 7, {0, 2, 3, 5, 7, 9, 11}},
    {"HIRAJ", 5, {0, 2, 3, 7, 8}},
};
#define NWANT ((int)(sizeof WANT / sizeof WANT[0]))

int main(void)
{
    const ParamDef *p = page_param(PG_KEYS, 1);
    int s, root, note, i, ok_all = 1, reach_all = 1;
    static uint8_t blob[4096];
    uint32_t size = 0;

    h1_init(44100);
    CHECK(p->max + 1 == NWANT, "the page offers every scale");
    for (s = 0; s < NWANT && s <= p->max; s++)
        CHECK(!strcmp(p->opts[s], WANT[s].name), WANT[s].name);
    for (s = 0; s < NWANT; s++) {
        st.keys[1] = (uint8_t)s;
        for (root = 0; root < 12; root++) {
            uint32_t in = 0, reached = 0;
            st.keys[0] = (uint8_t)root;
            for (i = 0; i < WANT[s].n; i++)
                in |= 1u << WANT[s].pc[i];
            for (note = 53 - 24; note < 53 + 27 + 24; note++) {   /* the 27 keys, two octaves down and up */
                int got = scale_snap(note), pc = ((got - root) % 12 + 12) % 12, k;
                int nearest = 1;
                if (!(in >> pc & 1u) || got > note || note - got >= 12)
                    ok_all = 0;
                for (k = got + 1; k <= note; k++)                     /* nothing of the scale between */
                    if (in >> (((k - root) % 12 + 12) % 12) & 1u)
                        nearest = 0;
                if (!nearest)
                    ok_all = 0;
                reached |= 1u << pc;
            }
            if (reached != in) {
                reach_all = 0;
                printf("  %s on root %d: reached %03X, wants %03X\n", WANT[s].name, root, reached, in);
            }
        }
    }
    CHECK(ok_all, "every key plays a note of its scale, the nearest at or below it");
    CHECK(reach_all, "and every note of every scale is reached");

    st.keys[1] = 15;                                 /* HIRAJ, saved and loaded */
    {
        const void *sp = h1_state(&size);   /* (first: size is set by the call) */
        memcpy(blob, sp, size);
    }
    h1_init(44100);
    CHECK(h1_state_load(blob, size) == 0 && st.keys[1] == 15, "the last scale keeps its number through a save");
    blob[offsetof(H1State, keys) + 1] = 200;
    h1_init(44100);
    CHECK(h1_state_load(blob, size) == 0 && st.keys[1] <= 15, "one past the end loads clamped");

    printf("scales: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
