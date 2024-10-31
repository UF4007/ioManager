#ifdef _WIN32
#define MEM_MAX_FILE_PATH 260
#else
#define MEM_MAX_FILE_PATH 4096
#endif

namespace file {
	using handleType = FILE*;

	void Remove(const char* c) {
		remove(c);
	}

	void* Fopen_r(const char* c) {
		return fopen(c, "rb");
	}

	void* Fopen_w(const char* c) {
		return fopen(c, "wb");
	}

	size_t GetSize(void* handle) {
		fseek((handleType)handle, 0, SEEK_END);

		size_t size = ftell((handleType)handle);

		fseek((handleType)handle, 0, SEEK_SET);
		return size;
	}

	size_t Fwrite(void* handle, const uint8_t* content, size_t size) {
		return fwrite(content, 1, size, (handleType)handle);
	}

	size_t Fread(void* handle, uint8_t* buffer, size_t size) {
		return fread(buffer, 1, size, (handleType)handle);
	}

	void Fclose(void* h) {
		fclose((handleType)h);
	}
}