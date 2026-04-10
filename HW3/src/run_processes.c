#include "../inc/process_spawn.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Process gövdeleri — şimdilik stub, sonraki adımlarda doldurulacak  */
/* ------------------------------------------------------------------ */

void run_word_carrier(t_proc_ctx *ctx)
{
    /* TODO: round-robin word seç, admission control */
    while (!ctx->shm->header->shutdown)
        usleep(50000);
}

void run_letter_carrier(t_proc_ctx *ctx)
{
    /* TODO: karakter task al, elevator isteği gönder */
    while (!ctx->shm->header->shutdown)
        usleep(50000);
}

void run_sorting_proc(t_proc_ctx *ctx)
{
    /* TODO: sorting_area tara, fixed/swap/move */
    while (!ctx->shm->header->shutdown)
        usleep(50000);
}

void run_delivery_elev(t_shm *shm, t_config *cfg)
{
    /* TODO: SCAN algoritması, request_sem bekle */
    (void)cfg;
    while (!shm->header->shutdown)
        usleep(50000);
}

void run_reposition_elev(t_shm *shm, t_config *cfg)
{
    /* TODO: idle LC'leri taşı */
    (void)cfg;
    while (!shm->header->shutdown)
        usleep(50000);
}