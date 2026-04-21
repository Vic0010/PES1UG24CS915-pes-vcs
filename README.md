# PES Version Control System (PES-VCS)

## Project Description

PES-VCS is a simplified version control system inspired by Git. It implements the core concepts of object storage, tree objects, staging area (index), and commit creation.

This project helps understand how Git internally stores files, tracks changes, and manages commits using object-based storage.

The system supports:

* Initializing a repository
* Storing file contents as blob objects
* Creating tree objects
* Managing staging area using index
* Creating commit objects
* Tracking project history

---

## Team Members

### Member 1

Name: Vikas R
SRN: PES1UG24CS915

### Member 2

Name: Kundan V
SRN: PES1UG24CSXXX

---

## Project Structure

```text
PES1UG24CS915-pes-vcs/
│
├── object.c
├── object.h
├── tree.c
├── tree.h
├── index.c
├── index.h
├── commit.c
├── commit.h
├── pes.c
├── pes.h
├── Makefile
├── README.md
└── .pes/
```

---

## Features Implemented

### Phase 1: Object Storage

* Blob object creation
* SHA-256 hashing
* Object storage inside `.pes/objects/`

### Phase 2: Tree Objects

* Tree serialization
* Tree parsing
* Tree creation from index

### Phase 3: Index (Staging Area)

* Load index
* Save index
* Add files to staging
* Remove files from staging
* Status checking

### Phase 4: Commit Creation

* Commit object generation
* HEAD update
* Parent commit linking
* Author and timestamp tracking

---

## Requirements

Install the following before running:

```bash
sudo apt update
sudo apt install build-essential libssl-dev git
```

---

## How to Run the Project

## Step 1: Clone Repository

```bash
git clone https://github.com/your-username/PES1UG24CS915-pes-vcs.git
cd PES1UG24CS915-pes-vcs
```

---

## Step 2: Build the Project

```bash
make clean
make
```

This creates:

```text
./pes
```

---

## Step 3: Initialize Repository

```bash
./pes init
```

Expected output:

```text
Initialized empty PES repository in .pes/
```

---

## Step 4: Create Sample Files

```bash
echo "hello" > file1.txt
echo "world" > file2.txt
```

---

## Step 5: Add Files to Staging

```bash
./pes add file1.txt
./pes add file2.txt
```

This stores blob objects inside:

```text
.pes/objects/
```

---

## Step 6: Check Status

```bash
./pes status
```

Expected output:

```text
Staged changes:
  staged: file1.txt
  staged: file2.txt
```

---

## Step 7: Create Commit

```bash
export PES_AUTHOR="Vikas R <PES1UG24CS915>"
./pes commit "Initial commit"
```

Expected output:

```text
Created commit successfully
```

---

## Step 8: Verify HEAD

```bash
cat .pes/HEAD
cat .pes/refs/heads/main
```

This shows the latest commit hash.

---

## Step 9: View Stored Objects

```bash
find .pes/objects -type f
```

To inspect object content:

```bash
xxd .pes/objects/REAL_PATH | head -20
```

---

## Git Commit History

To check commit history:

```bash
git log --oneline
```

Minimum 5 commits were maintained during implementation.

---

## Phase 5: Branching and Checkout

### Branch Checkout

A branch is stored inside `.pes/refs/heads/` and contains a commit hash.

`pes checkout <branch>` works by:

1. Reading the branch reference
2. Updating `.pes/HEAD`
3. Loading the commit tree
4. Restoring files in working directory

This ensures the working directory matches the selected branch.

### Dirty Working Directory Detection

Before checkout:

* Compare working directory files
* Compare `.pes/index`
* Compare current HEAD commit

If modified files may be overwritten, checkout must stop to prevent data loss.

### Detached HEAD

Detached HEAD happens when `.pes/HEAD` stores a commit hash directly instead of a branch reference.

Commits created in this state may be lost unless a new branch is created pointing to that commit.

---

## Phase 6: Garbage Collection

### Finding Unreachable Objects

To delete unreachable objects:

1. Start from all branches
2. Traverse all commits
3. Traverse all trees
4. Traverse all blobs

Store reachable hashes in a Hash Set.

Any object not present in the set is unreachable and can be deleted safely.

### Race Condition During GC

Garbage collection during commit creation is dangerous.

Example:

* New tree object is created
* HEAD is not updated yet
* GC runs and deletes the new object
* Commit later points to deleted object

This causes repository corruption.

Git prevents this using locks and temporary protection mechanisms.

---

## Conclusion

This project demonstrates the internal working of a version control system similar to Git.

It covers object storage, trees, staging area, commits, branching concepts, and garbage collection.

It provides practical understanding of how modern version control systems manage files and history efficiently.

This causes repository corruption.

Real Git avoids this by using locks and temporary protection mechanisms.

Objects being created are protected until references are fully updated.

GC usually runs only when no commit operation is actively modifying the repository.
