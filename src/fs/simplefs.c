#include "../../include/simplefs.h"
#include "../../include/ata.h"
#include <stddef.h>

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t max_files;
  uint32_t data_start_lba;
  uint8_t  padding[512 - 16];
} __attribute__((packed)) sfs_superblock_t;

static int g_sfs_ready = 0;
static sfs_entry_t g_dir[SFS_MAX_FILES];

static void sfs_memset(void *dst, uint8_t val, uint32_t len) {
  uint8_t *p = (uint8_t *)dst;
  for (uint32_t i = 0; i < len; i++) {
    p[i] = val;
  }
}

static void sfs_memcpy(void *dst, const void *src, uint32_t len) {
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  for (uint32_t i = 0; i < len; i++) {
    d[i] = s[i];
  }
}

static int sfs_strcmp(const char *a, const char *b) {
  while (*a && (*a == *b)) {
    a++;
    b++;
  }
  return (int)(uint8_t)(*a) - (int)(uint8_t)(*b);
}

static void sfs_strncpy(char *dst, const char *src, uint32_t max_len) {
  uint32_t i = 0;
  for (; i + 1 < max_len && src[i] != '\0'; i++) {
    dst[i] = src[i];
  }
  for (; i < max_len; i++) {
    dst[i] = '\0';
  }
}

static int sfs_load_dir(void) {
  uint8_t *raw = (uint8_t *)g_dir;
  for (uint32_t i = 0; i < SFS_DIR_SECTORS; i++) {
    if (ata_read_sector(SFS_DIR_START_LBA + i, raw + (i * 512)) != 0) {
      return -1;
    }
  }
  return 0;
}

static int sfs_save_dir(void) {
  const uint8_t *raw = (const uint8_t *)g_dir;
  for (uint32_t i = 0; i < SFS_DIR_SECTORS; i++) {
    if (ata_write_sector(SFS_DIR_START_LBA + i, raw + (i * 512)) != 0) {
      return -1;
    }
  }
  return 0;
}

int sfs_format(void) {
  if (!ata_is_present()) {
    return -1;
  }

  sfs_superblock_t sb;
  sfs_memset(&sb, 0, sizeof(sb));
  sb.magic = SFS_MAGIC;
  sb.version = 1;
  sb.max_files = SFS_MAX_FILES;
  sb.data_start_lba = SFS_DATA_START_LBA;

  if (ata_write_sector(SFS_SUPERBLOCK_LBA, (const uint8_t *)&sb) != 0) {
    return -2;
  }

  sfs_memset(g_dir, 0, sizeof(g_dir));
  if (sfs_save_dir() != 0) {
    return -3;
  }

  g_sfs_ready = 1;
  return 0;
}

int sfs_init(void) {
  if (!ata_is_present()) {
    if (!ata_init()) {
      g_sfs_ready = 0;
      return -1;
    }
  }

  sfs_superblock_t sb;
  if (ata_read_sector(SFS_SUPERBLOCK_LBA, (uint8_t *)&sb) != 0) {
    g_sfs_ready = 0;
    return -2;
  }

  if (sb.magic != SFS_MAGIC) {
    return sfs_format();
  }

  if (sfs_load_dir() != 0) {
    g_sfs_ready = 0;
    return -3;
  }

  g_sfs_ready = 1;
  return 0;
}

int sfs_is_ready(void) {
  return g_sfs_ready;
}

static int sfs_find_index(const char *name) {
  if (name == NULL) {
    return -1;
  }
  for (int i = 0; i < SFS_MAX_FILES; i++) {
    if (g_dir[i].used && sfs_strcmp(g_dir[i].name, name) == 0) {
      return i;
    }
  }
  return -1;
}

int sfs_read_file(const char *name, char *out_buf, uint32_t max_len, uint32_t *out_size) {
  if (!g_sfs_ready) {
    return -1;
  }
  if (name == NULL) {
    return -1;
  }
  if (out_buf == NULL) {
    return -1;
  }

  int idx = sfs_find_index(name);
  if (idx < 0) {
    return -2; /* Arquivo nao encontrado */
  }

  uint32_t to_read = g_dir[idx].size;
  if (to_read > SFS_FILE_MAX_SIZE) {
    to_read = SFS_FILE_MAX_SIZE;
  }
  if (max_len > 0 && to_read >= max_len) {
    to_read = max_len - 1;
  }

  uint8_t sector_buf[512];
  uint32_t copied = 0;
  uint32_t sectors_needed = (to_read + 511) / 512;

  for (uint32_t s = 0; s < sectors_needed; s++) {
    if (ata_read_sector(g_dir[idx].start_lba + s, sector_buf) != 0) {
      return -3;
    }
    uint32_t chunk = to_read - copied;
    if (chunk > 512) {
      chunk = 512;
    }
    sfs_memcpy(out_buf + copied, sector_buf, chunk);
    copied += chunk;
  }

  if (max_len > 0) {
    out_buf[copied] = '\0';
  }
  if (out_size != NULL) {
    *out_size = copied;
  }
  return 0;
}

int sfs_write_file(const char *name, const char *data, uint32_t size) {
  if (!g_sfs_ready) {
    return -1;
  }
  if (name == NULL) {
    return -1;
  }
  if (name[0] == '\0') {
    return -1;
  }
  if (size > SFS_FILE_MAX_SIZE) {
    size = SFS_FILE_MAX_SIZE;
  }

  int idx = sfs_find_index(name);
  if (idx < 0) {
    for (int i = 0; i < SFS_MAX_FILES; i++) {
      if (!g_dir[i].used) {
        idx = i;
        break;
      }
    }
  }

  if (idx < 0) {
    return -2; /* Diretorio cheio */
  }

  uint32_t start_lba = SFS_DATA_START_LBA + ((uint32_t)idx * SFS_FILE_MAX_SECTORS);
  uint8_t sector_buf[512];
  uint32_t written = 0;
  uint32_t sectors_needed = (size + 511) / 512;

  for (uint32_t s = 0; s < sectors_needed; s++) {
    sfs_memset(sector_buf, 0, 512);
    uint32_t chunk = size - written;
    if (chunk > 512) {
      chunk = 512;
    }
    if (data != NULL && chunk > 0) {
      sfs_memcpy(sector_buf, data + written, chunk);
    }
    if (ata_write_sector(start_lba + s, sector_buf) != 0) {
      return -3;
    }
    written += chunk;
  }

  g_dir[idx].used = 1;
  sfs_strncpy(g_dir[idx].name, name, SFS_MAX_NAME);
  g_dir[idx].start_lba = start_lba;
  g_dir[idx].size = size;

  return sfs_save_dir();
}

int sfs_delete_file(const char *name) {
  if (!g_sfs_ready) {
    return -1;
  }
  int idx = sfs_find_index(name);
  if (idx < 0) {
    return -2;
  }
  sfs_memset(&g_dir[idx], 0, sizeof(sfs_entry_t));
  return sfs_save_dir();
}

int sfs_get_entry(int index, sfs_entry_t *out_entry) {
  if (!g_sfs_ready) {
    return 0;
  }
  if (index < 0) {
    return 0;
  }
  if (index >= SFS_MAX_FILES) {
    return 0;
  }
  if (!g_dir[index].used) {
    return 0;
  }
  if (out_entry != NULL) {
    *out_entry = g_dir[index];
  }
  return 1;
}