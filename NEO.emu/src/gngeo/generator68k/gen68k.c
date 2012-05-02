/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include <stdio.h>
#include <stdlib.h>

#include "generator.h"

#include "def68k-iibs.h"

//#include "tab68k.c"

/* forward references */

void generate(FILE *output, int topnibble);
void generate_ea(FILE *o, t_iib *iib, t_type type, int update);
void generate_eaval(FILE *o, t_iib *iib, t_type type);
void generate_eastore(FILE *o, t_iib *iib, t_type type);
void generate_outdata(FILE *o, t_iib *iib, const char *init);
void generate_cc(FILE *o, t_iib *iib);
void generate_stdflag_n(FILE *o, t_iib *iib);
void generate_stdflag_z(FILE *o, t_iib *iib);
void generate_clrflag_v(FILE *o, t_iib *iib);
void generate_clrflag_c(FILE *o, t_iib *iib);
void generate_clrflag_n(FILE *o, t_iib *iib);
void generate_setflag_z(FILE *o, t_iib *iib);
void generate_subflag_c(FILE *o, t_iib *iib);
void generate_subflag_cx(FILE *o, t_iib *iib);
void generate_subxflag_cx(FILE *o, t_iib *iib);
void generate_subflag_v(FILE *o, t_iib *iib);
void generate_cmpaflag_c(FILE *o, t_iib *iib);
void generate_cmpaflag_v(FILE *o, t_iib *iib);
void generate_addflag_cx(FILE *o, t_iib *iib);
void generate_addflag_v(FILE *o, t_iib *iib);
void generate_addxflag_cx(FILE *o, t_iib *iib);
void generate_stdxflag_z(FILE *o, t_iib *iib);
void generate_negflag_cx(FILE *o, t_iib *iib);
void generate_negflag_v(FILE *o, t_iib *iib);
void generate_negxflag_cx(FILE *o, t_iib *iib);
void generate_negxflag_v(FILE *o, t_iib *iib);
void generate_bits(FILE *o, t_iib *iib);

/* defines */

#define HEADER "/*****************************************************************************/\n/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */\n/*****************************************************************************/\n/*                                                                           */\n/* cpu68k-%x.c                                                                */\n/*                                                                           */\n/*****************************************************************************/\n\n"

#define OUT(x) fputs(x,output);
#define FNAME_GEN68K_CPU_OUT "cpu68k-%x.c"

/* program entry routine */

int main(int argc, char *argv[])
{
  FILE *output;
  int i;
  char tmp[256];

  (void)argc;
  (void)argv;

  printf("Writing C files... ");
  fflush(stdout);

  for (i = 0; i < 16; i++) {

    printf("%d. ", i);
    fflush(stdout);

    /* make filename */
    sprintf(tmp, FNAME_GEN68K_CPU_OUT, i);

    /* open output file */
    if ((output = fopen(tmp, "w")) == NULL) {
      perror("fopen output");
      exit(1);
    }

    /* output header */
    fprintf(output, HEADER, i);
    fprintf(output, "#include \"cpu68k-inline.h\"\n\n");

    generate(output, i);

    /* close output */
    if (fclose(output)) {
      perror("fclose output");
      exit(1);
    }

  }

  printf("done.\n");
  fflush(stdout);

  /* normal program termination */
  return(0);
}

void generate(FILE *output, int topnibble)
{
  t_iib *iib;
  int i, flags, pcinc;
  int DEBUG_BRANCH = 0;
  int DEBUG_SR = 0;
  int DEBUG_RTE = 0;

  for (i = 0; i < iibs_num; i++) {
    iib = &iibs[i];

    if ((iib->mask & 0xF000) != 0xF000) {
      fprintf(stderr, "error: Strange mask %x\n", iib->mask);
      exit(1);
    }
    if (((iib->bits & 0xF000)>>12) != topnibble) {
      continue;
    }

	for (flags = 0; flags < 2; flags++) {

	    if (flags == 1 && iib->flags.set == 0) {
		/* there is no non-flags version, functable will already go
		   straight to the normal version anyway, so lets just skip it */
		continue;
	    }

	    fprintf(output, "void cpu_op_%i%s(t_ipc *ipc) /* %s */ {\n",
		    i, flags ? "b" : "a", mnemonic_table[iib->mnemonic].name);
	    fprintf(output, "  /* mask %04x, bits %04x, mnemonic %d, priv %d, ",
		    iib->mask, iib->bits, iib->mnemonic, iib->flags.priv);
	    fprintf(output, "endblk %d, imm_notzero %d, used %d",
		    iib->flags.endblk, iib->flags.imm_notzero, iib->flags.used);
	    fprintf(output, "     set %d, size %d, stype %d, dtype %d, sbitpos %d, ",
		    iib->flags.set, iib->size, iib->stype, iib->dtype, iib->sbitpos);
	    fprintf(output, "dbitpos %d, immvalue %d */\n", iib->dbitpos,
		    iib->immvalue);

	    pcinc = 1;

	    switch(iib->mnemonic) {

	    case i_OR:
	    case i_AND:
	    case i_EOR:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		generate_outdata(output, iib, "dstdata");
		OUT("\n");
		switch(iib->mnemonic) {
		case i_OR:
		    OUT("  outdata|= srcdata;\n");
		    break;
		case i_AND:
		    OUT("  outdata&= srcdata;\n");
		    break;
		case i_EOR:
		    OUT("  outdata^= srcdata;\n");
		    break;
		default:
		    OUT("ERROR\n");
		    break;
		}
		generate_eastore(output, iib, tp_dst);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_clrflag_v(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_C)
		    generate_clrflag_c(output, iib);
		break;

	    case i_ORSR:
	    case i_ANDSR:
	    case i_EORSR:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		OUT("  unsigned int sr = regs.sr.sr_struct.s;\n");
		OUT("\n");
		if (DEBUG_SR)
		    fputs("  printf(\"SR: %08X %04X\\n\", PC, regs.sr.sr_int);\n",
			  output);
		if (iib->size == sz_word) {
		    OUT("  if (!SFLAG)\n");
		    fprintf(output, "    reg68k_internal_vector(V_PRIVILEGE, PC+%d);\n",
			    (iib->wordlen)*2);
		    OUT("\n");
		}
		switch(iib->mnemonic) {
		case i_ORSR:
		    OUT("  SR|= srcdata;\n");
		    break;
		case i_ANDSR:
		    if (iib->size == sz_byte) {
			OUT("  SR = (SR & 0xFF00) | (SR & srcdata);\n");
		    } else {
			OUT("  SR&= srcdata;\n");
		    }
		    break;
		case i_EORSR:
		    OUT("  SR^= srcdata;\n");
		    break;
		default:
		    OUT("ERROR\n");
		    break;
		}
		OUT("  if (sr != (uint8)regs.sr.sr_struct.s) {\n");
		OUT("    /* mode change, swap SP and A7 */\n");
		OUT("    ADDRREG(7)^= SP; SP^= ADDRREG(7); ADDRREG(7)^= SP;\n");
		OUT("  }\n");
		if (DEBUG_SR)
		    fputs("  printf(\"SR: %08X %04X\\n\", PC, regs.sr.sr_int);\n",
			  output);
		break;

	    case i_SUB:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		switch (iib->size) {
		case sz_byte:
		    generate_outdata(output, iib, "(sint8)dstdata - (sint8)srcdata");
		    break;
		case sz_word:
		    generate_outdata(output, iib, "(sint16)dstdata - (sint16)srcdata");
		    break;
		case sz_long:
		    generate_outdata(output, iib, "(sint32)dstdata - (sint32)srcdata");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		OUT("\n");
		generate_eastore(output, iib, tp_dst);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_subflag_v(output, iib);
		if (flags && ((iib->flags.set & IIB_FLAG_C) ||
			      (iib->flags.set & IIB_FLAG_X)))
		    generate_subflag_cx(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		break;

	    case i_SUBA:
		
		if (iib->dtype != dt_Areg)
		    OUT("Error\n");
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		OUT("  uint32 dstdata = ADDRREG(dstreg);\n");
		switch (iib->size) {
		case sz_byte:
		    OUT("  uint32 outdata = (sint32)dstdata - (sint8)srcdata;\n");
		    break;
		case sz_word:
		    OUT("  uint32 outdata = (sint32)dstdata - (sint16)srcdata;\n");
		    break;
		case sz_long:
		    OUT("  uint32 outdata = (sint32)dstdata - (sint32)srcdata;\n");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		OUT("\n");
		OUT("  ADDRREG(dstreg) = outdata;\n");
		break;

	    case i_SUBX:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		switch (iib->size) {
		case sz_byte:
		    generate_outdata(output, iib, "(sint8)dstdata - (sint8)srcdata "
				     "- XFLAG");
		    break;
		case sz_word:
		    generate_outdata(output, iib, "(sint16)dstdata - (sint16)srcdata"
				     "- XFLAG");
		    break;
		case sz_long:
		    generate_outdata(output, iib, "(sint32)dstdata - (sint32)srcdata"
				     "- XFLAG");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		OUT("\n");
		generate_eastore(output, iib, tp_dst);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_subflag_v(output, iib);
		if (flags && ((iib->flags.set & IIB_FLAG_C) ||
			      (iib->flags.set & IIB_FLAG_X)))
		    generate_subxflag_cx(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdxflag_z(output, iib);
		break;
	
	    case i_ADD:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		switch (iib->size) {
		case sz_byte:
		    generate_outdata(output, iib, "(sint8)dstdata + (sint8)srcdata");
		    break;
		case sz_word:
		    generate_outdata(output, iib, "(sint16)dstdata + (sint16)srcdata");
		    break;
		case sz_long:
		    generate_outdata(output, iib, "(sint32)dstdata + (sint32)srcdata");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		OUT("\n");
		generate_eastore(output, iib, tp_dst);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_addflag_v(output, iib);
		if (flags && ((iib->flags.set & IIB_FLAG_C) ||
			      (iib->flags.set & IIB_FLAG_X)))
		    generate_addflag_cx(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		break;

	    case i_ADDA:
		
		if (iib->dtype != dt_Areg)
		    OUT("Error\n");
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		OUT("  uint32 dstdata = ADDRREG(dstreg);\n");
		switch (iib->size) {
		case sz_byte:
		    OUT("  uint32 outdata = (sint32)dstdata + (sint8)srcdata;\n");
		    break;
		case sz_word:
		    OUT("  uint32 outdata = (sint32)dstdata + (sint16)srcdata;\n");
		    break;
		case sz_long:
		    OUT("  uint32 outdata = (sint32)dstdata + (sint32)srcdata;\n");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		OUT("\n");
		OUT("  ADDRREG(dstreg) = outdata;\n");
		break;

	    case i_ADDX:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		switch (iib->size) {
		case sz_byte:
		    generate_outdata(output, iib, "(sint8)dstdata + (sint8)srcdata "
				     "+ XFLAG");
		    break;
		case sz_word:
		    generate_outdata(output, iib, "(sint16)dstdata + (sint16)srcdata"
				     "+ XFLAG");
		    break;
		case sz_long:
		    generate_outdata(output, iib, "(sint32)dstdata + (sint32)srcdata"
				     "+ XFLAG");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		OUT("\n");
		generate_eastore(output, iib, tp_dst);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_addflag_v(output, iib);
		if (flags && ((iib->flags.set & IIB_FLAG_C) ||
			      (iib->flags.set & IIB_FLAG_X)))
		    generate_addxflag_cx(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdxflag_z(output, iib);
		break;

	    case i_MULU:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		OUT("\n");
		OUT("  uint32 outdata = (uint32)srcdata * (uint32)dstdata;\n");
		if (iib->dtype != dt_Dreg)
		    OUT("ERROR dtype\n");
		OUT("  DATAREG(dstreg) = outdata;\n");
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_N)
		    OUT("  NFLAG = ((sint32)outdata) < 0;\n");
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_clrflag_v(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_C)
		    generate_clrflag_c(output, iib);
		break;

	    case i_MULS:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		OUT("\n");
		OUT("  uint32 outdata = (sint32)(sint16)srcdata * "
		    "(sint32)(sint16)dstdata;\n");
		if (iib->dtype != dt_Dreg)
		    OUT("ERROR dtype\n");
		OUT("  DATAREG(dstreg) = outdata;\n");
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_N)
		    OUT("  NFLAG = ((sint32)outdata) < 0;\n");
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_clrflag_v(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_C)
		    generate_clrflag_c(output, iib);
		break;

	    case i_CMP:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		switch (iib->size) {
		case sz_byte:
		    generate_outdata(output, iib, "(sint8)dstdata - (sint8)srcdata");
		    break;
		case sz_word:
		    generate_outdata(output, iib, "(sint16)dstdata - (sint16)srcdata");
		    break;
		case sz_long:
		    generate_outdata(output, iib, "(sint32)dstdata - (sint32)srcdata");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_subflag_v(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_C)
		    generate_subflag_c(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		break;

	    case i_CMPA:
		
		if (iib->dtype != dt_Areg || iib->size != sz_word)
		    OUT("Error\n");
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		iib->size = sz_long;
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		OUT("  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;\n");
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_cmpaflag_v(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_C)
		    generate_cmpaflag_c(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		iib->size = sz_word;
		break;

	    case i_BTST:
	    case i_BCHG:
	    case i_BCLR:
	    case i_BSET:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		switch (iib->size) {
		case sz_byte:
		    OUT("  uint32 bitpos = 1<<(srcdata & 7);");
		    break;
		case sz_long:
		    OUT("  uint32 bitpos = 1<<(srcdata & 31);");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		OUT("\n");
		switch(iib->mnemonic) {
		case i_BTST:
		    break;
		case i_BCHG:
		    generate_outdata(output, iib, "dstdata ^ bitpos");
		    generate_eastore(output, iib, tp_dst);
		    break;
		case i_BCLR:
		    generate_outdata(output, iib, "dstdata & ~bitpos");
		    generate_eastore(output, iib, tp_dst);
		    break;
		case i_BSET:
		    generate_outdata(output, iib, "dstdata | bitpos");
		    generate_eastore(output, iib, tp_dst);
		    break;
		default:
		    OUT("ERROR\n");
		    break;
		}
		OUT("\n");
		OUT("  ZFLAG = !(dstdata & bitpos);\n");
		break;

	    case i_MOVE:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_outdata(output, iib, "srcdata");
		OUT("\n");
		generate_eastore(output, iib, tp_dst);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_clrflag_v(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_C)
		    generate_clrflag_c(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		break;

	    case i_MOVEA:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		if (iib->dtype != dt_Areg || iib->size != sz_word)
		    OUT("Error\n");
		OUT("\n");
		OUT("  ADDRREG(dstreg) = (sint32)(sint16)srcdata;\n");
		break;

	    case i_MOVEPMR:
		
		generate_ea(output, iib, tp_src, 1);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		switch(iib->size) {
		case sz_word:
		    generate_outdata(output, iib, "(fetchbyte(srcaddr) << 8) + "
				     "fetchbyte(srcaddr+2)");
		    break;
		case sz_long:
		    generate_outdata(output, iib, "(fetchbyte(srcaddr) << 24) | "
				     "(fetchbyte(srcaddr+2) << 16) | "
				     "\n    (fetchbyte(srcaddr+4) << 8) | "
				     "fetchbyte(srcaddr+6)");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		OUT("\n");
		generate_eastore(output, iib, tp_dst);
		break;

	    case i_MOVEPRM:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		OUT("\n");
		switch(iib->size) {
		case sz_word:
		    OUT("  storebyte(dstaddr, (srcdata >> 8) & 0xFF);\n");
		    OUT("  storebyte(dstaddr+2, srcdata & 0xFF);\n");
		    break;
		case sz_long:
		    OUT("  storebyte(dstaddr, (srcdata >> 24) & 0xFF);\n");
		    OUT("  storebyte(dstaddr+2, (srcdata >> 16) & 0xFF);\n");
		    OUT("  storebyte(dstaddr+4, (srcdata >> 8) & 0xFF);\n");
		    OUT("  storebyte(dstaddr+6, srcdata & 0xFF);\n");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		break;

	    case i_MOVEFSR:
		
		generate_ea(output, iib, tp_src, 1);
		generate_outdata(output, iib, NULL);
		OUT("\n");
		OUT("  outdata = SR;\n");
		generate_eastore(output, iib, tp_src);
		break;

	    case i_MOVETSR:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		OUT("  unsigned int sr = regs.sr.sr_struct.s;\n");
		OUT("\n");
		if (DEBUG_SR)
		    fputs("  printf(\"SR: %08X %04X\\n\", PC, regs.sr.sr_int);\n",
			  output);
		switch (iib->size) {
		case sz_byte:
		    OUT("  SR = (SR & ~0xFF) | srcdata;\n");
		    break;
		case sz_word:
		    OUT("  if (!SFLAG)\n");
		    fprintf(output, "    reg68k_internal_vector(V_PRIVILEGE, PC+%d);\n",
			    (iib->wordlen)*2);
		    OUT("\n");
		    OUT("  SR = srcdata;\n");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		OUT("  if (sr != (uint8)regs.sr.sr_struct.s) {\n");
		OUT("    /* mode change, swap SP and A7 */\n");
		OUT("    ADDRREG(7)^= SP; SP^= ADDRREG(7); ADDRREG(7)^= SP;\n");
		OUT("  }\n");
		if (DEBUG_SR)
		    fputs("  printf(\"SR: %08X %04X\\n\", PC, regs.sr.sr_int);\n",
			  output);
		break;

	    case i_MOVEMRM:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 0);
		if (iib->dtype == dt_Adec) {
		    OUT("  uint8 datamask = (srcdata & 0xFF00) >> 8;\n");
		    OUT("  uint8 addrmask = srcdata & 0xFF;");
		    OUT("\n");
		    switch(iib->size) {
		    case sz_word:
			OUT("  while (addrmask) {\n");
			OUT("    dstaddr-= 2;\n");
			OUT("    storeword(dstaddr, ADDRREG((7-movem_bit[addrmask])));\n");
			OUT("    addrmask&= ~(1<<movem_bit[addrmask]);\n");
			OUT("  }\n");
			OUT("  while (datamask) {\n");
			OUT("    dstaddr-= 2;\n");
			OUT("    storeword(dstaddr, DATAREG((7-movem_bit[datamask])));\n");
			OUT("    datamask&= ~(1<<movem_bit[datamask]);\n");
			OUT("  }\n");
			break;
		    case sz_long:
			OUT("  while (addrmask) {\n");
			OUT("    dstaddr-= 4;\n");
			OUT("    storelong(dstaddr, ADDRREG((7-movem_bit[addrmask])));\n");
			OUT("    addrmask&= ~(1<<movem_bit[addrmask]);\n");
			OUT("  }\n");
			OUT("  while (datamask) {\n");
			OUT("    dstaddr-= 4;\n");
			OUT("    storelong(dstaddr, ");
			OUT("DATAREG((7-movem_bit[datamask])));\n");
			OUT("    datamask&= ~(1<<movem_bit[datamask]);\n");
			OUT("  }\n");
			break;
		    default:
			OUT("ERROR\n");
			break;
		    }
		    OUT("  ADDRREG(dstreg) = dstaddr;");
		} else {
		    OUT("  uint8 addrmask = (srcdata & 0xFF00) >> 8;\n");
		    OUT("  uint8 datamask = srcdata & 0xFF;");
		    OUT("\n");
		    switch(iib->size) {
		    case sz_word:
			OUT("  while (datamask) {\n");
			OUT("    storeword(dstaddr, DATAREG(movem_bit[datamask]));\n");
			OUT("    datamask&= ~(1<<movem_bit[datamask]);\n");
			OUT("    dstaddr+= 2;\n");
			OUT("  }\n");
			OUT("  while (addrmask) {\n");
			OUT("    storeword(dstaddr, ADDRREG(movem_bit[addrmask]));\n");
			OUT("    addrmask&= ~(1<<movem_bit[addrmask]);\n");
			OUT("    dstaddr+= 2;\n");
			OUT("  }\n");
			break;
		    case sz_long:
			OUT("  while (datamask) {\n");
			OUT("    storelong(dstaddr, DATAREG(movem_bit[datamask]));\n");
			OUT("    datamask&= ~(1<<movem_bit[datamask]);\n");
			OUT("    dstaddr+= 4;\n");
			OUT("  }\n");
			OUT("  while (addrmask) {\n");
			OUT("    storelong(dstaddr, ADDRREG(movem_bit[addrmask]));\n");
			OUT("    addrmask&= ~(1<<movem_bit[addrmask]);\n");
			OUT("    dstaddr+= 4;\n");
			OUT("  }\n");
			break;
		    default:
			OUT("ERROR\n");
			break;
		    }
		    if (iib->dtype == dt_Ainc) {
			/* not supported */
			OUT("ERROR\n");
		    }
		}
		break;

	    case i_MOVEMMR:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 0);
		OUT("  uint8 addrmask = (srcdata & 0xFF00) >> 8;\n");
		OUT("  uint8 datamask = srcdata & 0xFF;");
		OUT("\n");
		switch(iib->size) {
		case sz_word:
		    OUT("  while (datamask) {\n");
		    OUT("    DATAREG(movem_bit[datamask]) = ");
		    OUT("(sint32)(sint16)fetchword(dstaddr);\n");
		    OUT("    datamask&= ~(1<<movem_bit[datamask]);\n");
		    OUT("    dstaddr+= 2;\n");
		    OUT("  }\n");
		    OUT("  while (addrmask) {\n");
		    OUT("    ADDRREG(movem_bit[addrmask]) = ");
		    OUT("(sint32)(sint16)fetchword(dstaddr);\n");
		    OUT("    addrmask&= ~(1<<movem_bit[addrmask]);\n");
		    OUT("    dstaddr+= 2;\n");
		    OUT("  }\n");
		    break;
		case sz_long:
		    OUT("  while (datamask) {\n");
		    OUT("    DATAREG(movem_bit[datamask]) = fetchlong(dstaddr);\n");
		    OUT("    datamask&= ~(1<<movem_bit[datamask]);\n");
		    OUT("    dstaddr+= 4;\n");
		    OUT("  }\n");
		    OUT("  while (addrmask) {\n");
		    OUT("    ADDRREG(movem_bit[addrmask]) = fetchlong(dstaddr);\n");
		    OUT("    addrmask&= ~(1<<movem_bit[addrmask]);\n");
		    OUT("    dstaddr+= 4;\n");
		    OUT("  }\n");
		    break;
		default:
		    OUT("ERROR\n");
		    break;
		}
		if (iib->dtype == dt_Ainc) {
		    OUT("  ADDRREG(dstreg) = dstaddr;\n");
		} else if (iib->dtype == dt_Adec) {
		    /* not supported */
		    OUT("ERROR\n");
		}
		break;

	    case i_MOVETUSP:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		OUT("\n");
		OUT("  if (!SFLAG)\n");
		fprintf(output, "    reg68k_internal_vector(V_PRIVILEGE, PC+%d);\n",
			(iib->wordlen)*2);
		OUT("\n");
		OUT("  SP = srcdata;\n");
		break;

	    case i_MOVEFUSP:
		
		generate_ea(output, iib, tp_src, 1);
		OUT("  uint32 outdata;\n");
		OUT("\n");
		OUT("  if (!SFLAG)\n");
		fprintf(output, "    reg68k_internal_vector(V_PRIVILEGE, PC+%d);\n",
			(iib->wordlen)*2);
		OUT("\n");
		OUT("  outdata = SP;\n");
		generate_eastore(output, iib, tp_src);
		break;

	    case i_NEG:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		switch (iib->size) {
		case sz_byte:
		    generate_outdata(output, iib, "0 - (sint8)srcdata");
		    break;
		case sz_word:
		    generate_outdata(output, iib, "0 - (sint16)srcdata");
		    break;
		case sz_long:
		    generate_outdata(output, iib, "0 - (sint32)srcdata");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		OUT("\n");
		generate_eastore(output, iib, tp_src);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_negflag_v(output, iib);
		if (flags && ((iib->flags.set & IIB_FLAG_C) ||
			      (iib->flags.set & IIB_FLAG_X)))
		    generate_negflag_cx(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		break;

	    case i_NEGX:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		switch (iib->size) {
		case sz_byte:
		    generate_outdata(output, iib, "0 - (sint8)srcdata - XFLAG");
		    break;
		case sz_word:
		    generate_outdata(output, iib, "0 - (sint16)srcdata - XFLAG");
		    break;
		case sz_long:
		    generate_outdata(output, iib, "0 - (sint32)srcdata - XFLAG");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		OUT("\n");
		generate_eastore(output, iib, tp_src);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_negxflag_v(output, iib);
		if (flags && ((iib->flags.set & IIB_FLAG_C) ||
			      (iib->flags.set & IIB_FLAG_X)))
		    generate_negxflag_cx(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdxflag_z(output, iib);
		break;
	
	    case i_CLR:
		
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src); /* read before write */
		generate_outdata(output, iib, "0");
		OUT("\n");
		generate_eastore(output, iib, tp_src);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_clrflag_v(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_C)
		    generate_clrflag_c(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_clrflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_setflag_z(output, iib);
		break;

	    case i_NOT:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src); /* read before write */
		generate_outdata(output, iib, "~srcdata");
		OUT("\n");
		generate_eastore(output, iib, tp_src);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_clrflag_v(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_C)
		    generate_clrflag_c(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		break;

	    case i_ABCD:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		generate_outdata(output, iib, NULL);
		OUT("\n");
		OUT("  uint8 outdata_low = (dstdata & 0xF) + (srcdata & 0xF) ");
		OUT("+ XFLAG;\n");
		OUT("  uint16 precalc = dstdata + srcdata + XFLAG;\n");
		OUT("  uint16 outdata_tmp = precalc;\n");
		OUT("\n");
		OUT("  if (outdata_low > 0x09)\n");
		OUT("    outdata_tmp+= 0x06;\n");
		OUT("  if (outdata_tmp > 0x90) {\n");
		OUT("    outdata_tmp+= 0x60;\n");
		if (flags && iib->flags.set & IIB_FLAG_C)
		    OUT("    CFLAG = 1;\n");
		if (flags && iib->flags.set & IIB_FLAG_X)
		    OUT("    XFLAG = 1;\n");
		OUT("  } else {\n");
		if (flags && iib->flags.set & IIB_FLAG_C)
		    OUT("    CFLAG = 0;\n");
		if (flags && iib->flags.set & IIB_FLAG_X)
		    OUT("    XFLAG = 0;\n");
		OUT("  }\n");
		OUT("  outdata = outdata_tmp;\n");
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    OUT("  if (outdata) ZFLAG = 0;\n");
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_V)
		    OUT("  VFLAG = ((precalc & 1<<7) == 0) && (outdata & 1<<7);\n");
		generate_eastore(output, iib, tp_dst);
		break;

	    case i_SBCD:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		generate_outdata(output, iib, NULL);
		OUT("\n");
		OUT("  sint8 outdata_low = (dstdata & 0xF) - (srcdata & 0xF) ");
		OUT("- XFLAG;\n");
		OUT("  sint16 precalc = dstdata - srcdata - XFLAG;\n");
		OUT("  sint16 outdata_tmp = precalc;\n");
		OUT("\n");
		OUT("  if (outdata_low < 0)\n");
		OUT("    outdata_tmp-= 0x06;\n");
		OUT("  if (outdata_tmp < 0) {\n");
		OUT("    outdata_tmp-= 0x60;\n");
		if (flags && iib->flags.set & IIB_FLAG_C)
		    OUT("    CFLAG = 1;\n");
		if (flags && iib->flags.set & IIB_FLAG_X)
		    OUT("    XFLAG = 1;\n");
		OUT("  } else {\n");
		if (flags && iib->flags.set & IIB_FLAG_C)
		    OUT("    CFLAG = 0;\n");
		if (flags && iib->flags.set & IIB_FLAG_X)
		    OUT("    XFLAG = 0;\n");
		OUT("  }\n");
		OUT("  outdata = outdata_tmp;\n");
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    OUT("  if (outdata) ZFLAG = 0;\n");
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_V)
		    OUT("  VFLAG = (precalc & 1<<7) && ((outdata & 1<<7) == 0);\n");
		generate_eastore(output, iib, tp_dst);
		break;

	    case i_NBCD:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_outdata(output, iib, NULL);
		OUT("\n");
		OUT("  sint8 outdata_low = - (srcdata & 0xF) - XFLAG;\n");
		OUT("  sint16 precalc = - srcdata - XFLAG;\n");
		OUT("  sint16 outdata_tmp = precalc;\n");
		OUT("\n");
		OUT("  if (outdata_low < 0)\n");
		OUT("    outdata_tmp-= 0x06;\n");
		OUT("  if (outdata_tmp < 0) {\n");
		OUT("    outdata_tmp-= 0x60;\n");
		if (flags && iib->flags.set & IIB_FLAG_C)
		    OUT("    CFLAG = 1;\n");
		if (flags && iib->flags.set & IIB_FLAG_X)
		    OUT("    XFLAG = 1;\n");
		OUT("  } else {\n");
		if (flags && iib->flags.set & IIB_FLAG_C)
		    OUT("    CFLAG = 0;\n");
		if (flags && iib->flags.set & IIB_FLAG_X)
		    OUT("    XFLAG = 0;\n");
		OUT("  }\n");
		OUT("  outdata = outdata_tmp;\n");
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    OUT("  if (outdata) ZFLAG = 0;\n");
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_V)
		    OUT("  VFLAG = (precalc & 1<<7) && ((outdata & 1<<7) == 0);\n");
		generate_eastore(output, iib, tp_src);
		break;

	    case i_SWAP:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_outdata(output, iib, "(srcdata>>16) | (srcdata<<16)");
		OUT("\n");
		generate_eastore(output, iib, tp_src);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_clrflag_v(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_C)
		    generate_clrflag_c(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		break;

	    case i_PEA:
		generate_ea(output, iib, tp_src, 1);
		OUT("\n");
		OUT("  ADDRREG(7)-= 4;\n");
		OUT("  storelong(ADDRREG(7), srcaddr);\n");
		break;

	    case i_LEA:
		generate_ea(output, iib, tp_src, 1);
		generate_ea(output, iib, tp_dst, 1);
		generate_outdata(output, iib, "srcaddr");
		OUT("\n");
		generate_eastore(output, iib, tp_dst);
		break;

	    case i_EXT:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		OUT("\n");
		switch(iib->size) {
		case sz_word:
		    generate_outdata(output, iib, "(sint16)(sint8)(srcdata)");
		    break;
		case sz_long:
		    generate_outdata(output, iib, "(sint32)(sint16)(srcdata)");
		    break;
		default:
		    fprintf(output, "ERROR size\n");
		    break;
		}
		generate_eastore(output, iib, tp_src);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_clrflag_v(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_C)
		    generate_clrflag_c(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		break;

	    case i_EXG:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		OUT("\n");
		switch (iib->dtype) {
		case dt_Dreg:
		    OUT("  DATAREG(dstreg) = srcdata;\n");
		    break;
		case dt_Areg:
		    OUT("  ADDRREG(dstreg) = srcdata;\n");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		switch (iib->stype) {
		case dt_Dreg:
		    OUT("  DATAREG(srcreg) = dstdata;\n");
		    break;
		case dt_Areg:
		    OUT("  ADDRREG(srcreg) = dstdata;\n");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		break;

	    case i_TST:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_outdata(output, iib, "srcdata");
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_clrflag_v(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_C)
		    generate_clrflag_c(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		break;

	    case i_TAS:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_outdata(output, iib, "srcdata");
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_clrflag_v(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_C)
		    generate_clrflag_c(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
#ifndef BROKEN_TAS
		switch(iib->size) {
		case sz_byte:
		    OUT("  outdata|= 1<<7;\n");
		    break;
		case sz_word:
		    OUT("  outdata|= 1<<15;\n");
		    break;
		case sz_long:
		    OUT("  outdata|= 1<<31;\n");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		generate_eastore(output, iib, tp_src);
#endif
		break;

	    case i_CHK:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		fprintf(output, "\n");
		if (iib->size != sz_word)
		    OUT("ERROR size\n");
		fprintf(output, "  if ((sint16)srcdata < 0) {\n");
		if (flags)
		    OUT("    NFLAG = 1;\n");
		fprintf(output, "    reg68k_internal_vector(V_CHK, PC+%d);\n",
			(iib->wordlen)*2);
		OUT("  } else if (dstdata > srcdata) {\n");
		if (flags)
		    OUT("    NFLAG = 0;\n");
		fprintf(output, "    reg68k_internal_vector(V_CHK, PC+%d);\n",
			(iib->wordlen)*2);
		OUT("  }\n");
		break;

	    case i_TRAPV:
		OUT("  if (VFLAG) {\n");
		fprintf(output, "    reg68k_internal_vector(V_TRAPV, PC+%d);\n",
			(iib->wordlen)*2);
		OUT("  }\n");
		break;

	    case i_TRAP:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		OUT("\n");
		fprintf(output, "  reg68k_internal_vector(V_TRAP+srcdata, PC+%d);\n",
			(iib->wordlen)*2);
		pcinc = 0;
		break;

	    case i_RESET:
		OUT("  printf(\"RESET @ %x\\n\", PC);\n");
		OUT("  exit(1);\n");
		break;

	    case i_NOP:
		/* NOP */
		break;

	    case i_STOP:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		OUT("\n");
		OUT("  if (regs.stop)\n");
		OUT("    return;\n");
		OUT("  if (!(SFLAG && (srcdata & 1<<13))) {\n");
		fprintf(output, "    reg68k_internal_vector(V_PRIVILEGE, PC+%d);\n",
			(iib->wordlen)*2);
		fprintf(output, "    PC+= %d;\n", (iib->wordlen)*2);
		OUT("  } else {\n");
		OUT("    SR = srcdata;\n");
		OUT("    STOP = 1;\n");
		OUT("  }\n");
		pcinc = 0;
		break;

	    case i_LINK:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		if (iib->stype != dt_ImmW)
		    OUT("ERROR stype\n");
		OUT("\n");
		OUT("  ADDRREG(7)-= 4;\n");
		OUT("  storelong(ADDRREG(7), dstdata);\n");
		OUT("  ADDRREG(dstreg) = ADDRREG(7);\n");
		OUT("  ADDRREG(7)+= (sint16)srcdata;\n");
		break;

	    case i_UNLK:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		OUT("\n");
		OUT("  ADDRREG(srcreg) = fetchlong(srcdata);\n");
		OUT("  ADDRREG(7) = srcdata+4;\n");
		break;

	    case i_RTE:
		if (DEBUG_RTE)
		    fputs("  printf(\"RTE: 0x%X\\n\", PC);\n", output);
		if (DEBUG_SR)
		    fputs("  printf(\"SR: %08X %04X\\n\", PC, regs.sr.sr_int);\n",
			  output);
		OUT("  if (!SFLAG)\n");
		fprintf(output, "    reg68k_internal_vector(V_PRIVILEGE, PC+%d);\n",
			(iib->wordlen)*2);
		OUT("\n");
		OUT("  SR = fetchword(ADDRREG(7));\n");
		OUT("  PC = fetchlong(ADDRREG(7)+2);\n");
		OUT("  ADDRREG(7)+= 6;\n");
		OUT("  if (!regs.sr.sr_struct.s) {\n");
		OUT("    /* mode change, swap SP and A7 */\n");
		OUT("    ADDRREG(7)^= SP; SP^= ADDRREG(7); ADDRREG(7)^= SP;\n");
		OUT("  }\n");
		if (DEBUG_RTE)
		    fputs("  printf(\"RTE: ->0x%X\\n\", PC);\n", output);
		if (DEBUG_SR)
		    fputs("  printf(\"SR: %08X %04X\\n\", PC, regs.sr.sr_int);\n",
			  output);
		pcinc = 0;
		break;

	    case i_RTS:
		if (DEBUG_BRANCH)
		    fputs("  printf(\"RTS: 0x%X\\n\", PC);", output);
		OUT("  PC = fetchlong(ADDRREG(7));\n");
		OUT("  ADDRREG(7)+= 4;\n");
		if (DEBUG_BRANCH)
		    fputs("  printf(\"RTS: ->0x%X\\n\", PC);", output);
		pcinc = 0;
		break;

	    case i_RTR:
		if (DEBUG_BRANCH)
		    fputs("  printf(\"RTR: 0x%X\\n\", PC);\n", output);
		if (DEBUG_SR)
		    fputs("  printf(\"SR: %08X %04X\\n\", PC, regs.sr.sr_int);\n",
			  output);
		OUT("  SR = (SR & ~0xFF) | (fetchword(ADDRREG(7)) & 0xFF);\n");
		OUT("  PC = fetchlong(ADDRREG(7)+2);\n");
		OUT("  ADDRREG(7)+= 6;\n");
		if (DEBUG_BRANCH)
		    fputs("  printf(\"RTR: ->0x%X\\n\", PC);\n", output);
		if (DEBUG_SR)
		    fputs("  printf(\"SR: %08X %04X\\n\", PC, regs.sr.sr_int);\n",
			  output);
		pcinc = 0;
		break;

	    case i_JSR:
		generate_ea(output, iib, tp_src, 1);
		OUT("\n");
		if (DEBUG_BRANCH)
		    fputs("  printf(\"JSR: 0x%X\\n\", PC);\n", output);
		OUT("  ADDRREG(7)-= 4;\n");
		fprintf(output, "  storelong(ADDRREG(7), PC+%d);\n", (iib->wordlen)*2);
		OUT("  PC = srcaddr;\n");
		if (DEBUG_BRANCH)
		    fputs("  printf(\"JSR: ->0x%X\\n\", PC);", output);
		pcinc = 0;
		break;

	    case i_JMP:
		generate_ea(output, iib, tp_src, 1);
		OUT("\n");
		if (DEBUG_BRANCH)
		    fputs("  printf(\"JMP: 0x%X\\n\", PC);", output);
		OUT("  PC = srcaddr;\n");
		if (DEBUG_BRANCH)
		    fputs("  printf(\"JMP: ->0x%X\\n\", PC);\n", output);
		pcinc = 0;
		break;
	
	    case i_Scc:
		generate_ea(output, iib, tp_src, 1);
		generate_cc(output, iib);
		generate_outdata(output, iib, "cc ? (uint8)(-1) : 0");
		OUT("\n");
		generate_eastore(output, iib, tp_src);
		break;

	    case i_SF:
		generate_ea(output, iib, tp_src, 1);
		generate_outdata(output, iib, "0");
		OUT("\n");
		generate_eastore(output, iib, tp_src);
		break;

	    case i_DBcc:
		/* special case where ipc holds the already PC-relative value */
		fprintf(output, "  uint32 srcdata = ipc->src;\n");
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		generate_cc(output, iib);
		fprintf(output, "\n");
		if (iib->size != sz_word) {
		    OUT("ERROR size\n");
		}
		OUT("  if (!cc) {\n");
		OUT("    dstdata-= 1;\n");
		OUT("    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)\n");
		OUT("| (dstdata & 0xFFFF);\n");
		OUT("    if ((sint16)dstdata != -1)\n");
		OUT("      PC = srcdata;\n");
		OUT("    else\n");
		fprintf(output, "      PC+= %d;\n", (iib->wordlen)*2);
		OUT("  } else\n");
		fprintf(output, "    PC+= %d;\n", (iib->wordlen)*2);
		pcinc = 0;
		break;

	    case i_DBRA:
		/* special case where ipc holds the already PC-relative value */
		fprintf(output, "  uint32 srcdata = ipc->src;\n");
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		OUT("\n");
		if (iib->size != sz_word) {
		    OUT("ERROR size\n");
		}
		OUT("  dstdata-= 1;\n");
		OUT("  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF) | ");
		OUT("(dstdata & 0xFFFF);\n");
		OUT("  if ((sint16)dstdata != -1)\n");
		OUT("    PC = srcdata;\n");
		OUT("  else\n");
		fprintf(output, "    PC+= %d;\n", (iib->wordlen)*2);
		pcinc = 0;
		break;

	    case i_Bcc:
		/* special case where ipc holds the already PC-relative value */
		OUT("  uint32 srcdata = ipc->src;\n");
		generate_cc(output, iib);
		OUT("\n");
		if (DEBUG_BRANCH)
		    fputs("  printf(\"Bcc: 0x%X\\n\", PC);\n", output);
		OUT("  if (cc)\n");
		OUT("    PC = srcdata;\n");
		OUT("  else\n");
		fprintf(output, "    PC+= %d;\n", (iib->wordlen)*2);
		if (DEBUG_BRANCH)
		    fputs("  printf(\"Bcc: ->0x%X\\n\", PC);\n", output);
		pcinc = 0;
		break;

	    case i_BSR:
		/* special case where ipc holds the already PC-relative value */
		OUT("  uint32 srcdata = ipc->src;\n");
		OUT("\n");
		if (DEBUG_BRANCH)
		    fputs("  printf(\"BSR: 0x%X\\n\", PC);\n", output);
		OUT("  ADDRREG(7)-= 4;\n");
		fprintf(output, "  storelong(ADDRREG(7), PC+%d);\n", (iib->wordlen)*2);
		OUT("  PC = srcdata;\n");
		if (DEBUG_BRANCH)
		    fputs("  printf(\"BSR: ->0x%X\\n\", PC);\n", output);
		pcinc = 0;
		break;

	    case i_DIVU:
		/* DIVx is the only instruction that has different sizes for the
		   source and destination! */
		if (iib->dtype != dt_Dreg)
		    OUT("ERROR dtype\n");
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src); /* 16bit EA */
		generate_ea(output, iib, tp_dst, 1); /* 32bit Dn */
		OUT("  uint32 dstdata = DATAREG(dstreg);\n");
		OUT("  uint32 quotient;\n");
		OUT("\n");
		OUT("  if (srcdata == 0) {\n");
		fprintf(output, "    reg68k_internal_vector(V_ZERO, PC+%d);\n",
			(iib->wordlen)*2);
		OUT("    return;\n");
		OUT("  }\n");
		OUT("  quotient = dstdata / srcdata;\n");
		OUT("  if ((quotient & 0xffff0000) == 0) {\n");
		OUT("    DATAREG(dstreg) = quotient | ");
		OUT("(((uint16)(dstdata % srcdata))<<16);\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    OUT("    VFLAG = 0;\n");
		if (flags && iib->flags.set & IIB_FLAG_N)
		    OUT("    NFLAG = ((sint16)quotient) < 0;\n");
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    OUT("  ZFLAG = !((uint16)quotient);\n");
		if (flags && (iib->flags.set & IIB_FLAG_V ||
			      iib->flags.set & IIB_FLAG_N)) {
		    OUT("  } else {\n");
		    if (flags && iib->flags.set & IIB_FLAG_V)
			OUT("    VFLAG = 1;\n");
		    if (flags && iib->flags.set & IIB_FLAG_N)
			OUT("    NFLAG = 1;\n");
		}
		OUT("  }\n");
		if (flags && iib->flags.set & IIB_FLAG_C)
		    OUT("  CFLAG = 0;\n");
		break;

	    case i_DIVS:
		/* DIVx is the only instruction that has different sizes for the
		   source and destination! */
		if (iib->dtype != dt_Dreg)
		    OUT("ERROR dtype\n");
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src); /* 16bit EA */
		generate_ea(output, iib, tp_dst, 1); /* 32bit Dn */
		OUT("  sint32 dstdata = DATAREG(dstreg);\n");
		OUT("  sint32 quotient;\n");
		OUT("  sint16 remainder;\n");
		OUT("\n");
		OUT("  if (srcdata == 0) {\n");
		fprintf(output, "    reg68k_internal_vector(V_ZERO, PC+%d);\n",
			(iib->wordlen)*2);
		OUT("    return;\n");
		OUT("  }\n");
		OUT("  quotient = dstdata / (sint16)srcdata;\n");
		OUT("  remainder = dstdata % (sint16)srcdata;\n");
		OUT("  if (((quotient & 0xffff8000) == 0) ||\n");
		OUT("      ((quotient & 0xffff8000) == 0xffff8000)) {\n");
		
		/*
		  PEPONE: remainder sign depend on dstdata
		  OUT("    if ((quotient < 0) != (remainder < 0))\n");
		*/

		OUT("    if (((sint32)dstdata < 0) != (remainder < 0))\n");

		OUT("      remainder = -remainder;\n");
		OUT("    DATAREG(dstreg) = ((uint16)quotient) | ");
		OUT("(((uint16)(remainder))<<16);\n");
		if (flags && iib->flags.set & IIB_FLAG_V)
		    OUT("    VFLAG = 0;\n");
		if (flags && iib->flags.set & IIB_FLAG_N)
		    OUT("    NFLAG = ((sint16)quotient) < 0;\n");
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    OUT("    ZFLAG = !((uint16)quotient);\n");
		if (flags && (iib->flags.set & IIB_FLAG_V ||
			      iib->flags.set & IIB_FLAG_N)) {
		    OUT("  } else {\n");
		    if (flags && iib->flags.set & IIB_FLAG_V)
			OUT("    VFLAG = 1;\n");
		    if (flags && iib->flags.set & IIB_FLAG_N)
			OUT("    NFLAG = 1;\n");
		}
		OUT("  }\n");
		if (flags && iib->flags.set & IIB_FLAG_C)
		    OUT("  CFLAG = 0;\n");
		break;

	    case i_ASR:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		generate_bits(output, iib);
		OUT("  uint8 count = srcdata & 63;\n");
		switch (iib->size) {
		case sz_byte:
		    generate_outdata(output, iib, "((sint8)dstdata) >> "
				     "(count > 7 ? 7 : count)");
		    break;
		case sz_word:
		    generate_outdata(output, iib, "((sint16)dstdata) >> "
				     "(count > 15 ? 15 : count)");
		    break;
		case sz_long:
		    generate_outdata(output, iib, "((sint32)dstdata) >> "
				     "(count > 31 ? 31 : count)");
		    break;
		default:
		    OUT("ERROR size\n");
		    break;
		}
		OUT("\n");
		generate_eastore(output, iib, tp_dst);
		if (flags) {
		    OUT("\n");
		    OUT("  if (!srcdata)\n");
		    if (iib->flags.set & IIB_FLAG_C)
			OUT("    CFLAG = 0;\n");
		    OUT("  else if (srcdata >= bits) {\n");
		    if (iib->flags.set & IIB_FLAG_C)
			OUT("    CFLAG = dstdata>>(bits-1);\n");
		    if (iib->flags.set & IIB_FLAG_X)
			OUT("    XFLAG = dstdata>>(bits-1);\n");
		    OUT("  } else {\n");
		    if (iib->flags.set & IIB_FLAG_C)
			OUT("    CFLAG = dstdata>>(count-1) & 1;\n");
		    if (iib->flags.set & IIB_FLAG_X)
			OUT("    XFLAG = dstdata>>(count-1) & 1;\n");
		    OUT("  }\n");
		    if (iib->flags.set & IIB_FLAG_V)
			generate_clrflag_v(output, iib);
		    if (iib->flags.set & IIB_FLAG_N)
			generate_stdflag_n(output, iib);
		    if (iib->flags.set & IIB_FLAG_Z)
			generate_stdflag_z(output, iib);
		}
		break;

	    case i_LSR:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		generate_bits(output, iib);
		OUT("  uint8 count = srcdata & 63;\n");
		generate_outdata(output, iib,
				 "dstdata >> (count > (bits-1) ? (bits-1) : count)");
		OUT("\n");
		generate_eastore(output, iib, tp_dst);
		if (flags) {
		    OUT("\n");
		    OUT("  if (!count)\n");
		    if (iib->flags.set & IIB_FLAG_C)
			OUT("    CFLAG = 0;\n");
		    OUT("  else if (count >= bits) {\n");
		    if (iib->flags.set & IIB_FLAG_C)
			OUT("    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;\n");
		    if (iib->flags.set & IIB_FLAG_X)
			OUT("    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;\n");
		    OUT("  } else {\n");
		    if (iib->flags.set & IIB_FLAG_C)
			OUT("    CFLAG = dstdata>>(count-1) & 1;\n");
		    if (iib->flags.set & IIB_FLAG_X)
			OUT("    XFLAG = dstdata>>(count-1) & 1;\n");
		    OUT("  }\n");
		    if (iib->flags.set & IIB_FLAG_V)
			generate_clrflag_v(output, iib);
		    if (iib->flags.set & IIB_FLAG_N)
			generate_stdflag_n(output, iib);
		    if (iib->flags.set & IIB_FLAG_Z)
			generate_stdflag_z(output, iib);
		}
		break;

	    case i_ASL:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		generate_bits(output, iib);
		OUT("  uint8 count = srcdata & 63;\n");
		generate_outdata(output, iib,
				 "count >= bits ? 0 : (dstdata << count)");
		OUT("\n");
		generate_eastore(output, iib, tp_dst);
		if (flags) {
		    OUT("\n");
		    OUT("  if (!count)\n");
		    if (iib->flags.set & IIB_FLAG_C)
			OUT("    CFLAG = 0;\n");
		    OUT("  else if (count >= bits) {\n");
		    if (iib->flags.set & IIB_FLAG_C)
			OUT("    CFLAG = (count == bits) ? dstdata & 1 : 0;\n");
		    if (iib->flags.set & IIB_FLAG_X)
			OUT("    XFLAG = (count == bits) ? dstdata & 1 : 0;\n");
		    if (iib->flags.set & IIB_FLAG_V)
			OUT("    VFLAG = !dstdata;\n");
		    OUT("  } else {\n");
		    if (iib->flags.set & IIB_FLAG_C)
			OUT("    CFLAG = dstdata>>(bits-count) & 1;\n");
		    if (iib->flags.set & IIB_FLAG_X)
			OUT("    XFLAG = dstdata>>(bits-count) & 1;\n");
		    if (iib->flags.set & IIB_FLAG_V) {
			OUT("    {\n");
			switch (iib->size) {
			case sz_byte:
			    OUT("      uint8 mask = 0xff << (7-count);\n")
				break;
			case sz_word:
			    OUT("      uint16 mask = 0xffff << (15-count);\n");
			    break;
			case sz_long:
			    OUT("      uint32 mask = 0xffffffff <<(31-count);\n");
			    break;
			default:
			    OUT("ERROR size\n");
			    break;
			}
			OUT("      VFLAG = ((dstdata & mask) != mask) && ");
			OUT("((dstdata & mask) != 0);\n");
			OUT("    }\n");
			OUT("  }\n");
		    }
		    if (iib->flags.set & IIB_FLAG_N)
			generate_stdflag_n(output, iib);
		    if (iib->flags.set & IIB_FLAG_Z)
			generate_stdflag_z(output, iib);
		}
		break;

	    case i_LSL:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		generate_bits(output, iib);
		OUT("  uint8 count = srcdata & 63;\n");
		generate_outdata(output, iib,
				 "count >= bits ? 0 : (dstdata << count)");
		OUT("\n");
		generate_eastore(output, iib, tp_dst);
		if (flags) {
		    OUT("\n");
		    OUT("  if (!count)\n");
		    if (iib->flags.set & IIB_FLAG_C)
			OUT("    CFLAG = 0;\n");
		    OUT("  else if (count >= bits) {\n");
		    if (iib->flags.set & IIB_FLAG_C)
			OUT("    CFLAG = (count == bits) ? dstdata & 1 : 0;\n");
		    if (iib->flags.set & IIB_FLAG_X)
			OUT("    XFLAG = (count == bits) ? dstdata & 1 : 0;\n");
		    OUT("  } else {\n");
		    if (iib->flags.set & IIB_FLAG_C)
			OUT("    CFLAG = dstdata>>(bits-count) & 1;\n");
		    if (iib->flags.set & IIB_FLAG_X)
			OUT("    XFLAG = dstdata>>(bits-count) & 1;\n");
		    OUT("  }\n");
		    if (iib->flags.set & IIB_FLAG_V)
			generate_clrflag_v(output, iib);
		    if (iib->flags.set & IIB_FLAG_N)
			generate_stdflag_n(output, iib);
		    if (iib->flags.set & IIB_FLAG_Z)
			generate_stdflag_z(output, iib);
		}
		break;

		/* 64 */

	    case i_ROXR:
	    case i_ROXL:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		generate_bits(output, iib);
		OUT("  uint8 loop = srcdata & 63;\n");
		OUT("  uint8 cflag = CFLAG;\n");
		OUT("  uint8 xflag = XFLAG;\n");
		generate_outdata(output, iib, "dstdata");
		OUT("\n");
		if (iib->mnemonic == i_ROXR) {
		    OUT("  while(loop) {\n");
		    OUT("    cflag = outdata & 1;\n");
		    OUT("    outdata>>= 1;\n");
		    OUT("    if (xflag)\n");
		    OUT("      outdata |= 1<<(bits-1);\n");
		    OUT("    xflag = cflag;\n");
		    OUT("    loop--;\n");
		    OUT("  }\n");
		} else {
		    OUT("  while(loop) {\n");
		    OUT("    cflag = outdata & 1<<(bits-1) ? 1 : 0;\n");
		    OUT("    outdata<<= 1;\n");
		    OUT("    outdata |= xflag;\n");
		    OUT("    xflag = cflag;\n");
		    OUT("    loop--;\n");
		    OUT("  }\n");
		}
		generate_eastore(output, iib, tp_dst);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_X)
		    OUT("  XFLAG = xflag;\n");
		if (flags && iib->flags.set & IIB_FLAG_C)
		    OUT("  CFLAG = xflag;\n");
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_clrflag_v(output, iib);
		break;

	    case i_ROR:
	    case i_ROL:
		generate_ea(output, iib, tp_src, 1);
		generate_eaval(output, iib, tp_src);
		generate_ea(output, iib, tp_dst, 1);
		generate_eaval(output, iib, tp_dst);
		generate_bits(output, iib);
		OUT("  uint8 loop = srcdata & 63;\n");
		OUT("  uint8 cflag = 0;\n");
		generate_outdata(output, iib, "dstdata");
		OUT("\n");
		if (iib->mnemonic == i_ROR) {
		    OUT("  while(loop) {\n");
		    OUT("    cflag = outdata & 1;\n");
		    OUT("    outdata>>= 1;\n");
		    OUT("    if (cflag)\n");
		    OUT("      outdata |= 1<<(bits-1);\n");
		    OUT("    loop--;\n");
		    OUT("  }\n");
		} else {
		    OUT("  while(loop) {\n");
		    OUT("    cflag = outdata & 1<<(bits-1) ? 1 : 0;\n");
		    OUT("    outdata<<= 1;\n");
		    OUT("    if (cflag)\n");
		    OUT("      outdata |= 1;\n");
		    OUT("    loop--;\n");
		    OUT("  }\n");
		}
		generate_eastore(output, iib, tp_dst);
		if (flags)
		    OUT("\n");
		if (flags && iib->flags.set & IIB_FLAG_C)
		    OUT("  CFLAG = cflag;\n");
		if (flags && iib->flags.set & IIB_FLAG_N)
		    generate_stdflag_n(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_Z)
		    generate_stdflag_z(output, iib);
		if (flags && iib->flags.set & IIB_FLAG_V)
		    generate_clrflag_v(output, iib);
		break;

	    case i_LINE10:
		OUT("\n");
		fprintf(output, "  reg68k_internal_vector(V_LINE10, PC);\n");
		pcinc = 0;
		break;

	    case i_LINE15:
		OUT("\n");
		fprintf(output, "  reg68k_internal_vector(V_LINE15, PC);\n");
		pcinc = 0;
		break;

	    case i_ILLG:
		OUT("  printf(\"Illegal instruction @ %x\\n\", PC);\n");
		OUT("  exit(1);\n");
		break;

	    } /* switch */
	    if (pcinc) {
		fprintf(output, "  PC+= %d;\n", (iib->wordlen)*2);
	    }
	    OUT("}\n\n");
	}
    }

  }

void generate_ea(FILE *o, t_iib *iib, t_type type, int update)
{
  t_datatype datatype = type ? iib->dtype : iib->stype;

  /* generate information about EA to be used in calculations */

  switch(datatype) {
  case dt_Dreg:
  case dt_Areg:
  case dt_Aind:
  case dt_Ainc:
  case dt_Adec:
  case dt_Adis:
  case dt_Aidx:
    if (type == tp_src)
      fprintf(o, "  int srcreg = (ipc->opcode >> %d) & 7;\n", iib->sbitpos);
    else
      fprintf(o, "  int dstreg = (ipc->opcode >> %d) & 7;\n", iib->dbitpos);
    break;
  default:
    break;
  }

  if (datatype == dt_Ainc && update) {

    /* Ainc and update */

    switch(iib->size) {
    case sz_byte:
      if (type == tp_src) {
	fprintf(o, "  uint32 srcaddr = ADDRREG(srcreg);\n");
	fprintf(o, "  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= "
		"(srcreg == 7 ? 2 : 1), 0);\n");
      } else {
	fprintf(o, "  uint32 dstaddr = ADDRREG(dstreg);\n");
	fprintf(o, "  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= "
		"(dstreg == 7 ? 2 : 1), 0);\n");
      }
      break;
    case sz_word:
      if (type == tp_src)
	fprintf(o, "  uint32 srcaddr = (ADDRREG(srcreg)+=2, "
		"ADDRREG(srcreg)-2);\n");
      else
	fprintf(o, "  uint32 dstaddr = (ADDRREG(dstreg)+=2, "
		"ADDRREG(dstreg)-2);\n");
      break;
    case sz_long:
      if (type == tp_src)
	fprintf(o, "  uint32 srcaddr = (ADDRREG(srcreg)+=4, "
		"ADDRREG(srcreg)-4);\n");
      else
	fprintf(o, "  uint32 dstaddr = (ADDRREG(dstreg)+=4, "
		"ADDRREG(dstreg)-4);\n");
      break;
    default:
      fprintf(o, "ERROR size\n");
      break;
    }

  } else if (datatype == dt_Adec && update) {

    /* Adec and update */

    switch(iib->size) {
    case sz_byte:
      if (type == tp_src) {
	fprintf(o, "  uint32 srcaddr = (ADDRREG(srcreg)-= "
		"(srcreg == 7 ? 2 : 1));\n");
      } else {
	fprintf(o, "  uint32 dstaddr = (ADDRREG(dstreg)-= "
		"(dstreg == 7 ? 2 : 1));\n");
      }
      break;
    case sz_word:
      if (type == tp_src)
	fprintf(o, "  uint32 srcaddr = ADDRREG(srcreg)-=2;\n");
      else
	fprintf(o, "  uint32 dstaddr = ADDRREG(dstreg)-=2;\n");
      break;
    case sz_long:
      if (type == tp_src)
	fprintf(o, "  uint32 srcaddr = ADDRREG(srcreg)-=4;\n");
      else
	fprintf(o, "  uint32 dstaddr = ADDRREG(dstreg)-=4;\n");
      break;
    default:
      fprintf(o, "ERROR size\n");
      break;
    }

  } else {

    /* no update required */

    switch(datatype) {
    case dt_Dreg:
    case dt_Areg:
      break;
    case dt_Aind:
    case dt_Adec:
    case dt_Ainc:
      if (type == tp_src)
	fprintf(o, "  uint32 srcaddr = ADDRREG(srcreg);\n");
      else
	fprintf(o, "  uint32 dstaddr = ADDRREG(dstreg);\n");
      break;
    case dt_Adis:
      if (type == tp_src)
	fprintf(o, "  uint32 srcaddr = (sint32)ADDRREG(srcreg) + "
		"(sint32)(sint16)ipc->src;\n");
      else
	fprintf(o, "  uint32 dstaddr = (sint32)ADDRREG(dstreg) + "
		"(sint32)(sint16)ipc->dst;\n");
      break;
    case dt_Aidx:
      if (type == tp_src) {
	fprintf(o, "  uint32 srcaddr = (sint32)ADDRREG(srcreg) + "
		"idxval_src(ipc);\n");
      } else {
	fprintf(o, "  uint32 dstaddr = (sint32)ADDRREG(dstreg) + "
		"idxval_dst(ipc);\n");
      }
      break;
    case dt_AbsW:
    case dt_AbsL:
    case dt_Pdis:
      if (type == tp_src)
	fprintf(o, "  uint32 srcaddr = ipc->src;\n");
      else
	fprintf(o, "  uint32 dstaddr = ipc->dst;\n");
      break;
    case dt_Pidx:
      if (type == tp_src) {
	fprintf(o, "  uint32 srcaddr = idxval_src(ipc);\n");
      } else {
	fprintf(o, "  uint32 dstaddr = idxval_dst(ipc);\n");
      }
      break;
    case dt_ImmB:
    case dt_ImmW:
    case dt_ImmL:
    case dt_ImmS:
    case dt_Imm3:
    case dt_Imm4:
    case dt_Imm8:
    case dt_Imm8s:
      /* no address - it is immediate */
      break;
    default:
      fprintf(o, "ERROR\n");
      break;
    }
  }
}

void generate_eaval(FILE *o, t_iib *iib, t_type type)
{
  t_datatype datatype = type ? iib->dtype : iib->stype;

  /* get value in EA */

  switch(datatype) {
  case dt_Dreg:
    switch(iib->size) {
    case sz_byte:
      if (type == tp_src)
	fprintf(o, "  uint8 srcdata = DATAREG(srcreg);\n");
      else
	fprintf(o, "  uint8 dstdata = DATAREG(dstreg);\n");
      break;
    case sz_word:
      if (type == tp_src)
	fprintf(o, "  uint16 srcdata = DATAREG(srcreg);\n");
      else
	fprintf(o, "  uint16 dstdata = DATAREG(dstreg);\n");
      break;
    case sz_long:
      if (type == tp_src)
	fprintf(o, "  uint32 srcdata = DATAREG(srcreg);\n");
      else
	fprintf(o, "  uint32 dstdata = DATAREG(dstreg);\n");
      break;
    default:
      fprintf(o, "ERROR size\n");
      break;
    }
    break;
  case dt_Areg:
    switch(iib->size) {
    case sz_byte:
      if (type == tp_src)
	fprintf(o, "  uint8 srcdata = ADDRREG(srcreg);\n");
      else
	fprintf(o, "  uint8 dstdata = ADDRREG(dstreg);\n");
      break;
    case sz_word:
      if (type == tp_src)
	fprintf(o, "  uint16 srcdata = ADDRREG(srcreg);\n");
      else
	fprintf(o, "  uint16 dstdata = ADDRREG(dstreg);\n");
      break;
    case sz_long:
      if (type == tp_src)
	fprintf(o, "  uint32 srcdata = ADDRREG(srcreg);\n");
      else
	fprintf(o, "  uint32 dstdata = ADDRREG(dstreg);\n");
      break;
    default:
      fprintf(o, "ERROR size\n");
      break;
    }
    break;
  case dt_Aind:
  case dt_Adec:
  case dt_Ainc:
  case dt_Adis:
  case dt_Aidx:
  case dt_AbsW:
  case dt_AbsL:
  case dt_Pdis:
  case dt_Pidx:
    switch(iib->size) {
    case sz_byte:
      if (type == tp_src)
	fprintf(o, "  uint8 srcdata = fetchbyte(srcaddr);\n");
      else
	fprintf(o, "  uint8 dstdata = fetchbyte(dstaddr);\n");
      break;
    case sz_word:
      if (type == tp_src)
	fprintf(o, "  uint16 srcdata = fetchword(srcaddr);\n");
      else
	fprintf(o, "  uint16 dstdata = fetchword(dstaddr);\n");
      break;
    case sz_long:
      if (type == tp_src)
	fprintf(o, "  uint32 srcdata = fetchlong(srcaddr);\n");
      else
	fprintf(o, "  uint32 dstdata = fetchlong(dstaddr);\n");
      break;
    default:
      fprintf(o, "ERROR size\n");
      break;
    }
    break;
  case dt_ImmB:
    if (type == tp_src)
      fprintf(o, "  uint8 srcdata = ipc->src;\n");
    else
      fprintf(o, "  uint8 dstdata = ipc->dst;\n");
    break;
  case dt_ImmW:
    if (type == tp_src)
      fprintf(o, "  uint16 srcdata = ipc->src;\n");
    else
      fprintf(o, "  uint16 dstdata = ipc->dst;\n");
    break;
  case dt_ImmL:
    if (type == tp_src)
      fprintf(o, "  uint32 srcdata = ipc->src;\n");
    else
      fprintf(o, "  uint32 dstdata = ipc->dst;\n");
    break;
  case dt_ImmS:
    if (type == tp_src)
      fprintf(o, "  unsigned int srcdata = %d;\n", iib->immvalue);
    else
      fprintf(o, "  unsigned int dstdata = %d;\n", iib->immvalue);
    break;
  case dt_Imm3:
  case dt_Imm4:
  case dt_Imm8:
    if (type == tp_src)
      fprintf(o, "  unsigned int srcdata = ipc->src;\n");
    else
      fprintf(o, "  unsigned int dstdata = ipc->dst;\n");
    break;
  case dt_Imm8s:
    if (type == tp_src)
      fprintf(o, "  signed int srcdata = ipc->src;\n");
    else
      fprintf(o, "  signed int dstdata = ipc->dst;\n");
    break;
  default:
    fprintf(o, "ERROR\n");
  }
}

void generate_eastore(FILE *o, t_iib *iib, t_type type)
{
  /* get value in EA */

  switch(type == tp_dst ? iib->dtype : iib->stype) {
  case dt_Dreg:
    switch (iib->size) {
    case sz_byte:
      if (type == tp_src)
        fprintf(o, "  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | "
                "outdata;\n");
      else
        fprintf(o, "  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | "
                "outdata;\n");
      break;
    case sz_word:
      if (type == tp_src)
        fprintf(o, "  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xffff) | "
                "outdata;\n");
      else
        fprintf(o, "  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | "
                "outdata;\n");
      break;
    case sz_long:
      if (type == tp_src)
        fprintf(o, "  DATAREG(srcreg) = outdata;\n");
      else
        fprintf(o, "  DATAREG(dstreg) = outdata;\n");
      break;
    default:
      fprintf(o, "ERROR size\n");
      break;
    }
    break;
  case dt_Areg:
    switch(iib->size) {
    case sz_byte:
      if (type == tp_src)
        fprintf(o, "  ADDRREG(srcreg) = (ADDRREG(srcreg) & ~0xff) | "
                "outdata;\n");
      else
        fprintf(o, "  ADDRREG(dstreg) = (ADDRREG(dstreg) & ~0xff) | "
                "outdata;\n");
      break;
    case sz_word:
      if (type == tp_src)
        fprintf(o, "  ADDRREG(srcreg) = (ADDRREG(srcreg) & ~0xffff) | "
                "outdata;\n");
      else
        fprintf(o, "  ADDRREG(dstreg) = (ADDRREG(dstreg) & ~0xffff) | "
                "outdata;\n");
      break;
    case sz_long:
      if (type == tp_src)
        fprintf(o, "  ADDRREG(srcreg) = outdata;\n");
      else
        fprintf(o, "  ADDRREG(dstreg) = outdata;\n");
      break;
    default:
      fprintf(o, "ERROR size\n");
      break;
    }
    break;
  case dt_Adec:
    switch(iib->size) {
    case sz_byte:
      if (type == tp_src)
        fprintf(o, "  storebyte(srcaddr, outdata);\n");
      else
        fprintf(o, "  storebyte(dstaddr, outdata);\n");
      break;
    case sz_word:
      if (type == tp_src)
        fprintf(o, "  storeword(srcaddr, outdata);\n");
      else
        fprintf(o, "  storeword(dstaddr, outdata);\n");
      break;
    case sz_long:
      fprintf(o, "  /* pre-decrement long store must write low 16 bits\n"
                 "     in -2 first, then upper 16 bits in -4 second */\n");
      if (type == tp_src) {
        fprintf(o, "  storeword(srcaddr + 2, (uint16)outdata);\n");
        fprintf(o, "  storeword(srcaddr, (uint16)(outdata >> 16));\n");
      } else {
        fprintf(o, "  storeword(dstaddr + 2, (uint16)outdata);\n");
        fprintf(o, "  storeword(dstaddr, (uint16)(outdata >> 16));\n");
      }
      break;
    default:
      fprintf(o, "ERROR size\n");
    }
    break;
  case dt_Aind:
  case dt_Ainc:
  case dt_Adis:
  case dt_Aidx:
  case dt_AbsW:
  case dt_AbsL:
  case dt_Pdis:
  case dt_Pidx:
    switch(iib->size) {
    case sz_byte:
      if (type == tp_src)
        fprintf(o, "  storebyte(srcaddr, outdata);\n");
      else
        fprintf(o, "  storebyte(dstaddr, outdata);\n");
      break;
    case sz_word:
      if (type == tp_src)
        fprintf(o, "  storeword(srcaddr, outdata);\n");
      else
        fprintf(o, "  storeword(dstaddr, outdata);\n");
      break;
    case sz_long:
      if (type == tp_src)
        fprintf(o, "  storelong(srcaddr, outdata);\n");
      else
        fprintf(o, "  storelong(dstaddr, outdata);\n");
      break;
    default:
      fprintf(o, "ERROR size\n");
    }
    break;
  default:
    fprintf(o, "ERROR type\n");
  }
}

void generate_outdata(FILE *o, t_iib *iib, const char *init)
{
  switch(iib->size) {
  case sz_byte:
    fprintf(o, "  uint8 ");
    break;
  case sz_word:
    fprintf(o, "  uint16 ");
    break;
  case sz_long:
    fprintf(o, "  uint32 ");
    break;
  default:
    fprintf(o, "ERROR size\n");
    break;
  }
  fprintf(o, "outdata%s%s;\n", (init && init[0]) ? " = " : "",
	  init ? init : "");
}

void generate_cc(FILE *o, t_iib *iib)
{
  switch(iib->cc) {
  case 0:                                                     /*  T */
    fprintf(o, "  uint8 cc = 1;\n");
    break;
  case 1:                                                     /*  F */
    fprintf(o, "  uint8 cc = 0;\n");
    break;
  case 2:                                                     /* HI */
    fprintf(o, "  uint8 cc = !(CFLAG || ZFLAG);\n");
    break;
  case 3:                                                     /* LS */
    fprintf(o, "  uint8 cc = CFLAG || ZFLAG;\n");
    break;
  case 4:                                                     /* CC */
    fprintf(o, "  uint8 cc = !CFLAG;\n");
    break;
  case 5:                                                     /* CS */
    fprintf(o, "  uint8 cc = CFLAG;\n");
    break;
  case 6:                                                     /* NE */
    fprintf(o, "  uint8 cc = !ZFLAG;\n");
    break;
  case 7:                                                     /* EQ */
    fprintf(o, "  uint8 cc = ZFLAG;\n");
    break;
  case 8:                                                     /* VC */
    fprintf(o, "  uint8 cc = !VFLAG;\n");
    break;
  case 9:                                                     /* VS */
    fprintf(o, "  uint8 cc = VFLAG;\n");
    break;
  case 10:                                                    /* PL */
    fprintf(o, "  uint8 cc = !NFLAG;\n");
    break;
  case 11:                                                    /* MI */
    fprintf(o, "  uint8 cc = NFLAG;\n");
    break;
  case 12:                                                    /* GE */
    fprintf(o, "  uint8 cc = (NFLAG == VFLAG);\n");
    break;
  case 13:                                                    /* LT */
    fprintf(o, "  uint8 cc = (NFLAG != VFLAG);\n");
    break;
  case 14:                                                    /* GT */
    fprintf(o, "  uint8 cc = !ZFLAG && (NFLAG == VFLAG);\n");
    break;
  case 15:                                                    /* LE */
    fprintf(o, "  uint8 cc = ZFLAG || (NFLAG != VFLAG);\n");
    break;
  default:
    fprintf(o, "ERROR cc\n");
    break;
  }
}

void generate_stdflag_n(FILE *o, t_iib *iib)
{
  switch(iib->size) {
  case sz_byte:
    fprintf(o, "  NFLAG = ((sint8)outdata) < 0;\n");
    break;
  case sz_word:
    fprintf(o, "  NFLAG = ((sint16)outdata) < 0;\n");
    break;
  case sz_long:
    fprintf(o, "  NFLAG = ((sint32)outdata) < 0;\n");
    break;
  default:
    fprintf(o, "ERROR size\n");
    break;
  }
}

void generate_stdflag_z(FILE *o, t_iib *iib)
{
  (void)iib;
  fprintf(o, "  ZFLAG = !outdata;\n");
}

void generate_clrflag_v(FILE *o, t_iib *iib)
{
  (void)iib;
  fprintf(o, "  VFLAG = 0;\n");
}

void generate_clrflag_c(FILE *o, t_iib *iib)
{
  (void)iib;
  fprintf(o, "  CFLAG = 0;\n");
}

void generate_clrflag_n(FILE *o, t_iib *iib)
{
  (void)iib;
  fprintf(o, "  NFLAG = 0;\n");
}

void generate_setflag_z(FILE *o, t_iib *iib)
{
  (void)iib;
  fprintf(o, "  ZFLAG = 1;\n");
}

void generate_subflag_c(FILE *o, t_iib *iib)
{
  /* C = (Sm && !Dm) || (Rm && !Dm) || (Sm && Rm)
     carry is performed as if both source and destination are unsigned,
     so this is simply if the source is greater than the destination - I
     have proven this to be the case using a truth table and the above
     motorola definition */
  (void)iib;
  fprintf(o, "  CFLAG = srcdata > dstdata;\n");
}

void generate_subflag_cx(FILE *o, t_iib *iib)
{
  /* C = (Sm && !Dm) || (Rm && !Dm) || (Sm && Rm)
     carry is performed as if both source and destination are unsigned,
     so this is simply if the source is greater than the destination - I
     have proven this to be the case using a truth table and the above
     motorola definition */
  (void)iib;
  fprintf(o, "  XFLAG = CFLAG = srcdata > dstdata;\n");
}

void generate_subxflag_cx(FILE *o, t_iib *iib)
{
  /* V = (Sm && !Dm) || (Rm && (!Dm || Sm)) */
  fprintf(o, "  {\n");
  switch(iib->size) {
  case sz_byte:
    fprintf(o, "    int Sm = (sint8)srcdata < 0;\n");
    fprintf(o, "    int Dm = (sint8)dstdata < 0;\n");
    fprintf(o, "    int Rm = (sint8)outdata < 0;\n");
    break;
  case sz_word:
    fprintf(o, "    int Sm = (sint16)srcdata < 0;\n");
    fprintf(o, "    int Dm = (sint16)dstdata < 0;\n");
    fprintf(o, "    int Rm = (sint16)outdata < 0;\n");
    break;
  case sz_long:
    fprintf(o, "    int Sm = (sint32)srcdata < 0;\n");
    fprintf(o, "    int Dm = (sint32)dstdata < 0;\n");
    fprintf(o, "    int Rm = (sint32)outdata < 0;\n");
    break;
  default:
    fprintf(o, "ERROR size\n");
    break;
  }
  fprintf(o, "    XFLAG = CFLAG = (Sm && !Dm) || (Rm && (!Dm || Sm));\n");
  fprintf(o, "  }\n");
}

void generate_cmpaflag_c(FILE *o, t_iib *iib)
{
  /* see generate_subflag_c - this is just the same but with a sign extend
     on the source */
  (void)iib;
  fprintf(o, "  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;\n");
}

void generate_subflag_v(FILE *o, t_iib *iib)
{
  /* V = (!Sm && Dm && !Rm) || (Sm && !Dm && Rm)
     overflow is performed as if both source and destination are signed,
     the only two condtions are if we've added too much to a +ve number
     or we've subtracted too much from a -ve number - I have proven the
     this to be the case using a truth table and the above motorola
     definition */
  /* the technique to implement the above formula is to make sure the sign
     of Sm != the sign of Dm, and the sign of Dm != the sign of Rm. */
  switch(iib->size) {
  case sz_byte:
    fprintf(o, "  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) ");
    fprintf(o, "&&\n    (((sint8)dstdata < 0) != ((sint8)outdata < 0));\n");
    break;
  case sz_word:
    fprintf(o, "  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) ");
    fprintf(o, "&&\n    (((sint16)dstdata < 0) != ((sint16)outdata < 0));\n");
    break;
  case sz_long:
    fprintf(o, "  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) ");
    fprintf(o, "&&\n    (((sint32)dstdata < 0) != ((sint32)outdata < 0));\n");
    break;
  default:
    fprintf(o, "ERROR size\n");
    break;
  }
}

void generate_cmpaflag_v(FILE *o, t_iib *iib)
{
  /* see generate_subflag_v - this is just the sz_long version with a sign
     extend on the source */
  (void)iib;
  fprintf(o, "  VFLAG = (((sint32)(sint16)srcdata < 0) != ");
  fprintf(o, "((sint32)dstdata < 0)) ");
  fprintf(o, "&&\n    (((sint32)dstdata < 0) != ((sint32)outdata < 0));\n");
}

void generate_stdxflag_z(FILE *o, t_iib *iib)
{
  (void)iib;
  fprintf(o, "  if (outdata) ZFLAG = 0;\n");
}

void generate_addflag_cx(FILE *o, t_iib *iib)
{
  /* C = (Sm && Dm) || (!Rm && Dm) || (Sm && !Rm)
     carry is performed as if both source and destination are unsigned,
     so this is simply if the source is bigger than how much can be added
     to the destination without wrapping - this is of course just the bit
     inverse of the destination. */
  /* 0xFFFF - dstdata is obviously just ~dstdata, but try and get the
     compiler to do that without producing a warning, go on... */
  switch (iib->size) {
  case sz_byte:
    fprintf(o, "  XFLAG = CFLAG = srcdata > (0xFFu - (uint8)dstdata);\n");
    break;
  case sz_word:
    fprintf(o, "  XFLAG = CFLAG = srcdata > (0xFFFFu - (uint16)dstdata);\n");
    break;
  case sz_long:
    fprintf(o, "  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;\n");
    break;
  default:
    fprintf(o, "ERROR size\n");
    break;
  }
  /* gcc doesn't like a one's compliment of an unsigned - take out the
     casts in the above and it will warn you incorrectly */
}

void generate_addxflag_cx(FILE *o, t_iib *iib)
{
  /* V = (Sm && Dm) || (!Rm && (Dm || Sm)) */
  fprintf(o, "  {\n");
  switch(iib->size) {
  case sz_byte:
    fprintf(o, "    int Sm = (sint8)srcdata < 0;\n");
    fprintf(o, "    int Dm = (sint8)dstdata < 0;\n");
    fprintf(o, "    int Rm = (sint8)outdata < 0;\n");
    break;
  case sz_word:
    fprintf(o, "    int Sm = (sint16)srcdata < 0;\n");
    fprintf(o, "    int Dm = (sint16)dstdata < 0;\n");
    fprintf(o, "    int Rm = (sint16)outdata < 0;\n");
    break;
  case sz_long:
    fprintf(o, "    int Sm = (sint32)srcdata < 0;\n");
    fprintf(o, "    int Dm = (sint32)dstdata < 0;\n");
    fprintf(o, "    int Rm = (sint32)outdata < 0;\n");
    break;
  default:
    fprintf(o, "ERROR size\n");
    break;
  }
  fprintf(o, "    XFLAG = CFLAG = (Sm && Dm) || (!Rm && (Dm || Sm));\n");
  fprintf(o, "  }\n");
}

void generate_addflag_v(FILE *o, t_iib *iib)
{
  /* V = (Sm && Dm && !Rm) || (!Sm && !Dm && Rm)
     overflow is performed as if both source and destination are signed,
     the only two condtions are if we've added too much to a +ve number
     or we've subtracted too much from a -ve number */
  /* the technique to implement the above formula is to make sure the sign
     of Sm == the sign of Dm, and the sign of Dm != the sign of Rm. */
  switch(iib->size) {
  case sz_byte:
    fprintf(o, "  VFLAG = (((sint8)srcdata < 0) == ((sint8)dstdata < 0)) ");
    fprintf(o, "&&\n    (((sint8)dstdata < 0) != ((sint8)outdata < 0));\n");
    break;
  case sz_word:
    fprintf(o, "  VFLAG = (((sint16)srcdata < 0) == ((sint16)dstdata < 0)) ");
    fprintf(o, "&&\n    (((sint16)dstdata < 0) != ((sint16)outdata < 0));\n");
    break;
  case sz_long:
    fprintf(o, "  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) ");
    fprintf(o, "&&\n    (((sint32)dstdata < 0) != ((sint32)outdata < 0));\n");
    break;
  default:
    fprintf(o, "ERROR size\n");
    break;
  }
}

void generate_negflag_cx(FILE *o, t_iib *iib)
{
  (void)iib;
  /* C = (Dm || Rm) */
  fprintf(o, "  XFLAG = CFLAG = srcdata ? 1 : 0;\n");
}

void generate_negflag_v(FILE *o, t_iib *iib)
{
  /* V = (Dm && Rm)
     which is the same as V = src == 1<<15 as the only case where both the
     data and the result are both negative is in the case of the extreme
     negative number (1<<15 in the case of 16 bit) since 0 - 1<<15 = 1<<15. */
  switch(iib->size) {
  case sz_byte:
    fprintf(o, "  VFLAG = (srcdata == (1u<<7));\n");
    break;
  case sz_word:
    fprintf(o, "  VFLAG = (srcdata == (1u<<15));\n");
    break;
  case sz_long:
    fprintf(o, "  VFLAG = (srcdata == (1u<<31));\n");
    break;
  default:
    fprintf(o, "ERROR size\n");
    break;
  }
}

void generate_negxflag_cx(FILE *o, t_iib *iib)
{
  /* C = (Dm || Rm)
     unlike NEG, NEGX is done properly */
  fprintf(o, "  XFLAG = CFLAG ");
  switch(iib->size) {
  case sz_byte:
    fprintf(o, "= ((sint8)srcdata < 0) || ((sint8)outdata < 0);\n");
    break;
  case sz_word:
    fprintf(o, "= ((sint16)srcdata < 0) || ((sint16)outdata < 0);\n");
    break;
  case sz_long:
    fprintf(o, "= ((sint32)srcdata < 0) || ((sint32)outdata < 0);\n");
    break;
  default:
    fprintf(o, "ERROR size\n");
    break;
  }
}

void generate_negxflag_v(FILE *o, t_iib *iib)
{
  /* V = (Dm && Rm)
     unlike NEG, NEGX is done properly */
  switch(iib->size) {
  case sz_byte:
    fprintf(o, "  VFLAG = ((sint8)srcdata < 0) && ((sint8)outdata < 0);\n");
    break;
  case sz_word:
    fprintf(o, "  VFLAG = ((sint16)srcdata < 0) && ((sint16)outdata < 0);\n");
    break;
  case sz_long:
    fprintf(o, "  VFLAG = ((sint32)srcdata < 0) && ((sint32)outdata < 0);\n");
    break;
  default:
    fprintf(o, "ERROR size\n");
    break;
  }
}

void generate_bits(FILE *o, t_iib *iib)
{
  switch (iib->size) {
  case sz_byte:
    fprintf(o, "  uint8 bits = 8;\n");
    break;
  case sz_word:
    fprintf(o, "  uint8 bits = 16;\n");
    break;
  case sz_long:
    fprintf(o, "  uint8 bits = 32;\n");
    break;
  default:
    fprintf(o, "ERROR size\n");
    break;
  }
}
