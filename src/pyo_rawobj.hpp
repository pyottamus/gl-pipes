#pragma once
#include <stdint.h>
#include <stdio.h>
#include <system_error>
#include <vector>

#include <limits.h>

#if ULONG_MAX == 0xffffffffUL //32 bit long
#ifdef _WIN32
#define fseek64(stream, offset, origin) _fseeki64(stream, offset, origin)
#define ftell64(stream) _ftelli64(stream)
#else
#error "long type is 32 bit. Please define a platfrom depependent fseek64 and ftell64"
#endif
#else
#define fseek64(stream, offset, origin) fseek(stream, offset, origin)
#define ftell64(stream) ftell(stream)
#endif

__pragma(pack(push, 1))
struct header_t {
	uint16_t BOM;
	uint8_t type;
	uint8_t flags;
	uint32_t obj_count;

	bool isFloat(){
		return flags & 1;
	}
	bool hasNormal(){
		return flags & 2;
	}
	bool hasUV(){
		return flags & 4;
	}
	bool indexed(){
		return flags & 8;
	}
	uint8_t indexSize() {
		return flags >> 4;
	}
};

__pragma(pack(pop))

constexpr uint16_t BOM = (uint16_t('P') << 8) | uint16_t('J');
constexpr uint16_t ANTI_BOM = (uint16_t('J') << 8) | uint16_t('P');

static_assert(sizeof(header_t) == 8);

#ifdef _WIN32
#include <Windows.h>

bool printWin32Error(DWORD errorMessageID, FILE* file) {
	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
	if (messageBuffer == NULL) {
		fprintf(file, "Error id %ul. Failed to format message", errorMessageID);
		return false;
	}
	else {
		fwrite(messageBuffer, 1, size, file);
		LocalFree(messageBuffer);
		return true;
	}

}

#endif // _WIN32


struct CPPFile {
	FILE* file;
	CPPFile(const char* path, const char* mode) {
		file = fopen(path, mode);

		if (file == NULL) {
			throw std::system_error(GetLastError(), std::system_category());
		}
	}

	~CPPFile() {
		if (fclose(file) == EOF) {
			printWin32Error(GetLastError(), stderr);
		}
	}
	size_t read(void* buffer, size_t size, size_t count) noexcept {
		return fread(buffer, size, count, file);
	}

	void tread(void* buffer, size_t size, size_t count){
		if (fread(buffer, size, count, file) != count) {
			throw std::system_error(GetLastError(), std::system_category());
		}

	}
	template<typename T>
	size_t read(T* buffer, size_t count) noexcept {
		return fread(buffer, sizeof(T), count, file);
	}
	template<typename T>
	void tread(T* buffer, size_t count) {
		if (fread(buffer, sizeof(T), count, file) != count) {
			throw std::system_error(GetLastError(), std::system_category());
		}
	}
	int64_t tell() noexcept{
		return ftell64(file);
	}

	int64_t ttell() {
		int64_t pos = ftell64(file);
		if (pos == -1L) {
			throw std::system_error(GetLastError(), std::system_category());
		}

		return pos;
	}

	int seek(long offset, int origin) noexcept {
		return fseek64(file, offset, origin);
	}

	int tseek(long offset, int origin) {
		if (fseek64(file, offset, origin) != 0) {
			throw std::system_error(GetLastError(), std::system_category());
		}

		return 0;
	}


};

struct Subobject {
	size_t triangles_start;
	size_t triangles_end;

	size_t verticies_start;
	size_t verticies_end;

	size_t normals_start;
	size_t normals_end;

	size_t uvs_start;
	size_t uvs_end;

};
struct RawobjectReader {
	CPPFile file;
	header_t header;
	uint8_t sizeof_index_t;
	uint8_t sizeof_element_t;
	uint8_t attrib_count;
	struct counts_t {
		size_t triangles = 0;
		size_t verticies = 0;
	} counts;

	std::vector<counts_t> obj_counts;
	std::vector<Subobject> sub_offsets;
	Subobject offsets;

	size_t index_start;
	size_t index_end;

	size_t arrays_start;
	size_t arrays_end;

	RawobjectReader(const char* path) : file{ path, "rb" } {
		file.tread(&header, 1);
		if (header.BOM != BOM) {
			if (header.BOM == ANTI_BOM) {
				throw std::invalid_argument("File has wrong endianness");
			}
			else {
				throw std::invalid_argument("File is not a JP file");
			}
		}
		if (header.type != 0) {
			throw std::invalid_argument("File is not a JP Rawobject file");
		}

		sizeof_index_t = 1 << header.indexSize();
		sizeof_element_t = header.isFloat() ? 4 : 8;
		attrib_count = 1 + header.indexed() + header.hasNormal() + header.hasUV();

		obj_counts.resize(header.obj_count);
		sub_offsets.resize(header.obj_count);

		for (uint32_t i = 0; i < header.obj_count; ++i) {
			if (header.indexed()) {
				
				file.tread((void*)&obj_counts[i].triangles, sizeof_index_t, 1);
				counts.triangles += obj_counts[i].triangles;
			}
			file.tread((void*)&obj_counts[i].verticies, sizeof_index_t, 1);
			counts.verticies += obj_counts[i].verticies;
		}
		size_t pos = file.ttell();
		pos = (pos + 7ull) & (~7ull); // round up to 8

		index_start = 0;
		index_end = 0;
		if (header.indexed()) {

			index_start = pos;

			offsets.triangles_start = pos;
			
			
			for (uint32_t i = 0; i < header.obj_count; ++i) {
				sub_offsets[i].triangles_start = pos;
				pos += sizeof_index_t * 3 * obj_counts[i].triangles;
				sub_offsets[i].triangles_end = pos;
			}
			offsets.triangles_end = pos;

			pos = (pos + 7ull) & (~7ull); // round up to 8
			index_end = pos;
		}

		offsets.verticies_start = pos;
		arrays_start = pos;

		for (uint32_t i = 0; i < header.obj_count; ++i) {
			sub_offsets[i].verticies_start = pos;
			pos += sizeof_element_t * 3 * obj_counts[i].verticies;
			sub_offsets[i].verticies_end = pos;
		}
		offsets.verticies_end = pos;
		pos = (pos + 7ull) & (~7ull); // round up to 8
		arrays_end = pos;

		if (header.hasNormal()) {
			offsets.normals_start = pos;

			for (uint32_t i = 0; i < header.obj_count; ++i) {
				sub_offsets[i].normals_start = pos;
				pos += sizeof_element_t * 3 * obj_counts[i].verticies;
				sub_offsets[i].normals_end = pos;
			}
			offsets.normals_end = pos;
			pos = (pos + 7ull) & (~7ull); // round up to 8
			arrays_end = pos;
		}
		if (header.hasUV()) {

			offsets.uvs_start = pos;
			for (uint32_t i = 0; i < header.obj_count; ++i) {
				sub_offsets[i].uvs_start = pos;
				pos += sizeof_element_t * 2 * obj_counts[i].verticies;
				sub_offsets[i].uvs_end = pos;
			}
			offsets.uvs_end = pos;
			pos = (pos + 7ull) & (~7ull); // round up to 8
			arrays_end = pos;
		}

		
	}

	~RawobjectReader() {
	}
};

