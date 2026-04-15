#include "../../inc/process_spawn.h"
#include "../../inc/log.h"
#include <unistd.h>

static int has_request_in_direction(int *requests, int nf, int cur, int dir)
{
    int i;
    if (dir == ELEV_UP)
        for (i = cur + 1; i < nf; i++) { if (requests[i] > 0) return 1; }
    else
        for (i = cur - 1; i >= 0; i--) { if (requests[i] > 0) return 1; }
    return 0;
}

static int any_request(int *requests, int nf)
{
    int i;
    for (i = 0; i < nf; i++) if (requests[i] > 0) return 1;
    return 0;
}

static void run_elevator(t_shm *shm, t_elevator *elev, int *requests,
                         int nf, const char *label)
{
    int floor, i;

    while (!shm->header->shutdown){
        sem_wait(&elev->request_sem);
        if (shm->header->shutdown) break;

        sem_wait(&elev->mutex);

        if (elev->current_load == 0 &&
            !has_request_in_direction(requests, nf, elev->current_floor, elev->direction))
        {
            for (i = 0; i < nf; i++)
                if (requests[i] > 0){
                    elev->direction = (i > elev->current_floor) ? ELEV_UP : ELEV_DOWN;
                    break;
                }
        }

        floor = elev->current_floor + elev->direction;
        if (floor >= nf) { floor = nf - 1; elev->direction = ELEV_DOWN; }
        if (floor < 0)   { floor = 0;      elev->direction = ELEV_UP;   }
        elev->current_floor = floor;

        if (requests[floor] > 0){
            requests[floor] = 0;
            log_fmt("[PID:%d] %s arrived at floor %d\n", getpid(), label, floor);
        }

        if (!has_request_in_direction(requests, nf, floor, elev->direction)
            && elev->current_load == 0)
            elev->direction = -elev->direction;

        sem_post(&elev->mutex);

        if (any_request(requests, nf))
            sem_post(&elev->request_sem);

        usleep(10000);
    }
}

void run_delivery_elev(t_shm *shm, t_config *cfg)
{
    run_elevator(shm, shm->delivery, shm->delivery_requests,
                 cfg->num_floors, "Delivery elevator");
}

void run_reposition_elev(t_shm *shm, t_config *cfg)
{
    run_elevator(shm, shm->reposition, shm->reposition_requests,
                 cfg->num_floors, "Reposition elevator");
}