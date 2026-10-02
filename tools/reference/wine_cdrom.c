/* Virtual Linux CD device for the unmodified Windows reference under Wine.
 * Only descriptors of DD2_CD_DEVICE are handled. Wine's own mcicda driver
 * performs digital playback using the exact provisioned CDDA sectors.
 * Other descriptors go to libc; unsupported virtual device commands fail.
 * The generated TOC comes from the verified BIN/CUE manifest. No EXE or
 * engine memory changes are needed to pass the original CD check.
 */
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "toc.h"

static int failure(int error) { errno = error; return -1; }

static void to_msf(union cdrom_addr *address, int sector) {
    sector += CD_MSF_OFFSET;
    address->msf.minute = sector / (60 * 75);
    address->msf.second = sector / 75 % 60;
    address->msf.frame = sector % 75;
}

static int read_audio(const char *root, struct cdrom_read_audio *read) {
    int at, left = read->nframes;
    unsigned char *out = read->buf;
    if (left < 0 || !out) return failure(EINVAL);
    if (read->addr_format == CDROM_LBA) at = read->addr.lba;
    else if (read->addr_format == CDROM_MSF) {
        if (read->addr.msf.second >= 60 || read->addr.msf.frame >= 75)
            return failure(EINVAL);
        at = (read->addr.msf.minute * 60 + read->addr.msf.second) * 75
             + read->addr.msf.frame - CD_MSF_OFFSET;
    } else return failure(EINVAL);
    if (at < starts[1] || at >= starts[TRACK_COUNT] ||
        left > starts[TRACK_COUNT] - at) return failure(EINVAL);
    while (left > 0) {
        int track = 1, count, input, length;
        size_t done = 0, bytes;
        char path[4096];
        for (; track < TRACK_COUNT - 1 && at >= starts[track + 1]; track++);
        count = starts[track + 1] - at;
        if (count > left) count = left;
        length = snprintf(path, sizeof(path), "%s/track%02d.cdda", root, track + 1);
        if (length < 0 || length >= (int)sizeof(path)) return failure(ENAMETOOLONG);
        input = open(path, O_RDONLY);
        if (input < 0) return -1;
        bytes = (size_t)count * CD_FRAMESIZE_RAW;
        while (done < bytes) {
            ssize_t got = pread(input, out + done, bytes - done,
                (off_t)(at - starts[track]) * CD_FRAMESIZE_RAW + done);
            if (got < 0 && errno == EINTR) continue;
            if (got <= 0) {
                int error = got < 0 ? errno : EIO;
                close(input);
                return failure(error);
            }
            done += got;
        }
        close(input);
        at += count;
        left -= count;
        out += bytes;
    }
    return 0;
}

static int virtual_ioctl(int fd, unsigned long request, void *arg, int time64) {
    struct stat file, marker;
    const char *root = getenv("DD2_CD_ROOT"), *device = getenv("DD2_CD_DEVICE");
    int (*original)(int, unsigned long, ...) = dlsym(RTLD_NEXT,
        time64 ? "__ioctl_time64" : "ioctl");
    if (!original) return failure(ENOSYS);
    if (!root || !device || fstat(fd, &file) || stat(device, &marker) ||
        file.st_dev != marker.st_dev || file.st_ino != marker.st_ino)
        return original(fd, request, arg);
    if (getenv("DD2_CD_TRACE")) fprintf(stderr, "[VCD] fd=%d ioctl=%lx\n", fd, request);
    switch (request) {
    case CDROMREADTOCHDR: {
        struct cdrom_tochdr *header = arg;
        if (!header) return failure(EFAULT);
        header->cdth_trk0 = 1;
        header->cdth_trk1 = TRACK_COUNT;
        return 0;
    }
    case CDROMREADTOCENTRY: {
        struct cdrom_tocentry *entry = arg;
        int track;
        if (!entry) return failure(EFAULT);
        if (entry->cdte_track != CDROM_LEADOUT &&
            (entry->cdte_track < 1 || entry->cdte_track > TRACK_COUNT))
            return failure(EINVAL);
        track = entry->cdte_track == CDROM_LEADOUT ? TRACK_COUNT : entry->cdte_track - 1;
        if (track < 0 || track > TRACK_COUNT) return failure(EINVAL);
        if (entry->cdte_format != CDROM_MSF && entry->cdte_format != CDROM_LBA)
            return failure(EINVAL);
        entry->cdte_adr = 1;
        entry->cdte_ctrl = track == 0 ? CDROM_DATA_TRACK : 0;
        entry->cdte_datamode = track == 0 ? 2 : 0;
        if (entry->cdte_format == CDROM_MSF) to_msf(&entry->cdte_addr, starts[track]);
        else entry->cdte_addr.lba = starts[track];
        return 0;
    }
    case CDROMSUBCHNL: {
        struct cdrom_subchnl *channel = arg;
        if (!channel) return failure(EFAULT);
        if (channel->cdsc_format != CDROM_MSF && channel->cdsc_format != CDROM_LBA)
            return failure(EINVAL);
        /* No analogue transport: Wine's DirectSound playback tracks its own
         * playing/paused/stopped state and position. This is its idle device. */
        channel->cdsc_audiostatus = CDROM_AUDIO_NO_STATUS;
        channel->cdsc_adr = 1;
        channel->cdsc_ctrl = 0;
        channel->cdsc_trk = 2;
        channel->cdsc_ind = 1;
        memset(&channel->cdsc_reladdr, 0, sizeof(channel->cdsc_reladdr));
        if (channel->cdsc_format == CDROM_MSF) to_msf(&channel->cdsc_absaddr, starts[1]);
        else channel->cdsc_absaddr.lba = starts[1];
        return 0;
    }
    case CDROMREADAUDIO:
        if (!arg) return failure(EFAULT);
        return read_audio(root, arg);
    case CDROM_DRIVE_STATUS: return CDS_DISC_OK;
    case CDROM_DISC_STATUS: return CDS_MIXED;
    case CDROM_MEDIA_CHANGED: return 0;
    case CDROMSTOP: return 0;
    default: return failure(ENOTTY);
    }
}

int ioctl(int fd, unsigned long request, ...) {
    va_list args;
    void *arg;
    va_start(args, request); arg = va_arg(args, void *); va_end(args);
    return virtual_ioctl(fd, request, arg, 0);
}

/* Debian's 32-bit Wine imports this glibc entry, rather than ioctl. */
int __ioctl_time64(int fd, unsigned long request, ...) {
    va_list args;
    void *arg;
    va_start(args, request); arg = va_arg(args, void *); va_end(args);
    return virtual_ioctl(fd, request, arg, 1);
}
