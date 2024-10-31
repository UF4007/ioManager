template<typename T>
struct is_template : std::false_type {};
template<template<typename...> class T, typename... Args>
struct is_template<T<Args...>> : std::true_type {};

template<typename T>
struct extract_template_memUnit { using type = void; };
template<typename T, bool releaseable>
struct extract_template_memUnit<memPtr<T, releaseable>> { using type = T; };

template<typename T>
struct is_impPtr {
private:
	template <class U>
	static std::true_type check(const impPtr<U>&);
	static std::false_type check(...);
public:
	static constexpr bool value = decltype(check(std::declval<T>()))::value;
};

template<typename T>
struct is_memPtr {
private:
	template <class U>
	static std::true_type check(const memPtr<U>&);
	static std::false_type check(...);
public:
	static constexpr bool value = decltype(check(std::declval<T>()))::value;
};

template<typename T>
struct has_save_fetch_struct {

	template <typename V, typename U = void>
	struct has_save_fetch_struct_helper : std::false_type {};
	template <typename V>
	struct has_save_fetch_struct_helper<V, std::void_t<decltype(V::save_fetch_size), decltype(std::declval<V>().save_fetch_struct(std::declval<uint8_t*>(), std::declval<memPara&>()))>> : std::true_type {};

	static constexpr bool value = has_save_fetch_struct_helper<T>::value;
};

//template <typename T>
//struct has_save_fetch_struct_helper {
//	template<typename V> using save_fetch_struct_ptr = void(V::*)(uint8_t*, memPara);
//	template <typename U>
//	static std::enable_if_t<std::is_same<save_fetch_struct_ptr<U>, decltype(U::save_fetch_struct)>::value, std::true_type> check(int);
//	template <typename U>
//	static std::false_type check(...);
//	static constexpr bool value = decltype(check<T>(0))::value;
//};

template <typename T, typename = void>
struct enum_type {
	using type = int;
};

template <typename T>
struct enum_type<T, std::void_t<typename std::underlying_type<T>::type>> {
	using type = typename std::underlying_type<T>::type;
};

template <typename T>
struct is_string : std::false_type {
	static constexpr size_t byteSize = 0;
};

template <typename CharT, typename Traits, typename Allocator>
struct is_string<std::basic_string<CharT, Traits, Allocator>> : std::true_type {
	using base_type = CharT;
	static constexpr size_t byteSize = sizeof(CharT);
};

template <typename T>
struct is_arithmetic_or_its_array {
	static constexpr bool value = std::is_arithmetic<T>::value ||
		(std::is_arithmetic<typename std::remove_all_extents<T>::type>::value &&
			std::is_array<T>::value);
};

template <typename T, typename = void>
struct has_iterator : std::false_type {};

template <typename T>
struct has_iterator<T, std::void_t<typename T::iterator>> : std::true_type {};

template <typename T>
struct is_array : std::false_type {};

template <typename T, size_t _size>
struct is_array<std::array<T, _size>> : std::true_type {};

template <typename T>
struct is_vector : std::false_type {};

template <typename T>
struct is_vector<std::vector<T>> : std::true_type {};

template <>
struct is_vector<std::vector<bool>> : std::false_type {};					//notoriously

template <typename T, typename = void>
struct is_forward_list : std::true_type {};

template <typename T>
struct is_forward_list<T, std::void_t<decltype(std::declval<T>().size())>> : std::false_type {};

template <typename T>
struct is_stack : std::false_type {};

template <typename T, typename _container>
struct is_stack<std::stack<T,_container>> : std::true_type {};

template <typename T>
struct is_queue : std::false_type {};

template <typename T, typename _container>
struct is_queue<std::queue<T, _container>> : std::true_type {};

template <typename T>
struct is_stl_container {
	static constexpr bool value = (has_iterator<T>::value || is_array<T>::value// || is_stack<T>::value || is_queue<T>::value		//not allowed for special containers.
		) && !is_string<T>::value
		 && !std::is_same_v<T ,std::vector<bool>>;							//notoriously
};

template <typename T>
struct is_variant : std::false_type {};

template <typename...Args>
struct is_variant<std::variant<Args...>> : std::true_type {};

template <typename T, typename = void>
struct is_pair : std::false_type {};

template <typename first, typename second>
struct is_pair<std::pair<first, second>> : std::true_type {};

template <typename, typename = void>
struct has_emplace_back : std::false_type {};

template <typename T>
struct has_emplace_back<T, std::void_t<decltype(std::declval<T>().emplace_back(std::declval<typename T::value_type>()))>> : std::true_type {};

template <typename, typename = void>
struct has_emplace : std::false_type {};

template <typename T>
struct has_emplace<T, std::void_t<decltype(std::declval<T>().emplace(std::declval<typename T::value_type>()))>> : std::true_type {};


template <typename, typename = void>
struct has_emplace_front : std::false_type {};

template <typename T>
struct has_emplace_front<T, std::void_t<decltype(std::declval<T>().emplace_front(std::declval<typename T::value_type>()))>> : std::true_type {};

template<typename T>
struct is_atomic : std::false_type {};

template<typename T>
struct is_atomic<std::atomic<T>> : std::true_type {};

template<typename T>
struct atomic_under_type{
	using type = void;
};

template<typename T>
struct atomic_under_type<std::atomic<T>> {
	using type = T;
};

