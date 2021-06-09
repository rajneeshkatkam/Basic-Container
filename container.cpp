
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sched.h>
#include <string.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>

//Error printing function
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)

#define STACK_SIZE (1024 * 1024)    /* Stack size for cloned child */
static char child_stack[STACK_SIZE];

struct nmspace{
    char *hostname;
    char *root_filesystem;
    char *net_namespace;
    char *veth_netns_ip_addr;
    char *veth_parent_ip_addr;
};   

int child_process(void *arg)
{
    sleep(2);
    struct utsname uts;
    struct nmspace *nm=(struct nmspace *)arg;

    /* Change hostname in UTS namespace of child */
    if (sethostname(nm->hostname, strlen((char *)nm->hostname)) == -1)
        errExit("sethostname");


    /* Retrieve and display hostname */
    if (uname(&uts) == -1)
        errExit("uname");
    printf("Hostname in child:  %s\n", uts.nodename);



    // changing directory
    char root_directory[200];
    strcpy(root_directory,"./");
    strcat(root_directory, nm->root_filesystem);
    strcat(root_directory,"/");
    chdir(root_directory);
    
    // changing root filesystem
    if (chroot("./") != 0) {
        perror("chroot ./");
        return 1;
    }


    //mounting proc file.
    char mount_point[]="/proc";
    if (mount("proc", mount_point, "proc", 0, NULL) == -1)
        errExit("mount");


    //Executing shell program in child instance
    char process_name[]="/bin/bash";
    char *name[] = {
        process_name,
        NULL
    };
    
    int child_fork = fork();

    if(child_fork==0)
    {
        execvp(name[0], name);
        printf("Error : execvp did not execute successfully.\n");
        return -1;
    }
    else
    {
        if (waitpid(child_fork, NULL, 0) == -1)      /* Wait for child */
        errExit("waitpid");
    }

    return 0;
               /* Terminates child */
}


int main(int argc, char *argv[])
{
    pid_t child_pid;
    struct utsname uts;
    int cgroup_enable_flag=0;


    if (argc < 6) {
        fprintf(stderr, "Usage: %s <hostname> <root_filesystem_name> <network_namespace> <veth_netns_ip_addr> <veth_parent_ip_addr> <cgroup_enable_flag>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if(strcmp(argv[4], argv[5])==0)
    {
        printf("veth parent and netns ip address cannot be same.\n");
        exit(EXIT_FAILURE);
    }

    struct nmspace nm{
        .hostname=argv[1],
        .root_filesystem=argv[2],
        .net_namespace=argv[3],
        .veth_netns_ip_addr=argv[4],
        .veth_parent_ip_addr=argv[5]
    };

    cgroup_enable_flag=atoi(argv[6]);

    //system("ip netns add netns1");

    if(cgroup_enable_flag)
    {
        printf("\ncgroup Enabled\n");
        //echo "20804" > /sys/fs/cgroup/cpu/demo/cgroup.procs
        char cgroup_cpu[200];
        strcpy(cgroup_cpu,"echo ");
        strcat(cgroup_cpu, std::to_string(getpid()).c_str());
        strcat(cgroup_cpu, " > /sys/fs/cgroup/cpu/demo/cgroup.procs");
        system(cgroup_cpu);
    }
    else
    {
        printf("\ncgroup Disabled\n");
    }
    



    child_pid = clone(child_process, 
                    child_stack + STACK_SIZE,   /* Points to start of 
                                                   downwardly growing stack */ 
                    CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNET | CLONE_NEWNS | SIGCHLD, &nm);
    
    

    if (child_pid == -1)
        errExit("clone");
   

    //Network namespace setup
    char attach_nmns[200];
    strcpy(attach_nmns,"ip netns attach ");
    strcat(attach_nmns, nm.net_namespace);
    strcat(attach_nmns, " ");
    strcat(attach_nmns, std::to_string(child_pid).c_str());
    system(attach_nmns);


    char up_nmns[200];
    strcpy(up_nmns,"ip netns exec ");
    strcat(up_nmns, nm.net_namespace);
    strcat(up_nmns, " ip link set dev lo up");
    system(up_nmns);


    char add_veth_nmns[200];
    strcpy(add_veth_nmns,"ip link add ");
    strcat(add_veth_nmns, nm.net_namespace);
    strcat(add_veth_nmns, "-veth0 type veth peer name ");
    strcat(add_veth_nmns, nm.net_namespace);
    strcat(add_veth_nmns, "-veth1");
    system(add_veth_nmns);


    char ip_set_nmns[200];
    strcpy(ip_set_nmns,"ip link set ");
    strcat(ip_set_nmns, nm.net_namespace);
    strcat(ip_set_nmns, "-veth1 netns ");
    strcat(ip_set_nmns, nm.net_namespace);
    system(ip_set_nmns);


    char ip_ifveth1_nmns[200];
    strcpy(ip_ifveth1_nmns,"ip netns exec ");
    strcat(ip_ifveth1_nmns, nm.net_namespace);
    strcat(ip_ifveth1_nmns, " ifconfig ");
    strcat(ip_ifveth1_nmns, nm.net_namespace);
    strcat(ip_ifveth1_nmns, "-veth1 ");
    strcat(ip_ifveth1_nmns, nm.veth_netns_ip_addr);
    strcat(ip_ifveth1_nmns, "/24 up");
    system(ip_ifveth1_nmns);


    char ip_ifveth0_nmns[200];
    strcpy(ip_ifveth0_nmns,"ifconfig ");
    strcat(ip_ifveth0_nmns, nm.net_namespace);
    strcat(ip_ifveth0_nmns, "-veth0 ");
    strcat(ip_ifveth0_nmns, nm.veth_parent_ip_addr);
    strcat(ip_ifveth0_nmns, "/24 up");
    system(ip_ifveth0_nmns);


    printf("\nParent namespaces:\n");
    char parent_ns[200];
    strcpy(parent_ns,"ls -l /proc/");
    strcat(parent_ns, std::to_string(getpid()).c_str());
    strcat(parent_ns, "/ns");
    system(parent_ns);

    printf("\n\nPID of child created by clone() is %ld\nChild namespaces:\n", (long) child_pid);
    char child_ns[200];
    strcpy(child_ns,"ls -l /proc/");
    strcat(child_ns, std::to_string(child_pid).c_str());
    strcat(child_ns, "/ns");
    system(child_ns);


    // Hostname in parent UTS namespace
    if (uname(&uts) == -1)
        errExit("uname");
    printf("\n\nHostname in parent: %s\n", uts.nodename);

    if (waitpid(child_pid, NULL, 0) == -1)      /* Wait for child */
        errExit("waitpid");

    //Unmount the proc file after child process exits.
    char delete_nmns[200];
    strcpy(delete_nmns,"ip netns delete ");
    strcat(delete_nmns, nm.net_namespace);
    system(delete_nmns);


    char umount_proc[200];
    strcpy(umount_proc,"./");
    strcat(umount_proc, nm.root_filesystem);
    strcat(umount_proc, "/proc");
    umount2(umount_proc, MNT_DETACH);


    printf("Child process terminated. Exiting...\n");

    exit(EXIT_SUCCESS);
}