#define WISE_ENUM_IMPL_ARG_N(                                                           \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,                                            \
    _11, _12, _13, _14, _15, _16, _17, _18, _19, _20,                                   \
    _21, _22, _23, _24, _25, _26, _27, _28, _29, _30,                                   \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40,                                   \
    _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,                                   \
    _51, _52, _53, _54, _55, _56, _57, _58, _59, _60,                                   \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70,                                   \
    _71, _72, _73, _74, _75, _76, _77, _78, _79, _80,                                   \
    _81, _82, _83, _84, _85, _86, _87, _88, _89, _90,                                   \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100,                                  \
    _101, _102, _103, _104, _105, _106, _107, _108, _109, _110,                         \
    _111, _112, _113, _114, _115, _116, _117, _118, _119, _120,                         \
    _121, _122, _123, _124, _125, N, ...                                                \
  )                                                                                     \
  N                                                                                     \

#define WISE_ENUM_IMPL_RSEQ_N()                                                         \
  125, 124, 123, 122, 121, 120, 119, 118, 117, 116,                                     \
  115, 114, 113, 112, 111, 110, 109, 108, 107, 106,                                     \
  105, 104, 103, 102, 101, 100, 99, 98, 97, 96,                                         \
  95, 94, 93, 92, 91, 90, 89, 88, 87, 86,                                               \
  85, 84, 83, 82, 81, 80, 79, 78, 77, 76,                                               \
  75, 74, 73, 72, 71, 70, 69, 68, 67, 66,                                               \
  65, 64, 63, 62, 61, 60, 59, 58, 57, 56,                                               \
  55, 54, 53, 52, 51, 50, 49, 48, 47, 46,                                               \
  45, 44, 43, 42, 41, 40, 39, 38, 37, 36,                                               \
  35, 34, 33, 32, 31, 30, 29, 28, 27, 26,                                               \
  25, 24, 23, 22, 21, 20, 19, 18, 17, 16,                                               \
  15, 14, 13, 12, 11, 10, 9, 8, 7, 6,                                                   \
  5, 4, 3, 2, 1, 0                                                                      \

#define WISE_ENUM_IMPL_LOOP_1(M, C, D, x) M(C, x)

#define WISE_ENUM_IMPL_LOOP_2(M, C, D, x, ...) M(C, x) D()                              \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_1(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_3(M, C, D, x, ...) M(C, x) D()                              \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_2(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_4(M, C, D, x, ...) M(C, x) D()                              \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_3(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_5(M, C, D, x, ...) M(C, x) D()                              \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_4(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_6(M, C, D, x, ...) M(C, x) D()                              \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_5(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_7(M, C, D, x, ...) M(C, x) D()                              \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_6(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_8(M, C, D, x, ...) M(C, x) D()                              \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_7(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_9(M, C, D, x, ...) M(C, x) D()                              \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_8(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_10(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_9(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_11(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_10(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_12(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_11(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_13(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_12(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_14(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_13(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_15(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_14(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_16(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_15(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_17(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_16(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_18(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_17(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_19(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_18(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_20(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_19(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_21(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_20(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_22(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_21(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_23(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_22(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_24(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_23(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_25(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_24(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_26(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_25(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_27(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_26(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_28(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_27(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_29(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_28(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_30(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_29(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_31(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_30(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_32(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_31(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_33(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_32(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_34(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_33(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_35(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_34(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_36(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_35(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_37(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_36(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_38(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_37(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_39(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_38(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_40(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_39(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_41(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_40(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_42(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_41(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_43(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_42(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_44(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_43(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_45(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_44(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_46(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_45(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_47(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_46(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_48(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_47(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_49(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_48(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_50(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_49(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_51(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_50(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_52(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_51(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_53(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_52(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_54(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_53(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_55(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_54(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_56(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_55(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_57(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_56(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_58(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_57(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_59(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_58(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_60(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_59(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_61(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_60(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_62(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_61(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_63(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_62(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_64(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_63(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_65(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_64(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_66(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_65(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_67(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_66(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_68(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_67(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_69(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_68(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_70(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_69(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_71(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_70(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_72(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_71(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_73(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_72(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_74(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_73(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_75(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_74(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_76(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_75(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_77(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_76(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_78(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_77(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_79(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_78(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_80(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_79(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_81(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_80(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_82(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_81(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_83(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_82(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_84(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_83(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_85(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_84(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_86(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_85(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_87(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_86(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_88(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_87(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_89(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_88(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_90(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_89(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_91(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_90(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_92(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_91(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_93(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_92(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_94(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_93(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_95(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_94(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_96(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_95(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_97(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_96(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_98(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_97(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_99(M, C, D, x, ...) M(C, x) D()                             \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_98(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_100(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_99(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_101(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_100(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_102(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_101(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_103(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_102(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_104(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_103(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_105(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_104(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_106(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_105(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_107(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_106(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_108(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_107(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_109(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_108(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_110(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_109(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_111(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_110(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_112(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_111(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_113(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_112(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_114(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_113(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_115(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_114(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_116(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_115(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_117(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_116(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_118(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_117(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_119(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_118(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_120(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_119(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_121(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_120(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_122(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_121(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_123(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_122(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_124(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_123(M, C, D, __VA_ARGS__))

#define WISE_ENUM_IMPL_LOOP_125(M, C, D, x, ...) M(C, x) D()                            \
  WISE_ENUM_IMPL_EXPAND(WISE_ENUM_IMPL_LOOP_124(M, C, D, __VA_ARGS__))

