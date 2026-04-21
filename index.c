// index.c — Staging area implementation
//
// Text format of .pes/index (one entry per line, sorted by path):
//
//   <mode-octal> <64-char-hex-hash> <mtime-seconds> <size> <path>
//
// Example:
//   100644 a1b2c3d4e5f6...  1699900000 42 README.md
//   100644 f7e8d9c0b1a2...  1699900100 128 src/main.c
//
// PROVIDED functions: index_find, index_remove, index_status
// TODO functions:     index_load, index_save, index_add

#include "index.h"
#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

// ─── PROVIDED ────────────────────────────────────────────────────────────────

IndexEntry* index_find(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0)
            return &index->entries[i];
    }
    return NULL;
}

int index_remove(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            int remaining = index->count - i - 1;
            if (remaining > 0)
                memmove(&index->entries[i],
                        &index->entries[i + 1],
                        remaining * sizeof(IndexEntry));

            index->count--;
            return index_save(index);
        }
    }

    fprintf(stderr, "error: '%s' is not in the index\n", path);
    return -1;
}

int index_status(const Index *index) {
    printf("Staged changes:\n");
    int staged_count = 0;

    for (int i = 0; i < index->count; i++) {
        printf("  staged:     %s\n", index->entries[i].path);
        staged_count++;
    }

    if (staged_count == 0)
        printf("  (nothing to show)\n");

    printf("\n");

    printf("Unstaged changes:\n");
    int unstaged_count = 0;

    for (int i = 0; i < index->count; i++) {
        struct stat st;

        if (stat(index->entries[i].path, &st) != 0) {
            printf("  deleted:    %s\n", index->entries[i].path);
            unstaged_count++;
        } else {
            if (st.st_mtime != (time_t)index->entries[i].mtime_sec ||
                st.st_size != (off_t)index->entries[i].size) {
                printf("  modified:   %s\n", index->entries[i].path);
                unstaged_count++;
            }
        }
    }

    if (unstaged_count == 0)
        printf("  (nothing to show)\n");

    printf("\n");

    printf("Untracked files:\n");
    int untracked_count = 0;

    DIR *dir = opendir(".");
    if (dir) {
        struct dirent *ent;

        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 ||
                strcmp(ent->d_name, "..") == 0)
                continue;

            if (strcmp(ent->d_name, ".pes") == 0)
                continue;

            if (strcmp(ent->d_name, "pes") == 0)
                continue;

            if (strstr(ent->d_name, ".o") != NULL)
                continue;

            int is_tracked = 0;

            for (int i = 0; i < index->count; i++) {
                if (strcmp(index->entries[i].path, ent->d_name) == 0) {
                    is_tracked = 1;
                    break;
                }
            }

            if (!is_tracked) {
                struct stat st;
                stat(ent->d_name, &st);

                if (S_ISREG(st.st_mode)) {
                    printf("  untracked:  %s\n", ent->d_name);
                    untracked_count++;
                }
            }
        }

        closedir(dir);
    }

    if (untracked_count == 0)
        printf("  (nothing to show)\n");

    printf("\n");
    return 0;
}

// ─── TODO IMPLEMENTATIONS ───────────────────────────────────────────────────

static int compare_index_entries(const void *a, const void *b) {
    const IndexEntry *ea = (const IndexEntry *)a;
    const IndexEntry *eb = (const IndexEntry *)b;
    return strcmp(ea->path, eb->path);
}

int index_load(Index *index) {
    index->count = 0;

    FILE *fp = fopen(".pes/index", "r");

    if (!fp) {
        return 0;
    }

    while (index->count < MAX_INDEX_ENTRIES) {
        IndexEntry *entry = &index->entries[index->count];
        char hash_hex[HASH_HEX_SIZE + 1];

        int result = fscanf(
            fp,
            "%o %64s %lu %u %511[^\n]\n",
            &entry->mode,
            hash_hex,
            &entry->mtime_sec,
            &entry->size,
            entry->path
        );

        if (result != 5)
            break;

        if (hex_to_hash(hash_hex, &entry->hash) != 0) {
            fclose(fp);
            return -1;
        }

        index->count++;
    }

    fclose(fp);
    return 0;
}

int index_save(const Index *index) {
    Index temp = *index;

    qsort(
        temp.entries,
        temp.count,
        sizeof(IndexEntry),
        compare_index_entries
    );

    FILE *fp = fopen(".pes/index.tmp", "w");
    if (!fp)
        return -1;

    for (int i = 0; i < temp.count; i++) {
        char hash_hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&temp.entries[i].hash, hash_hex);

        fprintf(
            fp,
            "%o %s %lu %u %s\n",
            temp.entries[i].mode,
            hash_hex,
            temp.entries[i].mtime_sec,
            temp.entries[i].size,
            temp.entries[i].path
        );
    }

    fflush(fp);
    fsync(fileno(fp));
    fclose(fp);

    if (rename(".pes/index.tmp", ".pes/index") != 0)
        return -1;

    return 0;
}

int index_add(Index *index, const char *path) {
    struct stat st;

    if (stat(path, &st) != 0) {
        fprintf(stderr, "error: cannot stat file %s\n", path);
        return -1;
    }

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "error: cannot open file %s\n", path);
        return -1;
    }

    size_t file_size = (size_t)st.st_size;

    if (file_size == 0) {
        fclose(fp);
        fprintf(stderr, "error: empty file not supported\n");
        return -1;
    }

    char *buffer = malloc(file_size);
    if (!buffer) {
        fclose(fp);
        return -1;
    }

    size_t n = fread(buffer, 1, file_size, fp);
    fclose(fp);

    if (n != file_size) {
        free(buffer);
        fprintf(stderr, "error: failed to read file\n");
        return -1;
    }

    ObjectID blob_id;

    if (object_write(OBJ_BLOB, buffer, file_size, &blob_id) != 0) {
        free(buffer);
        fprintf(stderr, "error: object_write failed\n");
        return -1;
    }

    free(buffer);

    IndexEntry *entry = index_find(index, path);

    if (!entry) {
        if (index->count >= MAX_INDEX_ENTRIES) {
            fprintf(stderr, "error: index full\n");
            return -1;
        }

        entry = &index->entries[index->count++];
    }

    if (st.st_mode & S_IXUSR)
        entry->mode = 0100755;
    else
        entry->mode = 0100644;

    entry->hash = blob_id;
    entry->mtime_sec = (uint64_t)st.st_mtime;
    entry->size = (uint32_t)st.st_size;

    strncpy(entry->path, path, sizeof(entry->path) - 1);
    entry->path[sizeof(entry->path) - 1] = '\0';

    return index_save(index);
}
