## Phase 5: Branching and Checkout

### Q5.1 Branching and Checkout

A branch in Git is just a file inside `.pes/refs/heads/` that stores a commit hash.  
To implement `pes checkout <branch>`, first check if the branch file exists.  
Then update `.pes/HEAD` so that it points to the selected branch reference like:

ref: refs/heads/branch_name

After that, read the commit hash stored in that branch file and load its tree object.  
The working directory must then be updated to match that tree by restoring files and directories exactly as stored in the commit.

This operation is complex because uncommitted local changes may be overwritten.  
The system must carefully compare the current working directory with the target branch before replacing files.

---

### Q5.2 Dirty Working Directory Detection

Before switching branches, the system must check whether tracked files have uncommitted changes.

This can be done by comparing:

1. Current working directory files
2. `.pes/index` staged entries
3. Object store blobs from the current HEAD commit

If a file exists in the index but its size or modification time differs from the working directory, it means the file was modified.

If that same file is also different in the target branch, checkout must stop and refuse the operation to prevent data loss.

This is called dirty working directory conflict detection.

---

### Q5.3 Detached HEAD

Detached HEAD happens when `.pes/HEAD` stores a commit hash directly instead of pointing to a branch reference.

Example:

Instead of:

ref: refs/heads/main

it contains:

a1b2c3d4...

If commits are made in this state, new commits are created normally, but no branch points to them.

This means those commits can be lost later because they are not reachable from any branch.

A user can recover them by creating a new branch manually pointing to that commit hash.

Example:

`.pes/refs/heads/recovery`

This saves the detached commits permanently.

---

## Phase 6: Garbage Collection and Space Reclamation

### Q6.1 Finding Unreachable Objects

Over time, some objects become unreachable because no branch points to them.

To delete them:

1. Start from all branch references inside `.pes/refs/heads/`
2. Read each commit hash
3. Traverse parent commits recursively
4. Visit all tree objects
5. Visit all blob objects connected to those trees

Store all reachable hashes inside a Hash Set for fast lookup.

After traversal, scan `.pes/objects/`

If an object hash is not present in the Hash Set, it is unreachable and can be safely deleted.

For a repository with 100,000 commits and 50 branches, nearly all commits, trees, and blobs may need to be visited once.

This could be several hundred thousand total objects.

---

### Q6.2 Garbage Collection Race Condition

Running garbage collection during a commit is dangerous because of race conditions.

Example:

1. Commit process creates a new tree object
2. Before updating HEAD, GC starts scanning reachable objects
3. Since HEAD is not updated yet, GC thinks the new tree is unreachable
4. GC deletes that tree object
5. Commit then updates HEAD to point to a commit that references a deleted object

This causes repository corruption.

Real Git avoids this by using locks and temporary protection mechanisms.

Objects being created are protected until references are fully updated.

GC usually runs only when no commit operation is actively modifying the repository.
