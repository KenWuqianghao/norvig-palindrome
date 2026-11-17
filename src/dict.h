#pragma once
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *xmalloc(size_t n) { void *p = malloc(n ? n : 1); if (!p) { perror("malloc"); exit(1); } return p; }
static void *xrealloc(void *p, size_t n) { p = realloc(p, n); if (!p) { perror("realloc"); exit(1); } return p; }

typedef struct { int child[26]; int count; int phrase; } TrieNode;
typedef struct { TrieNode *n; int len, cap; } Trie;

static int trie_new(Trie *t) {
    if (t->len == t->cap) { t->cap = t->cap ? t->cap * 2 : 1 << 16; t->n = xrealloc(t->n, t->cap * sizeof(TrieNode)); }
    TrieNode *x = &t->n[t->len];
    memset(x->child, -1, sizeof x->child); x->count = 0; x->phrase = -1;
    return t->len++;
}

/* Count every all-letter prefix of s; mark the end only when all of s is letters. */
static void trie_add(Trie *t, const char *s, int len, int phrase) {
    int node = 0;
    for (int i = 0; i < len; i++) {
        int c = s[i] - 'a';
        if (c < 0 || c >= 26) return;
        if (t->n[node].child[c] < 0) { int k = trie_new(t); t->n[node].child[c] = k; }
        node = t->n[node].child[c];
        t->n[node].count++;
    }
    t->n[node].phrase = phrase;
}

typedef struct {
    int n;
    char **key; int *klen; char **rkey; char **text;
    Trie fwd, rev;   /* fwd: keys; rev: reversed keys */
} Dict;

/* Norvig's letters(): drop \W, lowercase. Python 3 \w on this ASCII file is [A-Za-z0-9_]. */
static int norvig_key(const char *s, char *out) {
    int k = 0;
    for (; *s; s++) if (isalnum((unsigned char)*s) || *s == '_') out[k++] = tolower((unsigned char)*s);
    out[k] = 0;
    return k;
}

static int all_letters(const char *k) { for (; *k; k++) if (*k < 'a' || *k > 'z') return 0; return 1; }

typedef struct { char *k; int id; } HEnt;
typedef struct { HEnt *e; int cap; } Hash;
static unsigned long hstr(const char *s) { unsigned long h = 1469598103934665603UL; for (; *s; s++) h = (h ^ (unsigned char)*s) * 1099511628211UL; return h; }
static int hash_get(Hash *h, const char *k) {
    for (unsigned long i = hstr(k) & (h->cap - 1);; i = (i + 1) & (h->cap - 1)) {
        if (!h->e[i].k) return -1;
        if (!strcmp(h->e[i].k, k)) return h->e[i].id;
    }
}
static void hash_put(Hash *h, char *k, int id) {
    for (unsigned long i = hstr(k) & (h->cap - 1);; i = (i + 1) & (h->cap - 1))
        if (!h->e[i].k) { h->e[i].k = k; h->e[i].id = id; return; }
}

/* Keys keep Norvig's insertion order; the display text is the last line with that key (dict overwrite). */
static Dict dict_load(const char *path) {
    FILE *f = fopen(path, "r"); if (!f) { perror(path); exit(1); }
    Dict d = {0};
    int cap = 1 << 18;
    d.key = xmalloc(cap * sizeof(char *)); d.klen = xmalloc(cap * sizeof(int));
    d.rkey = xmalloc(cap * sizeof(char *)); d.text = xmalloc(cap * sizeof(char *));
    Hash h = { xmalloc((1 << 19) * sizeof(HEnt)), 1 << 19 };
    memset(h.e, 0, (1 << 19) * sizeof(HEnt));
    char line[1024], kb[1024];
    while (fgets(line, sizeof line, f)) {
        char *s = line; while (isspace((unsigned char)*s)) s++;
        int e = strlen(s); while (e && isspace((unsigned char)s[e - 1])) s[--e] = 0;
        int kl = norvig_key(s, kb);
        int id = hash_get(&h, kb);
        if (id < 0) {
            id = d.n++;
            d.key[id] = strdup(kb); d.klen[id] = kl;
            hash_put(&h, d.key[id], id);
        } else free(d.text[id]);
        d.text[id] = strdup(s);
    }
    fclose(f); free(h.e);
    trie_new(&d.fwd); trie_new(&d.rev);
    char rb[1024];
    for (int i = 0; i < d.n; i++) {
        int kl = d.klen[i];
        for (int j = 0; j < kl; j++) rb[j] = d.key[i][kl - 1 - j];
        rb[kl] = 0; d.rkey[i] = strdup(rb);
        int ok = kl > 0 && all_letters(d.key[i]);
        trie_add(&d.fwd, d.key[i], kl, ok ? i : -1);
        trie_add(&d.rev, rb, kl, ok ? i : -1);
    }
    return d;
}

static int dict_find(Dict *d, const char *key) {
    int node = 0;
    for (; *key; key++) { node = d->fwd.n[node].child[*key - 'a']; if (node < 0) return -1; }
    return d->fwd.n[node].phrase;
}

static int count_words(const char *s) {
    int w = 0, in = 0;
    for (; *s; s++) { if (isspace((unsigned char)*s)) in = 0; else if (!in) { in = 1; w++; } }
    return w;
}
