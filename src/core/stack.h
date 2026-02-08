#ifndef stack_h_INCLUDED
#define stack_h_INCLUDED

#define DEBUG_STACK true
#if DEBUG_STACK
	#define DEBUG_LOGGING false
	#define DEBUG_CAPACITY_WARNING false
#endif

typedef struct {
	u64 index;
	u64 capacity;
	u8* data;
	bool initialized;
#if DEBUG_STACK
	char debug_name[64];
#endif
} StackAllocator;

StackAllocator stack_init(u8* memory, u64 capacity, const char* debug_name);
void stack_clear(StackAllocator* stack);
void stack_clear_to_zero(StackAllocator* stack);
void* stack_alloc(StackAllocator* stack, u64 size);
void* stack_head(StackAllocator* stack);

#ifdef CSM_CORE_IMPLEMENTATION

StackAllocator stack_init(u8* memory, u64 capacity, const char* debug_name)
{
	StackAllocator stack;
	stack.data = memory;
	stack.index = 0;
	stack.capacity = capacity;
	stack.initialized = true;

#if DEBUG_STACK
	sprintf(stack.debug_name, "%s", debug_name);
#if DEBUG_LOGGING
	printf("%s: StackAllocator initialized with capacity %lu.\n", debug_name, capacity);
#endif
#endif

	return stack;
}

void stack_clear(StackAllocator* stack)
{
	stack->index = 0;
#if DEBUG_LOGGING
	printf("%s: StackAllocator cleared.\n", stack->debug_name);
#endif
}

void stack_clear_to_zero(StackAllocator* stack)
{
	memset(stack->data, 0, stack->index);
	stack_clear(stack);
}

void* stack_head(StackAllocator* stack)
{
	return (void*)&stack->data[stack->index];
}

void* stack_alloc(StackAllocator* stack, u64 size)
{
	assert(stack->data != NULL);
	if(stack->index + size >= stack->capacity) {
#if DEBUG_STACK
		printf("%s: StackAllocator overflow! Capacity: %lu, Requested: %lu\n", stack->debug_name, stack->capacity, stack->index + size);
#endif
		panic();
	}

#if DEBUG_LOGGING
	printf("%s: StackAllocator allocation from %lu-%lu (%lu bytes)\n", stack->debug_name, stack->index, stack->index + size, size);
#endif

	stack->index += size;

#if DEBUG_CAPACITY_WARNING
	if(stack->index > stack->capacity / 2) {
		printf("%s: \033[31mStackAllocator more than half full!\033[0m\n", stack->debug_name);
	}
#endif

	return &stack->data[stack->index - size];
}

#endif // CSM_CORE_IMPLEMENTATION
#endif // stack_h_INCLUDED
