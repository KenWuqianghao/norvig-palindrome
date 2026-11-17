#pragma once
/* Phrase-level state graph for "A man, a plan, ..., a canal, Panama" palindromes.
 *
 * The palindrome is built from both ends. The state is the unmatched middle text:
 *   EPS          both sides match.
 *   L-state s    the left side has extra letters s (a suffix of a key); the next phrase goes on the right.
 *   R-state u    the left side must still produce u (a suffix of a reversed key); the next phrase goes on the left.
 * An edge is one phrase. A palindrome is a walk from the start state to a terminal state (EPS, or a state
 * whose text is itself a palindrome) that uses each phrase at most once. */
#include "dict.h"

enum { EPS = 0, SL = 1, SR = 2 };

typedef struct {
    Dict d;
    char *valid;                 /* key is non-empty and letters-only */
    int *ford, *rord;            /* phrase ids in trie preorder */
    int *flo, *fhi, *rlo, *rhi;  /* per trie node: range in ford / rord */
    int ns; char *side; int *sp, *soff; char *term;
    int *lbase, *rbase, *lsuf, *rsuf; /* lsuf[lbase[p]+o]: L-state key[p][o:]; rsuf[rbase[p]+o]: R-state rkey[p][o:] */
} Graph;

static inline const char *st_str(Graph *g, int v) { return (g->side[v] == SL ? g->d.key : g->d.rkey)[g->sp[v]] + g->soff[v]; }
static inline int st_len(Graph *g, int v) { return g->d.klen[g->sp[v]] - g->soff[v]; }
static inline int LSUF(Graph *g, int p, int o) { return g->lsuf[g->lbase[p] + o]; }
static inline int RSUF(Graph *g, int p, int o) { return g->rsuf[g->rbase[p] + o]; }

static void preorder(Trie *t, int *ord, int *lo, int *hi) {
    int n = 0, sp = 0;
    int *stack = xmalloc(t->len * 2 * sizeof(int));
    stack[sp++] = 0;
    while (sp) {
        int x = stack[--sp];
        if (x < 0) { hi[-x - 1] = n; continue; }
        lo[x] = n;
        if (t->n[x].phrase >= 0) ord[n++] = t->n[x].phrase;
        stack[sp++] = -x - 1;
        for (int c = 25; c >= 0; c--) if (t->n[x].child[c] >= 0) stack[sp++] = t->n[x].child[c];
    }
    free(stack);
}

typedef struct { int v; } SEnt;
static unsigned long hsub(int side, const char *s, int len) {
    unsigned long h = 1469598103934665603UL ^ side;
    for (int i = 0; i < len; i++) h = (h ^ (unsigned char)s[i]) * 1099511628211UL;
    return h;
}

static int state_id(Graph *g, int *tab, int cap, int side, int p, int off) {
    const char *s = (side == SL ? g->d.key : g->d.rkey)[p] + off;
    int len = g->d.klen[p] - off;
    for (unsigned long i = hsub(side, s, len) & (cap - 1);; i = (i + 1) & (cap - 1)) {
        int v = tab[i];
        if (v < 0) {
            v = g->ns++;
            g->side[v] = side; g->sp[v] = p; g->soff[v] = off;
            int pal = 1;
            for (int a = 0, b = len - 1; a < b; a++, b--) if (s[a] != s[b]) { pal = 0; break; }
            g->term[v] = pal;
            tab[i] = v;
            return v;
        }
        if (g->side[v] == side && st_len(g, v) == len && !memcmp(st_str(g, v), s, len)) return v;
    }
}

static Graph graph_build(const char *path) {
    Graph g = {0};
    g.d = dict_load(path);
    int n = g.d.n;
    g.valid = xmalloc(n);
    long tot = 0;
    for (int p = 0; p < n; p++) { g.valid[p] = g.d.klen[p] > 0 && all_letters(g.d.key[p]); tot += g.d.klen[p]; }
    g.ford = xmalloc(n * sizeof(int)); g.rord = xmalloc(n * sizeof(int));
    g.flo = xmalloc(g.d.fwd.len * sizeof(int)); g.fhi = xmalloc(g.d.fwd.len * sizeof(int));
    g.rlo = xmalloc(g.d.rev.len * sizeof(int)); g.rhi = xmalloc(g.d.rev.len * sizeof(int));
    preorder(&g.d.fwd, g.ford, g.flo, g.fhi);
    preorder(&g.d.rev, g.rord, g.rlo, g.rhi);
    long maxs = 2 * tot + 1;
    g.side = xmalloc(maxs); g.term = xmalloc(maxs);
    g.sp = xmalloc(maxs * sizeof(int)); g.soff = xmalloc(maxs * sizeof(int));
    g.lbase = xmalloc(n * sizeof(int)); g.rbase = xmalloc(n * sizeof(int));
    g.lsuf = xmalloc(tot * sizeof(int)); g.rsuf = xmalloc(tot * sizeof(int));
    int cap = 1; while (cap < 4 * maxs) cap <<= 1;
    int *tab = xmalloc((long)cap * sizeof(int)); memset(tab, -1, (long)cap * sizeof(int));
    g.side[0] = EPS; g.term[0] = 1; g.sp[0] = 0; g.soff[0] = g.d.klen[0]; g.ns = 1;
    long b = 0;
    for (int p = 0; p < n; p++) {
        g.lbase[p] = g.rbase[p] = b;
        if (g.valid[p]) for (int o = 0; o < g.d.klen[p]; o++) {
            g.lsuf[b + o] = state_id(&g, tab, cap, SL, p, o);
            g.rsuf[b + o] = state_id(&g, tab, cap, SR, p, o);
        }
        b += g.d.klen[p];
    }
    free(tab);
    return g;
}

/* Call f(target, phrase) for every edge out of v that uses an unused phrase. */
#define FOR_EDGES(g, v, used, T, P, BODY) do {                                              \
    int _v = (v);                                                                         \
    if ((g)->side[_v] == EPS) {                                                           \
        for (int P = 0; P < (g)->d.n; P++) if ((g)->valid[P] && !(used)[P]) {             \
            int T = LSUF(g, P, 0); BODY } }                                               \
    else {                                                                                \
        int _sl = (g)->side[_v] == SL;                                                    \
        Trie *_t = _sl ? &(g)->d.rev : &(g)->d.fwd;                                       \
        const char *_s = st_str(g, _v); int _len = st_len(g, _v);                         \
        int _p0 = (g)->sp[_v], _o0 = (g)->soff[_v], _node = 0;                            \
        for (int _d = 0; _d < _len; _d++) {                                               \
            _node = _t->n[_node].child[_s[_d] - 'a'];                                      \
            if (_node < 0) break;                                                         \
            int P = _t->n[_node].phrase;                                                  \
            if (P >= 0 && !(used)[P]) {                                                   \
                int T = _d + 1 == _len ? EPS : (_sl ? LSUF(g, _p0, _o0 + _d + 1)          \
                                                    : RSUF(g, _p0, _o0 + _d + 1));        \
                BODY }                                                                    \
        }                                                                                 \
        if (_node >= 0) {                                                                 \
            int *_ord = _sl ? (g)->rord : (g)->ford;                                      \
            int _lo = (_sl ? (g)->rlo : (g)->flo)[_node], _hi = (_sl ? (g)->rhi : (g)->fhi)[_node]; \
            if (_t->n[_node].phrase >= 0) _lo++;                                          \
            for (int _i = _lo; _i < _hi; _i++) {                                          \
                int P = _ord[_i];                                                         \
                if (!(used)[P]) { int T = _sl ? RSUF(g, P, _len) : LSUF(g, P, _len); BODY } \
            }                                                                             \
        }                                                                                 \
    } } while (0)
