#pragma once

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include "../protect/oxorany.hpp"
#include "driver.h"

namespace proc {
    inline int      pid = -1;
    inline uint64_t lib = 0;
    // proc::pm removido — pagemap FD ficava aberto e nunca era lido,
    // deixando um handle pro /proc/<pid>/pagemap detectável em /proc/self/fd.
    inline int      pm  = -1;   // mantido só pra código legado, sempre = -1
}

#ifdef SYS_openat
#undef SYS_openat
#endif
#ifdef SYS_close
#undef SYS_close
#endif
#ifdef SYS_read
#undef SYS_read
#endif
#ifdef SYS_getdents64
#undef SYS_getdents64
#endif
#ifdef SYS_vm_read
#undef SYS_vm_read
#endif
#ifdef SYS_vm_write
#undef SYS_vm_write
#endif

namespace sys {
    #if defined(__aarch64__)
    static constexpr long SYS_openat     = 56;
    static constexpr long SYS_close      = 57;
    static constexpr long SYS_read       = 63;
    static constexpr long SYS_getdents64 = 61;
    static constexpr long SYS_vm_read    = 270;
    static constexpr long SYS_vm_write   = 271;
    #elif defined(__x86_64__)
    static constexpr long SYS_openat     = 257;
    static constexpr long SYS_close      = 3;
    static constexpr long SYS_read       = 0;
    static constexpr long SYS_getdents64 = 217;
    static constexpr long SYS_vm_read    = 310;
    static constexpr long SYS_vm_write   = 311;
    #endif

    #ifndef AT_FDCWD
    #define AT_FDCWD -100
    #endif

    static inline long sc6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
        long r;
        #if defined(__aarch64__)
        register long x8 __asm__("x8") = n;
        register long x0 __asm__("x0") = a1;
        register long x1 __asm__("x1") = a2;
        register long x2 __asm__("x2") = a3;
        register long x3 __asm__("x3") = a4;
        register long x4 __asm__("x4") = a5;
        register long x5 __asm__("x5") = a6;
        __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5) : "memory", "cc");
        r = x0;
        #elif defined(__x86_64__)
        register long rax asm("rax") = n;
        register long rdi asm("rdi") = a1;
        register long rsi asm("rsi") = a2;
        register long rdx asm("rdx") = a3;
        register long r10 asm("r10") = a4;
        register long r8  asm("r8")  = a5;
        register long r9  asm("r9")  = a6;
        asm volatile("syscall" : "=a"(r) : "a"(rax), "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
        #else
        r = syscall(n, a1, a2, a3, a4, a5, a6);
        #endif
        return r;
    }

    static inline int  openat(int fd, const char* p, int f)  { return static_cast<int>(sc6(SYS_openat, fd, reinterpret_cast<long>(p), f, 0, 0, 0)); }
    static inline int  close (int fd)                        { return static_cast<int>(sc6(SYS_close, fd, 0, 0, 0, 0, 0)); }
    static inline long read  (int fd, void* b, size_t c)     { return sc6(SYS_read, fd, reinterpret_cast<long>(b), c, 0, 0, 0); }
    static inline long getdents64(int fd, void* d, size_t c) { return sc6(SYS_getdents64, fd, reinterpret_cast<long>(d), c, 0, 0, 0); }
    static inline long vm_read (int pid, struct iovec* l, unsigned long lc, struct iovec* r, unsigned long rc) {
        return sc6(SYS_vm_read, pid, reinterpret_cast<long>(l), lc, reinterpret_cast<long>(r), rc, 0);
    }
    static inline long vm_write(int pid, struct iovec* l, unsigned long lc, struct iovec* r, unsigned long rc) {
        return sc6(SYS_vm_write, pid, reinterpret_cast<long>(l), lc, reinterpret_cast<long>(r), rc, 0);
    }
}

struct linux_dirent64 {
    uint64_t d_ino;
    int64_t  d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char d_name[];
};

// Wipe stack buffers depois de usar — nada de path linger em snapshots de memória.
static inline void wipe(void* p, size_t n) {
    volatile uint8_t* v = reinterpret_cast<volatile uint8_t*>(p);
    for (size_t i = 0; i < n; i++) v[i] = 0;
}

inline bool addr_valid(uint64_t a) { return a >= 0x1000; }

inline int get_proc_mem_fd() {
    static int s_fd = -1;
    static int s_last_pid = -1;
    if (proc::pid <= 0) return -1;
    if (proc::pid != s_last_pid || s_fd < 0) {
        if (s_fd >= 0) close(s_fd);
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/mem", proc::pid);
        s_fd = open(path, O_RDWR);
        s_last_pid = proc::pid;
    }
    return s_fd;
}

inline bool mem_read(uint64_t a, void* b, size_t l) {
    if (proc::pid < 0 || a < 0x1000 || l == 0 || b == nullptr) return false;
    
    if (select_driver_option == 1 || select_driver_option == 2 || select_driver_option == 3) {
        if (!driver) return false;
        driver->initialize(proc::pid);
        return driver->read(a, b, l);
    }

    struct iovec loc[1], rem[1];
    loc[0].iov_base = b;
    loc[0].iov_len  = l;
    rem[0].iov_base = reinterpret_cast<void*>(a);
    rem[0].iov_len  = l;
    ssize_t r = sys::vm_read(proc::pid, loc, 1, rem, 1);
    if (r == static_cast<long>(l)) return true;

    // Fallback to /proc/<pid>/mem pread64 if vm_read is denied
    int fd = get_proc_mem_fd();
    if (fd >= 0) {
        ssize_t pr = pread64(fd, b, l, static_cast<off64_t>(a));
        if (pr == static_cast<long>(l)) return true;
    }

    memset(b, 0, l);
    return false;
}

inline bool mem_write(uint64_t a, const void* b, size_t l) {
    if (proc::pid < 0 || a == 0 || l == 0 || b == nullptr) return false;

    if (select_driver_option == 1 || select_driver_option == 2 || select_driver_option == 3) {
        if (!driver) return false;
        driver->initialize(proc::pid);
        return driver->write(a, const_cast<void*>(b), l);
    }

    struct iovec loc[1], rem[1];
    loc[0].iov_base = const_cast<void*>(b);
    loc[0].iov_len  = l;
    rem[0].iov_base = reinterpret_cast<void*>(a);
    rem[0].iov_len  = l;
    if (sys::vm_write(proc::pid, loc, 1, rem, 1) == static_cast<long>(l)) return true;

    // Fallback to /proc/<pid>/mem pwrite64
    int fd = get_proc_mem_fd();
    if (fd >= 0) {
        return pwrite64(fd, b, l, static_cast<off64_t>(a)) == static_cast<long>(l);
    }
    return false;
}

template<typename T> inline T rpm(uint64_t a) {
    T v{};
    mem_read(a, &v, sizeof(T));
    return v;
}

template<typename T> inline bool wpm(uint64_t a, const T& d) {
    return mem_write(a, &d, sizeof(T));
}

// Chunk reader stealth — 1 syscall pra ler o struct do player inteiro.
struct MemChunk {
    uint64_t base;
    size_t   size;
    uint8_t  buf[0x2400];
    bool     ok;

    inline bool load(uint64_t addr, size_t len) {
        if (len > sizeof(buf)) len = sizeof(buf);
        base = addr;
        size = len;
        ok   = mem_read(addr, buf, len);
        return ok;
    }

    template<typename T>
    inline T at(size_t off) const {
        T v{};
        if (!ok || off + sizeof(T) > size) return v;
        memcpy(&v, buf + off, sizeof(T));
        return v;
    }
};

inline int get_pid() {
    int pfd = sys::openat(AT_FDCWD, oxorany("/proc"), O_RDONLY | O_DIRECTORY);
    if (pfd < 0) return -1;

    char buf[4096];
    int found = -1;

    while (true) {
        long nr = sys::getdents64(pfd, buf, sizeof(buf));
        if (nr <= 0) break;

        char* p = buf;
        while (p < buf + nr) {
            auto* e = reinterpret_cast<linux_dirent64*>(p);

            if (e->d_type == 4) {
                char* n = e->d_name;
                bool in = true;
                for (char* c = n; *c; c++) {
                    if (*c < '0' || *c > '9') { in = false; break; }
                }

                if (in && *n != '\0') {
                    int pid = 0;
                    for (char* c = n; *c; c++) pid = pid * 10 + (*c - '0');

                    char cp[256];
                    int ps = 0;
                    const char* ps_ = oxorany("/proc/");
                    while (*ps_) cp[ps++] = *ps_++;
                    for (char* c = n; *c; c++) cp[ps++] = *c;
                    const char* cs = oxorany("/cmdline");
                    while (*cs) cp[ps++] = *cs++;
                    cp[ps] = '\0';

                    int cfd = sys::openat(AT_FDCWD, cp, O_RDONLY);
                    wipe(cp, sizeof(cp));                       // scrub path
                    if (cfd >= 0) {
                        char cmd[256] = {0};
                        long b = sys::read(cfd, cmd, sizeof(cmd) - 1);
                        sys::close(cfd);

                        if (b > 0) {
                            cmd[b] = '\0';
                            auto match_tg = [&](const char* tg) {
                                int tl = 0;
                                while (tg[tl]) tl++;
                                if (b >= tl) {
                                    for (int i = 0; i <= b - tl; i++) {
                                        bool fd = true;
                                        for (int j = 0; j < tl; j++) {
                                            if (cmd[i + j] != tg[j]) { fd = false; break; }
                                        }
                                        if (fd) return true;
                                    }
                                }
                                return false;
                            };

                            if (match_tg(oxorany("com.dts.freefiremax")) || match_tg(oxorany("com.dts.freefireth"))) {
                                found = pid;
                            }
                            wipe(cmd, sizeof(cmd));             // scrub cmdline
                        }
                    }
                    if (found > 0) break;
                }
            }

            p += e->d_reclen;
        }
        if (found > 0) break;
    }

    sys::close(pfd);
    wipe(buf, sizeof(buf));
    return found;
}

inline uint64_t get_lib() {
    if (proc::pid <= 0) return 0;

    if (select_driver_option == 1 || select_driver_option == 2 || select_driver_option == 3) {
        if (!driver) return 0;
        driver->initialize(proc::pid);
        return driver->getModuleBase("libil2cpp.so");
    }

    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/maps", proc::pid);
    FILE* maps = fopen(path, "rt");
    if (!maps) return 0;

    char line[1024];
    uint64_t exact_base = 0;
    uint64_t fallback_base = 0;

    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, "libil2cpp.so")) {
            uint64_t start = 0, end = 0;
            if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
                // Must match old ff: r--p and size < 0x5100000 (skips giant APK zip mappings)
                if (strstr(line, "r--p") && ((end - start) < 0x5100000)) {
                    uint32_t magic = 0;
                    if (mem_read(start, &magic, sizeof(magic)) && magic == 0x464C457F) {
                        exact_base = start;
                        break;
                    }
                    if (fallback_base == 0) fallback_base = start;
                } else if (strstr(line, "r-xp") && ((end - start) < 0x5100000)) {
                    uint32_t magic = 0;
                    if (mem_read(start, &magic, sizeof(magic)) && magic == 0x464C457F) {
                        if (exact_base == 0) exact_base = start;
                    }
                }
            }
        }
    }
    fclose(maps);

    if (exact_base != 0) return exact_base;
    return fallback_base;
}

// Compatibilidade: main.cpp ainda chama isso, mas agora é no-op.
inline void init_mem() { /* pagemap removido */ }

namespace game {
    inline int  tick        = 0;
    inline int  jitter_next = 47;   // primeiro check em ~47 ticks

    inline bool valid() {
        if (proc::pid <= 0) {
            proc::pid = get_pid();
            if (proc::pid <= 0) return false;
        }

        if (proc::lib == 0) {
            proc::lib = get_lib();
            return false;
        }

        tick++;
        if (tick >= jitter_next) {
            int p = get_pid();
            if (p != proc::pid) {
                proc::pid = p;
                if (proc::pid > 0 && driver) driver->initialize(proc::pid);
                proc::lib = 0;
                tick = 0;
                jitter_next = 47;
                return false;
            }
            tick = 0;
            // xorshift barato pra jitter no próximo intervalo (40..70 ticks)
            static uint32_t s = 0xC5A1B4E7u;
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            jitter_next = 40 + (int)(s % 31u);
        }
        return true;
    }

    inline void init() {
        proc::pid = get_pid();
        if (proc::pid > 0) {
            if (driver) driver->initialize(proc::pid);
            proc::lib = get_lib();
        }
    }
}
