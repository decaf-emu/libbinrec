/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "src/endian.h"
#include "src/guest-ppc/guest-ppc-decode.h"
#include "src/guest-ppc/guest-ppc-internal.h"

/*************************************************************************/
/************************ Configuration settings *************************/
/*************************************************************************/

/**
 * BLOCKS_EXPAND_SIZE:  Number of entries by which to expand the block
 * table when full.
 */
#ifndef BLOCKS_EXPAND_SIZE
    #define BLOCKS_EXPAND_SIZE  32
#endif

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * get_block:  Return the basic block entry for the given block start
 * address, creating a new entry if one does not yet exist.  Returns
 * NULL if a new entry must be created but memory allocation fails.
 *
 * The returned pointer is only valid until the next successful get_block()
 * call.
 */
static GuestPPCBlockInfo *get_block(GuestPPCContext *ctx, uint32_t address)
{
    ASSERT(ctx);

    int low = 0, high = ctx->num_blocks - 1;  // Binary search
    while (low <= high) {
        const int mid = (low + high) / 2;
        if (address == ctx->blocks[mid].start) {
            return &ctx->blocks[mid];
        } else if (address < ctx->blocks[mid].start) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    if (ctx->num_blocks >= ctx->blocks_size) {
        const int new_size = ctx->blocks_size + BLOCKS_EXPAND_SIZE;
        GuestPPCBlockInfo *new_blocks = binrec_realloc(
            ctx->handle, ctx->blocks, sizeof(*ctx->blocks) * new_size);
        if (UNLIKELY(!new_blocks)) {
            return false;
        }
        ctx->blocks = new_blocks;
        ctx->blocks_size = new_size;
    }

    GuestPPCBlockInfo *block = &ctx->blocks[low];
    memmove(block+1, block, sizeof(*ctx->blocks) * (ctx->num_blocks - low));
    ctx->num_blocks++;
    memset(block, 0, sizeof(*block));
    block->start = address;
    return block;
}

/*-----------------------------------------------------------------------*/

/*
 * Helper functions for update_used_changed() to set register used/changed
 * bits.  The mark_*_used functions mark a register as used only if the
 * same register has not been set in the block (since if it has been set,
 * the value being read is that value, not the value on entry to the block).
 */

static inline void mark_gpr_used(GuestPPCBlockInfo *block, const int index) {
    if (!(block->gpr_changed & (1 << index))) block->gpr_used |= 1 << index;
}

static inline void mark_gpr_changed(GuestPPCBlockInfo *block, const int index) {
    block->gpr_changed |= 1 << index;
}

static inline void mark_fpr_used(GuestPPCBlockInfo *block, const int index) {
    if (!(block->fpr_changed & (1 << index))) block->fpr_used |= 1 << index;
}

static inline void mark_fpr_changed(GuestPPCBlockInfo *block, const int index) {
    block->fpr_changed |= 1 << index;
}

static inline void mark_cr_used(GuestPPCBlockInfo *block, const int index) {
    if (!(block->cr_changed & (1 << index))) block->cr_used |= 1 << index;
}

static inline void mark_cr_changed(GuestPPCBlockInfo *block, const int index) {
    block->cr_changed |= 1 << index;
}

static inline void mark_lr_used(GuestPPCBlockInfo *block) {
    if (!block->lr_changed) block->lr_used = 1;
}

static inline void mark_lr_changed(GuestPPCBlockInfo *block) {
    block->lr_changed = 1;
}

static inline void mark_ctr_used(GuestPPCBlockInfo *block) {
    if (!block->ctr_changed) block->ctr_used = 1;
}

static inline void mark_ctr_changed(GuestPPCBlockInfo *block) {
    block->ctr_changed = 1;
}

static inline void mark_xer_used(GuestPPCBlockInfo *block) {
    if (!block->xer_changed) block->xer_used = 1;
}

static inline void mark_xer_changed(GuestPPCBlockInfo *block) {
    block->xer_changed = 1;
}

static inline void mark_fpscr_used(GuestPPCBlockInfo *block) {
    if (!block->fpscr_changed) block->fpscr_used = 1;
}

static inline void mark_fpscr_changed(GuestPPCBlockInfo *block) {
    block->fpscr_changed = 1;
}

/*-----------------------------------------------------------------------*/

/**
 * update_used_changed:  Update the register used and changed data in the
 * given block based on the given instruction.  The instruction is assumed
 * to be valid.
 */
static void update_used_changed(GuestPPCContext *ctx, GuestPPCBlockInfo *block,
                                const uint32_t insn)
{
    switch (insn_OPCD(insn)) {
      case OPCD_SC:
        /* No registers touched. */
        break;

      case OPCD_TWI:
        mark_gpr_used(block, insn_rA(insn));
        break;

      case OPCD_BC:
        if (!(insn_BO(insn) & 0x04)) {
            mark_ctr_used(block);
            mark_ctr_changed(block);
        }
        if (!(insn_BO(insn) & 0x10)) {
            mark_cr_used(block, insn_BI(insn) >> 2);
        }
        /* fall through */
      case OPCD_B:
        if (insn_LK(insn)) {
            mark_lr_changed(block);
        }
        break;

      case OPCD_CMPLI:
      case OPCD_CMPI:
        mark_gpr_used(block, insn_rA(insn));
        mark_xer_used(block);
        mark_cr_changed(block, insn_crfD(insn));
        break;

      case OPCD_ADDIC_:
        mark_cr_changed(block, 0);
        /* fall through */
      case OPCD_SUBFIC:
      case OPCD_ADDIC:
        mark_xer_used(block);
        mark_xer_changed(block);
        /* fall through */
      case OPCD_MULLI:
        mark_gpr_used(block, insn_rA(insn));
        mark_gpr_changed(block, insn_rD(insn));
        break;

      case OPCD_ADDIS:
        if (insn_rA(insn) != 0) {
            mark_gpr_used(block, insn_rA(insn));
        }
        mark_gpr_changed(block, insn_rD(insn));
        break;

      case OPCD_ORI:
        if (insn == 0x60000000) {  // nop
            break;
        }
        /* fall through */
      case OPCD_ORIS:
      case OPCD_XORI:
      case OPCD_XORIS:
        mark_gpr_used(block, insn_rS(insn));
        mark_gpr_changed(block, insn_rA(insn));
        break;

      case OPCD_ANDI_:
      case OPCD_ANDIS_:
        mark_xer_used(block);
        mark_cr_changed(block, 0);
        mark_gpr_used(block, insn_rS(insn));
        mark_gpr_changed(block, insn_rA(insn));
        break;

      case OPCD_ADDI:
      case OPCD_LWZ:
      case OPCD_LBZ:
      case OPCD_LHZ:
      case OPCD_LHA:
        if (insn_rA(insn) != 0) {
            mark_gpr_used(block, insn_rA(insn));
        }
        mark_gpr_changed(block, insn_rD(insn));
        break;

      case OPCD_RLWIMI:
        mark_gpr_used(block, insn_rA(insn));
        /* fall through */
      case OPCD_RLWINM:
        mark_gpr_used(block, insn_rS(insn));
        mark_gpr_changed(block, insn_rA(insn));
        if (insn_Rc(insn)) {
            mark_xer_used(block);
            mark_cr_changed(block, 0);
        }
        break;

      case OPCD_LWZU:
      case OPCD_LBZU:
      case OPCD_LHZU:
      case OPCD_LHAU:
        mark_gpr_used(block, insn_rA(insn));
        mark_gpr_changed(block, insn_rA(insn));
        mark_gpr_changed(block, insn_rD(insn));
        break;

      case OPCD_STW:
      case OPCD_STB:
      case OPCD_STH:
        if (insn_rA(insn) != 0) {
            mark_gpr_used(block, insn_rA(insn));
        }
        mark_gpr_used(block, insn_rD(insn));
        break;

      case OPCD_STWU:
      case OPCD_STBU:
      case OPCD_STHU:
        mark_gpr_used(block, insn_rA(insn));
        mark_gpr_used(block, insn_rD(insn));
        mark_gpr_changed(block, insn_rA(insn));
        break;

      case OPCD_RLWNM:
        mark_gpr_used(block, insn_rS(insn));
        mark_gpr_used(block, insn_rB(insn));
        mark_gpr_changed(block, insn_rA(insn));
        if (insn_Rc(insn)) {
            mark_xer_used(block);
            mark_cr_changed(block, 0);
        }
        break;

      case OPCD_LMW:
        if (insn_rA(insn)) {
            mark_gpr_used(block, insn_rA(insn));
        }
        for (int i = insn_rD(insn); i < 32; i++) {
            mark_gpr_changed(block, i);
        }
        break;

      case OPCD_STMW:
        if (insn_rA(insn)) {
            mark_gpr_used(block, insn_rA(insn));
        }
        for (int i = insn_rD(insn); i < 32; i++) {
            mark_gpr_used(block, i);
        }
        break;

      case OPCD_LFS:
      case OPCD_LFD:
      case OPCD_PSQ_L:
        if (insn_rA(insn)) {
            mark_gpr_used(block, insn_rA(insn));
        }
        mark_fpr_changed(block, insn_frD(insn));
        break;

      case OPCD_LFSU:
      case OPCD_LFDU:
      case OPCD_PSQ_LU:
        mark_gpr_used(block, insn_rA(insn));
        mark_gpr_changed(block, insn_rA(insn));
        mark_fpr_changed(block, insn_frD(insn));
        break;

      case OPCD_STFS:
      case OPCD_STFD:
      case OPCD_PSQ_ST:
        if (insn_rA(insn)) {
            mark_gpr_used(block, insn_rA(insn));
        }
        mark_fpr_used(block, insn_frD(insn));
        break;

      case OPCD_STFSU:
      case OPCD_STFDU:
      case OPCD_PSQ_STU:
        mark_gpr_used(block, insn_rA(insn));
        mark_fpr_used(block, insn_frD(insn));
        mark_gpr_changed(block, insn_rA(insn));
        break;

      case OPCD_x04:
        switch (insn_XO_5(insn)) {
          case 0x00:  // ps_cmp*
            mark_fpscr_used(block);
            mark_fpr_used(block, insn_frA(insn));
            mark_fpr_used(block, insn_frB(insn));
            mark_fpscr_changed(block);
            mark_cr_used(block, insn_crfD(insn));  // FIXME: temp until these insns are implemented, to avoid uninitialized values in CR merging
            mark_cr_changed(block, insn_crfD(insn));
            break;
          case 0x06:  // psq_lx, psq_lux
            if (insn_rA(insn)) {
                mark_gpr_used(block, insn_rA(insn));
            }
            mark_gpr_used(block, insn_rB(insn));
            if (insn_XO_10(insn) & 0x020) {
                mark_gpr_changed(block, insn_rA(insn));
            }
            mark_fpr_changed(block, insn_frD(insn));
            break;
          case 0x07:  // psq_stx, psq_stux
            if (insn_rA(insn)) {
                mark_gpr_used(block, insn_rA(insn));
            }
            mark_gpr_used(block, insn_rB(insn));
            mark_fpr_used(block, insn_frD(insn));
            if (insn_XO_10(insn) & 0x020) {
                mark_gpr_changed(block, insn_rA(insn));
            }
            break;
          case 0x08:  // ps_mr, etc.
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_changed(block, insn_frD(insn));
            if (insn_Rc(insn)) {
                mark_fpscr_used(block);
                mark_cr_changed(block, 1);
            }
            break;
          case 0x10:  // ps_merge*
            mark_fpr_used(block, insn_frA(insn));
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_changed(block, insn_frD(insn));
            if (insn_Rc(insn)) {
                mark_fpscr_used(block);
                mark_cr_changed(block, 1);
            }
            break;
            mark_fpscr_used(block);
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_changed(block, insn_frD(insn));
            mark_fpscr_changed(block);
            if (insn_Rc(insn)) {
                mark_cr_changed(block, 1);
            }
            break;
          case 0x16:  // dcbz_l
            if (insn_rA(insn) != 0) {
                mark_gpr_used(block, insn_rA(insn));
            }
            mark_gpr_used(block, insn_rB(insn));
            break;
          case XO_PS_DIV:
          case XO_PS_SUB:
          case XO_PS_ADD:
            mark_fpscr_used(block);
            mark_fpr_used(block, insn_frA(insn));
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_changed(block, insn_frD(insn));
            mark_fpscr_changed(block);
            if (insn_Rc(insn)) {
                mark_cr_changed(block, 1);
            }
            break;
          case XO_PS_SEL:
            mark_fpr_used(block, insn_frA(insn));
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_used(block, insn_frC(insn));
            mark_fpr_changed(block, insn_frD(insn));
            if (insn_Rc(insn)) {
                mark_fpscr_used(block);
                mark_cr_changed(block, 1);
            }
            break;
          case XO_PS_RES:
          case XO_PS_RSQRTE:
            mark_fpscr_used(block);
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_changed(block, insn_frD(insn));
            mark_fpscr_changed(block);
            if (insn_Rc(insn)) {
                mark_cr_changed(block, 1);
            }
            break;
          case XO_PS_MULS0:
          case XO_PS_MULS1:
          case XO_PS_MUL:
            mark_fpscr_used(block);
            mark_fpr_used(block, insn_frA(insn));
            mark_fpr_used(block, insn_frC(insn));
            mark_fpr_changed(block, insn_frD(insn));
            mark_fpscr_changed(block);
            if (insn_Rc(insn)) {
                mark_cr_changed(block, 1);
            }
            break;
          case XO_PS_SUM0:
          case XO_PS_SUM1:
          case XO_PS_MADDS0:
          case XO_PS_MADDS1:
          case XO_PS_MSUB:
          case XO_PS_MADD:
          case XO_PS_NMSUB:
          case XO_PS_NMADD:
            mark_fpscr_used(block);
            mark_fpr_used(block, insn_frA(insn));
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_used(block, insn_frC(insn));
            mark_fpr_changed(block, insn_frD(insn));
            mark_fpscr_changed(block);
            if (insn_Rc(insn)) {
                mark_cr_changed(block, 1);
            }
            break;
          default:
            ASSERT(!"impossible");
        }
        break;

      case OPCD_x13:
        switch (insn_XO_5(insn)) {
          case 0x00:  // mcrf
            mark_cr_used(block, insn_crfS(insn));
            mark_cr_changed(block, insn_crfD(insn));
            break;

          case 0x01:  // crand, etc.
            /* No dependency on crbA/crbB for crclr and crset. */
            if (!((insn_XO_10(insn) == XO_CRXOR || insn_XO_10(insn) == XO_CREQV)
                  && insn_crbA(insn) == insn_crbB(insn))) {
                mark_cr_used(block, insn_crbA(insn) >> 2);
                mark_cr_used(block, insn_crbB(insn) >> 2);
            }
            mark_cr_used(block, insn_crbD(insn) >> 2);
            mark_cr_changed(block, insn_crbD(insn) >> 2);
            break;

          case 0x10:  // bclr/bcctr
            if (insn_XO_10(insn) & 0x200) {  // bcctr
                mark_ctr_used(block);
                // FIXME: try to detect jump tables
            } else {  // bclr
                mark_lr_used(block);
                if (!(insn_BO(insn) & 0x04)) {
                    mark_ctr_used(block);
                    mark_ctr_changed(block);
                }
            }
            if (!(insn_BO(insn) & 0x10)) {
                mark_cr_used(block, insn_BI(insn) >> 2);
            }
            if (insn_LK(insn)) {
                mark_lr_changed(block);
            }
            break;

          case 0x12:  // rfi
          case 0x16:  // isync
            break;  // No user-level registers modified

          default:
            ASSERT(!"impossible");
        }
        break;

      case OPCD_x1F:
        switch (insn_XO_5(insn)) {
          case 0x00:  // cmp, cmpl, mcrxr
            mark_xer_used(block);
            if (insn_XO_10(insn) == XO_MCRXR) {
                mark_xer_changed(block);
            } else {
                mark_gpr_used(block, insn_rA(insn));
                mark_gpr_used(block, insn_rB(insn));
            }
            mark_cr_changed(block, insn_crfD(insn));
            break;

          case 0x04:  // tw
            mark_gpr_used(block, insn_rA(insn));
            mark_gpr_used(block, insn_rB(insn));
            break;

          case 0x08:  // sub*
          case 0x0A:  // add*
            mark_gpr_used(block, insn_rA(insn));
            if ((insn_XO_10(insn) & 0x1C0) != 0x0C0  // sub/addze, sub/addme
             && (insn_XO_10(insn) & 0x1FF) != XO_NEG) {
                mark_gpr_used(block, insn_rB(insn));
            }
            mark_gpr_changed(block, insn_rD(insn));
            if (insn_OE(insn) || !(insn_XO_10(insn) == XO_SUBF
                               || insn_XO_10(insn) == XO_NEG
                               || insn_XO_10(insn) == XO_ADD)) {
                mark_xer_used(block);
                mark_xer_changed(block);
            }
            if (insn_Rc(insn)) {
                mark_xer_used(block);
                mark_cr_changed(block, 0);
            }
            break;

          case 0x0B:  // mul*, div*
            mark_gpr_used(block, insn_rA(insn));
            mark_gpr_used(block, insn_rB(insn));
            mark_gpr_changed(block, insn_rD(insn));
            if (insn_OE(insn)) {
                mark_xer_used(block);
                mark_xer_changed(block);
            }
            if (insn_Rc(insn)) {
                mark_xer_used(block);
                mark_cr_changed(block, 0);
            }
            break;

          case 0x10:  // mtcrf
            mark_gpr_used(block, insn_rS(insn));
            /* CRM==0xFF is handled specially, see guest-ppc-rtl.c. */
            if (insn_CRM(insn) != 0xFF) {
                for (int i = 0; i < 8; i++) {
                    if (insn_CRM(insn) & (1 << (7 - i))) {
                        mark_cr_changed(block, i);
                    }
                }
            }
            break;

          case 0x12:  // mtmsr, mtsr, mtsrin, tlbie
            if (insn_XO_10(insn) != XO_TLBIE) {
                mark_gpr_used(block, insn_rS(insn));
            }
            if (insn_XO_10(insn) == XO_MTSRIN || insn_XO_10(insn) == XO_TLBIE) {
                mark_gpr_used(block, insn_rB(insn));
            }
            break;

          case 0x13:  // mf*r, mtspr
            if (insn_XO_10(insn) == XO_MTSPR) {
                mark_gpr_used(block, insn_rS(insn));
            } else {
                if (insn_XO_10(insn) == XO_MFCR) {
                    /* mfcr has special handling to load unused fields
                     * directly from the PSB, so we don't need to mark
                     * any fields here. */
                } else if (insn_XO_10(insn) == XO_MFSRIN) {
                    mark_gpr_used(block, insn_rB(insn));
                }
                mark_gpr_changed(block, insn_rD(insn));
            }
            if (insn_XO_10(insn) == XO_MFSPR) {
                const int spr = insn_spr(insn);
                if (spr == SPR_XER) {
                    mark_xer_used(block);
                } else if (spr == SPR_LR) {
                    mark_lr_used(block);
                } else if (spr == SPR_CTR) {
                    mark_ctr_used(block);
                }
            } else if (insn_XO_10(insn) == XO_MTSPR) {
                const int spr = insn_spr(insn);
                if (spr == SPR_XER) {
                    mark_xer_changed(block);
                } else if (spr == SPR_LR) {
                    mark_lr_changed(block);
                } else if (spr == SPR_CTR) {
                    mark_ctr_changed(block);
                }
            }
            break;

          case 0x14:  // lwarx
            if (insn_rA(insn) != 0) {
                mark_gpr_used(block, insn_rA(insn));
            }
            mark_gpr_used(block, insn_rB(insn));
            mark_gpr_changed(block, insn_rD(insn));
            break;

          case 0x15: {  // lsw*, stsw*
            const bool is_immediate = (insn_XO_10(insn) & 0x040) != 0;
            const bool is_store = (insn_XO_10(insn) & 0x080) != 0;
            if (insn_rA(insn)) {
                mark_gpr_used(block, insn_rA(insn));
            }
            if (is_immediate) {
                const int n = insn_NB(insn) ? insn_NB(insn) : 32;
                int rD = insn_rD(insn);
                for (int i = 0; i < n; i += 4, rD = (rD + 1) & 31) {
                    if (is_store) {
                        mark_gpr_used(block, rD);
                    } else {
                        mark_gpr_changed(block, rD);
                    }
                }
            } else {
                mark_gpr_used(block, insn_rB(insn));
                mark_xer_used(block);
                /* We have no way to tell at this point how many bytes
                 * will be transferred, so we essentially have to say
                 * "we no longer have any idea about GPRs in this block".
                 * We mark all GPRs as used so their values are available
                 * if needed, then for lswx we mark all GPRs as changed to
                 * ensure that whatever registers were modified get written
                 * back to the processor state block.  RTL optimization
                 * should be able to clear out some of the unnecessary
                 * loads and stores, but this is probably an example of
                 * how (from the PowerPC manual) "this instruction is
                 * likely to ... take longer to execute, perhaps much
                 * longer, than a sequence of individual load or store
                 * instructions that produce the same results." */
                for (int i = 0; i < 32; i++) {
                    mark_gpr_used(block, i);
                    if (!is_store) {
                        mark_gpr_changed(block, i);
                    }
                }
            }
            break;
          }

          case 0x16:  // Miscellaneous instructions
            switch (insn_XO_10(insn)) {
              case XO_DCBST:
              case XO_DCBF:
              case XO_DCBTST:
              case XO_DCBT:
              case XO_DCBI:
              case XO_ICBI:
              case XO_DCBZ:
                if (insn_rA(insn) != 0) {
                    mark_gpr_used(block, insn_rA(insn));
                }
                mark_gpr_used(block, insn_rB(insn));
                break;
              case XO_STWCX_:
                if (insn_rA(insn) != 0) {
                    mark_gpr_used(block, insn_rA(insn));
                }
                mark_gpr_used(block, insn_rB(insn));
                mark_gpr_used(block, insn_rS(insn));
                mark_xer_used(block);
                mark_cr_changed(block, 0);
                break;
              case XO_ECIWX:
              case XO_LWBRX:
              case XO_LHBRX:
                if (insn_rA(insn) != 0) {
                    mark_gpr_used(block, insn_rA(insn));
                }
                mark_gpr_used(block, insn_rB(insn));
                mark_gpr_changed(block, insn_rD(insn));
                break;
              case XO_ECOWX:
              case XO_STWBRX:
              case XO_STHBRX:
                if (insn_rA(insn) != 0) {
                    mark_gpr_used(block, insn_rA(insn));
                }
                mark_gpr_used(block, insn_rB(insn));
                mark_gpr_used(block, insn_rS(insn));
                break;
              case XO_TLBSYNC:
              case XO_SYNC:
              case XO_EIEIO:  // And on that farm he had a duck...
                break;
              default:
                ASSERT(!"impossible");
            }
            break;

          case 0x17: {  // Indexed load/store
            const bool is_fp = (insn_XO_10(insn) & 0x200) != 0;
            const bool is_store = (insn_XO_10(insn) & 0x080) != 0;
            const bool is_update = (insn_XO_10(insn) & 0x020) != 0;
            if (insn_rA(insn) != 0) {
                mark_gpr_used(block, insn_rA(insn));
            }
            mark_gpr_used(block, insn_rB(insn));
            if (is_fp) {
                if (is_store) {
                    mark_fpr_used(block, insn_frD(insn));
                } else {
                    mark_fpr_changed(block, insn_frD(insn));
                }
            } else {
                if (is_store) {
                    mark_gpr_used(block, insn_rD(insn));
                } else {
                    mark_gpr_changed(block, insn_rD(insn));
                }
            }
            if (is_update) {
                mark_gpr_changed(block, insn_rA(insn));
            }
            break;
          }

          case 0x18:  // Shift instructions
            mark_gpr_used(block, insn_rS(insn));
            if (insn_XO_10(insn) != XO_SRAWI) {
                mark_gpr_used(block, insn_rB(insn));
            }
            mark_gpr_changed(block, insn_rA(insn));
            if (insn_XO_10(insn) == XO_SRAW || insn_XO_10(insn) == XO_SRAWI) {
                mark_xer_used(block);
                mark_xer_changed(block);
            }
            if (insn_Rc(insn)) {
                mark_xer_used(block);
                mark_cr_changed(block, 0);
            }
            break;

          case 0x1A:  // cntlzw, extsb, extsh
            mark_gpr_used(block, insn_rS(insn));
            mark_gpr_changed(block, insn_rA(insn));
            if (insn_Rc(insn)) {
                mark_xer_used(block);
                mark_cr_changed(block, 0);
            }
            break;

          case 0x1C:  // Logical instructions
            mark_gpr_used(block, insn_rS(insn));
            mark_gpr_used(block, insn_rB(insn));
            mark_gpr_changed(block, insn_rA(insn));
            if (insn_Rc(insn)) {
                mark_xer_used(block);
                mark_cr_changed(block, 0);
            }
            break;

          default:
            ASSERT(!"impossible");
        }
        break;

      case OPCD_x3B:
        mark_fpscr_used(block);
        switch (insn_XO_5(insn)) {
          case XO_FDIVS:
          case XO_FSUBS:
          case XO_FADDS:
            mark_fpr_used(block, insn_frA(insn));
            mark_fpr_used(block, insn_frB(insn));
            break;

          case XO_FRES:
            mark_fpr_used(block, insn_frB(insn));
            break;

          case XO_FMULS:
            mark_fpr_used(block, insn_frA(insn));
            mark_fpr_used(block, insn_frC(insn));
            break;

          case XO_FMSUBS:
          case XO_FMADDS:
          case XO_FNMSUBS:
          case XO_FNMADDS:
            mark_fpr_used(block, insn_frA(insn));
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_used(block, insn_frC(insn));
            break;

          default:
            ASSERT(!"impossible");
        }
        mark_fpr_changed(block, insn_frD(insn));
        mark_fpscr_changed(block);
        if (insn_Rc(insn)) {
            mark_cr_changed(block, 1);
        }
        break;

      case OPCD_x3F:
        switch (insn_XO_5(insn)) {
          case 0x00:  // fcmpu, fcmpo, mcrfs
            mark_fpscr_used(block);
            if (insn_XO_10(insn) != XO_MCRFS) {
                mark_fpr_used(block, insn_frA(insn));
                mark_fpr_used(block, insn_frB(insn));
                mark_fpscr_changed(block);
            } else if (insn_crfS(insn) <= 3 || insn_crfS(insn) == 5) {
                mark_fpscr_changed(block);
            }
            mark_cr_used(block, insn_crfD(insn));  // FIXME: temp until these insns are implemented, to avoid uninitialized values in CR merging
            mark_cr_changed(block, insn_crfD(insn));
            break;

          case 0x06:  // mtfsb0, mtfsb1, mtfsfi
            mark_fpscr_used(block);
            mark_fpscr_changed(block);
            if (insn_Rc(insn)) {
                mark_cr_changed(block, 1);
            }
            break;

          case 0x07:  // mffs, mtfsf
            mark_fpscr_used(block);
            if (insn_XO_10(insn) == XO_MFFS) {
                mark_fpr_changed(block, insn_frD(insn));
            } else {
                mark_fpr_used(block, insn_frB(insn));
                mark_fpscr_changed(block);
            }
            if (insn_Rc(insn)) {
                mark_cr_changed(block, 1);
            }
            break;

          case 0x08:  // fmr, etc.
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_changed(block, insn_frD(insn));
            if (insn_Rc(insn)) {
                mark_fpscr_used(block);
                mark_cr_changed(block, 1);
            }
            break;

          case 0x0C:  // frsp
          case 0x0E:  // fctiw
          case 0x0F:  // fctiwz
            mark_fpscr_used(block);
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_changed(block, insn_frD(insn));
            mark_fpscr_changed(block);
            if (insn_Rc(insn)) {
                mark_cr_changed(block, 1);
            }
            break;

          case XO_FDIV:
          case XO_FSUB:
          case XO_FADD:
            mark_fpscr_used(block);
            mark_fpr_used(block, insn_frA(insn));
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_changed(block, insn_frD(insn));
            mark_fpscr_changed(block);
            if (insn_Rc(insn)) {
                mark_cr_changed(block, 1);
            }
            break;

          case XO_FSEL:
            mark_fpr_used(block, insn_frA(insn));
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_used(block, insn_frC(insn));
            mark_fpr_changed(block, insn_frD(insn));
            if (insn_Rc(insn)) {
                mark_fpscr_used(block);
                mark_cr_changed(block, 1);
            }
            break;

          case XO_FMUL:
            mark_fpscr_used(block);
            mark_fpr_used(block, insn_frA(insn));
            mark_fpr_used(block, insn_frC(insn));
            mark_fpr_changed(block, insn_frD(insn));
            mark_fpscr_changed(block);
            if (insn_Rc(insn)) {
                mark_cr_changed(block, 1);
            }
            break;

          case XO_FRSQRTE:
            mark_fpscr_used(block);
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_changed(block, insn_frD(insn));
            mark_fpscr_changed(block);
            if (insn_Rc(insn)) {
                mark_cr_changed(block, 1);
            }
            break;

          case XO_FMSUB:
          case XO_FMADD:
          case XO_FNMSUB:
          case XO_FNMADD:
            mark_fpscr_used(block);
            mark_fpr_used(block, insn_frA(insn));
            mark_fpr_used(block, insn_frB(insn));
            mark_fpr_used(block, insn_frC(insn));
            mark_fpr_changed(block, insn_frD(insn));
            mark_fpscr_changed(block);
            if (insn_Rc(insn)) {
                mark_cr_changed(block, 1);
            }
            break;

          default:
            ASSERT(!"impossible");
        }
        break;

      default:
        ASSERT(!"impossible");
    }  // switch (insn_OPCD(insn))
}

/*************************************************************************/
/************************* Scanning entry point **************************/
/*************************************************************************/

bool guest_ppc_scan(GuestPPCContext *ctx, uint32_t limit)
{
    ASSERT(ctx);

    ASSERT((ctx->start & 3) == 0);
    ASSERT(limit >= ctx->start + 3);
    const uint32_t start = ctx->start;
    /* max_insns is calculated in this manner to correctly handle the
     * pathological case where start == 0 and limit == -1. */
    const uint32_t aligned_limit = min(limit, ctx->handle->code_range_end) - 3;
    const uint32_t max_insns = (aligned_limit - ctx->start) / 4 + 1;
    const uint32_t *memory_base =
        (const uint32_t *)ctx->handle->setup.guest_memory_base;

    GuestPPCBlockInfo *block = NULL;

    uint32_t insn_count;
    for (insn_count = 0; insn_count < max_insns; insn_count++) {
        const uint32_t address = start + insn_count*4;
        const uint32_t insn = bswap_be32(memory_base[address/4]);
        const bool is_invalid = !is_valid_insn(insn);

        /* Start a new block if the previous one was terminated (or if
         * this is the first instruction scanned). */
        if (!block) {
            /* If this opcode is invalid, we may be passing over data for
             * a jump table, so don't treat this as part of a block.  But
             * always translate invalid instructions at the beginning of
             * a block. */
            if (address > start && UNLIKELY(is_invalid)) {
                continue;
            }
            /* Otherwise, start a new block at this address.  Do this even
             * if the current address is not a branch target, since the
             * previous instruction may have (for example) branched to loop
             * termination code which will in turn branch back here. */
            block = get_block(ctx, address);
            if (UNLIKELY(!block)) {
                log_error(ctx->handle, "Out of memory expanding basic block"
                          " table at 0x%X", address);
                return false;
            }
        }

        /* Update register usage state for this instruction's operands. */
        if (!is_invalid) {
            update_used_changed(ctx, block, insn);
        }

        /* Terminate the block if this is a branch or invalid instruction,
         * or at icbi.  Also terminate the entire unit if this looks like
         * the end of a function. */
        const PPCOpcode opcd = insn_OPCD(insn);
        const bool is_direct_branch = ((opcd & ~0x02) == OPCD_BC);
        const bool is_indirect_branch =
            (opcd == OPCD_x13 && (insn_XO_10(insn) == XO_BCLR
                                  || insn_XO_10(insn) == XO_BCCTR));
        const bool is_icbi = (opcd == OPCD_x1F && insn_XO_10(insn) == XO_ICBI);
        const bool is_sc =
            (opcd == OPCD_SC
             && (insn_count == max_insns - 1
                 || bswap_be32(memory_base[(address+4)/4]) != 0x4E800020));
        if (is_direct_branch || is_indirect_branch || is_invalid || is_icbi
         || is_sc) {
            block->len = (address + 4) - block->start;
            block = NULL;

            /* If this is a non-subroutine (LK=0) direct (not BCLR/BCCTR)
             * branch and the target address is within our scanning range,
             * add it to the basic block list. */
            if (is_direct_branch && !insn_LK(insn)) {
                const int32_t disp =
                    (opcd == OPCD_B) ? insn_LI(insn) : insn_BD(insn);
                uint32_t target;
                if (insn_AA(insn)) {
                    target = (uint32_t)disp;
                } else {
                    target = address + disp;
                }
                if (target >= start && target <= aligned_limit) {
                    if (UNLIKELY(!get_block(ctx, target))) {
                        log_error(ctx->handle, "Out of memory expanding basic"
                                  " block table for branch target 0x%X at"
                                  " 0x%X", target, address);
                        return false;
                    }
                }
            }

            /*
             * Functions typically end with an unconditional branch of some
             * sort, most often BLR but possibly a branch to the entry
             * point of some other function (a tail call).  However, a
             * function might have several copies of its epilogue as a
             * result of optimization, so we only treat an unconditional
             * branch (or invalid instruction) as end-of-unit if there are
             * no branch targets we haven't yet reached.
             *
             * Note that we don't check LK here because we currently return
             * from translated code on a subroutine branch.  If we add
             * support for translating LK=1 branches to native calls, those
             * should not be treated as terminal here.
             *
             * icbi and sc instructions always cause a return from
             * translated code, so they can potentially end the unit as well.
             * But make sure not to stop at an sc before a blr so we can
             * optimize the sc+blr case properly.
             */
            const int is_unconditional_branch =
                (opcd == OPCD_B || (insn_BO(insn) & 0x14) == 0x14);
            if (is_invalid || is_icbi || is_sc || is_unconditional_branch) {
                ASSERT(ctx->num_blocks > 0);
                if (ctx->blocks[ctx->num_blocks-1].start <= address) {
                    /* No more blocks follow this one, so we've reached
                     * the end of the function. */
                    break;
                }
            }
        }
    }

    if (insn_count < max_insns) {
        ASSERT(!block);
    } else {
        if (block) {
            block->len = (aligned_limit + 4) - block->start;
        }
        if (aligned_limit + 3 >= limit) {
            log_info(ctx->handle, "Scanning terminated at requested limit"
                     " 0x%X", limit);
        } else {
            log_warning(ctx->handle, "Scanning terminated at 0x%X due to code"
                        " range bounds", aligned_limit + 4);
        }
    }

    /* Backward branches may have inserted basic blocks in the block list
     * which were never updated because we had already scanned past them.
     * Look for such blocks and split their predecessors. */
    for (int i = 1; i < ctx->num_blocks; i++) {
        block = &ctx->blocks[i];
        if (block->len == 0) {
            GuestPPCBlockInfo *prev = &ctx->blocks[i-1];
            /* Careful calculation here to correctly handle the case of
             * prev->start + prev->len == 0 (top of address space). */
            if (block->start - prev->start < prev->len) {
                block->len = prev->len - (block->start - prev->start);
                prev->len = block->start - prev->start;
            }
        }
    }

    return true;
}

/*************************************************************************/
/*************************************************************************/
