#ifndef ABC_CEC_certificate
#define ABC_CEC_certificate

#include "aig/aig/aig.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum MutationType_t { MutationReplace = 0, MutationCreate = 1 } MutationType_t;

typedef struct MutationReplace_t {
    int old_id;
    int new_id;
    int complement;
} MutationReplace_t;

typedef struct MutationCreate_t {
    int id;
    int fanin0_id;
    int fanin0_compl;
    int fanin1_id;
    int fanin1_compl;
} MutationCreate_t;

typedef struct Mutation_t {
    MutationType_t type;
    union {
        MutationReplace_t replace;
        MutationCreate_t create;
    } mutation;
} Mutation_t;

static inline Mutation_t *new_mutation_replace(int old_id, int new_id, int complement) {
    Mutation_t *mut = (Mutation_t *)malloc(sizeof(Mutation_t));
    mut->type = MutationReplace;
    mut->mutation.replace.old_id = old_id;
    mut->mutation.replace.new_id = new_id;
    mut->mutation.replace.complement = complement;
    return mut;
}
static inline Mutation_t *new_mutation_create(int id, int fanin0_id, int fanin0_compl,
                                              int fanin1_id, int fanin1_compl) {
    Mutation_t *mut = (Mutation_t *)malloc(sizeof(Mutation_t));
    mut->type = MutationCreate;
    mut->mutation.create.id = id;
    mut->mutation.create.fanin0_id = fanin0_id;
    mut->mutation.create.fanin0_compl = fanin0_compl;
    mut->mutation.create.fanin1_id = fanin1_id;
    mut->mutation.create.fanin1_compl = fanin1_compl;
    return mut;
}

static inline void write_mutation(Mutation_t *mut, FILE *fp) {
    if (mut->type == MutationCreate) {
        fwrite(mut, sizeof(Mutation_t), 1, fp);
    } else {
        int flag = MutationReplace;
        fwrite(&flag, sizeof(int), 1, fp);
        fwrite(&mut->mutation.replace, sizeof(MutationReplace_t), 1, fp);
    }
}

typedef struct Hint_t {
    int id;
    int id_eq;
    int compl_eq;
} Hint_t;

static inline Hint_t *new_hint(int id, int id_eq, int compl_eq) {
    Hint_t *hint = (Hint_t *)malloc(sizeof(Hint_t));
    hint->id = id;
    hint->id_eq = id_eq;
    hint->compl_eq = compl_eq;
    return hint;
}

typedef struct Certificate_t {
    Vec_Ptr_t *mutations;
    Vec_Ptr_t *hints;
} Certificate_t;

static inline Certificate_t *new_certificate(Vec_Ptr_t *mutations, Vec_Ptr_t *hints) {
    Certificate_t *certif = (Certificate_t *)malloc(sizeof(Certificate_t));
    certif->mutations = mutations;
    certif->hints = hints;
    return certif;
}

static inline void write_certificate(Certificate_t *certificate, FILE *fp) {
    int n_mut = Vec_PtrSize(certificate->mutations);
    fwrite(&n_mut, sizeof(int), 1, fp);

    Mutation_t *mut;
    int i;
    Vec_PtrForEachEntry(Mutation_t *, certificate->mutations, mut, i) { write_mutation(mut, fp); }

    int n_hints = Vec_PtrSize(certificate->hints);
    fwrite(&n_hints, sizeof(int), 1, fp);

    Hint_t *hint;
    Vec_PtrForEachEntry(Hint_t *, certificate->hints, hint, i) {
        fwrite(hint, sizeof(Hint_t), 1, fp);
    }
}

static inline void write_certificates(Vec_Ptr_t *certificates, FILE *fp) {
    int n_certif = Vec_PtrSize(certificates);
    fwrite(&n_certif, sizeof(int), 1, fp);

    int i;
    Certificate_t *certif;
    Vec_PtrForEachEntry(Certificate_t *, certificates, certif, i) { write_certificate(certif, fp); }
}

typedef struct CertifIdMan_t {
    Aig_Man_t *p;

    // temporary drafty manager
    int internal;
} CertifIdMan_t;

static inline CertifIdMan_t *new_certif_id_man(Aig_Man_t *p) {
    int internal = Aig_ManObjNumMax(p) + 1;
    CertifIdMan_t *certif_man = (CertifIdMan_t *)malloc(sizeof(CertifIdMan_t));
    certif_man->p = p;
    certif_man->internal = internal;
    return certif_man;
}

static inline int fresh_certif_id(CertifIdMan_t *certif_man) {
    // Manager was not provided, let's return a dummy CertifId.
    if (certif_man == NULL)
        return 4;

    if (certif_man->internal == INT_MAX) {
        assert(0 && "max int reached");
    }

    return certif_man->internal++;
}

#endif
