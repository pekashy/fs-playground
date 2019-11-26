#include "proc_stat_parse.h"

int get_proc_info(uint32_t pid, procinfo *pinfo) {

    char szFileName[_POSIX_PATH_MAX],
            szStatStr[2048],
            *s, *t;
    FILE *fp;
    struct stat st;

    if (NULL == pinfo) {
        errno = EINVAL;
        return -1;
    }

    sprintf(szFileName, "/proc/%u/stat", (unsigned) pid);
    //printf("%s ", szFileName);
    if (-1 == access(szFileName, R_OK)) {
        return (pinfo->pid = -1);
    } /** if **/

    if (-1 != stat(szFileName, &st)) {
        pinfo->euid = st.st_uid;
        pinfo->egid = st.st_gid;
    } else {
        pinfo->euid = pinfo->egid = -1;
    }


    if ((fp = fopen(szFileName, "r")) == NULL) {
        return (pinfo->pid = -1);
    } /** IF_NULL **/

    if ((s = fgets(szStatStr, 2048, fp)) == NULL) {
        fclose(fp);
        return (pinfo->pid = -1);
    }

    /** pid **/
    sscanf(szStatStr, "%u", &(pinfo->pid));
    s = strchr(szStatStr, '(') + 1;
    t = strchr(szStatStr, ')');
    strncpy(pinfo->exName, s, t - s);
    pinfo->exName[t - s] = '\0';

    sscanf(t + 2, "%c %d %d %d %d %d %u %u %u %u %u %d %d %d %d %d %d %u %u %d %u %u %u %u %u %u %u %u %d %d %d %d %u",
            /*                 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33*/
           &(pinfo->state),
           &(pinfo->ppid),
           &(pinfo->pgrp),
           &(pinfo->session),
           &(pinfo->tty),
           &(pinfo->tpgid),
           &(pinfo->flags),
           &(pinfo->minflt),
           &(pinfo->cminflt),
           &(pinfo->majflt),
           &(pinfo->cmajflt),
           &(pinfo->utime),
           &(pinfo->stime),
           &(pinfo->cutime),
           &(pinfo->cstime),
           &(pinfo->counter),
           &(pinfo->priority),
           &(pinfo->timeout),
           &(pinfo->itrealvalue),
           &(pinfo->starttime),
           &(pinfo->vsize),
           &(pinfo->rss),
           &(pinfo->rlim),
           &(pinfo->startcode),
           &(pinfo->endcode),
           &(pinfo->startstack),
           &(pinfo->kstkesp),
           &(pinfo->kstkeip),
           &(pinfo->signal),
           &(pinfo->blocked),
           &(pinfo->sigignore),
           &(pinfo->sigcatch),
           &(pinfo->wchan));
    /*if (pinfo->tty != 0) {
        printf("%s \n ", t);
    }*/
    fclose(fp);
    return 0;
}
