#ifndef serialize_h_INCLUDED
#define serialize_h_INCLUDED

typedef enum { 
	SERIALIZE_MODE_WRITE, 
	SERIALIZE_MODE_READ 
} SerializeMode;

typedef struct {
	SerializeMode mode;
	u32 byte_offset;
	u32 bit_offset;

	char* data;
	StackAllocator* stack;
	u64 stack_offset;
} Bitstream;

typedef struct {
	u64 size_bytes;
	char* data;
} SerializeResult;

Bitstream bitstream_init(SerializeMode mode, char* data, StackAllocator* stack);
SerializeResult serialize_result(Bitstream* stream);
void serialize_bool(Bitstream* stream, bool* value);
void serialize_u8(Bitstream* stream, u8* value);
void serialize_u32(Bitstream* stream, u32* value);
void serialize_i32(Bitstream* stream, i32* value);
void serialize_f32(Bitstream* stream, f32* value);

#ifdef CSM_CORE_IMPLEMENTATION

Bitstream bitstream_init(SerializeMode mode, char* data, StackAllocator* stack)
{
	assert((stack == NULL && mode == SERIALIZE_MODE_READ) || 
		   (stack != NULL && mode == SERIALIZE_MODE_WRITE));

	Bitstream stream = (Bitstream) {
		.mode = mode, 
		.byte_offset = 0,
		.bit_offset = 0,
		.stack = stack,
		.stack_offset = 0
	};
	if(stream.mode == SERIALIZE_MODE_WRITE) {
		stream.data = (char*)stack_head(stack);
		stream.stack_offset = stack->index;
	} else {
		stream.data = data;
	}
	return stream;
}

SerializeResult serialize_result(Bitstream* stream)
{
	return (SerializeResult) { .size_bytes = stream->byte_offset + 1, .data = stream->data };
}

void bitstream_advance_bit(u32* byte_off, u32* bit_off)
{
	u32 original_bit = *bit_off;
	u32 original_byte = *byte_off;

	if(*bit_off == 7) {
		*byte_off += 1;
		*bit_off = 0;
	} else {
		*bit_off += 1;
	}

	assert(*bit_off == 0 || *bit_off == original_bit + 1);
	assert(*byte_off == original_byte || *byte_off == original_byte + 1);
}

void bitstream_write_bits(Bitstream* stream, char* value, u32 size_bits)
{
	u64 new_size_min = stream->stack_offset + stream->byte_offset + size_bits / 8;
	if(stream->stack->index <= new_size_min) {
		stack_alloc(stream->stack, new_size_min - stream->stack->index);
	}

	u32 val_byte_off = 0;
	u32 val_bit_off = 0;
	for(u32 bit_index = 0; bit_index < size_bits; bit_index++) {
		char* write_byte = &stream->data[stream->byte_offset];
		u8 bit_to_set = 1 << stream->bit_offset;

		// TODO: Found this alternative online. bit_is_set must be 0 or 1. Think
		// about it and test after getting serialization working generally.
		// byte = (byte & ~(1<<bit_to_set)) | (bit_is_set<<bit_to_set);
		if(value[val_byte_off] & 1 << val_bit_off) {
			*write_byte |= bit_to_set;
		} else {
			*write_byte &= ~bit_to_set;
		}

		bitstream_advance_bit(&val_byte_off, &val_bit_off);
		bitstream_advance_bit(&stream->byte_offset, &stream->bit_offset);
	}
}

void bitstream_read_bits(Bitstream* stream, char* value, u32 size_bits)
{
	*value = 0;
	u32 val_byte_off = 0;
	u32 val_bit_off = 0;
	for(u32 bit = 0; bit < size_bits; bit++) {
		char* write_byte = &value[val_byte_off];
		u8 bit_to_set = 1 << val_bit_off;

		if(stream->data[stream->byte_offset] & 1 << stream->bit_offset) {
			*write_byte |= bit_to_set;
		} else {
			*write_byte &= ~bit_to_set;
		}

		bitstream_advance_bit(&val_byte_off, &val_bit_off);
		bitstream_advance_bit(&stream->byte_offset, &stream->bit_offset);
	}
}

void serialize_bits(Bitstream* stream, char* value, u32 size_bits)
{
	if(stream->mode == SERIALIZE_MODE_WRITE) {
		bitstream_write_bits(stream, value, size_bits);
	} else {
		bitstream_read_bits(stream, value, size_bits);
	}
}

void serialize_bool(Bitstream* stream, bool* value)
{
	serialize_bits(stream, (char*)value, 1);
}

void serialize_u8(Bitstream* stream, u8* value)
{
	serialize_bits(stream, (char*)value, 8);
}

void serialize_u32(Bitstream* stream, u32* value)
{
	serialize_bits(stream, (char*)value, 32);
}

void serialize_i32(Bitstream* stream, i32* value)
{
	serialize_bits(stream, (char*)value, 32);
}

void serialize_f32(Bitstream* stream, f32* value)
{
	serialize_bits(stream, (char*)value, 32);
}

#endif // CSM_CORE_IMPLEMENTATION
#endif // serialize_h_INCLUDED
