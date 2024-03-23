#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> // EAI_NONAME
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <bits/types/res_state.h>
#include <stdbool.h>

#define RTLD_NEXT ((void*) -1l)
extern void* dlsym(void* handle, const char* name);

//#define LOOPBACK_IN 0x7f000001
#define LOOPBACK_IN 0x0100007f

//#define SNF_DEBUG
#ifdef SNF_DEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

// to make vscode happy
#ifndef EAI_NONAME
#define EAI_NONAME -2
#endif

#if 0
// Currently unused
static char* _snf_getenv(char* name, char* def) {
    char* value = getenv(name);

    if (value)
        return value;
    return def;
}
#endif

static bool _snf_getenv_bool(char* name, bool def) {
    char* value = getenv(name);

    if (!value)
        return def;

    if (!strcmp(value, "0") || !strcmp(value, "false") || !strcmp(value, "no") || !strcmp(value, "off"))
        return false;

    return true;
}

#define _snf_get_config_bool(name, def) {\
    static bool value = def;\
    static bool value_initialized = false;\
    \
    if (!value_initialized) {\
        value = _snf_getenv_bool(name, def);\
        value_initialized = true;\
    }\
    \
    return value;\
}

static bool _snf_loopback_allowed(void) {
    _snf_get_config_bool("SIMPLENETFILT_ALLOW_LOCALHOST", true);
}

static bool _snf_localnet_allowed(void) {
    _snf_get_config_bool("SIMPLENETFILT_ALLOW_LOCALNET", false);
}

static int _snf_ok_addr(const struct sockaddr* addr) {
    if (!addr)
        return 1;

    // TODO: support ipv6
    if (addr->sa_family == AF_INET6) {
        return 0;
    }

    // non-inet is ok
    if (addr->sa_family != AF_INET) {
        return 1;
    }

    // loopback is ok
    struct sockaddr_in* in = (struct sockaddr_in*) addr;
    debug("ok_addr: %d\n", in->sin_addr.s_addr);

    if (_snf_loopback_allowed() && in->sin_addr.s_addr == LOOPBACK_IN) {
        return 1;
    }

    if (_snf_localnet_allowed()) {
        // https://en.wikipedia.org/wiki/Private_network

        // 192.168.*.*
        if ((in->sin_addr.s_addr & 0xffff) == ((168<<8)+192)) {
            return 1;
        }

        // 10.*.*.*
        if ((in->sin_addr.s_addr & 0xff) == 10) {
            return 1;
        }

        // TODO: 172.16.0.0 – 172.31.255.255
    }

    return 0;
}

static bool _snf_host_is_ip(const char* host) {
    // FIXME: extremely hacky, only works for ipv4

    for (const char* c = host; *c; c++) {
        if (*c >= '0' && *c <= '9')
            continue;
        if (*c == '.')
            continue;

        return false;
    }

    return true;
}

static int _snf_ok_host(const char* host) {
    static char _hostname[1024];
    static int _hostname_set = 0;

    if (_hostname_set == 0) {
        _hostname[0] = 0; // for races?
        _hostname_set = 1;

        if (gethostname(_hostname, 1023)) {
            _hostname[0] = 0;
        }
    }

    debug("ok_host: %s\n", host);fflush(stdout);
    if (!host || !host[0])
        return 1;

    // Resolve IP addresses
    if (_snf_host_is_ip(host))
        return 1;

    // wine checks the local hostname
    // even if loopback isn't allowed, any system is expected to resolve these
    if (!strcmp(host, "localhost") || !strcmp(host, "127.0.0.1") || !strcmp(host, _hostname))
        return 1;

    // FIXME: resolve hostname.local even if localnet isn't allowed
    if (_snf_localnet_allowed()) {
        size_t len = strlen(host);
        char* substr = strstr(host, ".local");

        if (!substr)
            return 0;

        if (substr - host == len - 6)
            return 1;
    }

    return 0;
}

#define WRAP_FN(name)\
    static f_##name##_t orig_fn = NULL;\
    if (!orig_fn)\
        orig_fn = dlsym(RTLD_NEXT, #name);

typedef ssize_t(*f_sendto_t)(int, const void*, size_t, int, const struct sockaddr*, socklen_t);
ssize_t sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    WRAP_FN(sendto);

    if (!_snf_ok_addr(dest_addr)) {
        errno = EHOSTUNREACH;
        return -1;
    }

    return orig_fn(sockfd, buf, len, flags, dest_addr, addrlen);
}

typedef int (*f_connect_t)(int, const struct sockaddr*, socklen_t);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    WRAP_FN(connect);

    if (!_snf_ok_addr(addr)) {
        errno = EHOSTUNREACH;
        return -1;
    }

    return orig_fn(sockfd, addr, addrlen);
}

typedef int (*f_getaddrinfo_t)(const char*, const char*, const struct addrinfo*, struct addrinfo**);
int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo ** res) {
    WRAP_FN(getaddrinfo);

    if (!_snf_ok_host(node)) {
        return EAI_NONAME;
    }

    return orig_fn(node, service, hints, res);
}

typedef struct hostent* (*f_gethostbyname_t)(const char*);
struct hostent *gethostbyname(const char *name) {
    WRAP_FN(gethostbyname);

    if (!_snf_ok_host(name)) {
        errno = HOST_NOT_FOUND;
        return NULL;
    }

    return orig_fn(name);
}

typedef struct hostent* (*f_gethostbyname2_t)(const char*, int af);
struct hostent *gethostbyname2(const char *name, int af) {
    WRAP_FN(gethostbyname2);

    if (!_snf_ok_host(name)) {
        errno = HOST_NOT_FOUND;
        return NULL;
    }

    return orig_fn(name, af);
}

typedef int (*f_gethostbyname_r_t)(const char*, struct hostent*, char*, size_t, struct hostent**, int*);
int gethostbyname_r(const char* name, struct hostent* ret, char* buf, size_t buflen, struct hostent** result, int* h_errnop) {
    WRAP_FN(gethostbyname_r);

    if (!_snf_ok_host(name)) {
        *result = NULL;
        *h_errnop = HOST_NOT_FOUND;
        return HOST_NOT_FOUND;
    }

    return orig_fn(name, ret, buf, buflen, result, h_errnop);
}

// TODO: gethostbyname2_r
