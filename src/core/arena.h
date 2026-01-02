#ifndef arena_h_INCLUDED
#define arena_h_INCLUDED

#define DEBUG_LOG_ALLOCATIONS true
#define DEBUG_CAPACITY_WARNING true

typedef struct {
	u64 index;
	u64 capacity;
	char* data;
	bool initialized;
} Arena;

void arena_init(Arena* arena, u64 capacity, Arena* parent);
void arena_clear(Arena* arena);
void arena_destroy(Arena* arena);
void* arena_alloc(Arena* arena, u64 size);
void* arena_head(Arena* arena);

#ifdef CSM_CORE_IMPLEMENTATION

void arena_init(Arena* arena, u64 capacity, Arena* parent)
{
	if(parent == NULL) {
		arena->data = (char*)calloc(1, capacity);
	} else {
		arena->data = (char*)arena_alloc(parent, capacity);
	}
	arena->index = 0;
	arena->capacity = capacity;
	arena->initialized = true;
}

void arena_clear(Arena* arena)
{
	arena->index = 0;
}

void arena_destroy(Arena* arena)
{
	assert(arena->initialized);

	free(arena->data);
	arena->data = NULL;
	arena->index = 0;
	arena->capacity = 0;
	arena->initialized = false;
}

// Frees the current memory in dst if it exists, then copies src to dst.
void arena_copy(Arena* src, Arena* dst)
{
	if(dst->data != NULL) {
		arena_destroy(dst);
	}
	*dst = *src;
}

void* arena_head(Arena* arena)
{
	return (void*)&arena->data[arena->index];
}

void* arena_alloc(Arena* arena, u64 size)
{
	strict_assert(arena->data != NULL);
	if(arena->index + size >= arena->capacity) {
		printf("Arena overflow! Capacity: %u, Requested: %u\n", arena->index + size, arena->capacity);
		panic();
	}

#if DEBUG_LOG_ALLOCATIONS
	printf("Arena allocation from %u-%u (%u bytes)\n", arena->index, arena->index + size, size);
#endif

	arena->index += size;

#if DEBUG_CAPACITY_WARNING
	if(arena->index > arena->capacity / 2) {
		printf("\033[31mArena more than half full!\033[0m\n");
	}
#endif

	return &arena->data[arena->index - size];
}

#endif // CSM_CORE_IMPLEMENTATION
#endif // arena_h_INCLUDED
