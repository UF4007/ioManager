//archive file structure definition
constexpr uint8_t magicOfFile[] = { 0x17, 0x3a, 0x56 };
constexpr uint8_t magicOfFile32[] = { 0x17, 0x3a, 0x56, 0x32 };
constexpr uint8_t magicOfFile64[] = { 0x17, 0x3a, 0x56, 0x64 };
struct headerOfFile {
	uint8_t magic[4];
	uint64_t pointerOfFirst;
	uint32_t offsetOfSubfile;
	uint32_t offsetOfIngress;
	uint32_t offsetOfPtrTable;
	uint32_t sizeOfPtrTable;
	enum :uint8_t {
		Big_Endian = 0,
		Little_Endian = 1,
	} endian;
	uint8_t reserve[31];
};
// that means a single file cannot be larger than 4GB. that is enough. who needs a 4GB large file?


struct variantOfFile{
	uint32_t offset;
	uint32_t type;
};
//how the variant save in the file


struct pairOfFile {
	uint32_t offset;
	uint32_t sizeOfFirst;
};
//how the pair save in the file

inline bool is_big_endian() {
	static const union endian {
		int i;
		char c[4];
	}u = {1};
	return (u.c[0] == 0);
}
//judge the endian of the intend machine