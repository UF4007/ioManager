//up to 50 vars to declare
#pragma once
#ifndef DECLARE_VARS
#define DECLARE_VARS_GLUE(x, y) x##y
#define DECLARE_VARS_HELPER2(N) DECLARE_VARS_GLUE(DECLARE_VAR,N)    //same depth
#define DECLARE_VARS_HELPER3(N) DECLARE_VARS_HELPER2(N)
#define DECLARE_VARS_HELPER(N) DECLARE_VARS_HELPER3(N)
#define DECLARE_VARS_ARGS_IMPL_COUNT(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10, \
                                      _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
                                      _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
                                      _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
                                      _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
                                      _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
                                      _61,_62,_63,_64,_65,_66,_67,_68,_69,_70, \
                                      _71,_72,_73,_74,_75,_76,_77,_78,_79,_80, \
                                      _81,_82,_83,_84,_85,_86,_87,_88,_89,_90, \
                                      _91,_92,_93,_94,_95,_96,_97,_98,_99,_100, N, ...) N
#define DECLARE_VARS_ARGS_IMPL(args) DECLARE_VARS_ARGS_IMPL_COUNT args
#define COUNT_MACRO_VAR_ARGS(...) DECLARE_VARS_ARGS_IMPL((__VA_ARGS__, \
                                                          100,99,98,97,96,95,94,93,92,91, \
                                                          90,89,88,87,86,85,84,83,82,81, \
                                                          80,79,78,77,76,75,74,73,72,71, \
                                                          70,69,68,67,66,65,64,63,62,61, \
                                                          60,59,58,57,56,55,54,53,52,51, \
                                                          50,49,48,47,46,45,44,43,42,41, \
                                                          40,39,38,37,36,35,34,33,32,31, \
                                                          30,29,28,27,26,25,24,23,22,21, \
                                                          20,19,18,17,16,15,14,13,12,11, \
                                                          10,9,8,7,6,5,4,3,2,1,0))

#define DECLARE_VAR2(type, name) type name; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
    }

#define DECLARE_VAR4(type, name, type2, name2) type name; \
    type2 name2; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
    }

#define DECLARE_VAR6(type, name, type2, name2, type3, name3) type name; \
    type2 name2; \
    type3 name3; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
    }

#define DECLARE_VAR8(type, name, type2, name2, type3, name3, type4, name4) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
    }

#define DECLARE_VAR10(type, name, type2, name2, type3, name3, type4, name4, type5, name5) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
    }

#define DECLARE_VAR12(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
    }

#define DECLARE_VAR14(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
    }

#define DECLARE_VAR16(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
    }

#define DECLARE_VAR18(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
    }

#define DECLARE_VAR20(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
    }

#define DECLARE_VAR22(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
    }

#define DECLARE_VAR24(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
    }

#define DECLARE_VAR26(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
    }

#define DECLARE_VAR28(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
    }

#define DECLARE_VAR30(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
    }

#define DECLARE_VAR32(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
    }
#define DECLARE_VAR34(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
    }
#define DECLARE_VAR36(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
    }
#define DECLARE_VAR38(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
    }
#define DECLARE_VAR40(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
    }
#define DECLARE_VAR42(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
    }
#define DECLARE_VAR44(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22) type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
    }
    #define DECLARE_VAR46(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
    }
    #define DECLARE_VAR48(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
    }
    #define DECLARE_VAR50(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    ) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
    }
    #define DECLARE_VAR52(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
    }
    #define DECLARE_VAR54(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
    }
    #define DECLARE_VAR56(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
    }
    #define DECLARE_VAR58(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
    }
    #define DECLARE_VAR60(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
    }
    #define DECLARE_VAR62(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
    }
    #define DECLARE_VAR64(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
    }
    #define DECLARE_VAR66(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
    }
    #define DECLARE_VAR68(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
    }
    #define DECLARE_VAR70(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
    }
    #define DECLARE_VAR72(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
    }
    #define DECLARE_VAR74(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    ) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
    }
    #define DECLARE_VAR76(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    , type38, name38) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    type38 name38; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
        GWPP(#name38, name38, para); \
    }
    #define DECLARE_VAR78(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    , type38, name38, type39, name39) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    type38 name38; \
    type39 name39; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
        GWPP(#name38, name38, para); \
        GWPP(#name39, name39, para); \
    }
    #define DECLARE_VAR80(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    , type38, name38, type39, name39, type40, name40) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    type38 name38; \
    type39 name39; \
    type40 name40; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
        GWPP(#name38, name38, para); \
        GWPP(#name39, name39, para); \
        GWPP(#name40, name40, para); \
    }
    #define DECLARE_VAR82(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    , type38, name38, type39, name39, type40, name40, type41, name41) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    type38 name38; \
    type39 name39; \
    type40 name40; \
    type41 name41; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
        GWPP(#name38, name38, para); \
        GWPP(#name39, name39, para); \
        GWPP(#name40, name40, para); \
        GWPP(#name41, name41, para); \
    }
    #define DECLARE_VAR84(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    , type38, name38, type39, name39, type40, name40, type41, name41, type42, name42) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    type38 name38; \
    type39 name39; \
    type40 name40; \
    type41 name41; \
    type42 name42; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
        GWPP(#name38, name38, para); \
        GWPP(#name39, name39, para); \
        GWPP(#name40, name40, para); \
        GWPP(#name41, name41, para); \
        GWPP(#name42, name42, para); \
    }
    #define DECLARE_VAR86(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    , type38, name38, type39, name39, type40, name40, type41, name41, type42, name42, type43, name43) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    type38 name38; \
    type39 name39; \
    type40 name40; \
    type41 name41; \
    type42 name42; \
    type43 name43; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
        GWPP(#name38, name38, para); \
        GWPP(#name39, name39, para); \
        GWPP(#name40, name40, para); \
        GWPP(#name41, name41, para); \
        GWPP(#name42, name42, para); \
        GWPP(#name43, name43, para); \
    }
    #define DECLARE_VAR88(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    , type38, name38, type39, name39, type40, name40, type41, name41, type42, name42, type43, name43, type44, name44) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    type38 name38; \
    type39 name39; \
    type40 name40; \
    type41 name41; \
    type42 name42; \
    type43 name43; \
    type44 name44; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
        GWPP(#name38, name38, para); \
        GWPP(#name39, name39, para); \
        GWPP(#name40, name40, para); \
        GWPP(#name41, name41, para); \
        GWPP(#name42, name42, para); \
        GWPP(#name43, name43, para); \
        GWPP(#name44, name44, para); \
    }
    #define DECLARE_VAR90(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    , type38, name38, type39, name39, type40, name40, type41, name41, type42, name42, type43, name43, type44, name44, type45, name45) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    type38 name38; \
    type39 name39; \
    type40 name40; \
    type41 name41; \
    type42 name42; \
    type43 name43; \
    type44 name44; \
    type45 name45; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
        GWPP(#name38, name38, para); \
        GWPP(#name39, name39, para); \
        GWPP(#name40, name40, para); \
        GWPP(#name41, name41, para); \
        GWPP(#name42, name42, para); \
        GWPP(#name43, name43, para); \
        GWPP(#name44, name44, para); \
        GWPP(#name45, name45, para); \
    }
    #define DECLARE_VAR92(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    , type38, name38, type39, name39, type40, name40, type41, name41, type42, name42, type43, name43, type44, name44, type45, name45, type46, name46) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    type38 name38; \
    type39 name39; \
    type40 name40; \
    type41 name41; \
    type42 name42; \
    type43 name43; \
    type44 name44; \
    type45 name45; \
    type46 name46; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
        GWPP(#name38, name38, para); \
        GWPP(#name39, name39, para); \
        GWPP(#name40, name40, para); \
        GWPP(#name41, name41, para); \
        GWPP(#name42, name42, para); \
        GWPP(#name43, name43, para); \
        GWPP(#name44, name44, para); \
        GWPP(#name45, name45, para); \
        GWPP(#name46, name46, para); \
    }
    #define DECLARE_VAR94(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    , type38, name38, type39, name39, type40, name40, type41, name41, type42, name42, type43, name43, type44, name44, type45, name45, type46, name46, type47, name47) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    type38 name38; \
    type39 name39; \
    type40 name40; \
    type41 name41; \
    type42 name42; \
    type43 name43; \
    type44 name44; \
    type45 name45; \
    type46 name46; \
    type47 name47; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
        GWPP(#name38, name38, para); \
        GWPP(#name39, name39, para); \
        GWPP(#name40, name40, para); \
        GWPP(#name41, name41, para); \
        GWPP(#name42, name42, para); \
        GWPP(#name43, name43, para); \
        GWPP(#name44, name44, para); \
        GWPP(#name45, name45, para); \
        GWPP(#name46, name46, para); \
        GWPP(#name47, name47, para); \
    }
    #define DECLARE_VAR96(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    , type38, name38, type39, name39, type40, name40, type41, name41, type42, name42, type43, name43, type44, name44, type45, name45, type46, name46, type47, name47, type48, name48) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    type38 name38; \
    type39 name39; \
    type40 name40; \
    type41 name41; \
    type42 name42; \
    type43 name43; \
    type44 name44; \
    type45 name45; \
    type46 name46; \
    type47 name47; \
    type48 name48; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
        GWPP(#name38, name38, para); \
        GWPP(#name39, name39, para); \
        GWPP(#name40, name40, para); \
        GWPP(#name41, name41, para); \
        GWPP(#name42, name42, para); \
        GWPP(#name43, name43, para); \
        GWPP(#name44, name44, para); \
        GWPP(#name45, name45, para); \
        GWPP(#name46, name46, para); \
        GWPP(#name47, name47, para); \
        GWPP(#name48, name48, para); \
    }
    #define DECLARE_VAR98(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    , type38, name38, type39, name39, type40, name40, type41, name41, type42, name42, type43, name43, type44, name44, type45, name45, type46, name46, type47, name47, type48, name48, type49, name49) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    type38 name38; \
    type39 name39; \
    type40 name40; \
    type41 name41; \
    type42 name42; \
    type43 name43; \
    type44 name44; \
    type45 name45; \
    type46 name46; \
    type47 name47; \
    type48 name48; \
    type49 name49; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
        GWPP(#name38, name38, para); \
        GWPP(#name39, name39, para); \
        GWPP(#name40, name40, para); \
        GWPP(#name41, name41, para); \
        GWPP(#name42, name42, para); \
        GWPP(#name43, name43, para); \
        GWPP(#name44, name44, para); \
        GWPP(#name45, name45, para); \
        GWPP(#name46, name46, para); \
        GWPP(#name47, name47, para); \
        GWPP(#name48, name48, para); \
        GWPP(#name49, name49, para); \
    }
    #define DECLARE_VAR100(type, name, type2, name2, type3, name3, type4, name4, type5, name5, type6, name6, type7, name7, type8, name8, type9, name9, type10, name10, type11, name11, type12, name12, type13, name13 \
    , type14, name14, type15, name15, type16, name16, type17, name17, type18, name18, type19, name19, type20, name20, type21, name21, type22, name22, type23, name23, type24, name24, type25, name25 \
    , type26, name26, type27, name27, type28, name28, type29, name29, type30, name30, type31, name31, type32, name32, type33, name33, type34, name34, type35, name35, type36, name36, type37, name37 \
    , type38, name38, type39, name39, type40, name40, type41, name41, type42, name42, type43, name43, type44, name44, type45, name45, type46, name46, type47, name47, type48, name48, type49, name49, type50, name50) \
    type name; \
    type2 name2; \
    type3 name3; \
    type4 name4; \
    type5 name5; \
    type6 name6; \
    type7 name7; \
    type8 name8; \
    type9 name9; \
    type10 name10; \
    type11 name11; \
    type12 name12; \
    type13 name13; \
    type14 name14; \
    type15 name15; \
    type16 name16; \
    type17 name17; \
    type18 name18; \
    type19 name19; \
    type20 name20; \
    type21 name21; \
    type22 name22; \
    type23 name23; \
    type24 name24; \
    type25 name25; \
    type26 name26; \
    type27 name27; \
    type28 name28; \
    type29 name29; \
    type30 name30; \
    type31 name31; \
    type32 name32; \
    type33 name33; \
    type34 name34; \
    type35 name35; \
    type36 name36; \
    type37 name37; \
    type38 name38; \
    type39 name39; \
    type40 name40; \
    type41 name41; \
    type42 name42; \
    type43 name43; \
    type44 name44; \
    type45 name45; \
    type46 name46; \
    type47 name47; \
    type48 name48; \
    type49 name49; \
    type50 name50; \
    __MEMMNGR_INTERNAL_HEADER_PERMISSION \
    private:private:void save_fetch(mem::memPara para) override { \
        GWPP(#name, name, para); \
        GWPP(#name2, name2, para); \
        GWPP(#name3, name3, para); \
        GWPP(#name4, name4, para); \
        GWPP(#name5, name5, para); \
        GWPP(#name6, name6, para); \
        GWPP(#name7, name7, para); \
        GWPP(#name8, name8, para); \
        GWPP(#name9, name9, para); \
        GWPP(#name10, name10, para); \
        GWPP(#name11, name11, para); \
        GWPP(#name12, name12, para); \
        GWPP(#name13, name13, para); \
        GWPP(#name14, name14, para); \
        GWPP(#name15, name15, para); \
        GWPP(#name16, name16, para); \
        GWPP(#name17, name17, para); \
        GWPP(#name18, name18, para); \
        GWPP(#name19, name19, para); \
        GWPP(#name20, name20, para); \
        GWPP(#name21, name21, para); \
        GWPP(#name22, name22, para); \
        GWPP(#name23, name23, para); \
        GWPP(#name24, name24, para); \
        GWPP(#name25, name25, para); \
        GWPP(#name26, name26, para); \
        GWPP(#name27, name27, para); \
        GWPP(#name28, name28, para); \
        GWPP(#name29, name29, para); \
        GWPP(#name30, name30, para); \
        GWPP(#name31, name31, para); \
        GWPP(#name32, name32, para); \
        GWPP(#name33, name33, para); \
        GWPP(#name34, name34, para); \
        GWPP(#name35, name35, para); \
        GWPP(#name36, name36, para); \
        GWPP(#name37, name37, para); \
        GWPP(#name38, name38, para); \
        GWPP(#name39, name39, para); \
        GWPP(#name40, name40, para); \
        GWPP(#name41, name41, para); \
        GWPP(#name42, name42, para); \
        GWPP(#name43, name43, para); \
        GWPP(#name44, name44, para); \
        GWPP(#name45, name45, para); \
        GWPP(#name46, name46, para); \
        GWPP(#name47, name47, para); \
        GWPP(#name48, name48, para); \
        GWPP(#name49, name49, para); \
        GWPP(#name50, name50, para); \
    }

    //what is the problem with msvc and gcc, clang?

#ifdef _MSC_VER
    #define DECLARE_VARS2(...) DECLARE_VARS_HELPER(COUNT_MACRO_VAR_ARGS(__VA_ARGS__))
    #define DECLARE_VARS(...) DECLARE_VARS_GLUE(DECLARE_VARS2(__VA_ARGS__),(__VA_ARGS__))
#elif defined(__GNUC__)
    #define DECLARE_VARS(...) DECLARE_VARS_HELPER(COUNT_MACRO_VAR_ARGS(__VA_ARGS__))(__VA_ARGS__)
#else
    #define DECLARE_VARS(...) DECLARE_VARS_HELPER(COUNT_MACRO_VAR_ARGS(__VA_ARGS__))(__VA_ARGS__)
#endif
#endif // !DECLARE_VARS