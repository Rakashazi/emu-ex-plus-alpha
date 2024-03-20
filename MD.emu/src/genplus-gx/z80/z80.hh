#pragma once

#include <imagine/util/used.hh>
#include <cstdint>
#include <array>

enum
{
  /* line states */
  CLEAR_LINE = 0, /* clear (a fired, held or pulsed) line */
  ASSERT_LINE,    /* assert an interrupt immediately */
  HOLD_LINE,      /* hold interrupt line until acknowledged */
  PULSE_LINE     /* pulse interrupt line for one instruction */
};

enum {
  Z80_PC, Z80_SP,
  Z80_A, Z80_B, Z80_C, Z80_D, Z80_E, Z80_H, Z80_L,
  Z80_AF, Z80_BC, Z80_DE, Z80_HL,
  Z80_IX, Z80_IY,  Z80_AF2, Z80_BC2, Z80_DE2, Z80_HL2,
  Z80_R, Z80_I, Z80_IM, Z80_IFF1, Z80_IFF2, Z80_HALT,
  Z80_DC0, Z80_DC1, Z80_DC2, Z80_DC3, Z80_WZ
};

enum {
  Z80_TABLE_op,
  Z80_TABLE_cb,
  Z80_TABLE_ed,
  Z80_TABLE_xy,
  Z80_TABLE_xycb,
  Z80_TABLE_ex  /* cycles counts for taken jr/jp/call and interrupt latency (rst opcodes) */
};

/******************************************************************************
 * Union of uint8_t, UINT16 and UINT32 in native endianess of the target
 * This is used to access bytes and words in a machine independent manner.
 * The upper bytes h2 and h3 normally contain zero (16 bit CPU cores)
 * thus PAIR.d can be used to pass arguments to the memory system
 * which expects 'int' really.
 ******************************************************************************/

union PAIR
{
#ifdef LSB_FIRST
  struct { uint8_t l,h,h2,h3; } b;
  struct { uint16_t l,h; } w;
#else
  struct { uint8_t h3,h2,h,l; } b;
  struct { uint16_t h,l; } w;
#endif
  uint32_t d{};
};

/****************************************************************************/
/* The Z80 registers. HALT is set to 1 when the CPU is halted, the refresh  */
/* register is calculated as follows: refresh=(Z80.r&127)|(Z80.r2&128)      */
/****************************************************************************/
struct Z80_Regs
{
	PAIR pc{}, sp{}, af{}, bc{}, de{}, hl{}, ix{}, iy{}, wz{};
	PAIR af2{}, bc2{}, de2{}, hl2{};
	uint8_t r{}, r2{}, iff1{}, iff2{}, halt{}, im{}, i{};
	uint8_t nmi_state{};      /* nmi line state */
	uint8_t nmi_pending{};    /* nmi pending */
	uint8_t irq_state{};      /* irq line state */
	uint8_t after_ei{};      /* are we in the EI shadow? */
};

using IrqCallback = int(*)();
using WriteMem = void (*)(unsigned addr, uint8_t data);
using ReadMem = uint8_t (*)(unsigned addr);
using WritePort = void (*)(unsigned port, uint8_t data);
using ReadPort = uint8_t (*)(unsigned port);

struct Z80Desc
{
	IrqCallback onIrq;
	bool useStaticConfig{};
	std::array<uint8_t*, 64> staticReadMap{};
	std::array<uint8_t*, 64> staticWriteMap{};
	WriteMem staticWriteMem{};
	ReadMem staticReadMem{};
	WritePort staticWritePort{};
	ReadPort staticReadPort{};
};

template<Z80Desc desc>
class Z80CPU : public Z80_Regs
{
public:
	int cycleCount{};
	ConditionalMember<!desc.useStaticConfig, WriteMem> onWriteMem{};
	ConditionalMember<!desc.useStaticConfig, ReadMem> onReadMem{};
	ConditionalMember<!desc.useStaticConfig, WritePort> onWritePort{};
	ConditionalMember<!desc.useStaticConfig, ReadPort> onReadPort{};

	void init();
	void reset();
	void run(int cycles) __attribute__((hot));
	void setNmiLine(unsigned state);
	void setIRQ(unsigned state) { irq_state = state; }
	int irqCallback(unsigned) const { return desc.onIrq(); }
	void takeInterrupt();

	auto &readMap() requires(desc.useStaticConfig) { return desc.staticReadMap; }
	auto &readMap() requires(!desc.useStaticConfig) { return readmap; }
	auto &writeMap() requires(desc.useStaticConfig) { return desc.staticWriteMap; }
	auto &writeMap() requires(!desc.useStaticConfig) { return writemap; }
	auto readMem(unsigned addr) requires(desc.useStaticConfig) { return desc.staticReadMem(addr); }
	auto readMem(unsigned addr) requires(!desc.useStaticConfig) { return onReadMem(addr); }
	void writeMem(unsigned addr, uint8_t data) requires(desc.useStaticConfig) { desc.staticWriteMem(addr, data); }
	void writeMem(unsigned addr, uint8_t data) requires(!desc.useStaticConfig) { onWriteMem(addr, data); }
	auto readPort(unsigned port) requires(desc.useStaticConfig) { return desc.staticReadPort(port); }
	auto readPort(unsigned port) requires(!desc.useStaticConfig) { return onReadPort(port); }
	void writePort(unsigned port, uint8_t data) requires(desc.useStaticConfig) { desc.staticWritePort(port, data); }
	void writePort(unsigned port, uint8_t data) requires(!desc.useStaticConfig) { onWritePort(port, data); }

private:
	ConditionalMember<!desc.useStaticConfig, std::array<uint8_t*, 64>> readmap{};
	ConditionalMember<!desc.useStaticConfig, std::array<uint8_t*, 64>> writemap{};

	void RM16(unsigned addr, PAIR *r);
	void WM16(unsigned addr, PAIR *r);
	uint8_t cpu_readop(unsigned addr);
	uint8_t cpu_readop_arg(unsigned addr);
	uint8_t ROP();
	uint8_t ARG();
	uint32_t ARG16();
	uint8_t INC(uint8_t value);
	uint8_t DEC(uint8_t value);
	uint8_t RLC(uint8_t value);
	uint8_t RRC(uint8_t value);
	uint8_t RL(uint8_t value);
	uint8_t RR(uint8_t value);
	uint8_t SLA(uint8_t value);
	uint8_t SRA(uint8_t value);
	uint8_t SLL(uint8_t value);
	uint8_t SRL(uint8_t value);
	uint8_t RES(uint8_t bit, uint8_t value);
	uint8_t SET(uint8_t bit, uint8_t value);
	void illegal_1();
	void illegal_2();

#define PROTOTYPES(prefix) \
	void prefix##_00(); void prefix##_01(); void prefix##_02(); void prefix##_03(); \
	void prefix##_04(); void prefix##_05(); void prefix##_06(); void prefix##_07(); \
	void prefix##_08(); void prefix##_09(); void prefix##_0a(); void prefix##_0b(); \
	void prefix##_0c(); void prefix##_0d(); void prefix##_0e(); void prefix##_0f(); \
	void prefix##_10(); void prefix##_11(); void prefix##_12(); void prefix##_13(); \
	void prefix##_14(); void prefix##_15(); void prefix##_16(); void prefix##_17(); \
	void prefix##_18(); void prefix##_19(); void prefix##_1a(); void prefix##_1b(); \
	void prefix##_1c(); void prefix##_1d(); void prefix##_1e(); void prefix##_1f(); \
	void prefix##_20(); void prefix##_21(); void prefix##_22(); void prefix##_23(); \
	void prefix##_24(); void prefix##_25(); void prefix##_26(); void prefix##_27(); \
	void prefix##_28(); void prefix##_29(); void prefix##_2a(); void prefix##_2b(); \
	void prefix##_2c(); void prefix##_2d(); void prefix##_2e(); void prefix##_2f(); \
	void prefix##_30(); void prefix##_31(); void prefix##_32(); void prefix##_33(); \
	void prefix##_34(); void prefix##_35(); void prefix##_36(); void prefix##_37(); \
	void prefix##_38(); void prefix##_39(); void prefix##_3a(); void prefix##_3b(); \
	void prefix##_3c(); void prefix##_3d(); void prefix##_3e(); void prefix##_3f(); \
	void prefix##_40(); void prefix##_41(); void prefix##_42(); void prefix##_43(); \
	void prefix##_44(); void prefix##_45(); void prefix##_46(); void prefix##_47(); \
	void prefix##_48(); void prefix##_49(); void prefix##_4a(); void prefix##_4b(); \
	void prefix##_4c(); void prefix##_4d(); void prefix##_4e(); void prefix##_4f(); \
	void prefix##_50(); void prefix##_51(); void prefix##_52(); void prefix##_53(); \
	void prefix##_54(); void prefix##_55(); void prefix##_56(); void prefix##_57(); \
	void prefix##_58(); void prefix##_59(); void prefix##_5a(); void prefix##_5b(); \
	void prefix##_5c(); void prefix##_5d(); void prefix##_5e(); void prefix##_5f(); \
	void prefix##_60(); void prefix##_61(); void prefix##_62(); void prefix##_63(); \
	void prefix##_64(); void prefix##_65(); void prefix##_66(); void prefix##_67(); \
	void prefix##_68(); void prefix##_69(); void prefix##_6a(); void prefix##_6b(); \
	void prefix##_6c(); void prefix##_6d(); void prefix##_6e(); void prefix##_6f(); \
	void prefix##_70(); void prefix##_71(); void prefix##_72(); void prefix##_73(); \
	void prefix##_74(); void prefix##_75(); void prefix##_76(); void prefix##_77(); \
	void prefix##_78(); void prefix##_79(); void prefix##_7a(); void prefix##_7b(); \
	void prefix##_7c(); void prefix##_7d(); void prefix##_7e(); void prefix##_7f(); \
	void prefix##_80(); void prefix##_81(); void prefix##_82(); void prefix##_83(); \
	void prefix##_84(); void prefix##_85(); void prefix##_86(); void prefix##_87(); \
	void prefix##_88(); void prefix##_89(); void prefix##_8a(); void prefix##_8b(); \
	void prefix##_8c(); void prefix##_8d(); void prefix##_8e(); void prefix##_8f(); \
	void prefix##_90(); void prefix##_91(); void prefix##_92(); void prefix##_93(); \
	void prefix##_94(); void prefix##_95(); void prefix##_96(); void prefix##_97(); \
	void prefix##_98(); void prefix##_99(); void prefix##_9a(); void prefix##_9b(); \
	void prefix##_9c(); void prefix##_9d(); void prefix##_9e(); void prefix##_9f(); \
	void prefix##_a0(); void prefix##_a1(); void prefix##_a2(); void prefix##_a3(); \
	void prefix##_a4(); void prefix##_a5(); void prefix##_a6(); void prefix##_a7(); \
	void prefix##_a8(); void prefix##_a9(); void prefix##_aa(); void prefix##_ab(); \
	void prefix##_ac(); void prefix##_ad(); void prefix##_ae(); void prefix##_af(); \
	void prefix##_b0(); void prefix##_b1(); void prefix##_b2(); void prefix##_b3(); \
	void prefix##_b4(); void prefix##_b5(); void prefix##_b6(); void prefix##_b7(); \
	void prefix##_b8(); void prefix##_b9(); void prefix##_ba(); void prefix##_bb(); \
	void prefix##_bc(); void prefix##_bd(); void prefix##_be(); void prefix##_bf(); \
	void prefix##_c0(); void prefix##_c1(); void prefix##_c2(); void prefix##_c3(); \
	void prefix##_c4(); void prefix##_c5(); void prefix##_c6(); void prefix##_c7(); \
	void prefix##_c8(); void prefix##_c9(); void prefix##_ca(); void prefix##_cb(); \
	void prefix##_cc(); void prefix##_cd(); void prefix##_ce(); void prefix##_cf(); \
	void prefix##_d0(); void prefix##_d1(); void prefix##_d2(); void prefix##_d3(); \
	void prefix##_d4(); void prefix##_d5(); void prefix##_d6(); void prefix##_d7(); \
	void prefix##_d8(); void prefix##_d9(); void prefix##_da(); void prefix##_db(); \
	void prefix##_dc(); void prefix##_dd(); void prefix##_de(); void prefix##_df(); \
	void prefix##_e0(); void prefix##_e1(); void prefix##_e2(); void prefix##_e3(); \
	void prefix##_e4(); void prefix##_e5(); void prefix##_e6(); void prefix##_e7(); \
	void prefix##_e8(); void prefix##_e9(); void prefix##_ea(); void prefix##_eb(); \
	void prefix##_ec(); void prefix##_ed(); void prefix##_ee(); void prefix##_ef(); \
	void prefix##_f0(); void prefix##_f1(); void prefix##_f2(); void prefix##_f3(); \
	void prefix##_f4(); void prefix##_f5(); void prefix##_f6(); void prefix##_f7(); \
	void prefix##_f8(); void prefix##_f9(); void prefix##_fa(); void prefix##_fb(); \
	void prefix##_fc(); void prefix##_fd(); void prefix##_fe(); void prefix##_ff();

	PROTOTYPES(op)
	PROTOTYPES(cb)
	PROTOTYPES(dd)
	PROTOTYPES(ed)
	PROTOTYPES(fd)
	PROTOTYPES(xycb)
#undef PROTOTYPES
};
