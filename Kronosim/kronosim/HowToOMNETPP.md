// Tips: Omnet++ IDE is based on Eclipse IDE. Problems of Omnetpp can be solved with Eclipse solutions.

// -----------------------------------------------------------------------------------------------------------
Error: "java.nio.BufferUnderIndex"
or "java.lang.ArrayIndexOutOfBoundsException"
or ".pdom file gets generated"
or "Ctrl + space doesn't give any suggestion for functions"
or "F3 on methods don't open implementation"

HOW TO FIX 1: Project > C/C++ Index > Rebuild 

HOW TO FIX 2: *** https://bugs.eclipse.org/bugs/show_bug.cgi?id=393594#c1 > Comment 1 ***
 > close Omnetpp > go to home > omnetpp-5.6.1 > samples (workspace name) > (show hidden files) .metadata > 
 > .plugins > org.eclipse.cdt.core (di solito è la 2° cartella) > <project_name>.pdom --> delete all project.pdom files > open omnetpp
 > in "Progress" you should see a progressbar re-creating the "functions database" and the needed indexes
 // -----------------------------------------------------------------------------------------------------------
 
// During setup -----------------------------------------------------------------------------------------------
 > ./configure WITH_OSG=no

Per settare c++17 as compiler version in OMNeT⁠+⁠+:
See OMNET++ Installation Guide > 10.3. Using Different Compilers + 10.1. Configure.user Options:
> make cleanall
> ./configure CXXFLAGS=-std=c++17 WITH_TKENV=no WITH_SYSTEMC=no WITH_OSG=no WITH_OSGEARTH=no
> make
// -----------------------------------------------------------------------------------------------------------


// Add parameters to command: --------------------------------------------------------------------------------
Run > Run Configuration... > 'select configuration to modify' > More > Advanced > Additional arguments 
// -----------------------------------------------------------------------------------------------------------

// Linux change stack size allocated for programs ------------------------------------------------------------
stornace@stornace:~$ ulimit -s unlimited
stornace@stornace:~$ ulimit -a
core file size          (blocks, -c) 0
data seg size           (kbytes, -d) unlimited
scheduling priority             (-e) 0
file size               (blocks, -f) unlimited
pending signals                 (-i) 62879
max locked memory       (kbytes, -l) 65536
max memory size         (kbytes, -m) unlimited
open files                      (-n) 1024
pipe size            (512 bytes, -p) 8
POSIX message queues     (bytes, -q) 819200
real-time priority              (-r) 0
stack size              (kbytes, -s) unlimited // set to unlimited. Default was 8192
cpu time               (seconds, -t) unlimited
max user processes              (-u) 62879
virtual memory          (kbytes, -v) unlimited
file locks                      (-x) unlimited
// -----------------------------------------------------------------------------------------------------------

// Run simulation using Valgrind to detect memory leaks ------------------------------------------------------
https://wiki.eclipse.org/Linux_Tools_Project/Valgrind/User_Guide#Overview
--> Right click on Project Name > Profiling Tools > Profile With Valgrind.
If needed, go to Window > Show View > Other > Type 'Valgrind' > Open (to visualize Valgrind console)
// -----------------------------------------------------------------------------------------------------------









