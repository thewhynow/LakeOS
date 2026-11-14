# stuff i'm working on right now

## kernel allocator

- [x] dangling freelist
    - static pointer to first block that's never updated...
    - if it's fully consumed, it's still treated as unallocated!
- [] coalesce_neighbors
    - freeing a block between two free block merges only one side & fragments the heap over time
    - do next & prev merge independently
- [] krealloc
    - stored header size includes header + data, but compared only with data size
