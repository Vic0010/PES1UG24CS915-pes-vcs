// commit.c — Commit creation and history traversal
//
// Commit object format (stored as text, one field per line):
//
//   tree <64-char-hex-hash>
//   parent <64-char-hex-hash>
//   author <name> <unix-timestamp>
//   committer <name> <unix-timestamp>
//
//   <commit message>

#include "commit.h"
#include "index.h"
#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

// Forward declarations
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);
int object_read(const ObjectID *id, ObjectType *type_out, void **data_out, size_t *len_out);

// ─── PROVIDED FUNCTIONS (keep existing ones same) ───────────────────────────
// Keep your existing commit_parse(), commit_serialize(),
// commit_walk(), head_read(), head_update() unchanged

// ─── TODO IMPLEMENTATION ────────────────────────────────────────────────────

int commit_create(const char *message, ObjectID *commit_id_out) {
    Commit commit;
    memset(&commit, 0, sizeof(Commit));

    /* Step 1: Create tree from current index */
    if (tree_from_index(&commit.tree) != 0) {
        fprintf(stderr, "error: failed to create tree from index\n");
        return -1;
    }

    /* Step 2: Try reading parent commit (if exists) */
    if (head_read(&commit.parent) == 0) {
        commit.has_parent = 1;
    } else {
        commit.has_parent = 0;
    }

    /* Step 3: Author */
    const char *author = getenv("PES_AUTHOR");
    if (!author || strlen(author) == 0) {
        author = "Unknown Author";
    }

    strncpy(commit.author, author, sizeof(commit.author) - 1);
    commit.author[sizeof(commit.author) - 1] = '\0';

    /* Step 4: Timestamp */
    commit.timestamp = (uint64_t)time(NULL);

    /* Step 5: Commit message */
    strncpy(commit.message, message, sizeof(commit.message) - 1);
    commit.message[sizeof(commit.message) - 1] = '\0';

    /* Step 6: Serialize commit */
    void *serialized_data = NULL;
    size_t serialized_len = 0;

    if (commit_serialize(&commit, &serialized_data, &serialized_len) != 0) {
        fprintf(stderr, "error: commit serialization failed\n");
        return -1;
    }

    /* Step 7: Write commit object */
    if (object_write(
            OBJ_COMMIT,
            serialized_data,
            serialized_len,
            commit_id_out) != 0) {
        free(serialized_data);
        fprintf(stderr, "error: failed to write commit object\n");
        return -1;
    }

    free(serialized_data);

    /* Step 8: Update HEAD */
    if (head_update(commit_id_out) != 0) {
        fprintf(stderr, "error: failed to update HEAD\n");
        return -1;
    }

    return 0;
}
