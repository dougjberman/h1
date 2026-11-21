/* The MIDI IN page (SELECT from the MIDI page): an incoming clock followed, its Start and Stop, the notes taken.
 * On the core alone, run by tests/run_tests.sh.
 *
 *   the page     SELECT from the MIDI page reaches MIDI IN; its four columns live in st.seq[7]
 *   CLOCK        off: a clock changes nothing. FOLLOW: the tempo is the clock's, to a tenth of a BPM, at 72, 120.5
 *                and 200, also with every clock read up to a frame late (the age says how late) and with 2 ms of
 *                jitter where it does not; GLOBAL's tempo is left as it was; half a second without a clock and
 *                h1 is on its own tempo again
 *   START        a Start: the step pattern starts on the next clock, and every step lands on its clock (six to a
 *                sixteenth) to within 2 ms, still so after two minutes against a GLOBAL tempo far from the clock's;
 *                a Stop stops it; START off: a Start does nothing
 *   NOTES        ALL, one channel, OFF
 *   clock out    none of h1's own while it follows (THRU, in app.c, passes the one it follows: tests/clockin.txt)
 *   the state    the page through a save; 0.3.0's zero is everything as it was; any byte loads clamped */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../core/internal.h"
#include "../core/h1.h"

static uint32_t ms, frames;
static int fails, checks;
#define CHECK(c, what) do { checks++; if (!(c)) { fails++; printf("FAIL line %d: %s\n", __LINE__, what); } } while (0)

#define NMO 8192
static uint8_t mo[NMO][3];
static uint32_t mo_at[NMO];
static int nmo;
static void mo_fn(uint8_t s, uint8_t a, uint8_t b)
{
    if (nmo < NMO) {
        mo[nmo][0] = s;
        mo[nmo][1] = a;
        mo[nmo][2] = b;
        mo_at[nmo] = frames;
        nmo++;
    }
}
static int count(uint8_t s)
{
    int i, n = 0;
    for (i = 0; i < nmo; i++)
        n += mo[i][0] == s;
    return n;
}

/* the audio a control block at a time, a frame every 17 ms */
static uint32_t next_frame;
static void run_to(uint32_t f)
{
    static int32_t out[2 * CTRL];
    while (frames + CTRL <= f) {
        h1_render(out, CTRL);
        frames += CTRL;
        if (frames >= next_frame) {
            h1_frame(ms += 17);
            next_frame = frames + 750u;
        }
    }
}
static void go(int msec) { run_to(frames + (uint32_t)msec * 441u / 10u); }
static void tap(int b) { h1_button(b, 1); go(40); h1_button(b, 0); go(40); }
static void hold(int b) { h1_button(b, 1); go(900); h1_button(b, 0); go(40); }

static uint32_t rng = 12345u;
static double rnd(void) { rng = rng * 1664525u + 1013904223u; return (double)(rng >> 8) / 16777216.0; }

/* clocks at bpm for secs: each lands at its exact time; it is read up to late_ms later (a frame's wait), and the
 * age passed says so if aged, else the reading is off by up to jitter_ms either way */
static double clk_t;                             /* the next clock's landing time, frames */
static void clocks(double bpm, double secs, double late_ms, int aged, double jitter_ms)
{
    double step = 44100.0 * 60.0 / bpm / 24.0, end = clk_t + secs * 44100.0;
    while (clk_t < end) {
        double late = late_ms > 0 ? rnd() * late_ms * 44.1 : 0.0;
        double jit = jitter_ms > 0 ? (rnd() * 2.0 - 1.0) * jitter_ms * 44.1 : 0.0;
        double read = clk_t + late + (aged ? 0.0 : jit);
        if (read < frames) read = frames;
        run_to((uint32_t)read);
        h1_midi_at(0xF8, 0, 0, aged ? (uint32_t)((frames - clk_t) / 44.1 * 1000.0 > 0 ? (frames - clk_t) / 44.1 * 1000.0 : 0) : 0u);
        clk_t += step;
    }
}
static void rt(uint8_t s) { run_to((uint32_t)clk_t - 40u); h1_midi_at(s, 0, 0, 0u); }
static int near(double a, double b, double d) { return a > b - d && a < b + d; }

static void midi_in_page(void)
{
    if (ui_page() != PG_MIDI && ui_page() != PG_MIDI_IN)
        hold(H1_BTN_HOME);
    if (ui_page() == PG_MIDI)
        h1_encoder(H1_ENC_SELECT, 1);
}

int main(void)
{
    int i, a, b, n, worst_i = 0;
    double worst = 0.0, first_off = 0.0, step;
    static uint8_t blob[4096];
    uint32_t size = 0;

    h1_init(44100);
    h1_midi_out(mo_fn);
    go(300);
    clk_t = frames;

    /* ---- the page ---- */
    hold(H1_BTN_HOME);
    h1_encoder(H1_ENC_SELECT, 1);
    go(100);
    CHECK(ui_page() == PG_MIDI_IN, "SELECT from the MIDI page: MIDI IN");
    h1_encoder(H1_ENC_SELECT, -1);
    go(100);
    CHECK(ui_page() == PG_MIDI, "and back");
    tap(H1_BTN_HOME);

    /* ---- CLOCK off: nothing changes ---- */
    clk_t = frames;
    clocks(150.0, 2.0, 0, 0, 0);
    CHECK(eng_ext_bpm() == 0.0f, "CLOCK OFF: a clock is not followed");

    /* ---- FOLLOW ---- */
    midi_in_page();
    h1_control_set(H1_ENC_K1, 1);
    CHECK(MI_FOLLOW() && st.seq[7] == 1, "CLOCK FOLLOW, kept in st.seq[7]");
    tap(H1_BTN_HOME);
    st.glo[0] = 90;
    clk_t = frames;
    clocks(72.0, 4.0, 0, 1, 0);
    printf("  72 BPM: followed %.2f\n", eng_ext_bpm());
    CHECK(near(eng_ext_bpm(), 72.0, 0.1), "72 BPM followed to a tenth");
    clocks(120.5, 4.0, 20.0, 1, 0);
    printf("  120.5 BPM read up to 20 ms late, aged: %.2f\n", eng_ext_bpm());
    CHECK(near(eng_ext_bpm(), 120.5, 0.1), "120.5 BPM, every clock read up to a frame late: to a tenth");
    clocks(200.0, 4.0, 0, 0, 2.0);
    printf("  200 BPM with 2 ms of jitter: %.2f\n", eng_ext_bpm());
    CHECK(near(eng_ext_bpm(), 200.0, 0.5), "200 BPM with 2 ms of jitter: to half a BPM");
    CHECK(st.glo[0] == 90, "GLOBAL's tempo left as it was");
    go(700);
    CHECK(eng_ext_bpm() == 0.0f, "half a second without a clock: h1's own tempo again");

    /* ---- clock out: none of h1's own while following ---- */
    st.midi[2] = MC_CLOCK;                       /* (the MIDI page's CLOCK ON) */
    st.midi[1] = MS_ALL;
    nmo = 0;
    clk_t = frames;
    clocks(120.0, 2.0, 10.0, 1, 0);
    CHECK(count(0xF8) == 0, "following: h1 sends no clock of its own");
    st.midi[2] = MC_OFF;

    /* ---- START: the steps from the top on the next clock, each on its clock ---- */
    midi_in_page();
    h1_control_set(H1_ENC_K2, 1);
    tap(H1_BTN_HOME);
    st.seq[0] = SEQ_STEP;                         /* a pattern of sixteen sixteenths, a note on each */
    st.seq[1] = 0;
    st.seq[4] = 1;
    for (i = 0; i < NSTEP; i++)
        for (a = 0; a < NSNOTE; a++)
            st.steps[i][a] = 0;
    for (i = 0; i < 16; i++)
        st.steps[i][0] = (uint8_t)(61 + (i & 7));
    st.glo[0] = 90;                               /* GLOBAL far from the clock's */
    clk_t = frames;
    clocks(127.0, 1.0, 10.0, 1, 0);
    nmo = 0;
    rt(0xFA);                                     /* Start, then the clock that is beat one */
    {
        double first = clk_t;
        step = 44100.0 * 60.0 / 127.0 / 24.0 * 6.0;   /* a sixteenth: six clocks */
        clocks(127.0, 120.0, 15.0, 1, 0);
        n = 0;
        for (i = 0; i < nmo; i++)
            if ((mo[i][0] & 0xF0u) == 0x90u) {
                double want = first + step * n, d = fabs((double)mo_at[i] - want) / 44.1;
                if (n == 0)
                    first_off = d;               /* (the first: played when the Start's clock is read, up to 15 ms on) */
                else if (d > worst) { worst = d; worst_i = n; }
                n++;
            }
        printf("  two minutes of steps at 127 BPM against GLOBAL's 90, every clock read up to 15 ms late: %d steps, the "
               "first %.2f ms off, after it the farthest %.2f ms (step %d)\n", n, first_off, worst, worst_i);
        CHECK(n >= 1000, "the steps ran for two minutes");
        CHECK(first_off <= 15.5, "the first step as soon as the Start's clock is read");
        CHECK(worst <= 2.0, "every step after it on its clock to within 2 ms, to the last: no drift");
    }
    a = count(0x90);
    rt(0xFC);                                     /* Stop */
    clocks(127.0, 1.0, 0, 1, 0);
    CHECK(count(0x90) == a, "Stop: the steps stop");
    midi_in_page();
    h1_control_set(H1_ENC_K2, 0);
    tap(H1_BTN_HOME);
    rt(0xFA);
    clocks(127.0, 1.0, 0, 1, 0);
    CHECK(count(0x90) == a, "START OFF: a Start does nothing");

    /* ---- NOTES ---- */
    st.seq[0] = SEQ_LOOP;
    go(500);
    nmo = 0;
    b = h1_voices_active();
    h1_midi_at(0x90, 60, 100, 0);
    go(50);
    CHECK(h1_voices_active() > b, "NOTES ALL: a note plays");
    h1_midi_at(0x80, 60, 0, 0);
    go(400);
    midi_in_page();
    h1_control_set(H1_ENC_K3, MI_NOTES_OFF);
    tap(H1_BTN_HOME);
    b = h1_voices_active();
    h1_midi_at(0x90, 62, 100, 0);
    h1_midi_at(0xB0, 20, 0, 0);
    go(50);
    CHECK(h1_voices_active() == b, "NOTES OFF: no note plays");
    h1_midi_at(0x80, 62, 0, 0);
    midi_in_page();
    h1_control_set(H1_ENC_K3, 3);                 /* channel 3 */
    tap(H1_BTN_HOME);
    h1_midi_at(0x90, 64, 100, 0);
    go(50);
    CHECK(h1_voices_active() == b, "NOTES 3: a note on channel 1 does not play");
    h1_midi_at(0x80, 64, 0, 0);
    h1_midi_at(0x92, 65, 100, 0);
    go(50);
    CHECK(h1_voices_active() > b, "NOTES 3: a note on channel 3 does");
    h1_midi_at(0x82, 65, 0, 0);
    go(400);

    /* ---- THRU ---- */
    CHECK(!h1_midi_thru(), "THRU off");
    midi_in_page();
    h1_control_set(H1_ENC_K4, 1);
    CHECK(h1_midi_thru(), "THRU on");
    tap(H1_BTN_HOME);

    /* ---- the state ---- */
    {
        const void *p = h1_state(&size);
        memcpy(blob, p, size);
    }
    h1_init(44100);
    CHECK(st.seq[7] == 0 && !MI_FOLLOW() && !MI_THRU() && MI_NOTES() == 0, "a new h1: nothing followed, every note");
    CHECK(h1_state_load(blob, size) == 0, "the state loads");
    CHECK(MI_FOLLOW() && !MI_START() && MI_NOTES() == 3 && MI_THRU(), "with the MIDI IN page as it was");
    CHECK(midi_in[0] == 1 && midi_in[2] == 3 && midi_in[3] == 1, "and the page shows it");
    blob[offsetof(H1State, seq) + 7] = 0xFF;      /* NOTES 31: past the end */
    h1_init(44100);
    CHECK(h1_state_load(blob, size) == 0 && MI_NOTES() <= MI_NOTES_OFF && midi_in[2] <= MI_NOTES_OFF,
          "any byte there loads clamped");
    blob[offsetof(H1State, seq) + 7] = 0;         /* as 0.3.0 saves it */
    h1_init(44100);
    CHECK(h1_state_load(blob, size) == 0 && st.seq[7] == 0 && MI_NOTES() == 0, "0.3.0's zero: h1's MIDI in as it was");

    printf("clockin: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
