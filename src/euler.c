/* Grow the palindrome as a walk in the state graph.
 *   fill: splice shortest cycles of unused phrases into the walk at any visited state.
 *   lns:  remove a short segment of the walk, reconnect its ends by a shortest path that avoids one of the
 *         removed phrases, fill around the new path; keep the change if the phrase count does not drop.
 * usage: euler npdict.txt out.txt seconds seed [init.txt] */
#include "graph.h"
#include <time.h>

static Graph G;
static char *used, *dead;
static int *seen, gen, *par_v, *par_p, *queue, *cs, *cp;
static int *Ws, *Wp, Wm;   /* walk: states Ws[0..Wm]; Wp[i] is the phrase of the edge Ws[i-1] -> Ws[i] */
static int *deadlog, ndeadlog;
static unsigned long rng = 88172645463325252UL;
static unsigned long rnd(void) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return rng; }

static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }

/* Shortest path a -> b (a cycle when a == b) over unused phrases, no phrase twice.
 * Fills cs (target states) and cp (phrases) in walk order; returns the length, or 0. */
static int bfs_path(int a, int b) {
    gen++;
    int qh = 0, qt = 0, hit_from = -1, hit_p = -1;
    seen[a] = gen; queue[qt++] = a;
    while (qh < qt) {
        int x = queue[qh++];
        FOR_EDGES(&G, x, used, t, p, {
            int on_path = 0;
            for (int y = x; y != a; y = par_v[y]) if (par_p[y] == p) { on_path = 1; break; }
            if (on_path) continue;
            if (t == b) { hit_from = x; hit_p = p; goto done; }
            if (seen[t] != gen) { seen[t] = gen; par_v[t] = x; par_p[t] = p; queue[qt++] = t; }
        });
    }
done:
    if (hit_from < 0) return 0;
    int n = 0;
    cs[n] = b; cp[n] = hit_p; n++;
    for (int x = hit_from; x != a; x = par_v[x]) { cs[n] = x; cp[n] = par_p[x]; n++; }
    for (int i = 0, j = n - 1; i < j; i++, j--) { int t = cs[i]; cs[i] = cs[j]; cs[j] = t; t = cp[i]; cp[i] = cp[j]; cp[j] = t; }
    return n;
}

/* Replace edges i+1..k of the walk by the m edges in cs/cp (cs[m-1] must equal Ws[k]). */
static void replace(int i, int k, int m) {
    int d = m - (k - i);
    memmove(Ws + k + 1 + d, Ws + k + 1, (Wm - k) * sizeof(int));
    memmove(Wp + k + 1 + d, Wp + k + 1, (Wm - k) * sizeof(int));
    for (int j = 0; j < m; j++) { Ws[i + 1 + j] = cs[j]; Wp[i + 1 + j] = cp[j]; used[cp[j]] = 1; }
    Wm += d;
}

/* Splice cycles at walk positions lo..hi (hi grows with each splice). Returns phrases added. */
static int fill(int lo, int hi) {
    int added = 0;
    for (int pos = lo; pos <= hi && pos <= Wm; pos++) {
        int v = Ws[pos];
        while (!dead[v]) {
            int m = bfs_path(v, v);
            if (!m) { dead[v] = 1; deadlog[ndeadlog++] = v; break; }
            replace(pos, pos, m);
            added += m; hi += m;
        }
    }
    return added;
}

static int step(int v, int p) {
    if (G.side[v] == EPS) return LSUF(&G, p, 0);
    int sl = G.side[v] == SL, len = st_len(&G, v), kl = G.d.klen[p];
    const char *s = st_str(&G, v), *k = (sl ? G.d.rkey : G.d.key)[p];
    if (memcmp(s, k, kl < len ? kl : len)) return -1;
    if (kl == len) return EPS;
    if (kl < len) return sl ? LSUF(&G, G.sp[v], G.soff[v] + kl) : RSUF(&G, G.sp[v], G.soff[v] + kl);
    return sl ? RSUF(&G, p, len) : LSUF(&G, p, len);
}

/* Read a palindrome (one phrase per line, seeds included) into the walk. */
static void load_walk(const char *path) {
    FILE *f = fopen(path, "r"); if (!f) { perror(path); exit(1); }
    int *P = xmalloc(G.d.n * sizeof(int)), np = 0;
    char line[1024], kb[1024];
    while (fgets(line, sizeof line, f)) {
        if (!norvig_key(line, kb)) continue;
        int p = dict_find(&G.d, kb);
        if (p < 0) { fprintf(stderr, "unknown phrase %s", line); exit(1); }
        P[np++] = p;
    }
    fclose(f);
    for (int i = 0; i < np; i++) used[P[i]] = 1;
    int i = 2, j = np - 3;
    while (i <= j) {
        int p = G.side[Ws[Wm]] == SL ? P[j--] : P[i++];
        int t = step(Ws[Wm], p);
        if (t < 0) { fprintf(stderr, "init walk breaks at phrase %s\n", G.d.key[p]); exit(1); }
        Wm++; Ws[Wm] = t; Wp[Wm] = p;
    }
    if (!G.term[Ws[Wm]]) { fprintf(stderr, "init walk does not end at a terminal state\n"); exit(1); }
    free(P);
}

static const char *seedk[] = { "aman", "aplan", "acanal", "panama" };

static void save(const char *path, int *words_out) {
    int n = G.d.n, *lft = xmalloc(n * sizeof(int)), *rgt = xmalloc(n * sizeof(int)), nl = 0, nr = 0;
    for (int i = 1; i <= Wm; i++) { if (G.side[Ws[i - 1]] == SL) rgt[nr++] = Wp[i]; else lft[nl++] = Wp[i]; }
    char tmp[1024]; snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "w"); int words = 0;
#define OUT(p) do { fprintf(f, "%s\n", G.d.text[p]); words += count_words(G.d.text[p]); } while (0)
    for (int i = 0; i < 2; i++) OUT(dict_find(&G.d, seedk[i]));
    for (int i = 0; i < nl; i++) OUT(lft[i]);
    for (int i = nr - 1; i >= 0; i--) OUT(rgt[i]);
    for (int i = 2; i < 4; i++) OUT(dict_find(&G.d, seedk[i]));
    fclose(f); rename(tmp, path);
    free(lft); free(rgt);
    if (words_out) *words_out = words;
}

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: euler npdict.txt out.txt seconds seed [init.txt]\n"); return 1; }
    double t0 = now(), budget = atof(argv[3]);
    rng ^= strtoul(argv[4], 0, 10) * 0x9E3779B97F4A7C15UL; for (int i = 0; i < 10; i++) rnd();
    G = graph_build(argv[1]);
    int n = G.d.n, ns = G.ns;
    used = calloc(n, 1); dead = calloc(ns, 1);
    seen = calloc(ns, sizeof(int)); par_v = xmalloc(ns * sizeof(int)); par_p = xmalloc(ns * sizeof(int));
    queue = xmalloc(ns * sizeof(int)); cs = xmalloc(ns * sizeof(int)); cp = xmalloc(ns * sizeof(int));
    deadlog = xmalloc(ns * sizeof(int));
    for (int i = 0; i < 4; i++) used[dict_find(&G.d, seedk[i])] = 1;
    int start = RSUF(&G, dict_find(&G.d, "acanal"), 3); /* left must still produce "aca" */
    Ws = xmalloc((n + 1) * sizeof(int)); Wp = xmalloc((n + 1) * sizeof(int));
    int *Bs = xmalloc((n + 1) * sizeof(int)), *Bp = xmalloc((n + 1) * sizeof(int));
    char *bused = xmalloc(n);
    Ws[0] = start; Wm = 0;
    if (argc > 5) load_walk(argv[5]);
    fprintf(stderr, "initial walk: %d phrases\n", Wm + 4);
    fill(0, Wm);
    int best = Wm, words;
    save(argv[2], &words);
    fprintf(stderr, "fill: %d phrases (%.0fs)\n", Wm + 4, now() - t0);
    long it = 0, acc = 0, nopath = 0;
    double last = now();
    while (now() - t0 < budget) {
        it++;
        if (it % 5000 == 0) { /* freed phrases can revive any state: full sweep */
            memset(dead, 0, ns); fill(0, Wm);
            fprintf(stderr, "it %ld acc %ld nopath %ld phrases %d best %d (%.0fs)\n", it, acc, nopath, Wm + 4, best + 4, now() - t0);
        }
        int i = rnd() % Wm, k = i + 1 + rnd() % 6;
        if (k > Wm) k = Wm;
        int a = Ws[i], b = Ws[k], old = Wm;
        memcpy(Bs, Ws, (Wm + 1) * sizeof(int)); memcpy(Bp, Wp, (Wm + 1) * sizeof(int)); memcpy(bused, used, n);
        ndeadlog = 0;
        for (int j = i + 1; j <= k; j++) used[Wp[j]] = 0;
        int ban = Wp[i + 1 + rnd() % (k - i)];
        used[ban] = 1;
        int m = a == b ? 0 : bfs_path(a, b);
        used[ban] = 0;
        if (a != b && !m) { nopath++; memcpy(used, bused, n); continue; }
        replace(i, k, m);
        for (int j = i; j <= i + m; j++) if (dead[Ws[j]]) dead[Ws[j]] = 0;
        fill(i, i + m);
        if (Wm >= old) {
            acc++;
            if (Wm > best) {
                best = Wm;
                if (now() - last > 30) { save(argv[2], &words); last = now(); }
            }
        } else {
            Wm = old;
            memcpy(Ws, Bs, (Wm + 1) * sizeof(int)); memcpy(Wp, Bp, (Wm + 1) * sizeof(int)); memcpy(used, bused, n);
            for (int j = 0; j < ndeadlog; j++) dead[deadlog[j]] = 0;
        }
    }
    save(argv[2], &words);
    printf("Pal: %d phrases, %d words (it %ld, accepted %ld, %.0f s)\n", Wm + 4, words, it, acc, now() - t0);
    return 0;
}
