#include "process_monitor.h"


int rsc_ps_pids[RSC_TYPES];
const char *resource_names[] = {
    "ALPHA", "BETA", "GAMMA", "DELTA"
};

const char *pspri_names[] = {
    "HIGH", "COMMON", "LOW"
};
const char *pst_names[] = {
    "READY", "RUNNING", "BLOCKED"
};

int ps_cnt;
ps *ps_list[PS_LIST_SIZE];

const int MAX_CC_PS = 16;
int cc_ps;
psqe *ready_q[PS_PRIORITY_NUM];
psqe *running_q;
psqe *rsc_q[RSC_TYPES][PS_PRIORITY_NUM];
psqe *io_q[PS_PRIORITY_NUM];
psqe *io_ps;


ps *make_process (char *_name, int _pri) {
    ps *sub = (ps*)malloc(sizeof(ps));
    sub->pid = ps_cnt;
    strcpy(sub->name, _name);
    if (_pri < 0 || _pri > 2)  {
        fprintf(stderr, "404Shell: Illegal Arg: Priority of process must be an integer between 0 and 2, "
            "given %d.\n", _pri);
        SAFE_DELETE(sub);
        return NULL;
    }
    sub->pri = _pri;
    sub->state = READY;
    for(int i = 0; i < RSC_TYPES; i++) {
        sub->rsc[i] = false;
    }
    return sub;
}

ps *get_process (int pid) {
    for(int i = 0; i < PS_LIST_SIZE; i++) {
        if(ps_list[i] != NULL && ps_list[i]->pid == pid) {
            return ps_list[i];
        }
    }
    return NULL;
}

psqe *make_psqe (ps *_data) {
    psqe *sub = malloc(sizeof(psqe));
    sub->data = _data;
    sub->next = NULL;
    return sub;
}

void psq_push (psqe **q, psqe *_psqe) {
    if(*q == NULL) {
        *q = _psqe;
    } else {
        psqe *t = *q;
        while (t->next != NULL) {
            t = t->next;
        }
        t->next = _psqe;
    }
}

psqe *psq_pop(psqe **q, int pid) {
    psqe *last = make_psqe(NULL);

    if (!last) {
        fprintf(stderr, "Error: Allocation Error.\n");
        exit(EXIT_FAILURE);
    }

    last->next = *q;
    while (last->next != NULL && last->next->data->pid != pid) {
        last = last->next;
    }

    psqe *res = last->next;
    if(res != NULL) {
        if(res == *q) {
            *q = res->next;
            SAFE_DELETE(last);
        } else {
        last->next = res->next;
        }
        res->next = NULL;
    }
    return res;
}
