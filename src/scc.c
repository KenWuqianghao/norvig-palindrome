/* Diagnostics: strongly connected components of the state graph over unused phrases. */
#include "graph.h"

static Graph G;
static char *used;

int main(int argc, char **argv) {
    G = graph_build(argv[1]);
    int n = G.d.n, ns = G.ns;
    used = calloc(n, 1);
    const char *seed[] = { "aman", "aplan", "acanal", "panama" };
    for (int i = 0; i < 4; i++) used[dict_find(&G.d, seed[i])] = 1;
    if (argc > 2) {
        FILE *f = fopen(argv[2], "r"); char line[1024], kb[1024];
        while (fgets(line, sizeof line, f)) if (norvig_key(line, kb)) used[dict_find(&G.d, kb)] = 1;
        fclose(f);
    }
    /* edge list (CSR) */
    long ne = 0;
    for (int v = 0; v < ns; v++) FOR_EDGES(&G, v, used, t, p, { ne++; });
    int *off = xmalloc((ns + 1) * sizeof(int)), *to = xmalloc(ne * sizeof(int)), *ph = xmalloc(ne * sizeof(int));
    long e = 0;
    for (int v = 0; v < ns; v++) { off[v] = e; FOR_EDGES(&G, v, used, t, p, { to[e] = t; ph[e] = p; e++; }); }
    off[ns] = e;
    fprintf(stderr, "states %d edges %ld\n", ns, ne);
    /* iterative Tarjan */
    int *idx = xmalloc(ns * sizeof(int)), *low = xmalloc(ns * sizeof(int)), *comp = xmalloc(ns * sizeof(int));
    int *st = xmalloc(ns * sizeof(int)), *cs = xmalloc(ns * sizeof(int)); long *it = xmalloc(ns * sizeof(long));
    char *on = calloc(ns, 1);
    memset(idx, -1, ns * sizeof(int));
    int counter = 0, sp = 0, nc = 0;
    for (int r = 0; r < ns; r++) {
        if (idx[r] >= 0) continue;
        int csp = 0; cs[csp++] = r; idx[r] = low[r] = counter++; st[sp++] = r; on[r] = 1; it[r] = off[r];
        while (csp) {
            int v = cs[csp - 1];
            if (it[v] < off[v + 1]) {
                int w = to[it[v]++];
                if (idx[w] < 0) { idx[w] = low[w] = counter++; st[sp++] = w; on[w] = 1; it[w] = off[w]; cs[csp++] = w; }
                else if (on[w] && idx[w] < low[v]) low[v] = idx[w];
            } else {
                csp--;
                if (csp && low[v] < low[cs[csp - 1]]) low[cs[csp - 1]] = low[v];
                if (low[v] == idx[v]) { int w; do { w = st[--sp]; on[w] = 0; comp[w] = nc; } while (w != v); nc++; }
            }
        }
    }
    int *csize = calloc(nc, sizeof(int)); for (int v = 0; v < ns; v++) csize[comp[v]]++;
    int big = 0; for (int c = 0; c < nc; c++) if (csize[c] > csize[big]) big = c;
    char *inbig = calloc(n, 1); long ein = 0;
    for (int v = 0; v < ns; v++) if (comp[v] == big) for (long k = off[v]; k < off[v + 1]; k++) if (comp[to[k]] == big) { ein++; inbig[ph[k]] = 1; }
    int pin = 0; for (int p = 0; p < n; p++) pin += inbig[p];
    int start = RSUF(&G, dict_find(&G.d, "acanal"), 3);
    int nontriv = 0; for (int c = 0; c < nc; c++) if (csize[c] > 1) nontriv++;
    printf("components %d (non-trivial %d); largest %d states, %ld edges, %d distinct phrases\n", nc, nontriv, csize[big], ein, pin);
    printf("eps in largest: %d; start in largest: %d (start comp size %d)\n", comp[0] == big, comp[start] == big, csize[comp[start]]);
    return 0;
}
