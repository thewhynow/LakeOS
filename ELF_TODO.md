# ELF Support — To-Do List

Tailored to the current LakeOS codebase.

## Current state

- **`kernel/include/kernel/elf.h`** — solid coverage of the ELF header, *section*
  headers, symbol table, and relocations. But it's **missing the program header
  struct** — and that's the one structure an executable loader actually uses.
  Section headers are for linkers; you load segments via program headers.
- **`kernel/src/elf.c`** — empty (just `#include`), and **not in `build.sh`**.
- **VMM** — single address space (one page dir). `vmm_map_page(paddr, vaddr, ring3)`
  exists; no per-process page directory or `cr3` switch yet.
- **`jump_ring3`** (`kernel/asm/user.s`) — hardcodes entry symbol `user` + stack
  `0xBFFFF000`. Needs to become parameterized.
- **TSS** — `esp0` set to the boot stack; fine for a single process.
- **Syscalls** — read/write/open/close. No `exec`/`exit`.
- **⚠️ VFS has no `seek`** — `VFS_read(desc, buf, len)` reads sequentially. The
  loader needs to read at arbitrary file offsets (`p_header_off`, each segment's
  `offset`). This is a real blocker (see Phase 0).

---

## Phase 0 — Unblock file access

- [ ] Add a seek capability so the loader can read at arbitrary offsets. Either:
  - add `f_Seek` to `t_VFSOperations` + `VFS_seek()` and implement in the FAT
    driver, **or**
  - add a "read whole file into a kmalloc'd buffer" helper (`Stat` gives you
    `size`) and parse the ELF from memory. *Simpler — recommend this first.*

## Phase 1 — Finish `elf.h` (the missing piece)

- [ ] Add the **program header struct**:

  ```c
  typedef struct {
    elf32_word type;    /* PT_* */
    elf32_off  offset;  /* file offset of segment */
    elf32_addr vaddr;   /* virtual addr to load at */
    elf32_addr paddr;   /* ignored on x86 */
    elf32_word filesz;  /* bytes present in file */
    elf32_word memsz;   /* bytes in memory (>= filesz; rest is bss) */
    elf32_word flags;   /* PF_* */
    elf32_word align;
  } elf32_p_header_t;
  ```

- [ ] Add segment types: `PT_NULL=0, PT_LOAD=1, PT_DYNAMIC=2, PT_INTERP=3,
      PT_NOTE=4, PT_PHDR=6`.
- [ ] Add segment flags: `PF_X=1, PF_W=2, PF_R=4`.

## Phase 2 — Implement the loader in `elf.c`

- [ ] `elf_validate(header)`: check magic (`0x7F 'E' 'L' 'F'`), `CLASS_32`,
      `DATA_2LSB`, `type == EXEC`, `machine == 386`, `version == CURR`.
- [ ] `elf_load(path, uint32_t *out_entry)`:
  1. Open + read ELF header; validate.
  2. Seek to `p_header_off`; iterate `p_header_num_entries` program headers
     (`p_header_entry_size` stride).
  3. For each `PT_LOAD` segment:
     - Map user pages (`ring3=true`, writable) covering `[vaddr, vaddr+memsz)`,
       page-aligned down. Handle segments not starting on a page boundary and
       segments spanning multiple pages / overlapping a previous segment's last
       page.
     - Copy `filesz` bytes from file `offset` -> `vaddr`.
     - Zero the trailing `memsz - filesz` bytes (the `.bss`).
  4. Return `header.entry` via `out_entry`.
- [ ] (Optional, later) Re-protect non-writable segments read-only after copying
      (W^X).

## Phase 3 — Generalize the userspace handoff

- [ ] Rewrite `jump_ring3` -> `enter_user(uint32_t entry, uint32_t user_stack)`:
      take entry + stack as args instead of the hardcoded `user`/`0xBFFFF000`,
      build the same `iret` frame.
- [ ] Allocate and map a **user stack** (e.g. a few pages, `ring3=true`) and pass
      its top as `user_stack`.
- [ ] Confirm `g_tss.esp0` points to a valid ring-0 stack before entering (it does
      today).

## Phase 4 — Syscalls

- [ ] Add `SYSCALL_EXEC` and `SYSCALL_EXIT` to `e_SYSCALL_NUMS` and the `switch`
      in `sys.c`.
- [ ] `sys_exec(path)`: `elf_load` -> set up user stack -> `enter_user(entry, stack)`.
- [ ] `sys_exit(code)`: for now, halt / return to a kernel idle loop (your
      `user.s` currently spins forever — `exit` lets it return cleanly).

## Phase 5 — Build & test

- [ ] Add `elf.c` to `build.sh` (near line ~116:
      `$compiler_path -c kernel/src/elf.c -o elf.o $c_flags`).
- [ ] Build a tiny freestanding user ELF: `-ffreestanding -nostdlib -static`,
      custom linker script with a fixed entry vaddr (e.g. `0x08048000`), that does
      an `int 0x80` write then `exit`.
- [ ] Place it on `fat.img` (you already have `debug-fat-hd`/`mformat` flow;
      `mcopy` it in).
- [ ] In `kernel_main`, replace the hardcoded `jump_ring3()` with
      `sys_exec("/your_prog.elf")` and verify it prints from a *loaded* ELF.

## Phase 6 — Later / harder

- [ ] Per-process address spaces (clone a page directory, switch `cr3` on `exec`)
      — required before you can run more than one program or reuse the same vaddrs.
- [ ] `argc`/`argv`/`envp` on the user stack per the System V i386 ABI.
- [ ] PIE/`ET_DYN` + relocations, and `PT_INTERP`/dynamic linking — skip until
      static `ET_EXEC` works end-to-end.

---

**Start here:** the two things that will bite you first are **no VFS seek**
(Phase 0) and **no program header struct** (Phase 1) — everything in Phase 2
depends on both.
