//Trial code(demo)

//configs
#define MEM_REFLECTION_ON 0	//not finish yet, do not turn on it.
#define MEM_RJSON_ON 1

//include after config
#include "memManager.h"
using namespace mem;

//trial functions
inline int testFoo(int a, int b) { return a + b; }
inline int testFoo2(int a, int b) { return a * b; }
inline int testFoo3(int a, int b) { return a - b; }
INITIALIZE_PFUNCTION(int(int, int), 75342, testFoo, testFoo2, testFoo3);


//forward declaration
class testUnit;
class testManager;
class testUnit2;
class testIngr;
class testStlContainer;

class testUnit :public memUnit {
	//declare variables with MACRO
public:
	DECLARE_VARS(
		memPtr<testUnit>, anotherone,
		double, num,
		std::string, str
		)
public:
	testUnit(memManager* manager) :memUnit(manager) {};
};
class testIngr :public Ingress {
public:
	DECLARE_VARS(
		memPtr<testIngr>, anotherone,
		double, num,
		std::string, str
	)
public:
	testIngr(memManager* manager) :Ingress(manager) {};
	const char* getConstTypeName() override { return "testIngr"; }
};
class testUnit2 :public testUnit {
	//declare without MACRO, manually
	__MEMMNGR_INTERNAL_HEADER_PERMISSION
	void save_fetch(memPara para) override {
		GWPP("egressTest", egressTest, para);
		GWPP("genetest", genetest, para);
		GWPP("funcTest", funcTest, para);
		GWPP("ptrback", ptrback, para);
		GWPP("curiousTest", curiousTest, para);
	}
public:
	testUnit2(memManager* manager) :testUnit(manager) {};
	pEgress<testIngr> egressTest;
	std::variant<memPtr<testUnit>, memPtr<testUnit2>, int, std::string> genetest;
	pVariant<testUnit, testUnit2> pvtest;		//aka std::variant<memPtr<>, memPtr<>>
	pFunction<int(int, int), 75342> funcTest;
	memPtr<testManager> ptrback;
	std::vector<memPtr<testStlContainer>> curiousTest[10];
}; 
#include <list>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <array>
#include <forward_list>
#include <bitset>
class testStlContainer : public memUnit {
	__MEMMNGR_INTERNAL_HEADER_PERMISSION
public:
	testStlContainer(memManager* manager) : memUnit(manager) {};

	void save_fetch(memPara para) override {
		GWPP("testList", testList, para);
		GWPP("testDeque", testDeque, para);
		GWPP("testForwardList", testForwardList, para);
		GWPP("testArray", testArray, para);
		GWPP("testSet", testSet, para);
		GWPP("testUnorderedSet", testUnorderedSet, para);
		GWPP("testMultiset", testMultiset, para);
		GWPP("testUnorderedMultiset", testUnorderedMultiset, para);
		GWPP("testMap", testMap, para);
		GWPP("testUnorderedMap", testUnorderedMap, para);
		GWPP("testMultimap", testMultimap, para);
		GWPP("testUnorderedMultimap", testUnorderedMultimap, para);
		//GWPP("testStack", testStack, para);						//unsupported yet
		//GWPP("testQueue", testQueue, para);
		//GWPP("testPriorityQueue", testPriorityQueue, para);
		//GWPP("testBitset", testBitset, para);
		//GWPP("testTuple", testTuple, para);
	}

	// List
	std::list<int> testList;

	// Deque
	std::deque<int> testDeque;

	// Set
	std::set<int> testSet;

	// Unordered Set
	std::unordered_set<int> testUnorderedSet;

	// Map
	std::map<int, std::string> testMap;

	// Unordered Map
	std::unordered_map<int, std::string> testUnorderedMap;

	// Stack (implemented using deque)
	std::stack<int> testStack;

	// Queue (implemented using deque)
	std::queue<int> testQueue;

	// Priority Queue (implemented using vector)
	std::priority_queue<int> testPriorityQueue;

	// Multiset
	std::multiset<int> testMultiset;

	// Multimap
	std::multimap<int, std::string> testMultimap;

	// Unordered Multiset
	std::unordered_multiset<int> testUnorderedMultiset;

	// Unordered Multimap
	std::unordered_multimap<int, std::string> testUnorderedMultimap;

	// Array
	std::array<int, 10> testArray;

	// Forward List
	std::forward_list<int> testForwardList;

	// Bitset
	std::bitset<10> testBitset;

	// Tuple
	std::tuple<int, std::string, double> testTuple;
};
class testManager :public memManager {
	__MEMMNGR_INTERNAL_HEADER_PERMISSION
public:
	memPtr<testUnit> anothert1;
	memPtr<testUnit> anothert2;
	memPtr<testUnit2> tu2;
	memPtr<testIngr> tin;
	vector_memPtr<testUnit> vec;
	uint32_t num;
	char n1;
	uint8_t n2;
	int16_t n3;
	uint16_t n4;
	int32_t n5;
	uint32_t n6;
	int64_t n7;
	uint64_t n8;
	float n9;
	double n10;
	std::atomic<double> atom;
	std::wstring wstr;
	enum { ea, eb, ec }n11;
	bool n12;
	char n13[30];
private:
	void save_fetch(memPara para) override {
		GWPP("anothert1", anothert1, para);
		GWPP("anothert2", anothert2, para);
		GWPP("tu2", tu2, para);
		GWPP("vec", vec, para);
		GWPP("tin", tin, para);
		GWPP("num", num, para);
		GWPP("n1", n1, para);
		GWPP("n2", n2, para);
		GWPP("n3", n3, para);
		GWPP("n4", n4, para);
		GWPP("n5", n5, para);
		GWPP("n6", n6, para);
		GWPP("n7", n7, para);
		GWPP("n8", n8, para);
		GWPP("n9", n9, para);
		GWPP("n10", n10, para);
		GWPP("atom", atom, para);
		GWPP("wstr", wstr, para);
		GWPP("n11", n11, para);
		GWPP("n12", n12, para);
		GWPP("n13", n13, para);
	}
};
inline void mem_testmain()
{
	char url[100] = "C:\\Users\\R\\Desktop\\newsave.guma";		//must be a valid path when trying this demo in other machines
	char url2[100] = "C:\\Users\\R\\Desktop\\oldsave.guma";		//on windows
        //on linux, system will create a file on local directory named "C:\Users\R\Desktop\newsave.guma"    :)

	//char* url = "newsave";									//in esp32
	//char* url2 = "oldsave";
	
	//test: memory leak (repeat)
	while (1) {

		memPtr<testManager> testManagerA = new testManager();
		testManager* testManagerADisplay = *testManagerA;
		memPtr<testUnit> testUnitA = new testUnit(testManagerADisplay);
		memPtr<testUnit> testUnitB = new testUnit(*testManagerA);
		testUnitA->anotherone = testUnitB;
		testUnitA->num = -1000;
		testUnitA->str = "t1";
		testUnitB->anotherone = testUnitB;
		testUnitB->num = 2000;
		testUnitB->str = "t2";
		testManagerA->anothert1 = testUnitA;
		memPtr<testUnit> copyTest = new testUnit(**testManagerA->anothert1);
		testManagerA->anothert2 = testUnitB;
		testManagerA->num = 4000;
		testManagerA->setUrl(url);

		testManagerA->atom.store(567.23);
		testManagerA->wstr = L"t4";
		testManagerA->vec.emplace_back(testUnitA);
		testManagerA->vec.emplace_back(*testUnitB);
		testManagerA->vec.emplace_back(*testUnitA);
		testManagerA->vec.emplace_back(new testUnit(*testManagerA));
		testManagerA->vec.emplace_back(new testUnit(*testManagerA));
		testManagerA->vec.at(2) = nullptr;
		memPtr<testUnit> t5 = new testUnit(**testUnitB);
		testManagerA->tu2 = new testUnit2(*testManagerA);
		testManagerA->tu2->ptrback = testManagerA;
		memPtr<testUnit2> testUnitC = testManagerA->tu2;

		//all stl container adaptive
		if (true)
		{
			testStlContainer* container = new testStlContainer(*testManagerA);
			//vector is definitely supported

			//List
			container->testList.push_back(50);
			container->testList.push_back(49);

			//Deque
			container->testDeque.push_back(48);
			container->testDeque.push_back(47);

			//ForwardList
			container->testForwardList.push_front(45);
			container->testForwardList.push_front(46);

			//Array
			container->testArray[0] = 44;
			container->testArray[1] = 43;

			//Set
			container->testSet.emplace(42);
			container->testSet.emplace(41);

			//UnorderedSet
			container->testUnorderedSet.emplace(40);
			container->testUnorderedSet.emplace(39);

			//Multiset
			container->testMultiset.emplace(38);
			container->testMultiset.emplace(37);

			//UnorderedMultiset
			container->testUnorderedMultiset.emplace(36);
			container->testUnorderedMultiset.emplace(35);

			//Map
			container->testMap.emplace(34, "34 test");
			container->testMap.emplace(33, "test 33");

			//UnorderedMap
			container->testUnorderedMap.emplace(32, "32 test uom");
			container->testUnorderedMap.emplace(31, "test 31 uomap");

			//Multimap
			container->testMultimap.emplace(30, "30 test mm");
			container->testMultimap.emplace(29, "29 test mm");

			//UnorderedMultimap
			container->testUnorderedMultimap.emplace(28, "28 test uomm");
			container->testUnorderedMultimap.emplace(27, "27 test uomm");

			testUnitC->curiousTest[2].push_back(container);
			testUnitC->curiousTest[2].push_back(container);
			testUnitC->curiousTest[5].push_back(container);
		}

		//variant test
		testManagerA->tu2->genetest = testUnitA;
		testUnit* geneGet1 = **std::get_if<memPtr<testUnit>>(&(testManagerA->tu2->genetest));
		testUnit2* geneGet3;
		mem::pVariantGet(testManagerA->tu2->genetest, geneGet3);
		//testManagerA->tu2->genetest = nullptr;
		//testManagerA->tu2->genetest = new testUnit2(*testManagerA);
		//speed test of variant
		//MSVC/C++ cannot use constexpr in variant templates. it runs o(n). theoretically o(1).
		/*for (int i = 0; i < 10000000; i++)
		{
			testManagerA->tu2->genetest.getPtr(geneGet3);
		}
		for (int i = 0; i < 10000000; i++)
		{
			testManagerA->tu2->genetest.getPtr(geneGet1);
		}*/

		//test of maximum or minimum value (values in the save file are inaccurate)
		testManagerA->n1 = INT8_MAX;
		testManagerA->n2 = UINT8_MAX;
		testManagerA->n3 = INT16_MAX;
		testManagerA->n4 = UINT16_MAX;
		testManagerA->n5 = INT32_MAX;
		testManagerA->n6 = UINT32_MAX;
		testManagerA->n7 = INT64_MAX;
		testManagerA->n8 = UINT64_MAX;
		testManagerA->n9 = 3.1415926535897932f;
		testManagerA->n10 = 3.1415926535897932;
		testManagerA->n11 = testManager::ec;
		memcpy(testManagerA->n13, "char[] test", 10);

		testManagerA->n1 = INT8_MIN;
		testManagerA->n3 = INT16_MIN;
		testManagerA->n5 = INT32_MIN;
		testManagerA->n7 = INT64_MIN;

		//recusive depth test(up to 2000 recursive for MSVC 1MB stack in -O2 optimize)
		memPtr<testUnit> rdt = testManagerA->anothert1;
		//for (int i = 0; i < 2000; i++)
		//{
		//	rdt->anotherone = new testUnit(*testManagerA);
		//	rdt = rdt->anotherone;
		//}

		//benchmark of rw
		//for (int i = 0; i < 500000; i++)
		//{
		//	testManagerA->vec.emplace_back(new testUnit(*testManagerA));
		//}
		//for (auto i : testManagerA->vec)
		//{
		//	if (i.isFilled())
		//	{
		//		i->num = 10000000000000;
		//		i->num *= 10000000000000;
		//		i->anotherone = new testUnit(*testManagerA);
		//		i->anotherone->anotherone = new testUnit(*testManagerA);
		//		i->str = "i5-9300HF costs 1458ms for 500k meta units serialize, 1202ms for deserialize.";
		//	}
		//}

		//pFunction test
		testUnitC->funcTest = testFoo2;
		testUnitC->funcTest = testFoo3;
		std::cout << "mem test inited\n";

		//std::cout << "\033[A";
		//std::cout << "\033[K";

		int ret = testManagerA->download();
		std::cout << "mem test downloaded\n";

		memPtr<testManager> testManagerB = new testManager();
		testManagerB->setUrl(url);
		testManagerB->upload();
		std::cout << "mem test uploaded\n";
		std::cout << "mem test testManagerB->n1 :" << (int)testManagerB->n1 << "\n";
		std::cout << "mem test testManagerB->n2 :" << (int)testManagerB->n2 << "\n";
		std::cout << "mem test testManagerB->n10 :" << testManagerB->n10 << "\n";

		int fooTestRet = testManagerB->tu2->funcTest(10, 5);

		testManagerB->setUrl(url2);

		dumbPtr<testIngr> ingrtest;
		dumbPtr<Ingress> ing = dumbPtr<Ingress>(ingrtest);
		ing = ingrtest;

		//ingress/egress test
		testManagerB->tin = new testIngr(*testManagerB);
		testManagerA->tu2->egressTest.makeEIPair(*testManagerA, testManagerB->tin, "ingressTest");

		//ingress/egress test of speed
		/*for (int i = 0; i < 100000; i++)
		{

			testManagerA->tu2->egressTest.makeEIPair(*testManagerA, testManagerB->anothert1, "ingressTest");
		}*/

		//egress vector
		vector_pEgress<testIngr> egressvector = vector_pEgress<testIngr>();
		egressvector.emplace_back(*testManagerA, testManagerB->tin, "ingressTest2");

		testManagerB->download();
		testManagerA->download();

		testManagerA.release();
		testManagerA.release();
		testManagerB.release();

		testManagerB = new testManager();			//swap the testManagers
		testManagerB->setUrl(url);
		testManagerB->upload();
		testManagerA = new testManager();
		testManagerA->setUrl(url2);
		testManagerA->upload();
		testIngr* ingressSuccess;
		//testManagerA->anothert1.release();
		std::cout << "mem test swaped\n";
		std::cout << "mem test testManagerA->n1 :" << (int)testManagerA->n1 << "\n";
		std::cout << "mem test testManagerA->n2 :" << (int)testManagerA->n2 << "\n";
		std::cout << "mem test testManagerA->n10 :" << testManagerA->n10 << "\n";

		//getTarget find target memManager in global file. must upload target memManager upload before get something
		auto retFindErr = testManagerB->tu2->egressTest.getTarget(ingressSuccess);



		// reflection test
		// read
#if MEM_REFLECTION_ON
		ReflectResult refRes;
		memUnit* refTestMu = *testManagerA;
		refTestMu->reflectionRead(&refRes);

		ReflectResult refRes2;
		memUnit* refTestMu2 = *testManagerA->tu2;
		refTestMu2->reflectionRead(&refRes2);

		//write
		uint64_t refNum = 123456;
		ReflectResultKeyValue refKW = ReflectResultKeyValue("num", (uint32_t&)refNum);
		refTestMu->reflectionWrite(refKW);
		refNum = 654321;
		refKW = ReflectResultKeyValue("n8", refNum);
		refTestMu->reflectionWrite(refKW);
		wchar_t refWC[] = "wchar test";
		refTestMu->reflectionWrite(ReflectResultKeyValue("wc", refWC));
		refTestMu->reflectionWrite(ReflectResultKeyValue("wc", "wchar test directly"));
		refKW = ReflectResultKeyValue("anothert1", testManagerA->anothert2);	//legal pointer write test
		bool refSuccess = refTestMu->reflectionWrite(refKW);
		refKW = ReflectResultKeyValue("anothert1", testManagerA->vec);			//illegal pointer write test
		refSuccess = refTestMu->reflectionWrite(refKW);

		memUnit* refTestVariant = *testManagerA->anothert1;
		refKW = ReflectResultKeyValue("genetest", refTestVariant);
		refTestMu2->reflectionWrite(refKW);
		std::cout << "mem test reflection writen\n";
		std::cout << "mem test testManagerA->num" << testManagerA->num << "\n";
		std::cout << "mem test testManagerA->n8" << testManagerA->n8 << "\n";
		std::cout << "mem test testManagerA->wc" << testManagerA->wc.getWcharPtr() << "\n";
#endif


		// rapid Json test
#if MEM_RJSON_ON
		//rapid Json serialize
		std::string serializeDumpJson;
		testManagerA->tu2->genetest = std::string("variant string");
		testManagerA->serializeJson(&serializeDumpJson);		//rapid json: i5-9300HF costs 1315ms for 500k meta units serialize, 1266ms for deserialize. No wonder.

		//rapid Json deserialize
		memPtr<testManager> testManagerD = new testManager();
		testManagerD->deserializeJson(&serializeDumpJson[0], serializeDumpJson.size());
		std::cout << "mem test json work complete\n";



		//serialize test(memUnit)
		std::vector<uint8_t> serializeDump;
		testManagerA->anothert1->num = 2.72;
		testManagerA->anothert1->serialize(&serializeDump);

		//deserialize test(memUnit)
		testManagerB.release();
		testManagerB = new testManager();
		testManagerB->anothert1 = new testUnit(*testManagerB);
		testManagerB->anothert1->deserialize(&serializeDump[0], serializeDump.size());
		serializeDump.clear();
		testManagerB.release();

		//serialize test(memManager)
		if (true)
		{
			testManagerA->serialize(&serializeDump);
			testManagerA.release();

			//dserialize test(memManager)
			testManagerB = new testManager();
			testManagerB->deserialize(&serializeDump[0], serializeDump.size());
		}
#endif
		//memManager is recommended to be released manually bcs some pointers within memManager could point to the manager, which will cause recurring references and shared pointer count errors.
		testManagerA.release();
		testManagerB.release();
		//test: memory leak (repeat)
	}
	return;
}