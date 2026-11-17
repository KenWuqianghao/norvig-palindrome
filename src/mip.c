/* Integer program: choose at most one edge per phrase so that every state is balanced (a circulation
 * through the start state). Modes:
 *   mip gen  npdict.txt model.lp            write the model for the start state's strongly connected component
 *   mip walk npdict.txt solution.sol out.txt read a HiGHS solution, keep the part connected to the start,
 *                                            write the palindrome (Euler circuit from the start state) */
#include "graph.h"

static Graph G;
static char *used;
static int ns, start;
static long ne;
static int *efrom, *eto, *eph, *comp;
static long *eid; /* variable name x<eid>: index in the full edge list */

static void build_component(void) {
    long cnt = 0;
    for (int v = 0; v < ns; v++) FOR_EDGES(&G, v, used, t, p, { (void)t; (void)p; cnt++; });
    int *off = xmalloc((ns + 1) * sizeof(int)), *to = xmalloc(cnt * sizeof(int)), *ph = xmalloc(cnt * sizeof(int));
    long e = 0;
    for (int v = 0; v < ns; v++) { off[v] = e; FOR_EDGES(&G, v, used, t, p, { to[e] = t; ph[e] = p; e++; }); }
    off[ns] = e;
    /* forward reach from start, then backward reach: the start's strongly connected component */
    char *fw = calloc(ns, 1), *bw = calloc(ns, 1);
    int *q = xmalloc(ns * sizeof(int)), qh = 0, qt = 0;
    fw[start] = 1; q[qt++] = start;
    while (qh < qt) { int v = q[qh++]; for (long k = off[v]; k < off[v + 1]; k++) if (!fw[to[k]]) { fw[to[k]] = 1; q[qt++] = to[k]; } }
    int *roff = calloc(ns + 1, sizeof(int)), *rfrom = xmalloc(cnt * sizeof(int));
    for (long k = 0; k < cnt; k++) roff[to[k] + 1]++;
    for (int v = 0; v < ns; v++) roff[v + 1] += roff[v];
    int *fill = xmalloc(ns * sizeof(int)); memcpy(fill, roff, ns * sizeof(int));
    for (int v = 0; v < ns; v++) for (long k = off[v]; k < off[v + 1]; k++) rfrom[fill[to[k]]++] = v;
    qh = qt = 0; bw[start] = 1; q[qt++] = start;
    while (qh < qt) { int v = q[qh++]; for (int k = roff[v]; k < roff[v + 1]; k++) if (!bw[rfrom[k]]) { bw[rfrom[k]] = 1; q[qt++] = rfrom[k]; } }
    comp = xmalloc(ns * sizeof(int));
    for (int v = 0; v < ns; v++) comp[v] = fw[v] && bw[v];
    eid = xmalloc(cnt * sizeof(long)); efrom = xmalloc(cnt * sizeof(int)); eto = xmalloc(cnt * sizeof(int)); eph = xmalloc(cnt * sizeof(int));
    ne = 0;
    for (int v = 0; v < ns; v++) if (comp[v]) for (long k = off[v]; k < off[v + 1]; k++) if (comp[to[k]]) {
        efrom[ne] = v; eto[ne] = to[k]; eph[ne] = ph[k]; eid[ne] = ne; ne++;
    }
    free(off); free(to); free(ph); free(fw); free(bw); free(q); free(roff); free(rfrom); free(fill);
}

static char *read_sol(const char *solpath, double thr) {
    char *x = calloc(ne, 1);
    FILE *f = fopen(solpath, "r"); if (!f) { perror(solpath); exit(1); }
    char line[256]; int in_cols = 0;
    while (fgets(line, sizeof line, f)) {
        if (!strncmp(line, "# Columns", 9)) { in_cols = 1; continue; }
        if (!strncmp(line, "# Rows", 6)) break; /* primal columns only; the dual section repeats "# Columns" */
        long e; double val;
        if (in_cols && sscanf(line, "x%ld %lf", &e, &val) == 2 && e >= 0 && e < ne && val > thr) x[e] = 1;
    }
    fclose(f);
    return x;
}

static void write_terms(FILE *f, const char *name, long *list, char *sign, int n, const char *rhs) {
    fprintf(f, " %s:", name);
    for (int i = 0; i < n; i++) { fprintf(f, " %c x%ld", sign[i], eid[list[i]]); if (i % 40 == 39) fputc('\n', f); }
    fprintf(f, " %s\n", rhs);
}

static void gen(const char *path, char *mask) {
    if (mask) { long k = 0; for (long e = 0; e < ne; e++) if (mask[e]) { efrom[k] = efrom[e]; eto[k] = eto[e]; eph[k] = eph[e]; eid[k] = eid[e]; k++; } ne = k; }
    FILE *f = fopen(path, "w");
    fprintf(f, "Maximize\n obj:");
    for (long e = 0; e < ne; e++) { fprintf(f, " + x%ld", eid[e]); if (e % 40 == 39) fputc('\n', f); }
    fprintf(f, "\nSubject To\n");
    /* balance rows: group edges by state */
    int *deg = calloc(ns + 1, sizeof(int));
    for (long e = 0; e < ne; e++) { deg[efrom[e] + 1]++; if (eto[e] != efrom[e]) deg[eto[e] + 1]++; }
    for (int v = 0; v < ns; v++) deg[v + 1] += deg[v];
    long *lst = xmalloc(deg[ns] * sizeof(long)); char *sg = xmalloc(deg[ns]);
    int *fill = xmalloc(ns * sizeof(int)); memcpy(fill, deg, ns * sizeof(int));
    for (long e = 0; e < ne; e++) {
        if (eto[e] == efrom[e]) continue;
        lst[fill[efrom[e]]] = e; sg[fill[efrom[e]]++] = '+';
        lst[fill[eto[e]]] = e; sg[fill[eto[e]]++] = '-';
    }
    char name[64];
    for (int v = 0; v < ns; v++) if (fill[v] > deg[v]) { snprintf(name, sizeof name, "b%d", v); write_terms(f, name, lst + deg[v], sg + deg[v], fill[v] - deg[v], "= 0"); }
    /* phrase rows */
    int n = G.d.n;
    int *pd = calloc(n + 1, sizeof(int));
    for (long e = 0; e < ne; e++) pd[eph[e] + 1]++;
    for (int p = 0; p < n; p++) pd[p + 1] += pd[p];
    long *pl = xmalloc(ne * sizeof(long)); char *ps = xmalloc(ne); int *pf = xmalloc(n * sizeof(int));
    memcpy(pf, pd, n * sizeof(int));
    for (long e = 0; e < ne; e++) { pl[pf[eph[e]]] = e; ps[pf[eph[e]]++] = '+'; }
    for (int p = 0; p < n; p++) if (pd[p + 1] - pd[p] > 1) { snprintf(name, sizeof name, "p%d", p); write_terms(f, name, pl + pd[p], ps + pd[p], pd[p + 1] - pd[p], "<= 1"); }
    /* the walk must leave the start state */
    int k = 0; for (long e = 0; e < ne; e++) if (efrom[e] == start) { lst[k] = e; sg[k++] = '+'; }
    write_terms(f, "start", lst, sg, k, ">= 1");
    fprintf(f, "Bounds\n");
    for (long e = 0; e < ne; e++) fprintf(f, " 0 <= x%ld <= 1\n", eid[e]);
    fprintf(f, "Binaries\n");
    for (long e = 0; e < ne; e++) { fprintf(f, " x%ld", eid[e]); if (e % 40 == 39) fputc('\n', f); }
    fprintf(f, "\nEnd\n");
    fclose(f);
    int np = 0; for (int p = 0; p < n; p++) np += pd[p + 1] > pd[p];
    int nsc = 0; for (int v = 0; v < ns; v++) nsc += comp[v];
    fprintf(stderr, "component: %d states, %ld edges, %d phrases\n", nsc, ne, np);
}

static void walk(const char *solpath, const char *out) {
    char *x = read_sol(solpath, 0.5);
    /* adjacency of chosen edges */
    int *od = calloc(ns + 1, sizeof(int)); long m = 0;
    for (long e = 0; e < ne; e++) if (x[e]) { od[efrom[e] + 1]++; m++; }
    for (int v = 0; v < ns; v++) od[v + 1] += od[v];
    long *adj = xmalloc((m + 1) * sizeof(long)); int *fill = xmalloc(ns * sizeof(int)); memcpy(fill, od, ns * sizeof(int));
    for (long e = 0; e < ne; e++) if (x[e]) adj[fill[efrom[e]]++] = e;
    /* checks: balance and one use per phrase */
    int *bal = calloc(ns, sizeof(int)); char *pu = calloc(G.d.n, 1); int bad = 0;
    for (long e = 0; e < ne; e++) if (x[e]) { bal[efrom[e]]++; bal[eto[e]]--; if (pu[eph[e]]++) bad++; }
    for (int v = 0; v < ns; v++) if (bal[v]) bad++;
    fprintf(stderr, "solution: %ld edges, %d violations\n", m, bad);
    /* Hierholzer from start */
    int *ptr = xmalloc(ns * sizeof(int)); memcpy(ptr, od, ns * sizeof(int));
    long *stk = xmalloc((m + 1) * sizeof(long)), *circ = xmalloc((m + 1) * sizeof(long)); int sp = 0; long nc = 0;
    int *vst = xmalloc((m + 1) * sizeof(int));
    vst[sp] = start; stk[sp++] = -1;
    while (sp) {
        int v = vst[sp - 1];
        if (ptr[v] < od[v + 1]) { long e = adj[ptr[v]++]; vst[sp] = eto[e]; stk[sp++] = e; }
        else { long e = stk[--sp]; if (e >= 0) circ[nc++] = e; }
    }
    fprintf(stderr, "connected to start: %ld of %ld edges\n", nc, m);
    /* circ is in reverse order */
    int *lft = xmalloc(nc * sizeof(int)), *rgt = xmalloc(nc * sizeof(int)), nl = 0, nr = 0;
    for (long i = nc - 1; i >= 0; i--) { long e = circ[i]; if (G.side[efrom[e]] == SL) rgt[nr++] = eph[e]; else lft[nl++] = eph[e]; }
    const char *seed[] = { "aman", "aplan", "acanal", "panama" };
    FILE *o = fopen(out, "w"); int words = 0, cnt = 0;
#define OUT(p) do { fprintf(o, "%s\n", G.d.text[p]); words += count_words(G.d.text[p]); cnt++; } while (0)
    for (int i = 0; i < 2; i++) OUT(dict_find(&G.d, seed[i]));
    for (int i = 0; i < nl; i++) OUT(lft[i]);
    for (int i = nr - 1; i >= 0; i--) OUT(rgt[i]);
    for (int i = 2; i < 4; i++) OUT(dict_find(&G.d, seed[i]));
    fclose(o);
    printf("Pal: %d phrases, %d words\n", cnt, words);
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: mip gen npdict.txt model.lp | mip walk npdict.txt sol out.txt\n"); return 1; }
    G = graph_build(argv[2]);
    ns = G.ns;
    used = calloc(G.d.n, 1);
    const char *seed[] = { "aman", "aplan", "acanal", "panama" };
    for (int i = 0; i < 4; i++) used[dict_find(&G.d, seed[i])] = 1;
    start = RSUF(&G, dict_find(&G.d, "acanal"), 3);
    build_component();
    if (!strcmp(argv[1], "gen")) gen(argv[3], argc > 4 ? read_sol(argv[4], 1e-6) : NULL);
    else walk(argv[3], argv[4]);
    return 0;
}
