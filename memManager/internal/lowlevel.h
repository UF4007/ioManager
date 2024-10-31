//hidden low level methods

//internal lowlevel class, never use it
class lowlevel {
	__MEMMNGR_INTERNAL_HEADER_PERMISSION

	inline static void logWideString(const wchar_t* wideString) noexcept {
		// Allocate buffer for multibyte (UTF-8) string
		size_t bufferSize = std::wcslen(&wideString[0]) + 1;
		std::vector<char> multiByteString(bufferSize);
		size_t convertedChars = wcstombs(multiByteString.data(), wideString, bufferSize);
		multiByteString[bufferSize - 1] = '\0';
		std::cout << multiByteString.data();
	}
	inline static void wcscpy_mem(wchar_t* dest, const wchar_t* scr, size_t size) noexcept {
		size = std::min(size, std::wcslen(scr) + 1);
		memcpy(dest, scr, size * sizeof(wchar_t));
	}
	template <size_t size>
	inline static void wcscpy_mem(wchar_t(&dest)[size], const wchar_t* scr) noexcept {
		wcscpy_mem(dest, scr, size);
	}

	template <size_t size>
	inline static void _wtos_mem(char(&dest)[size], const wchar_t* wcs) {
		std::mbstate_t state = std::mbstate_t();
		std::wcsrtombs((char*)&dest, &wcs, size, &state);
	}
	template <size_t size>
	inline static void _stow_mem(wchar_t(&dest)[size], const char* str) {
		std::mbstate_t state = std::mbstate_t();
		std::mbsrtowcs((wchar_t*) & dest, &str, size, &state);
	}



	//serialize
	//append "key\0"
	inline static void appendKeyTo_section(const char* key, std::vector<uint8_t>* section) noexcept
	{
		uint32_t size_K = strnlen(key, maxKey);
		section->resize(section->size() + 1 + size_K);
		uint8_t* keyPtr = (uint8_t*)&*(section->end() - 1) - size_K;
		memcpy(keyPtr, key, size_K);
	}
	//concrete write of memory
	template <size_t size_T>
	inline static typename std::enable_if<size_T <= 8, void>::type
	mem_toSpecialChars(uint8_t* holder, uint8_t* content) noexcept {
		*holder = 0xff;
		for (uint8_t i = 0; i < size_T; i++)
		{
			if (*content == 0)
			{
				*holder &= ~(1 << i);
				*content = 0xff;
			}
			content++;
		}
	}
	//short value
	template <typename _T>
	inline static typename std::enable_if<(sizeof(_T) <= 8), void>::type
	mem_toBytes(const _T& value, std::vector<uint8_t>* section, std::vector<uint8_t>* entireFile) noexcept {
		constexpr uint8_t size_T = sizeof(_T);
		section->resize(section->size() + size_T);
		uint8_t* dataPtr = (uint8_t*)&*(section->end() - 1) - size_T + 1;
		memcpy(dataPtr, &value, size_T);
	}
	//allocate a zone for long value (length must be confrimed, returns where the new memory placed in the entireFile), and record the offset
	inline static void mem_toBytes(uint32_t& offsetOut, uint32_t length, std::vector<uint8_t>* section, std::vector<uint8_t>* entireFile) noexcept {
		if (length == 0)
		{
			offsetOut = 0;
			mem_toBytes(offsetOut, section, entireFile);
			return;
		}
		entireFile->resize(entireFile->size() + length + sizeof(uint32_t));
		uint8_t* savePosition = (uint8_t*)&*(entireFile->end() - 1) - length + 1;
		uint8_t* lengthPosition = savePosition - sizeof(uint32_t);
		memcpy(lengthPosition, &length, sizeof(uint32_t));
		offsetOut = lengthPosition - (uint8_t*)&*(entireFile->begin());
		mem_toBytes(offsetOut, section, entireFile);
		offsetOut += sizeof(uint32_t);
	}
	//long value (length confirmed when compile)
	template <typename _T>
	inline static typename std::enable_if<(sizeof(_T) > 8), void>::type
	mem_toBytes(const _T& value, std::vector<uint8_t>* section, std::vector<uint8_t>* entireFile) noexcept {
		uint32_t offset;
		mem_toBytes(offset, sizeof(_T), section, entireFile);
		memcpy(&entireFile[offset], &value, sizeof(_T));
	}



	//deserialize
	inline static void turnToNextKey(uint8_t*& sectionLikely) noexcept {
		while (*sectionLikely != 0) {
			sectionLikely++;
		}
		sectionLikely++;
		return;
	}
	//find key from section
	inline static bool findKeyFrom_section(const char* key, uint8_t*& sectionLikely) noexcept
	{
		uint8_t*& section = sectionLikely;
		const char* keyRepeat = key;
		while (1)
		{
			if (*section == 0 && *keyRepeat == 0)	//key matched
			{
				section++;
				return true;
			}
			if (*keyRepeat != *section)
			{
				if (*section == 0 && *(section + 1) == 0 && *(section - 1) == 0)		//end of section
					return false;
				while (*section != 0)			//unrelated key
				{
					section++;
				}
				section += 2;
				if (*section == 0)					//end of section and bad position
					return false;
				while (*section != 0)			//unrelated value
				{
					section++;
				}
				section++;
				keyRepeat = key;				//rejudge key
			}
			else
			{
				keyRepeat++;
				section++;
			}
		}
	}
	//find key from section (double times)
	inline static bool findKeyFrom_section(const char* key, uint8_t*& sectionLikely, uint8_t*&sectionBegin) noexcept {
		if (findKeyFrom_section(key, sectionLikely) == false)
		{
			sectionLikely = sectionBegin;
			if (findKeyFrom_section(key, sectionLikely) == false)
			{
				sectionLikely = sectionBegin;
				return false;
			}
		}
		return true;
	}
	//concrete write of memory
	template <size_t size_T>
	inline static typename std::enable_if<size_T <= 8, bool>::type
	SpecialCharsTo_mem(const uint8_t* holder, uint8_t* content) noexcept {
		for (uint8_t i = 0; i < size_T; i++)
		{
			if (*content == 0)
			{
				return false;		//truncated
			}
			if (!(*holder & 1 << i))
			{
				*content = 0;
			}
			content++;
		}
		return true;
	}
	//short value
	template <typename _T>
	inline static typename std::enable_if<(sizeof(_T) <= 8), bool>::type
	BytesTo_mem(_T& value,const uint8_t* position, const uint8_t* entireFile, const uint8_t* endOfFile) noexcept {
		constexpr uint8_t size_T = sizeof(_T);
		memcpy(&value, position, size_T);
		return true;
	}
	//read long value zone (only returns where the memory placed, checked before the return)
	inline static bool BytesTo_mem(uint8_t*& valuePos, uint32_t& length, const uint8_t* position, const uint8_t* entireFile, const uint8_t* endOfFile) noexcept {
		uint32_t offset;
		if (BytesTo_mem(offset, position, entireFile, endOfFile) == false)
			return false;
		if (offset == 0)
		{
			length = 0;
			return true;
		}
		const uint8_t* lengthPosition = entireFile + offset;
		if (lengthPosition < entireFile)
			return false;
		if (lengthPosition + *(uint32_t*)lengthPosition > endOfFile)
			return false;
		valuePos = (uint8_t*)(lengthPosition + sizeof(uint32_t));
		memcpy(&length, lengthPosition, sizeof(uint32_t));
		return true;
	}
	//long value (length confirmed when compile, copy entire size when execute)
	template <typename _T>
	inline static typename std::enable_if<(sizeof(_T) > 8), bool>::type
	BytesTo_mem(_T& value, const uint8_t* position, const uint8_t* entireFile, const uint8_t* endOfFile) noexcept {
		constexpr uint32_t size_T = sizeof(_T);
		uint8_t* valuePos;
		uint32_t length;
		if (BytesTo_mem(valuePos, length, position, entireFile, endOfFile) == false)
			return false;
		length = std::min(size_T, length);
		memcpy(&value, valuePos, length);
		return true;
	}



	//variant adaptive
	template <typename First, typename ...Args>
	struct pushVariantHelper {
		__MEMMNGR_INTERNAL_HEADER_PERMISSION
	private:
		template<bool _void>
		inline bool createIter(uint32_t i, std::variant<First, Args...>& variant, memUnit* mu, memPara& para) noexcept
		{
			return false;
			//assert(!("An error has happened when creating pVariant from an archive file. \n Matched type not found."));
		}
		template<bool _void, typename IterFirst, typename... IterArgs>
		bool createIter(uint32_t i, std::variant<First, Args...>& variant, memUnit* mu, memPara& para) noexcept;
		void* pv;
	public :
		inline bool push(uint32_t index, std::variant<First, Args...>& variant, memUnit* mu, void* pValue, memPara& para) noexcept
		{
			pv = pValue;
			return createIter<false, First, Args...>(index, variant, mu, para);
		}
		memUnit* mu;
	};



	//memPtr command block
	struct memPtrComm {
		int count;
		memUnit* content;
		inline memPtrComm() {}
		inline memPtrComm(memUnit* m) noexcept
		{
			count = 1;
			content = m;
		}
		inline static std::atomic_flag lockPool = ATOMIC_FLAG_INIT; 
		inline static std::atomic_flag lockFreed = ATOMIC_FLAG_INIT; 
		static std::deque<memPtrComm> memPool; 
		static std::stack<memPtrComm*> memFreed; 
		static void* operator new(size_t size) noexcept; 
		static void operator delete(void* pthis) noexcept;
	};

	//corresponding pointer
	struct memPtrCorr {
		memUnit* ptrRUN;
		union {
			uint32_t sectionStartOffset;			//r w
			memUnit* ptrRUN2;
		};
		bool isConstructed = false;
		inline memPtrCorr(memUnit* a, uint32_t b) noexcept {
			ptrRUN = a;
			sectionStartOffset = b;
		}
		inline bool operator<(const memPtrCorr& ptr) const noexcept
		{
			return ptr.ptrRUN < ptrRUN;
		}
		inline void setOffset(uint32_t a)  noexcept { sectionStartOffset = a; }
	};
};
inline std::deque<mem::lowlevel::memPtrComm> mem::lowlevel::memPtrComm::memPool = {}; 
inline std::stack<mem::lowlevel::memPtrComm*> mem::lowlevel::memPtrComm::memFreed = {}; 
inline void* mem::lowlevel::memPtrComm::operator new(size_t size) noexcept
{
	while (lockFreed.test_and_set(std::memory_order_acquire));
	if (memFreed.size() == 0)
	{
		lockFreed.clear(std::memory_order_release);
		while (lockPool.test_and_set(std::memory_order_acquire));
		memPool.emplace_back();
		mem::lowlevel::memPtrComm* ret = &*(memPool.end() - 1);
		lockPool.clear(std::memory_order_release);
		return ret;
	}
	else
	{
		mem::lowlevel::memPtrComm* ret = memFreed.top();
		memFreed.pop();
		lockFreed.clear(std::memory_order_release);
		return ret;
	}
}
inline void mem::lowlevel::memPtrComm::operator delete(void* pthis) noexcept
{
	while (lockFreed.test_and_set(std::memory_order_acquire));
	memFreed.push((mem::lowlevel::memPtrComm*)pthis);
	lockFreed.clear(std::memory_order_release);
}