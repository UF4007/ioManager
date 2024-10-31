#include <stdio.h>
#include <stdlib.h>
#include <cwchar>
#include <iostream>
#include <vector>
#include <set>
#include <deque>
#include <stack>
#include <queue>
#include <atomic>
#include <string>
#include <variant>
#include <cassert>
#include <sstream>
#include <mutex>
#include <locale>
//#include <codecvt>	//deprecated.
#include <algorithm>
#include <type_traits>
#include "memUnitMacro.h"



#if MEM_RJSON_ON
#include "../rapidjson/rapidjson.h"
#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"
#include "../rapidjson/istreamwrapper.h"
#include "base64.h"
#include "../utf8/utf8.h"
#endif