#pragma once
// Include GLEW
#include <glad/glad.h>
#include <stdexcept>
#include <memory>
template<size_t N>
struct GLBuffers {
	GLuint buffers[N];

	GLBuffers() {
		glCreateBuffers(N, buffers);
	}
	/*
	GLBuffers(const GLBuffers& other) = delete; // copy constructor
	GLBuffers& operator=(const GLBuffers& that) = delete;

	GLBuffers(GLBuffers&& other) {

	}
	GLBuffers& operator=(GLBuffers&& that) {

	}
	*/
	~GLBuffers() {
		glDeleteBuffers(N, buffers);
	}
};
class gl_error : std::exception {};

struct GLMapedBuffer {
	GLuint buffer;
	void* data;
	GLMapedBuffer(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access) : buffer(buffer) {
		data = glMapNamedBufferRange(buffer, offset, length, access);
		if (data == nullptr) {
			throw gl_error();
		}
	}
	~GLMapedBuffer() {
		glUnmapNamedBuffer(buffer);
	}
};

template<size_t N>
struct VertexArrays {
	GLuint vertex_arrays[N];

	VertexArrays() {
		glGenVertexArrays(N, vertex_arrays);
	}

	~VertexArrays() {
		glDeleteVertexArrays(N, vertex_arrays);
	}
};


struct GLBuffer {
	GLuint buffer;

	GLBuffer() {
		glCreateBuffers(1, &buffer);
	}
	/*
	GLBuffers(const GLBuffers& other) = delete; // copy constructor
	GLBuffers& operator=(const GLBuffers& that) = delete;

	GLBuffers(GLBuffers&& other) {

	}
	GLBuffers& operator=(GLBuffers&& that) {

	}
	*/
	~GLBuffer() {
		glDeleteBuffers(1, &buffer);
	}

	
	operator GLuint() const noexcept {
		return buffer;
	}
};

GLbitfield storage_flags(bool read, bool write) noexcept {
	return (read ? GL_MAP_READ_BIT : 0) | (write ? GL_MAP_WRITE_BIT : 0);
}

inline void* mapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access) {
	void* result = glMapNamedBufferRange(buffer, offset, length, access);
	if (result == nullptr)
		throw gl_error();
	return result;
}

class GLPersistentBuffer {
private:
	GLsizeiptr size;
	GLBuffer _buffer;
	void* _data;
	GLbitfield rw_flags;
public:
	void* begin() const noexcept {
		return _data;
	}

	void* end() const noexcept {
		return (void*)((char*)_data + size);
	}

	GLPersistentBuffer(GLsizeiptr size, bool read, bool write) : size{ size }, rw_flags{ storage_flags(read, write) } {
		glNamedBufferStorage(_buffer, size, nullptr, rw_flags | GL_MAP_PERSISTENT_BIT);
		_data = mapNamedBufferRange(_buffer, 0, size, rw_flags | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
	}
	template<typename T = char>
	void flush(GLintptr index, GLsizeiptr count) {
		glFlushMappedBufferRange(_buffer, index * sizeof(T), count * sizeof(T));
	}
	void reallocate(GLsizeiptr newSize) {
		char alignas(alignof(GLBuffer)) tmp_data[sizeof(GLBuffer)];
		GLBuffer* tmp = new (tmp_data) GLBuffer;
		glNamedBufferStorage(*tmp, newSize, nullptr, rw_flags | GL_MAP_PERSISTENT_BIT);
		glCopyBufferSubData(_buffer, *tmp, 0, 0, size);
		_buffer.~GLBuffer();
		memcpy(&_buffer, tmp, sizeof(GLBuffer));
		size = newSize;
		_data = mapNamedBufferRange(_buffer, 0, size, rw_flags | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
	}
	void flush() {
		flush(0, size);
	}
	GLsizeiptr capacity() const noexcept {
		return size;
	}
	const GLBuffer& buffer() noexcept {
		return _buffer;
	}
};


template<class T>
struct write_only {
	void operator=(const T& val) {
		memcpy((void*)this, (void*)(&val), sizeof(T));
	}
};



template<typename T>
class write_only_pointer {

private:
	void* data;
public:

	using difference_type = ptrdiff_t;
	write_only_pointer(T* data) : data{ (void*)data } {}
	write_only_pointer& operator+=(size_t i) {
		data = (void*)((char*)data + sizeof(T) * i);
		return *this;
	}

	write_only_pointer& operator-=(size_t i) {
		data = (void*)((char*)data - sizeof(T) * i);
		return *this;
	}

	write_only_pointer& operator++() {
		return this->operator+=(1);
	}
	write_only_pointer operator++(int) {
		write_only_pointer ret{ data };
		this->operator++();
		return ret;
	}


	write_only_pointer& operator--() {
		return this->operator-=(1);
	}

	write_only_pointer operator--(int) {
		write_only_pointer ret{ data };
		this->operator--();
		return ret;
	}

	write_only<T>& operator*() {
		return *reinterpret_cast<write_only<T>*>(data);
	}

	difference_type operator-(const write_only_pointer& other) {
		return (T*)data - (T*)other.data;
	}

	difference_type operator-(const void* other) {
		return (T*)data - (T*)other;
	}

	bool operator<=(const write_only_pointer& other) {
		return data <= other.data;
	}
	bool operator>(const write_only_pointer& other) {
		return data > other.data;
	}
	friend difference_type operator-(T* lhs, const write_only_pointer& rhs) {
		return lhs - (T*)rhs.data;
	}

	friend difference_type operator-(const write_only_pointer& lhs, T* rhs) {
		return (T*)lhs.data - rhs;
	}

};

template<typename T>
class GLPersistentBufferDoubleStack {
	using value_type = T;

	GLPersistentBuffer mapped_buffer;
	write_only_pointer<T> left;
	write_only_pointer<T> right;
public:
	GLPersistentBufferDoubleStack(GLsizeiptr initial_size) : mapped_buffer{ initial_size * (GLsizeiptr)sizeof(value_type), false, true },
		left{ (glm::mat4*)mapped_buffer.begin() }, right{ (glm::mat4*)mapped_buffer.end() } {
	}
	
	void alloc_1() {

		if (right > left)
			return;
		
		reallocate(capacity() * 2);

	}
	void push_left(const T& value) {
		alloc_1();

		*left = value;
		mapped_buffer.flush<T>(left - mapped_buffer.begin(), 1);
		++left;
	}
	void update_left(const T& value) {
		--left;
		*left = value;
		mapped_buffer.flush<T>(left - mapped_buffer.begin(), 1);
		++left;
	}
	GLsizeiptr left_offset() const noexcept{
		return 0;
	}

	GLsizeiptr right_offset()  const noexcept {
		return (capacity() - right_size());
	}
	void push_right(const T& value) {
		alloc_1();

		--right;
		*right = value;
		mapped_buffer.flush<T>(right - mapped_buffer.begin(), 1);

	}

	void update_right(const T& value) {
		*right = value;
		mapped_buffer.flush<T>(right - mapped_buffer.begin(), 1);
	}

	GLsizeiptr left_size() const noexcept {
		return left - (T*)mapped_buffer.begin();
	}
	GLsizeiptr right_size() const noexcept {
		return (T*)mapped_buffer.end() - right;
	}

	GLsizeiptr capacity() const noexcept {
		return mapped_buffer.capacity() / sizeof(T);
	}

	const GLBuffer& buffer() noexcept {
		return mapped_buffer.buffer();
	}

	void reallocate(GLsizeiptr newSize) {
		GLsizeiptr leftSize, rightSize;
		leftSize = left_size();
		rightSize = right_size();

		alignas(GLPersistentBuffer) char tmp_data[sizeof(GLPersistentBuffer)];
		GLPersistentBuffer* tmp = new (tmp_data) GLPersistentBuffer{ newSize * (GLsizeiptr)sizeof(T), false, true };
		glCopyNamedBufferSubData(mapped_buffer.buffer(), tmp->buffer(), 0, 0, leftSize * sizeof(T));
		glCopyNamedBufferSubData(mapped_buffer.buffer(), tmp->buffer(), right_offset() * sizeof(T), (newSize - rightSize)*sizeof(T), rightSize * sizeof(T));


		mapped_buffer.~GLPersistentBuffer();
		memcpy(&mapped_buffer, tmp, sizeof(GLPersistentBuffer));
		new (&left) write_only_pointer<T>((T*)mapped_buffer.begin() + leftSize);
		new (&right) write_only_pointer<T>((T*)mapped_buffer.end() - rightSize);
	}
};