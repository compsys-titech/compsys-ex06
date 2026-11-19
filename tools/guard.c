/* Native process monitor and source-boundary checker. Not a security sandbox. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#endif

static char *read_text(const char *path)
{
    FILE *f = fopen(path,"rb");
    if (!f) return NULL;
    if (fseek(f,0,SEEK_END) != 0) { fclose(f); return NULL; }
    long length = ftell(f);
    if (length < 0 || length > 4 * 1024 * 1024) { fclose(f); return NULL; }
    rewind(f);
    char *s = malloc((size_t)length + 1);
    if (!s) { fclose(f); return NULL; }
    size_t n = fread(s,1,(size_t)length,f);
    fclose(f);
    size_t j = 0;
    for (size_t i = 0; i < n; ++i) if (s[i] != '\r') s[j++] = s[i];
    s[j] = '\0';
    return s;
}

static uint64_t hash_text(const char *s)
{
    uint64_t h = UINT64_C(14695981039346656037);
    while (*s) { h ^= (unsigned char)*s++; h *= UINT64_C(1099511628211); }
    return h;
}

static int boundaries(const char *path, const char *original,
                      const char *const names[], unsigned count)
{
    char *actual = read_text(path), *base = read_text(original);
    if (!actual || !base) { free(actual); free(base); return 0; }
    const char *a = actual, *b = base;
    int ok = 1;
    for (unsigned i = 0; i < count && ok; ++i) {
        char start[100], end[100];
        snprintf(start,sizeof start,"/* TODO-BEGIN %s */",names[i]);
        snprintf(end,sizeof end,"/* TODO-END %s */",names[i]);
        const char *as = strstr(a,start), *bs = strstr(b,start);
        const char *ae = as ? strstr(as + strlen(start),end) : NULL;
        const char *be = bs ? strstr(bs + strlen(start),end) : NULL;
        if (!as || !bs || !ae || !be || strstr(as + strlen(start),start) ||
            strstr(ae + strlen(end),end)) { ok = 0; break; }
        size_t na = (size_t)(as - a) + strlen(start);
        size_t nb = (size_t)(bs - b) + strlen(start);
        if (na != nb || memcmp(a,b,na)) { ok = 0; break; }
        as += strlen(start); bs += strlen(start);
        if (strstr(names[i],"HELPERS") == NULL &&
            ae - as == be - bs && !memcmp(as,bs,(size_t)(ae - as)))
            printf("[UNFINISHED] starter region %s (%s)\n",names[i],path);
        a = ae; b = be;
    }
    if (ok && strcmp(a,b)) ok = 0;
    /* Extra reserved markers inside an allowed body are also a SOURCE error. */
    unsigned starts = 0, ends = 0;
    for (const char *p = actual; (p = strstr(p,"TODO-BEGIN")) != NULL; ++p) ++starts;
    for (const char *p = actual; (p = strstr(p,"TODO-END")) != NULL; ++p) ++ends;
    if (starts != count || ends != count) ok = 0;
    free(actual); free(base);
    if (!ok) fprintf(stderr,"[SOURCE] missing/duplicate boundary or protected text changed: %s\n",path);
    return ok;
}

static int source_check(void)
{
    FILE *f = fopen("tests/protected.fnv","r");
    if (!f) { fputs("[SOURCE] missing tests/protected.fnv\n",stderr); return 8; }
    char path[256], line[320];
    uint64_t expected;
    unsigned count = 0;
    int ok = 1;
    while (fgets(line,sizeof line,f)) {
        if (sscanf(line,"%" SCNx64 " %255s",&expected,path) != 2) { ok = 0; break; }
        char *s = read_text(path);
        if (!s || hash_text(s) != expected) {
            fprintf(stderr,"[SOURCE] supplied file changed/missing: %s\n",path); ok = 0;
        }
        free(s); ++count;
    }
    fclose(f);
    if (count == 0) ok = 0;
    const char *const a[] = {"A_HELPERS","REGISTER","DECODER5","MUX32","REGISTER_FILE"};
    const char *const b[] = {"B_HELPERS","INST_MEMORY","DATA_MEMORY"};
    if (!boundaries("tasks/register_task.c","tests/register_task.original.c",a,5)) ok = 0;
    if (!boundaries("tasks/memory_task.c","tests/memory_task.original.c",b,3)) ok = 0;
    if (ok) printf("[PASS] SOURCE protected=%u; task boundaries intact (CRLF accepted)\n",count);
    return ok ? 0 : 8;
}

static volatile sig_atomic_t interrupted = 0;
#ifdef _WIN32
static BOOL WINAPI on_console(DWORD event)
{
    (void)event; interrupted = 1; return TRUE;
}
static double now_seconds(void) { return (double)GetTickCount64() / 1000.0; }
/* Windows CreateProcess uses one command line. Quote using its documented
   backslash-before-quote rules; never invoke cmd.exe for a C executable. */
static char *command_line(char **argv)
{
    size_t size = 1;
    for (unsigned i = 0; argv[i]; ++i) size += strlen(argv[i]) * 2 + 4;
    char *line = malloc(size), *p = line;
    if (!line) return NULL;
    for (unsigned i = 0; argv[i]; ++i) {
        *p++ = '"';
        unsigned slashes = 0;
        for (const char *s = argv[i]; ; ++s) {
            if (*s == '\\') { ++slashes; continue; }
            unsigned repeat = (*s == '"' || *s == '\0') ? slashes * 2 : slashes;
            while (repeat--) *p++ = '\\';
            slashes = 0;
            if (*s == '"') *p++ = '\\';
            if (!*s) break;
            *p++ = *s;
        }
        *p++ = '"'; *p++ = ' ';
    }
    *p = '\0';
    return line;
}
#else
static void on_signal(int sig) { interrupted = sig; }
static double now_seconds(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}
#endif

static int run_child(int argc, char **argv)
{
    /* guard run seconds bytes log-prefix completion-group executable args... */
    if (argc < 7) return 2;
    char *end = NULL;
    double timeout = strtod(argv[2],&end);
    if (!end || *end || !(timeout > 0 && timeout <= 3600)) return 2;
    unsigned long limit = strtoul(argv[3],&end,10);
    if (!end || *end || limit < 1024 || limit > 64UL * 1024 * 1024) return 2;
    char path[4096];
    FILE *logs[2];
    snprintf(path,sizeof path,"%s.stdout",argv[4]); logs[0] = fopen(path,"wb");
    snprintf(path,sizeof path,"%s.stderr",argv[4]); logs[1] = fopen(path,"wb");
    snprintf(path,sizeof path,"%s.meta",argv[4]); FILE *meta = fopen(path,"w");
    if (!logs[0] || !logs[1] || !meta) return 7;
    fprintf(meta,"start_epoch=%lld\ntimeout=%.3f\noutput_limit=%lu\n",
            (long long)time(NULL),timeout,limit);
    for (int i = 6; i < argc; ++i) fprintf(meta,"argv[%d]=%s\n",i-6,argv[i]);
    fflush(meta);
    char *captured = calloc((size_t)limit + 1,1);
    if (!captured) return 7;
    char *errors = calloc((size_t)limit + 1,1);
    if (!errors) { free(captured); return 7; }
    size_t total = 0, out_length = 0, err_length = 0;
    int reason = 0, raw = -1, exited = 0;
    double start = now_seconds();
#ifdef _WIN32
    SetConsoleCtrlHandler(on_console,TRUE);
    SECURITY_ATTRIBUTES sa = {sizeof sa,NULL,TRUE};
    HANDLE reads[2], writes[2];
    for (unsigned i = 0; i < 2; ++i) {
        if (!CreatePipe(&reads[i],&writes[i],&sa,0)) return 7;
        SetHandleInformation(reads[i],HANDLE_FLAG_INHERIT,0);
    }
    HANDLE job = CreateJobObjectA(NULL,NULL);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits;
    memset(&limits,0,sizeof limits);
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!job || !SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof limits)) return 7;
    STARTUPINFOA si; PROCESS_INFORMATION pi;
    memset(&si,0,sizeof si); memset(&pi,0,sizeof pi);
    si.cb = sizeof si; si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = writes[0]; si.hStdError = writes[1]; si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    char *line = command_line(argv + 6);
    int wlen = line ? MultiByteToWideChar(CP_UTF8,0,line,-1,NULL,0) : 0;
    wchar_t *wline = wlen ? malloc((size_t)wlen * sizeof(wchar_t)) : NULL;
    if (wline) MultiByteToWideChar(CP_UTF8,0,line,-1,wline,wlen);
    STARTUPINFOW wsi;
    memset(&wsi,0,sizeof wsi); wsi.cb = sizeof wsi; wsi.dwFlags = si.dwFlags;
    wsi.hStdInput = si.hStdInput; wsi.hStdOutput = si.hStdOutput; wsi.hStdError = si.hStdError;
    if (!wline || !CreateProcessW(NULL,wline,NULL,NULL,TRUE,CREATE_SUSPENDED,NULL,NULL,&wsi,&pi)) {
        fprintf(stderr,"[RUNTIME] cannot execute child: Windows error %lu\n",GetLastError());
        return 7;
    }
    free(line); free(wline);
    if (!AssignProcessToJobObject(job,pi.hProcess)) {
        TerminateProcess(pi.hProcess,7); WaitForSingleObject(pi.hProcess,INFINITE); return 7;
    }
    fprintf(meta,"pid=%lu\n",(unsigned long)pi.dwProcessId); fflush(meta);
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread); CloseHandle(writes[0]); CloseHandle(writes[1]);
#else
    struct sigaction action;
    memset(&action,0,sizeof action); action.sa_handler = on_signal;
    sigaction(SIGTERM,&action,NULL); sigaction(SIGINT,&action,NULL); sigaction(SIGHUP,&action,NULL);
    int pipes[2][2];
    if (pipe(pipes[0]) || pipe(pipes[1])) return 7;
    #ifdef __linux__
    if (prctl(PR_SET_CHILD_SUBREAPER,1) != 0) { fputs("[RUNTIME] subreaper setup failed\n",stderr); return 7; }
#endif
    pid_t pid = fork();
    if (pid < 0) return 7;
    if (pid == 0) {
        setpgid(0,0);
        signal(SIGTERM,SIG_DFL); signal(SIGINT,SIG_DFL); signal(SIGHUP,SIG_DFL);
        dup2(pipes[0][1],STDOUT_FILENO); dup2(pipes[1][1],STDERR_FILENO);
        for (unsigned i = 0; i < 2; ++i) { close(pipes[i][0]); close(pipes[i][1]); }
        execvp(argv[6],argv + 6);
        fprintf(stderr,"[RUNTIME] exec failed: %s\n",strerror(errno)); _exit(127);
    }
    setpgid(pid,pid);
    fprintf(meta,"pid=%ld\npgid=%ld\n",(long)pid,(long)pid); fflush(meta);
    for (unsigned i = 0; i < 2; ++i) {
        close(pipes[i][1]); fcntl(pipes[i][0],F_SETFL,O_NONBLOCK);
    }
#endif
    for (;;) {
        size_t read_this_cycle = 0;
        for (unsigned stream = 0; stream < 2 && !reason; ++stream) {
            for (;;) {
                char buffer[4096];
#ifdef _WIN32
                DWORD available = 0, n = 0;
                if (!PeekNamedPipe(reads[stream],NULL,0,NULL,&available,NULL) || !available) break;
                if (!ReadFile(reads[stream],buffer,sizeof buffer,&n,NULL) || !n) break;
#else
                ssize_t n = read(pipes[stream][0],buffer,sizeof buffer);
                if (n < 0 && errno == EINTR) continue;
                if (n <= 0) break;
#endif
                size_t keep = (size_t)n;
                if (total + keep > limit) { keep = (size_t)limit - total; reason = 7; }
                fwrite(buffer,1,keep,logs[stream]);
                if (stream == 0) { memcpy(captured + out_length,buffer,keep); out_length += keep; }
                if (stream == 1) { memcpy(errors + err_length,buffer,keep); err_length += keep; }
                total += keep; read_this_cycle += keep;
                if (reason) { fputs("[RUNTIME] output limit exceeded\n",stderr); break; }
                /* Bound each drain cycle even if the child writes continuously. */
                if (read_this_cycle >= 65536) break;
            }
        }
        if (interrupted) { reason = 7; fputs("[RUNTIME] interrupted; stopping owned process group/job\n",stderr); }
        if (!reason && now_seconds() - start > timeout) {
            reason = 6; fputs("[TIMEOUT] wall-clock execution limit\n",stderr);
        }
        /* An empty drain is final only if the child had already exited. */
        if (reason || (exited && read_this_cycle == 0)) break;
#ifdef _WIN32
        DWORD code;
        if (!exited && GetExitCodeProcess(pi.hProcess,&code) && code != STILL_ACTIVE) {
            raw = (int)code; exited = 1;
        }
#else
        int status;
        if (!exited && waitpid(pid,&status,WNOHANG) == pid) {
            raw = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
            exited = 1;
        }
#endif
        if (exited) continue;
#ifdef _WIN32
        Sleep(10);
#else
        struct timespec delay = {0,10000000}; nanosleep(&delay,NULL);
#endif
    }
    /* Also remove descendants left behind after an otherwise normal exit. */
#ifdef _WIN32
    TerminateJobObject(job,7);
    WaitForSingleObject(pi.hProcess,INFINITE);
    if (!exited) { DWORD code; GetExitCodeProcess(pi.hProcess,&code); raw = (int)code; }
    CloseHandle(pi.hProcess); CloseHandle(job);
    for (unsigned i = 0; i < 2; ++i) CloseHandle(reads[i]);
#else
    kill(-pid,SIGTERM);
    struct timespec grace = {0,50000000}; nanosleep(&grace,NULL);
    kill(-pid,SIGKILL);
    if (!exited) {
        int status;
        while (waitpid(pid,&status,0) < 0 && errno == EINTR) {}
        raw = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    }
    #ifdef __linux__
    /* Adopted descendants in this owned group have been stopped above. */
    while (waitpid(-pid,NULL,0) > 0 || errno == EINTR) {}
#endif
    for (unsigned i = 0; i < 2; ++i) close(pipes[i][0]);
#endif
    for (unsigned i = 0; i < 2; ++i) fclose(logs[i]);
    int result = reason;
    int expected_error = !strncmp(argv[5],"error/",6);
    if (!result && expected_error) {
        int expected_exit = 0; char id[80], component[80]; unsigned address, base;
        int parsed = sscanf(argv[5],"error/%d/%79[^/]/%79[^/]/%x/%x",&expected_exit,id,component,&address,&base);
        char identifier[100], unit[100], ad[100], ba[100];
        snprintf(identifier,sizeof identifier,"id=%s",parsed == 5 ? id : "INVALID");
        snprintf(unit,sizeof unit,"component=%s",parsed == 5 ? component : "INVALID");
        snprintf(ad,sizeof ad,"address=0x%08x",parsed == 5 ? address : 0);
        snprintf(ba,sizeof ba,"base=0x%08x",parsed == 5 ? base : 0);
        int match = parsed == 5 && raw == expected_exit && strstr(errors,identifier) && strstr(errors,unit);
        if (expected_exit == 10) match = match && strstr(errors,"[ADDRESS]") && strstr(errors,ad) && strstr(errors,ba);
        else if (expected_exit == 11) match = match && strstr(errors,"[CONTROL]") && strstr(errors,"mem_write=1 mem_read=1");
        else match = 0;
        const char *name = argc > 8 ? argv[8] : "unknown";
        const char *number = argc > 9 ? argv[9] : "unknown";
        printf("[%s] E_%s_%s expected_exit=%d actual_exit=%d expected_diagnostic=%s match=%s\n",
               match ? "PASS" : "FAIL",name,number,expected_exit,raw,argv[5],match ? "yes" : "NO");
        if (match) printf("RESULT E_%s_%s PASS cases=1 failures=0\n",name,number);
        else result = 5;
    }
    if (!result && !expected_error && raw != 0) {
        result = raw == 3 ? 3 : raw == 5 ? 5 : 7;
        fprintf(stderr,"[%s] child exit=%d\n",result == 3 ? "UNFINISHED" : result == 5 ? "FAIL" : "RUNTIME",raw);
    }
    if (!result && !expected_error && strcmp(argv[5],"-")) {
        char wanted[160];
        snprintf(wanted,sizeof wanted,"RESULT %s PASS",argv[5]);
        /* Require a final complete report, not just process exit zero. */
        char *last = captured;
        for (char *p = captured; *p; ++p) if (*p == '\n' && p[1]) last = p + 1;
        int valid = !strncmp(last,wanted,strlen(wanted));
        if (!strcmp(argv[5],"check"))
            valid = valid && (!strcmp(last + strlen(wanted),"\n") ||
                              !strcmp(last + strlen(wanted),"\r\n"));
        else {
            unsigned cases = 0, failures = 1; char extra;
            valid = valid && sscanf(last + strlen(wanted)," cases=%u failures=%u %c",&cases,&failures,&extra) == 2
                    && cases > 0 && failures == 0;
        }
        if (!valid) { fputs("[RUNTIME] missing/invalid final RESULT report\n",stderr); result = 7; }
    }
    fprintf(meta,"raw_exit=%d\nrunner_exit=%d\nbytes=%zu\nelapsed=%.3f\nreaped=yes\n",
            raw,result,total,now_seconds()-start);
    fclose(meta); free(captured); free(errors);
    return result;
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    /* Preserve Japanese/space-containing compiler paths on every Windows locale. */
    int wide_argc;
    wchar_t **wide_argv = CommandLineToArgvW(GetCommandLineW(),&wide_argc);
    if (!wide_argv) return 7;
    char **utf8_argv = calloc((size_t)wide_argc + 1,sizeof *utf8_argv);
    if (!utf8_argv) return 7;
    for (int i = 0; i < wide_argc; ++i) {
        int n = WideCharToMultiByte(CP_UTF8,0,wide_argv[i],-1,NULL,0,NULL,NULL);
        utf8_argv[i] = malloc((size_t)n);
        if (!utf8_argv[i]) return 7;
        WideCharToMultiByte(CP_UTF8,0,wide_argv[i],-1,utf8_argv[i],n,NULL,NULL);
    }
    argc = wide_argc; argv = utf8_argv;
    LocalFree(wide_argv);
#endif
    if (argc == 2 && !strcmp(argv[1],"source")) return source_check();
    if (argc > 1 && !strcmp(argv[1],"run")) return run_child(argc,argv);
    fputs("usage: guard source | guard run seconds bytes prefix group executable args...\n",stderr);
    return 2;
}
