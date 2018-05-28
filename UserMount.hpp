#pragma once

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <string>

extern "C" {
#include <errno.h>
#include <sched.h>
#include <sys/mount.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
}

typedef struct {
    unsigned int time;
    bool net;
    char *program;
    std::vector<char*> args;
    int pipe[2];
} Options;

class UserMount {
    public:
        UserMount(Options opts);
        ~UserMount();

        void start();
        void wait();
        void killChild();

        static int checkError(int err, std::string errMsg);

        static const int STACK_SIZE = 1000000;
        static const int NOBODY = 99;

    private:
        int pid;
        Options opts;

        void createFS();

        static int forkNew(void* arg);
        static char** getArgs(Options *opts);
};

