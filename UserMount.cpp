#include "UserMount.hpp"

UserMount::UserMount(Options opts) {
    this->opts = opts;
}

UserMount::~UserMount() {
    killChild();
}

char **UserMount::getArgs(Options *opts) {
    opts->args.push_back(NULL);
    opts->args.insert(opts->args.begin(), opts->program);

    return opts->args.data();
}

void UserMount::createFS() {
    checkError(mkdir("etc", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH));
    checkError(mkdir("usr", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH));
    checkError(mkdir("usr/lib", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH));
    checkError(mkdir("usr/bin", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH));

    checkError(symlink("usr/lib", "lib"));
    checkError(symlink("usr/bin", "bin"));
    checkError(symlink("usr/bin", "sbin"));

    checkError(mount("/etc/", "etc", "ext4", MS_BIND, "ro"));
    checkError(mount("/usr/lib", "usr/lib", "ext4", MS_BIND, "ro"));
    checkError(mount("/usr/bin", "usr/bin", "ext4", MS_BIND, "ro"));
}

int UserMount::forkNew(void *arg) {
    Options *opts = (Options*)arg;
    char buf;

    close(opts->pipe[1]);

    if (read(opts->pipe[0], &buf, 1) > 0) {
        fputs("Pipe corrupted", stderr);
        exit(-1);
    }

    close(opts->pipe[0]);

    // Create new root
    checkError(syscall(SYS_pivot_root, ".", "old_root"));
    checkError(umount2("old_root", MNT_DETACH));

    // Almost ready, just drop privileges
    checkError(setgid(NOBODY));
    checkError(setuid(NOBODY));
    checkError(execvp(opts->program, getArgs(opts)));

    exit(-1);
}
    

// TODO Add cgroup limits
void UserMount::start() {
    int net = opts.net?CLONE_NEWNET:0;
    char map[9];
    int uidMapFd, gidMapFd;
    void *stack = malloc(STACK_SIZE);
    stack = (char*)stack + STACK_SIZE;

    pipe(opts.pipe);

    snprintf(map, 9, "%d %d 1\n", NOBODY, NOBODY);

    // First, create new user namespace for capabilities
    checkError(unshare(CLONE_NEWUSER|CLONE_NEWNS));

    // Then, create namespace to run command in
    checkError(clone(forkNew, stack,
                CLONE_NEWCGROUP|net|CLONE_NEWPID|CLONE_NEWUSER, &opts));

    close(opts.pipe[0]);

    checkError(mount("none", "private", "tmpfs", 0, NULL));
    checkError(chdir("private"));

    createFS();

    uidMapFd = checkError(open("proc/1/uid_map", O_RDWR));
    checkError(write(uidMapFd, map, 8));
    checkError(close(uidMapFd));

    gidMapFd = checkError(open("proc/1/gid_map", O_RDWR));
    checkError(write(gidMapFd, map, 8));
    checkError(close(gidMapFd));

    close(opts.pipe[1]);

    // TODO Add support for stdin/stdout redirection
}

// TODO Add timing function
void UserMount::wait() {
    if (pid)
        waitpid(pid, NULL, 0);
}

void UserMount::killChild() {
    if (pid)
        checkError(kill(pid, SIGKILL));
    pid = 0;
}

int UserMount::checkError(int err) {
    if (err < 0) {
        perror(NULL);
        exit(-1);
    }
    return err;
}
