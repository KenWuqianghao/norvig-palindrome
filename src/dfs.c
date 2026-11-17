/* Port of Norvig's pal3 letter-by-letter depth-first search (Panama.search). */
#include "dict.h"
#include <time.h>

typedef struct { char type, n, act; char a[29]; } Cmd; /* type 'D': actions a[0..n); type 'U': undo act */

static Dict D;
static char *used;
static int *left, nleft, *right, nright;       /* right[nright-1] is the innermost phrase (Norvig's right[0]) */
static char Lb[512], Rb[512];                  /* Lb = L; Rb = reverse(R) */
static int Ln[513], Rn[513], Ll, Rl;           /* trie paths: Ln[i] = fwd node of L[:i]; Rn[i] = rev node of Rb[:i] */
static int *best, nbest;

static void setL(const char *k, int kl, int *path, char *buf, int *len, Trie *t) {
    *len = kl; path[0] = 0;
    for (int i = 0; i < kl; i++) { buf[i] = k[i]; path[i + 1] = t->n[path[i]].child[k[i] - 'a']; }
}

static int allowed(Trie *t, int node) { int p = t->n[node].phrase; return p >= 0 && !used[p]; }

static void do_act(char a) {
    if (a == ',') { int p = D.fwd.n[Ln[Ll]].phrase; used[p] = 1; left[nleft++] = p; Ll = 0; }
    else if (a == ';') { int p = D.rev.n[Rn[Rl]].phrase; used[p] = 1; right[nright++] = p; Rl = 0; }
    else {
        int c = a - 'a';
        Lb[Ll] = a; Ln[Ll + 1] = D.fwd.n[Ln[Ll]].child[c]; Ll++;
        Rb[Rl] = a; Rn[Rl + 1] = D.rev.n[Rn[Rl]].child[c]; Rl++;
    }
}

static void undo_act(char a) {
    if (a == ',') { int p = left[--nleft]; used[p] = 0; setL(D.key[p], D.klen[p], Ln, Lb, &Ll, &D.fwd); }
    else if (a == ';') { int p = right[--nright]; used[p] = 0; setL(D.rkey[p], D.klen[p], Rn, Rb, &Rl, &D.rev); }
    else { Ll--; Rl--; }
}

static int is_pal(void) {
    if (Ll == 0) { /* left[-1].endswith(R): reverse(left[-1]) starts with Rb */
        int p = left[nleft - 1];
        if (Rl <= D.klen[p] && !memcmp(D.rkey[p], Rb, Rl)) return 1;
    }
    if (Rl == 0) { int p = right[nright - 1]; if (Ll <= D.klen[p] && !memcmp(D.key[p], Lb, Ll)) return 1; }
    return 0;
}

static void check(void) {
    if (!is_pal()) return;
    int N = nleft + nright;
    if (N > nbest) {
        nbest = N;
        memcpy(best, left, nleft * sizeof(int));
        for (int i = 0; i < nright; i++) best[nleft + i] = right[nright - 1 - i];
    }
}

static Cmd actions(void) {
    Cmd c = { 'D', 0, 0, {0} };
    if (allowed(&D.fwd, Ln[Ll])) c.a[c.n++] = ',';
    if (allowed(&D.rev, Rn[Rl])) c.a[c.n++] = ';';
    long sc[26]; int ord[26];
    for (int i = 0; i < 26; i++) {
        int x = D.fwd.n[Ln[Ll]].child[i], y = D.rev.n[Rn[Rl]].child[i];
        sc[i] = (x < 0 || y < 0) ? 0 : (long)D.fwd.n[x].count * D.rev.n[y].count;
        ord[i] = i;
    }
    for (int i = 1; i < 26; i++) { /* stable insertion sort, ascending score */
        int o = ord[i], j = i;
        while (j > 0 && sc[ord[j - 1]] > sc[o]) { ord[j] = ord[j - 1]; j--; }
        ord[j] = o;
    }
    for (int i = 0; i < 26; i++) if (sc[ord[i]] > 0) c.a[c.n++] = 'a' + ord[i];
    return c;
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: dfs npdict.txt steps out.txt\n"); return 1; }
    D = dict_load(argv[1]);
    long steps = atol(argv[2]);
    used = calloc(D.n, 1);
    left = xmalloc(D.n * sizeof(int)); right = xmalloc(D.n * sizeof(int)); best = xmalloc(D.n * sizeof(int));
    int seedL[] = { dict_find(&D, "aman"), dict_find(&D, "aplan") };
    int seedR[] = { dict_find(&D, "panama"), dict_find(&D, "acanal") };
    for (int i = 0; i < 2; i++) { left[nleft++] = seedL[i]; used[seedL[i]] = 1; right[nright++] = seedR[i]; used[seedR[i]] = 1; }
    setL("aca", 3, Ln, Lb, &Ll, &D.fwd); Rl = 0; Rn[0] = 0;
    check();
    long cap = 1 << 20, sp = 0; Cmd *st = xmalloc(cap * sizeof(Cmd));
    st[sp++] = actions();
    clock_t t0 = clock();
    long i;
    for (i = 0; i < steps && sp; i++) {
        if (sp + 3 > cap) { cap *= 2; st = xrealloc(st, cap * sizeof(Cmd)); }
        Cmd *c = &st[--sp];
        if (c->type == 'U') undo_act(c->act);
        else if (c->n) {
            char a = c->a[--c->n];
            do_act(a); check();
            sp++; /* keep the remaining actions */
            st[sp].type = 'U'; st[sp].act = a; sp++;
            st[sp++] = actions();
        }
        if ((i & ((1 << 24) - 1)) == 0) fprintf(stderr, "step %ld best %d (%.0fs)\n", i, nbest, (double)(clock() - t0) / CLOCKS_PER_SEC);
    }
    int words = 0, letters = 0;
    FILE *f = fopen(argv[3], "w");
    for (int k = 0; k < nbest; k++) { fprintf(f, "%s\n", D.text[best[k]]); words += count_words(D.text[best[k]]); letters += D.klen[best[k]]; }
    fclose(f);
    printf("Pal: %d phrases, %d words, %d letters (%ld steps, %.0f s)\n", nbest, words, letters, i, (double)(clock() - t0) / CLOCKS_PER_SEC);
    return 0;
}
