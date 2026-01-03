#ifndef arena_h_INCLUDED
#define arena_h_INCLUDED

#define DEBUG_ARENA true
#if DEBUG_ARENA
	#define DEBUG_LOG_ALLOCATIONS true
	#define DEBUG_CAPACITY_WARNING true
#endif

typedef struct {
	u64 index;
	u64 capacity;
	char* data;
	bool initialized;
#if DEBUG_ARENA
	char debug_name[64];
#endif
} Arena;

void arena_init(Arena* arena, u64 capacity, Arena* parent, const char* debug_name);
void arena_clear(Arena* arena);
void arena_destroy(Arena* arena);
void* arena_alloc(Arena* arena, u64 size);
void* arena_head(Arena* arena);

#ifdef CSM_CORE_IMPLEMENTATION

void arena_init(Arena* arena, u64 capacity, Arena* parent, const char* debug_name)
{
	if(parent == NULL) {
		arena->data = (char*)calloc(1, capacity);
	} else {
		arena->data = (char*)arena_alloc(parent, capacity);
	}
	arena->index = 0;
	arena->capacity = capacity;
	arena->initialized = true;

#if DEBUG_ARENA
	sprintf(arena->debug_name, "%s", debug_name);
	printf("%s: Arena initialized with capacity %u.\n", debug_name, capacity);
#endif
}

void arena_clear(Arena* arena)
{
	arena->index = 0;
#if DEBUG_LOG_ALLOCATIONS
	printf("%s: Arena cleared.\n", arena->debug_name);
#endif
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
	assert(arena->data != NULL);
	if(arena->index + size >= arena->capacity) {
#if DEBUG_ARENA
		printf("%s: Arena overflow! Capacity: %u, Requested: %u\n", arena->debug_name, arena->index + size, arena->capacity);
#endif
		panic();
	}

#if DEBUG_LOG_ALLOCATIONS
	printf("%s: Arena allocation from %u-%u (%u bytes)\n", arena->debug_name, arena->index, arena->index + size, size);
#endif

	arena->index += size;

#if DEBUG_CAPACITY_WARNING
	if(arena->index > arena->capacity / 2) {
		printf("%s: \033[31mArena more than half full!\033[0m\n", arena->debug_name);
	}
#endif

	return &arena->data[arena->index - size];
}

#endif // CSM_CORE_IMPLEMENTATION
#endif // arena_h_INCLUDED
