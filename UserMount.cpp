extern "C" {
#include <string.h>
}
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

    checkError(mkdir("etc", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH), "making fs");
    checkError(mkdir("usr", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH), "making fs");
    checkError(mkdir("usr/lib", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH), "making fs");
    checkError(mkdir("usr/bin", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH), "making fs");
    checkError(mkdir("proc", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH), "making fs");
    checkError(mkdir("old_root", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH), "making fs");

    checkError(symlink("usr/lib", "lib"), "creating symlink");
    checkError(symlink("usr/lib", "lib64"), "creating symlink");
    checkError(symlink("usr/bin", "bin"), "creating symlink");
    checkError(symlink("usr/bin", "sbin"), "creating symlink");

    checkError(mount("/etc/", "etc", "ext4", MS_BIND|MS_REC, "ro"), "mounting");
    checkError(mount("/usr/lib", "usr/lib", "ext4", MS_BIND|MS_REC, "ro"), "mounting");
    checkError(mount("/usr/bin", "usr/bin", "ext4", MS_BIND|MS_REC, "ro"), "mounting");
    checkError(mount("/proc", "proc", "proc", MS_BIND|MS_REC, NULL), "mounting proc");
}

int UserMount::forkNew(void *arg) {
    Options *opts = (Options*)arg;
    char buf;

    close(opts->pipe[1]);

    if (checkError(read(opts->pipe[0], &buf, 1), "reading pipe") > 0) {
        fputs("Pipe corrupted", stderr);
        exit(-1);
    }

    close(opts->pipe[0]);

    // Almost ready, just drop privileges
    checkError(setgid(NOBODY), "setting group id");
    checkError(setuid(NOBODY), "setting user id");
    checkError(execvp(opts->program, getArgs(opts)), "exec call");

    exit(-1);
}
    

// TODO Add cgroup limits
void UserMount::start() {
    int net = opts.net?CLONE_NEWNET:0;
    char uMap[20], gMap[20], procDir[14+10];
    int uidMapFd, gidMapFd, length, setgroupsFd, nsFd;
    void *stack = malloc(STACK_SIZE+1);
    stack = (char*)stack + STACK_SIZE;

    pipe(opts.pipe);

    length = snprintf(uMap, 20, "0 %d 1\n", getuid());
    snprintf(gMap, 20, "0 %d 1\n", getgid());
    snprintf(procDir, 24, "/proc/%d/uid_map", getpid());

    // First, create new user namespace for capabilities
    checkError(unshare(CLONE_NEWUSER|CLONE_NEWNS), "unshare");

    uidMapFd = checkError(open(procDir, O_RDWR), "opening uid_map");
    checkError(write(uidMapFd, uMap, length-1), "writing uid_map");
    checkError(close(uidMapFd), "closing uid_map");

    snprintf(procDir, 24, "/proc/%d/setgroups", getpid());
    setgroupsFd = checkError(open(procDir, O_RDWR), "opening setgroups");
    checkError(write(setgroupsFd, "deny", 4), "writing setgroups");
    checkError(close(setgroupsFd), "closing setgroups");

    snprintf(procDir, 24, "/proc/%d/gid_map", getpid());
    gidMapFd = checkError(open(procDir, O_RDWR), "opening gid_map");
    checkError(write(gidMapFd, gMap, length-1), "writing gid_map");
    checkError(close(gidMapFd), "closing gid_map");

    checkError(mount("none", "private", "tmpfs", 0, NULL), "mounting tmpfs");
    checkError(chdir("private"), "changing directories");

    // Then, create namespace to run command in
    pid = checkError(clone(forkNew, stack,
                CLONE_NEWCGROUP|net|CLONE_NEWPID|CLONE_NEWUSER, &opts), "clone");

    close(opts.pipe[0]);

    snprintf(procDir, 24, "/proc/%d/ns/pid", pid);
    nsFd = checkError(open(procDir, O_RDONLY), "opening pid ns file");
    checkError(setns(nsFd, CLONE_NEWPID), "joining process namespace");
    close(nsFd);

    createFS();

    snprintf(procDir, 24, "/proc/%d/uid_map", pid);
    length = snprintf(uMap, 20, "99 %d 1\n", getuid());
    uidMapFd = checkError(open(procDir, O_RDWR), "opening inner uid_map");
    checkError(write(uidMapFd, uMap, length-1), "writing inner uid_map");
    checkError(close(uidMapFd), "closing inner uid_map");

    length = snprintf(gMap, 20, "99 %d 1\n", getgid());
    snprintf(procDir, 24, "/proc/%d/gid_map", pid);
    gidMapFd = checkError(open(procDir, O_RDWR), "opening inner gid_map");
    checkError(write(gidMapFd, gMap, length-1), "writing inner gid_map");
    checkError(close(gidMapFd), "closing inner gid_map");

    // Create new root
    checkError(syscall(SYS_pivot_root, ".", "old_root"), "pivoting root");
    checkError(umount2("old_root", MNT_DETACH), "unmounting root");

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
        checkError(kill(pid, SIGKILL), "killing child");
    pid = 0;
}

int UserMount::checkError(int err, std::string errMsg) {
    if (err < 0) {
        perror(errMsg.c_str());
        exit(-1);
    }
    return err;
}
