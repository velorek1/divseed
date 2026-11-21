#define _XOPEN_SOURCE 700
#include "compiler.h"
#include "utf8.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_LINES 512
#define RUN_SECONDS 10
#define RUN_OUTPUT_LIMIT (1024*1024)

extern char **environ;
static pid_t child;
static FILE *log_file;
static char status[128];
static char lines[MAX_LINES][COMPILER_COLUMNS*4+1];
static int count;
static char run_dir[sizeof "/tmp/divseed-run-XXXXXX"];
static char executable[sizeof run_dir+sizeof "/program"];
static int running_program;
static struct timespec run_started;
static const char *stop_reason;

static void remove_executable(void) {
 if(!run_dir[0]) return;
 unlink(executable);
 rmdir(run_dir);
 run_dir[0]=0;
}

static char *find_compiler(void) {
 const char *path=getenv("PATH");
 if(!path) path="/bin:/usr/bin";
 const char *names[]={"gcc","clang"};
 for(int i=0;i<2;i++) {
  const char *dir=path;
  do {
   size_t length=strcspn(dir,":");
   char *candidate=malloc(length+strlen(names[i])+3);
   if(!candidate) return NULL;
   if(length) snprintf(candidate,length+strlen(names[i])+3,"%.*s/%s",(int)length,dir,names[i]);
   else sprintf(candidate,"./%s",names[i]);
   struct stat st;
   if(stat(candidate,&st)==0 && S_ISREG(st.st_mode) && access(candidate,X_OK)==0) return candidate;
   free(candidate);
   dir+=length;
  } while(*dir++==':');
 }
 return NULL;
}

int compiler_available(void) {
 char *path=find_compiler();
 int available=path!=NULL;
 free(path);
 return available;
}

int compiler_busy(void) {return child>0;}
const char *compiler_status(void) {return status;}
int compiler_line_count(void) {return count;}
const char *compiler_line(int row) {return lines[row];}

static int spawn_logged(const char *program,char *const args[]) {
 int log_fd=fileno(log_file);
 int fd=fcntl(log_fd,F_DUPFD_CLOEXEC,STDERR_FILENO+1);
 if(fd<0) return errno;
 posix_spawn_file_actions_t actions;
 posix_spawnattr_t attrs;
 int error=posix_spawn_file_actions_init(&actions);
 if(!error) {
  error=posix_spawnattr_init(&attrs);
  if(!error) {
   error=posix_spawn_file_actions_addopen(&actions,STDIN_FILENO,"/dev/null",O_RDONLY,0);
   if(!error) error=posix_spawn_file_actions_adddup2(&actions,fd,STDOUT_FILENO);
   if(!error) error=posix_spawn_file_actions_adddup2(&actions,fd,STDERR_FILENO);
   if(!error) error=posix_spawn_file_actions_addclose(&actions,fd);
   if(!error && log_fd>STDERR_FILENO) error=posix_spawn_file_actions_addclose(&actions,log_fd);
   if(!error) error=posix_spawnattr_setflags(&attrs,POSIX_SPAWN_SETPGROUP);
   if(!error) error=posix_spawn(&child,program,&actions,&attrs,args,environ);
   posix_spawnattr_destroy(&attrs);
  }
  posix_spawn_file_actions_destroy(&actions);
 }
 close(fd);
 return error;
}

const char *compiler_start(const char *path,int run) {
 if(compiler_busy()) return "A compile or run is already in progress.";
 char *program=find_compiler();
 if(!program) return "GCC or Clang was not found on PATH.";
 char *source=malloc(strlen(path)+3);
 if(!source) {free(program);return "Cannot allocate the source file path.";}
 sprintf(source,"%s%s",path[0]=='/'?"":"./",path);
 log_file=tmpfile();
 if(!log_file) {free(source);free(program);return "Cannot create the compiler output log.";}

 if(run) {
  strcpy(run_dir,"/tmp/divseed-run-XXXXXX");
  if(!mkdtemp(run_dir)) {
   run_dir[0]=0;
   fclose(log_file);log_file=NULL;
   free(source);free(program);
   return "Cannot create a temporary directory for Run.";
  }
  snprintf(executable,sizeof executable,"%s/program",run_dir);
 }
 char *args[]={program,"-fdiagnostics-color=never","-x","c",source,"-o",run?executable:"/dev/null",run?NULL:"-c",NULL};
 int error=spawn_logged(program,args);
 running_program=0;
 stop_reason=NULL;
 free(source);
 if(error) {
  child=0;
  remove_executable();
  fclose(log_file);log_file=NULL;
  snprintf(status,sizeof status,"Cannot launch compiler (%s).",strerror(error));
 } else {
  count=0;
  snprintf(status,sizeof status,"Compiling with %s...",strrchr(program,'/')+1);
 }
 free(program);
 return error?status:NULL;
}

void compiler_poll(void) {
 if(!compiler_busy()) return;
 int result;
 pid_t done=waitpid(child,&result,WNOHANG);
 if(done<0 && errno==EINTR) return;
 if(!done) {
  if(running_program && !stop_reason) {
   struct timespec now;
   struct stat st;
   if(!fstat(fileno(log_file),&st) && st.st_size>=RUN_OUTPUT_LIMIT)
    stop_reason="Program stopped after reaching the output limit.";
   else if(!clock_gettime(CLOCK_MONOTONIC,&now) &&
           (now.tv_sec-run_started.tv_sec>RUN_SECONDS ||
            (now.tv_sec-run_started.tv_sec==RUN_SECONDS && now.tv_nsec>=run_started.tv_nsec)))
    stop_reason="Program stopped after 10 seconds.";
   if(stop_reason) kill(-child,SIGKILL);
  }
  return;
 }
 child=0;
 if(done<0) snprintf(status,sizeof status,"Could not read the process exit status.");
 else if(running_program) {
  if(stop_reason) snprintf(status,sizeof status,"%s",stop_reason);
  else if(WIFEXITED(result)) snprintf(status,sizeof status,"Program exited with code %d.",WEXITSTATUS(result));
  else if(WIFSIGNALED(result)) snprintf(status,sizeof status,"Program stopped (signal %d).",WTERMSIG(result));
 } else if(WIFEXITED(result) && WEXITSTATUS(result)==0) {
  if(run_dir[0]) {
   char *args[]={executable,NULL};
   int error=spawn_logged(executable,args);
   if(!error) {
    running_program=1;
    clock_gettime(CLOCK_MONOTONIC,&run_started);
    snprintf(status,sizeof status,"Running program...");
    return;
   }
   child=0;
   snprintf(status,sizeof status,"Cannot launch program (%s).",strerror(error));
  } else snprintf(status,sizeof status,"Compilation succeeded.");
 } else if(WIFEXITED(result)) snprintf(status,sizeof status,"Compilation failed (exit %d).",WEXITSTATUS(result));
 else if(WIFSIGNALED(result)) snprintf(status,sizeof status,"Compilation interrupted (signal %d).",WTERMSIG(result));
 remove_executable();

 rewind(log_file);
 char buffer[4096];
 int truncated=0;
 while(count<MAX_LINES-1 && fgets(buffer,sizeof buffer,log_file)) {
  size_t length=strcspn(buffer,"\r\n");
  for(size_t i=0;i<length;i++) if((unsigned char)buffer[i]<32) buffer[i]=' ';
  size_t start=0;
  do {
   size_t width=utf8_offset(buffer+start,length-start,COMPILER_COLUMNS);
   memcpy(lines[count],buffer+start,width);
   lines[count++][width]=0;
   start+=width;
  } while(start<length && count<MAX_LINES-1);
  truncated=start<length;
 }
 if(truncated || fgetc(log_file)!=EOF) strcpy(lines[count++],"Additional compiler output omitted.");
 else if(ferror(log_file)) strcpy(lines[count++],"Could not read compiler output.");
 fclose(log_file);log_file=NULL;
}

void compiler_destroy(void) {
 if(compiler_busy()) {
  kill(-child,SIGKILL);
  while(waitpid(child,NULL,0)<0 && errno==EINTR) {}
  child=0;
 }
 remove_executable();
 if(log_file) fclose(log_file);
 log_file=NULL;
 count=0;
}
