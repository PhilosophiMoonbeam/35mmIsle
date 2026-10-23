#ifndef ISLE_RUNTIME_CUSTODY_PROTOCOL_H
#define ISLE_RUNTIME_CUSTODY_PROTOCOL_H

#include <stdint.h>

/* Qualification-only Linux x86-64 protocol. Every field is little-endian u64;
 * offsets are from the start of the sealed configuration, lengths exclude NUL.
 * Strings are NUL-terminated, disjoint from the header/tables, and aliases are
 * strictly byte-sorted. No pointer, native link_map layout, or GAME state is wire data. */
#define ISLE_CUSTODY_CONFIG_FD 198
#define ISLE_CUSTODY_CONTROL_FD 199
#define ISLE_CUSTODY_VERSION UINT64_C(1)
#define ISLE_CUSTODY_CONFIG_MAGIC UINT64_C(0x31474643454c5349)
#define ISLE_CUSTODY_EVENT_MAGIC UINT64_C(0x31545645454c5349)
#define ISLE_CUSTODY_ACK_MAGIC UINT64_C(0x314b4341454c5349)
#define ISLE_CUSTODY_NO_IMAGE UINT64_MAX
#define ISLE_CUSTODY_NO_NAMESPACE UINT64_MAX
#define ISLE_CUSTODY_MAX_CONFIG UINT64_C(1048576)
#define ISLE_CUSTODY_MAX_IMAGES UINT64_C(256)
#define ISLE_CUSTODY_MAX_ALIASES UINT64_C(2048)
#define ISLE_CUSTODY_MAX_NAME UINT64_C(4096)
#define ISLE_CUSTODY_REJECT_EXIT 126

enum IsleCustodyRole {
    ISLE_CUSTODY_APPLICATION = 1,
    ISLE_CUSTODY_LOADER = 2,
    ISLE_CUSTODY_AUDITOR = 3,
    ISLE_CUSTODY_PROVIDER = 4
};
enum IsleCustodyEventKind {
    ISLE_CUSTODY_ACTIVATE = 1,
    ISLE_CUSTODY_SEARCH = 2,
    ISLE_CUSTODY_OPEN = 3,
    ISLE_CUSTODY_ACTIVITY = 4,
    ISLE_CUSTODY_PREINIT = 5,
    ISLE_CUSTODY_CLOSE = 6,
    ISLE_CUSTODY_FATAL = 7
};

struct IsleCustodyConfig {
    uint64_t magic, version, total_bytes, supervisor_pid;
    uint64_t image_count, alias_count, images_offset, aliases_offset;
    uint64_t strings_offset, application_index, loader_index, auditor_index;
};
struct IsleCustodyImage {
    uint64_t path_offset, path_length, role, supervisor_fd;
    uint64_t device_major, device_minor, inode, size;
};
struct IsleCustodyAlias {
    uint64_t name_offset, name_length, image_index;
};
/* One SOCK_SEQPACKET record is this header followed by name_bytes bytes, without
 * a trailing NUL. Each nonfatal event must receive the matching accepted ACK
 * before its loader callback returns. An unknown request/object, invalid config,
 * short/malformed channel record, rejected ACK or overflow terminates the whole
 * target with exit_group(126); no permissive host fallback is allowed. */
/* OPEN alone supplies an actual namespace/load bias. Other callbacks use
 * NO_NAMESPACE/zero and a referrer token; the supervisor joins its OPEN ledger.
 * OPEN assigns ((unique_object_serial << 9) | (image_index + 1)); low nine bits
 * zero identify the kernel vDSO. CLOSE can recover the image without a registry.
 * ACTIVATE identifies auditor_index and carries the offered loader version. */
struct IsleCustodyEvent {
    uint64_t magic, version, kind, serial;
    uint64_t pid, tid, namespace_id, image_index;
    uint64_t load_bias, flags, cookie, name_bytes;
};
struct IsleCustodyAck {
    uint64_t magic, version, serial, accepted;
};

#if defined(__cplusplus)
static_assert(sizeof(IsleCustodyConfig) == 96);
static_assert(sizeof(IsleCustodyImage) == 64);
static_assert(sizeof(IsleCustodyAlias) == 24);
static_assert(sizeof(IsleCustodyEvent) == 96);
static_assert(sizeof(IsleCustodyAck) == 32);
#else
_Static_assert(sizeof(struct IsleCustodyConfig) == 96, "custody config wire size");
_Static_assert(sizeof(struct IsleCustodyImage) == 64, "custody image wire size");
_Static_assert(sizeof(struct IsleCustodyAlias) == 24, "custody alias wire size");
_Static_assert(sizeof(struct IsleCustodyEvent) == 96, "custody event wire size");
_Static_assert(sizeof(struct IsleCustodyAck) == 32, "custody ack wire size");
#endif

#endif
