# container
Contains processes in a sandbox that limits access to system resources such as the filesystem, runtime, network devices, and process information.

## Compilation
The only requirements for compilation are a modern Linux kernel and build tools. Compilation can be achieved with a simple `make`.
## Usage
	Usage: container [OPTION...] command args...

	-n, --network Allow network access

	-t, --time=limit Execution time allowed

	-?, --help Give this help list

	--usage Give a short usage message
	
	Mandatory or optional arguments to long options are also mandatory or optional for any corresponding short options.

Time limits are currently not implemented, and will be silently ignored.

**Note:** On some systems, `/proc/sys/kernel/unprivileged_userns_clone` may need to be set to 1 to allow normal user access to namespaces.
	
## Description
### Namespaces
Container makes use of Linux namespaces to isolate the program being run. The program is run in its own user, process, mount, control group, and possibly network namespaces. Each of these provides separate isolation capabilities:

* User: Provides separate user and group identifiers, preventing access to other users' files.
* Process: Creates new `proc/` system, isolating pids and other process information.
* Mount: Prevents any new mounts from being visible to the rest of the system.
* Control group: Isolates control group hierarchy, hiding control group information.
* Network: Separates access to network devices by creating empty network namespace.

Further reading on namespace features can be found in [namespaces(7)](http://man7.org/linux/man-pages/man7/namespaces.7.html).

### Timing Alarms
Container will also be able to limit the total runtime of programs by using an alarm in the parent process. When the parent recieves SIGALRM, the subprocess is killed. Since it is the first process in the namespace, it will have pid 1, and will kill all other subprocesses when it dies.

### Pivot_Root
The program is isolated in a new root filesystem by bind mounting needed system files into a new tmpfs, and then switching the root to the new tmpfs. After this, the old root is unmounted. This is more secure than a simple chroot, because the rest of the parent filesystem is not even available to mount. See [pivot_root(2)](http://man7.org/linux/man-pages/man2/pivot_root.2.html).
