#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "runtime_custody_protocol.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <link.h>
#include <linux/futex.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/uio.h>

#if !defined(__linux__) || !defined(__x86_64__) || \
    __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "The custody audit wire and native syscalls require little-endian Linux x86-64"
#endif

_Static_assert(sizeof(long) == 8 && sizeof(uintptr_t) == 8,
               "custody audit requires the native x86-64 ABI");
_Static_assert(sizeof(struct stat) == 144, "native Linux x86-64 fstat layout");

/* No constructors, symbol auditing, libc calls, heap, or callback-time mappings.
 * The sealed configuration is retained as a shared, read-only mapping. Names
 * are sent directly from that mapping or from the callback's bounded argument.
 * The supervisor, not an object's name, admits its actual mapped PT_LOADs.
 * In particular, dlmopen can reach OPEN without passing through SEARCH. */
static const struct IsleCustodyConfig *custody_config;
static const struct IsleCustodyImage *custody_images;
static const struct IsleCustodyAlias *custody_aliases;
static const unsigned char *custody_bytes;
static uint64_t event_serial;
static uint64_t object_serial;
static uint32_t callback_owner;
static unsigned int activated;
static unsigned int empty_application_opened;

static inline long native_syscall6(long number, long a1, long a2, long a3,
                                   long a4, long a5, long a6)
{
    register long r10 __asm__("r10") = a4;
    register long r8 __asm__("r8") = a5;
    register long r9 __asm__("r9") = a6;
    long result;
    __asm__ volatile("syscall"
                     : "=a"(result)
                     : "0"(number), "D"(a1), "S"(a2), "d"(a3),
                       "r"(r10), "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory", "cc");
    return result;
}

static inline long native_syscall3(long number, long a1, long a2, long a3)
{
    return native_syscall6(number, a1, a2, a3, 0, 0, 0);
}

static inline long native_syscall0(long number)
{
    return native_syscall6(number, 0, 0, 0, 0, 0, 0);
}

__attribute__((noreturn)) static void terminate_process(void)
{
    /* Never use SYS_exit: rejection must kill all target threads. */
    for (;;) {
        native_syscall3(SYS_exit_group, ISLE_CUSTODY_REJECT_EXIT, 0, 0);
    }
}

static uint64_t next_event_serial(void)
{
    uint64_t previous = __atomic_load_n(&event_serial, __ATOMIC_RELAXED);
    for (;;) {
        if (previous == UINT64_MAX) {
            return 0;
        }
        if (__atomic_compare_exchange_n(&event_serial, &previous, previous + 1,
                                        0, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
            return previous + 1;
        }
    }
}

static void make_event(struct IsleCustodyEvent *event, uint64_t serial,
                       uint64_t kind, uint64_t namespace_id, uint64_t image,
                       uint64_t bias, uint64_t flags, uint64_t cookie,
                       uint64_t name_length)
{
    event->magic = ISLE_CUSTODY_EVENT_MAGIC;
    event->version = ISLE_CUSTODY_VERSION;
    event->kind = kind;
    event->serial = serial;
    event->pid = (uint64_t)native_syscall0(SYS_getpid);
    event->tid = (uint64_t)native_syscall0(SYS_gettid);
    event->namespace_id = namespace_id;
    event->image_index = image;
    event->load_bias = bias;
    event->flags = flags;
    event->cookie = cookie;
    event->name_bytes = name_length;
}

static long send_record(const struct IsleCustodyEvent *event, const char *name,
                        uint64_t name_length, int flags)
{
    struct iovec vectors[2];
    struct msghdr message;
    vectors[0].iov_base = (void *)event;
    vectors[0].iov_len = sizeof(*event);
    vectors[1].iov_base = (void *)name;
    vectors[1].iov_len = (size_t)name_length;
    message.msg_name = NULL;
    message.msg_namelen = 0;
    message.msg_iov = vectors;
    message.msg_iovlen = name_length == 0 ? 1 : 2;
    message.msg_control = NULL;
    message.msg_controllen = 0;
    message.msg_flags = 0;
    return native_syscall3(SYS_sendmsg, ISLE_CUSTODY_CONTROL_FD,
                           (long)(uintptr_t)&message, flags | MSG_NOSIGNAL);
}

__attribute__((noreturn)) static void fatal(const char *reason)
{
    uint64_t length = 0;
    uint64_t serial;
    struct IsleCustodyEvent event;
    while (length < ISLE_CUSTODY_MAX_NAME && reason[length] != '\0') {
        ++length;
    }
    serial = next_event_serial();
    if (serial != 0) {
        make_event(&event, serial, ISLE_CUSTODY_FATAL,
                   ISLE_CUSTODY_NO_NAMESPACE, ISLE_CUSTODY_NO_IMAGE,
                   0, 0, 0, length);
        /* A dead/rejected channel must not prevent process termination. A
         * successfully queued seqpacket survives exit for the parent to drain. */
        (void)send_record(&event, reason, length, MSG_DONTWAIT);
    }
    terminate_process();
}

static void lock_callback(void)
{
    long tid = native_syscall0(SYS_gettid);
    if (tid <= 0 || (uint64_t)tid > UINT32_MAX) {
        fatal("invalid audit thread identity");
    }
    for (;;) {
        uint32_t previous = 0;
        long result;
        if (__atomic_compare_exchange_n(&callback_owner, &previous, (uint32_t)tid,
                                        0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
            return;
        }
        if (previous == (uint32_t)tid) {
            fatal("recursive audit callback");
        }
        result = native_syscall6(SYS_futex, (long)(uintptr_t)&callback_owner,
                                 FUTEX_WAIT_PRIVATE, previous, 0, 0, 0);
        if (result < 0 && result != -EINTR && result != -EAGAIN) {
            fatal("audit callback lock failure");
        }
    }
}

static void unlock_callback(void)
{
    __atomic_store_n(&callback_owner, 0, __ATOMIC_RELEASE);
    if (native_syscall6(SYS_futex, (long)(uintptr_t)&callback_owner,
                        FUTEX_WAKE_PRIVATE, 1, 0, 0, 0) < 0) {
        fatal("audit callback unlock failure");
    }
}

static void emit_event(uint64_t kind, uint64_t namespace_id, uint64_t image,
                       uint64_t bias, uint64_t flags, uint64_t cookie,
                       const char *name, uint64_t name_length)
{
    struct IsleCustodyEvent event;
    struct IsleCustodyAck ack;
    struct iovec vector;
    struct msghdr message;
    uint64_t serial;
    long result;
    if (name_length > ISLE_CUSTODY_MAX_NAME ||
        (name_length != 0 && name == NULL)) {
        fatal("invalid event name bounds");
    }
    serial = next_event_serial();
    if (serial == 0) {
        fatal("audit event serial overflow");
    }
    make_event(&event, serial, kind, namespace_id, image, bias, flags, cookie,
               name_length);
    do {
        result = send_record(&event, name, name_length, 0);
    } while (result == -EINTR);
    if (result != (long)(sizeof(event) + name_length)) {
        fatal("audit event channel send failure");
    }

    vector.iov_base = &ack;
    vector.iov_len = sizeof(ack);
    message.msg_name = NULL;
    message.msg_namelen = 0;
    message.msg_iov = &vector;
    message.msg_iovlen = 1;
    message.msg_control = NULL;
    message.msg_controllen = 0;
    do {
        message.msg_flags = 0;
        result = native_syscall3(SYS_recvmsg, ISLE_CUSTODY_CONTROL_FD,
                                 (long)(uintptr_t)&message, MSG_TRUNC);
    } while (result == -EINTR);
    if (result != (long)sizeof(ack) ||
        (message.msg_flags & ~MSG_EOR) != 0 ||
        ack.magic != ISLE_CUSTODY_ACK_MAGIC ||
        ack.version != ISLE_CUSTODY_VERSION || ack.serial != serial ||
        ack.accepted != 1) {
        fatal("audit event acknowledgement rejected or malformed");
    }
}

static int compare_bytes(const char *left, uint64_t left_length,
                          const char *right, uint64_t right_length)
{
    uint64_t index;
    uint64_t common = left_length < right_length ? left_length : right_length;
    for (index = 0; index < common; ++index) {
        unsigned int a = (unsigned char)left[index];
        unsigned int b = (unsigned char)right[index];
        if (a != b) {
            return a < b ? -1 : 1;
        }
    }
    return left_length == right_length ? 0 : (left_length < right_length ? -1 : 1);
}

static uint64_t bounded_name_length(const char *name)
{
    uint64_t length;
    if (name == NULL) {
        fatal("null loader object name");
    }
    for (length = 0; length <= ISLE_CUSTODY_MAX_NAME; ++length) {
        if (name[length] == '\0') {
            return length;
        }
    }
    fatal("loader object name exceeds protocol bound");
}

static uint64_t table_end(uint64_t offset, uint64_t count, uint64_t stride,
                          uint64_t total)
{
    if (offset < sizeof(struct IsleCustodyConfig) || (offset & 7) != 0 ||
        offset > total || count > (total - offset) / stride) {
        fatal("invalid configuration table bounds");
    }
    return offset + count * stride;
}

static const char *configuration_string(const struct IsleCustodyConfig *config,
                                        uint64_t offset, uint64_t length)
{
    const unsigned char *bytes = (const unsigned char *)config;
    uint64_t index;
    if (length == 0 || length > ISLE_CUSTODY_MAX_NAME ||
        offset < config->strings_offset || offset >= config->total_bytes ||
        length >= config->total_bytes - offset) {
        fatal("invalid configuration string bounds");
    }
    if (bytes[offset + length] != 0) {
        fatal("unterminated configuration string");
    }
    for (index = 0; index < length; ++index) {
        if (bytes[offset + index] == 0) {
            fatal("embedded NUL in configuration string");
        }
    }
    return (const char *)(bytes + offset);
}

static uint64_t path_decimal(const char *path, uint64_t length, uint64_t *position)
{
    uint64_t value = 0;
    uint64_t start = *position;
    if (start >= length || path[start] < '0' || path[start] > '9') {
        fatal("invalid supervisor image path number");
    }
    while (*position < length && path[*position] >= '0' && path[*position] <= '9') {
        unsigned int digit = (unsigned int)(path[*position] - '0');
        if (value > (UINT64_MAX - digit) / 10) {
            fatal("supervisor image path number overflow");
        }
        value = value * 10 + digit;
        ++*position;
    }
    if (*position - start > 1 && path[start] == '0') {
        fatal("noncanonical supervisor image path number");
    }
    return value;
}

static void validate_image_path(const char *path, uint64_t length,
                                uint64_t supervisor_pid, uint64_t supervisor_fd)
{
    uint64_t position = 6;
    uint64_t pid;
    uint64_t fd;
    if (length < 12 || compare_bytes(path, 6, "/proc/", 6) != 0) {
        fatal("image path is not an exact supervisor FD path");
    }
    pid = path_decimal(path, length, &position);
    if (length - position < 4 ||
        compare_bytes(path + position, 4, "/fd/", 4) != 0) {
        fatal("invalid supervisor image FD path");
    }
    position += 4;
    fd = path_decimal(path, length, &position);
    if (position != length || pid != supervisor_pid || fd != supervisor_fd) {
        fatal("supervisor image FD path identity mismatch");
    }
}

static uint64_t find_alias(const unsigned char *bytes,
                           const struct IsleCustodyAlias *aliases,
                           uint64_t count, const char *name, uint64_t length)
{
    uint64_t first = 0;
    uint64_t end = count;
    while (first < end) {
        uint64_t middle = first + (end - first) / 2;
        const struct IsleCustodyAlias *alias = &aliases[middle];
        int order = compare_bytes(name, length,
                                  (const char *)(bytes + alias->name_offset),
                                  alias->name_length);
        if (order == 0) {
            return alias->image_index;
        }
        if (order < 0) {
            end = middle;
        } else {
            first = middle + 1;
        }
    }
    return ISLE_CUSTODY_NO_IMAGE;
}

static void initialize_configuration(void)
{
    struct stat status;
    const struct IsleCustodyConfig *config;
    const struct IsleCustodyImage *images;
    const struct IsleCustodyAlias *aliases;
    const unsigned char *bytes;
    uint64_t images_end;
    uint64_t aliases_end;
    uint64_t index;
    uint64_t required_seals = F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE;
    long seals = native_syscall3(SYS_fcntl, ISLE_CUSTODY_CONFIG_FD, F_GET_SEALS, 0);
    long mapped;
    if (seals < 0 || ((uint64_t)seals & required_seals) != required_seals) {
        fatal("configuration FD is not completely sealed");
    }
    if (native_syscall3(SYS_fstat, ISLE_CUSTODY_CONFIG_FD,
                        (long)(uintptr_t)&status, 0) != 0 ||
        !S_ISREG(status.st_mode) || status.st_size < (long)sizeof(*config) ||
        (uint64_t)status.st_size > ISLE_CUSTODY_MAX_CONFIG) {
        fatal("invalid configuration FD type or size");
    }
    mapped = native_syscall6(SYS_mmap, 0, status.st_size, PROT_READ, MAP_SHARED,
                             ISLE_CUSTODY_CONFIG_FD, 0);
    if ((unsigned long)mapped >= (unsigned long)-4095) {
        fatal("sealed configuration mapping failed");
    }
    config = (const struct IsleCustodyConfig *)(uintptr_t)mapped;
    bytes = (const unsigned char *)config;
    if (config->magic != ISLE_CUSTODY_CONFIG_MAGIC ||
        config->version != ISLE_CUSTODY_VERSION ||
        config->total_bytes != (uint64_t)status.st_size ||
        config->supervisor_pid == 0 || config->supervisor_pid > INT_MAX ||
        config->image_count < 3 || config->image_count > ISLE_CUSTODY_MAX_IMAGES ||
        config->alias_count == 0 || config->alias_count > ISLE_CUSTODY_MAX_ALIASES) {
        fatal("invalid sealed configuration header");
    }
    images_end = table_end(config->images_offset, config->image_count,
                            sizeof(struct IsleCustodyImage), config->total_bytes);
    aliases_end = table_end(config->aliases_offset, config->alias_count,
                             sizeof(struct IsleCustodyAlias), config->total_bytes);
    if ((config->images_offset < aliases_end && config->aliases_offset < images_end) ||
        config->strings_offset < images_end || config->strings_offset < aliases_end ||
        config->strings_offset >= config->total_bytes ||
        config->application_index >= config->image_count ||
        config->loader_index >= config->image_count ||
        config->auditor_index >= config->image_count ||
        config->application_index == config->loader_index ||
        config->application_index == config->auditor_index ||
        config->loader_index == config->auditor_index) {
        fatal("overlapping configuration regions or invalid bootstrap indices");
    }
    images = (const struct IsleCustodyImage *)(bytes + config->images_offset);
    aliases = (const struct IsleCustodyAlias *)(bytes + config->aliases_offset);
    for (index = 0; index < config->image_count; ++index) {
        const struct IsleCustodyImage *image = &images[index];
        const char *path = configuration_string(config, image->path_offset,
                                                 image->path_length);
        uint64_t previous;
        uint64_t role = ISLE_CUSTODY_PROVIDER;
        if (index == config->application_index) {
            role = ISLE_CUSTODY_APPLICATION;
        } else if (index == config->loader_index) {
            role = ISLE_CUSTODY_LOADER;
        } else if (index == config->auditor_index) {
            role = ISLE_CUSTODY_AUDITOR;
        }
        if (image->role != role || image->supervisor_fd > INT_MAX ||
            image->device_major > UINT32_MAX || image->device_minor > UINT32_MAX ||
            image->inode == 0 || image->size == 0 || image->size > INT64_MAX) {
            fatal("invalid sealed image role or identity fields");
        }
        validate_image_path(path, image->path_length, config->supervisor_pid,
                             image->supervisor_fd);
        for (previous = 0; previous < index; ++previous) {
            if (images[previous].supervisor_fd == image->supervisor_fd) {
                fatal("duplicate supervisor image FD path");
            }
        }
    }
    for (index = 0; index < config->alias_count; ++index) {
        const struct IsleCustodyAlias *alias = &aliases[index];
        const char *name = configuration_string(config, alias->name_offset,
                                                 alias->name_length);
        if (alias->image_index >= config->image_count) {
            fatal("configuration alias image index out of range");
        }
        if (index != 0) {
            const struct IsleCustodyAlias *previous = &aliases[index - 1];
            if (compare_bytes((const char *)(bytes + previous->name_offset),
                               previous->name_length, name, alias->name_length) >= 0) {
                fatal("configuration aliases are not strictly byte sorted");
            }
        }
    }
    for (index = 0; index < config->image_count; ++index) {
        const struct IsleCustodyImage *image = &images[index];
        uint64_t target = find_alias(bytes, aliases, config->alias_count,
                                     (const char *)(bytes + image->path_offset),
                                     image->path_length);
        if (target != ISLE_CUSTODY_NO_IMAGE && target != index) {
            fatal("configuration alias redirects a different sealed image path");
        }
    }
    custody_bytes = bytes;
    custody_images = images;
    custody_aliases = aliases;
    custody_config = config;
}

static void require_activation(void)
{
    if (!activated || custody_config == NULL) {
        fatal("audit callback before accepted activation");
    }
}

static uint64_t supplied_cookie(const uintptr_t *cookie)
{
    if (cookie == NULL) {
        fatal("null loader audit cookie");
    }
    return (uint64_t)*cookie;
}

static uint64_t close_image(uint64_t cookie)
{
    uint64_t serial = cookie >> 9;
    uint64_t image_tag = cookie & UINT64_C(511);
    if (serial == 0 || serial > object_serial || image_tag > custody_config->image_count) {
        fatal("invalid closing object cookie");
    }
    return image_tag == 0 ? ISLE_CUSTODY_NO_IMAGE : image_tag - 1;
}

unsigned int la_version(unsigned int version)
{
    int socket_type;
    unsigned int socket_type_size = sizeof(socket_type);
    lock_callback();
    if (activated || custody_config != NULL) {
        fatal("duplicate audit activation");
    }
    /* The argument is the loader's highest supported version, not the version
     * of the definitions used to build this module. Never echo it unchecked. */
    if (version < LAV_CURRENT) {
        fatal("loader does not support the compiled audit interface version");
    }
    if (native_syscall6(SYS_getsockopt, ISLE_CUSTODY_CONTROL_FD, SOL_SOCKET,
                        SO_TYPE, (long)(uintptr_t)&socket_type,
                        (long)(uintptr_t)&socket_type_size, 0) != 0 ||
        socket_type_size != sizeof(socket_type) || socket_type != SOCK_SEQPACKET) {
        fatal("audit control FD is not a seqpacket socket");
    }
    initialize_configuration();
    emit_event(ISLE_CUSTODY_ACTIVATE, ISLE_CUSTODY_NO_NAMESPACE,
                custody_config->auditor_index, 0, version, 0, NULL, 0);
    activated = 1;
    unlock_callback();
    return LAV_CURRENT;
}

char *la_objsearch(const char *name, uintptr_t *cookie, unsigned int flag)
{
    uint64_t length;
    uint64_t image;
    uint64_t referrer;
    const char *path;
    lock_callback();
    require_activation();
    length = bounded_name_length(name);
    referrer = supplied_cookie(cookie);
    if (flag != LA_SER_ORIG && flag != LA_SER_LIBPATH && flag != LA_SER_RUNPATH &&
        flag != LA_SER_CONFIG && flag != LA_SER_DEFAULT && flag != LA_SER_SECURE) {
        fatal("unknown loader search origin flag");
    }
    image = find_alias(custody_bytes, custody_aliases, custody_config->alias_count,
                       name, length);
    if (image == ISLE_CUSTODY_NO_IMAGE) {
        fatal("loader requested an unapproved exact alias");
    }
    path = (const char *)(custody_bytes + custody_images[image].path_offset);
    emit_event(ISLE_CUSTODY_SEARCH, ISLE_CUSTODY_NO_NAMESPACE, image, 0, flag,
                referrer, name, length);
    unlock_callback();
    return (char *)path;
}

unsigned int la_objopen(struct link_map *map, Lmid_t lmid, uintptr_t *cookie)
{
    uint64_t length;
    uint64_t image = ISLE_CUSTODY_NO_IMAGE;
    uint64_t index;
    uint64_t object_cookie;
    int empty_application = 0;
    lock_callback();
    require_activation();
    if (map == NULL || cookie == NULL || lmid < 0) {
        fatal("invalid loader object callback arguments");
    }
    length = bounded_name_length(map->l_name);
    if (length == 0) {
        if (lmid != LM_ID_BASE || empty_application_opened) {
            fatal("empty object name outside the initial base application");
        }
        image = custody_config->application_index;
        empty_application = 1;
    } else if (compare_bytes(map->l_name, length, "linux-vdso.so.1", 15) == 0) {
        /* Only the parent can establish that this is the actual kernel vDSO,
         * not a file which happens to carry the pseudoobject's name. */
        image = ISLE_CUSTODY_NO_IMAGE;
    } else {
        for (index = 0; index < custody_config->image_count; ++index) {
            const struct IsleCustodyImage *candidate = &custody_images[index];
            if (compare_bytes(map->l_name, length,
                               (const char *)(custody_bytes + candidate->path_offset),
                               candidate->path_length) == 0) {
                image = index;
                break;
            }
        }
        if (image == ISLE_CUSTODY_NO_IMAGE) {
            image = find_alias(custody_bytes, custody_aliases,
                                custody_config->alias_count, map->l_name, length);
        }
        if (image == ISLE_CUSTODY_NO_IMAGE) {
            fatal("loader opened an unapproved object name");
        }
    }
    if (object_serial == (UINT64_MAX >> 9)) {
        fatal("audit object cookie serial overflow");
    }
    ++object_serial;
    object_cookie = (object_serial << 9) |
                    (image == ISLE_CUSTODY_NO_IMAGE ? 0 : image + 1);
    *cookie = (uintptr_t)object_cookie;
    emit_event(ISLE_CUSTODY_OPEN, (uint64_t)lmid, image, (uint64_t)map->l_addr,
                0, object_cookie, map->l_name, length);
    if (empty_application) {
        empty_application_opened = 1;
    }
    unlock_callback();
    /* Zero disables symbol-binding audit only; rejection always exits above. */
    return 0;
}

void la_activity(uintptr_t *cookie, unsigned int flag)
{
    uint64_t referrer;
    lock_callback();
    require_activation();
    referrer = supplied_cookie(cookie);
    if (flag != LA_ACT_CONSISTENT && flag != LA_ACT_ADD && flag != LA_ACT_DELETE) {
        fatal("unknown loader activity flag");
    }
    emit_event(ISLE_CUSTODY_ACTIVITY, ISLE_CUSTODY_NO_NAMESPACE,
                ISLE_CUSTODY_NO_IMAGE, 0, flag, referrer, NULL, 0);
    unlock_callback();
}

void la_preinit(uintptr_t *cookie)
{
    uint64_t referrer;
    lock_callback();
    require_activation();
    referrer = supplied_cookie(cookie);
    emit_event(ISLE_CUSTODY_PREINIT, ISLE_CUSTODY_NO_NAMESPACE,
                ISLE_CUSTODY_NO_IMAGE, 0, 0, referrer, NULL, 0);
    unlock_callback();
}

unsigned int la_objclose(uintptr_t *cookie)
{
    uint64_t object_cookie;
    uint64_t image;
    lock_callback();
    require_activation();
    object_cookie = supplied_cookie(cookie);
    image = close_image(object_cookie);
    emit_event(ISLE_CUSTODY_CLOSE, ISLE_CUSTODY_NO_NAMESPACE, image, 0, 0,
                object_cookie, NULL, 0);
    unlock_callback();
    return 0;
}
