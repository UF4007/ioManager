#define __MEMMNGR_INTERNAL_HEADER_PERMISSION														friend class mem::memUnit;\
																									friend class mem::memManager;\
												template<typename mu2, bool releaseable2>			friend class mem::memPtr;\
												template<typename mu2, bool releaseable2>			friend class mem::impPtr;\
												template<typename mu2, bool releaseable2>			friend class mem::dumbPtr;\
																									friend struct mem::memPara;\
																									friend class mem::ReflectResultKeyValue;\
																									friend struct mem::ReflectResult;\
																									friend class mem::Ingress;\
																									friend class mem::refIngress;\
																									friend class mem::Egress;\
												template<class cast2>								friend struct mem::pEgress;\
																									friend class mem::Subfile;\
																									friend class mem::lowlevel;\
												template <typename T2>								friend struct mem::has_save_fetch_struct;\

#define MEM_PERMISSION __MEMMNGR_INTERNAL_HEADER_PERMISSION

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------

class memUnit;
class memManager;
template<typename mu, bool releaseable = true> class dumbPtr;
template<typename mu, bool releaseable = true> class impPtr;
template<typename mu, bool releaseable = true> class memPtr;
struct memPara;
class ReflectResultKeyValue;
struct ReflectResult;
class Ingress;
class refIngress;
class Egress;
template<class cast> struct pEgress;
class Subfile;
template<typename illg, int ID = 0> class pFunction;

template<class mu, bool releaseable = true> using vector_dumbPtr = std::vector<dumbPtr<mu, releaseable>>;
template<class mu, bool releaseable = true> using vector_impPtr = std::vector<impPtr<mu, releaseable>>;
template<class mu, bool releaseable = true> using vector_memPtr = std::vector<memPtr<mu, releaseable>>;

template<class cast> using vector_pEgress = std::vector<pEgress<cast>>;

template<typename...Args> using pVariant = std::variant<memPtr<Args>...>;

template<typename...Args> using vector_pVariant = std::vector<pVariant<Args...>>;