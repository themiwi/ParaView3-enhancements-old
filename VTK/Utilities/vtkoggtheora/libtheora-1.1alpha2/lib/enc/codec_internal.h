/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2008                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function:
  last mod: $Id: codec_internal.h 16044 2009-05-27 02:05:43Z tterribe $

 ********************************************************************/

#ifndef ENCODER_INTERNAL_H
#define ENCODER_INTERNAL_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/*#define OC_COLLECT_METRICS*/

/* KITWARE_OGGTHEORA_CHANGE make sure we include the right headers */
#include "vtkoggtheora/include/theora/theora.h"
#include "../internal.h"
#include "encoder_huffman.h"
#include "huffenc.h"
#include "../dec/ocintrin.h"



typedef struct oc_enc_opt_vtable oc_enc_opt_vtable;
typedef struct CP_INSTANCE       CP_INSTANCE;


struct oc_enc_opt_vtable{
  unsigned (*frag_sad)(const unsigned char *_src,
   const unsigned char *_ref,int _ystride);
  unsigned (*frag_sad_thresh)(const unsigned char *_src,
   const unsigned char *_ref,int _ystride,unsigned _thresh);
  unsigned (*frag_sad2_thresh)(const unsigned char *_src,
   const unsigned char *_ref1,const unsigned char *_ref2,int _ystride,
   unsigned _thresh);
  unsigned (*frag_satd_thresh)(const unsigned char *_src,
   const unsigned char *_ref,int _ystride,unsigned _thresh);
  unsigned (*frag_satd2_thresh)(const unsigned char *_src,
   const unsigned char *_ref1,const unsigned char *_ref2,int _ystride,
   unsigned _thresh);
  unsigned (*frag_intra_satd)(const unsigned char *_src,int _ystride);
  void     (*frag_sub)(ogg_int16_t _diff[64],const unsigned char *_src,
   const unsigned char *_ref,int _ystride);
  void     (*frag_sub_128)(ogg_int16_t _diff[64],
   const unsigned char *_src,int _ystride);
  void     (*frag_copy)(unsigned char *_dst,
   const unsigned char *_src,int _ystride);
  void     (*frag_copy2)(unsigned char *_dst,
   const unsigned char *_src1,const unsigned char *_src2,int _ystride);
  void     (*frag_recon_intra)(unsigned char *_dst,int _ystride,
   const ogg_int16_t _residue[64]);
  void     (*frag_recon_inter)(unsigned char *_dst,
   const unsigned char *_src,int _ystride,const ogg_int16_t _residue[64]);
  void     (*fdct8x8)(ogg_int16_t _y[64],const ogg_int16_t _x[64]);
  void     (*dequant_idct8x8)(ogg_int16_t _y[64],const ogg_int16_t _x[64],
   int _last_zzi,int _ncoefs,ogg_uint16_t _dc_quant,
   const ogg_uint16_t _ac_quant[64]);
  void     (*enc_loop_filter)(CP_INSTANCE *cpi,int _flimit);
  void     (*restore_fpu)(void);
};


void oc_enc_vtable_init(CP_INSTANCE *_cpi);

/* Baseline dct height and width. */
#define BLOCK_HEIGHT_WIDTH          8
#define HFRAGPIXELS                 8
#define VFRAGPIXELS                 8

/* Baseline dct block size */
#define BLOCK_SIZE              (BLOCK_HEIGHT_WIDTH * BLOCK_HEIGHT_WIDTH)

/* Border is for unrestricted mv's */
#define UMV_BORDER              16
#define STRIDE_EXTRA            (UMV_BORDER * 2)

#define KEY_FRAME               0
#define DELTA_FRAME             1

#define MAX_MODES               8
#define MODE_BITS               3
#define MODE_METHODS            8
#define MODE_METHOD_BITS        3

#define MAX_MV_EXTENT 31  /* Max search distance in half pixel increments */

/** block coding modes */
typedef enum{
  CODE_INTER_NO_MV        = 0x0, /* INTER prediction, (0,0) motion
                                    vector implied.  */
    CODE_INTRA            = 0x1, /* INTRA i.e. no prediction. */
    CODE_INTER_PLUS_MV    = 0x2, /* INTER prediction, non zero motion
                                    vector. */
    CODE_INTER_LAST_MV    = 0x3, /* Use Last Motion vector */
    CODE_INTER_PRIOR_LAST = 0x4, /* Prior last motion vector */
    CODE_USING_GOLDEN     = 0x5, /* 'Golden frame' prediction (no MV). */
    CODE_GOLDEN_MV        = 0x6, /* 'Golden frame' prediction plus MV. */
    CODE_INTER_FOURMV     = 0x7  /* Inter prediction 4MV per macro block. */
} coding_mode_t;

/** Huffman table entry */
typedef struct HUFF_ENTRY {
  struct HUFF_ENTRY *ZeroChild;
  struct HUFF_ENTRY *OneChild;
  struct HUFF_ENTRY *Previous;
  struct HUFF_ENTRY *Next;
  ogg_int32_t        Value;
  ogg_uint32_t       Frequency;
} HUFF_ENTRY;

typedef struct mc_state mc_state;

struct mc_state{
  int                candidates[12][2];
  int                setb0;
  int                ncandidates;
  ogg_int32_t        mvapw1[2];
  ogg_int32_t        mvapw2[2];
};

typedef struct macroblock {
  /* the blocks comprising this macroblock */
  int Ryuv[3][4]; /* [Y,U,V][raster order] */
  int Hyuv[3][4]; /* [Y,U,V][hilbert order] */
  int ysb;
  int usb;
  int vsb;

  int cneighbors[4];
  int ncneighbors;
  int pneighbors[4];
  int npneighbors; 

  coding_mode_t mode;

  oc_mv block_mv[4];
  oc_mv ref_mv[4];
  /* per-block final motion vectors */
  /* raster order */
  oc_mv mv[4];
  /*Per-block final chroma motion vectors.*/
  oc_mv cbmvs[4];

  /* Motion vectors for a macro block for the current frame and the
     previous two frames.

     Each is a set of 2 vectors against the previous frame and against
     the golden frame, which can be used to judge constant velocity
     and constant acceleration.

     Uninitialized MVs are (0,0).*/
  oc_mv analysis_mv[3][2]; /* [cur,prev,prev2][frame,golden] */
  oc_mv unref_mv[2];
  /*Minimum motion estimation error from the analysis stage.*/
  int    aerror;
  int    gerror;
  int    asatd;
  int    gsatd;
  int    block_satd[4];

  char coded;
  char refined;
} macroblock_t;

#define SB_MB_BLFRAG(sb,mbnum) ((sb).f[ ((mbnum)<2? ((mbnum)==0?0:4) : ((mbnum)==2?8:14)) ])
typedef struct superblock {
  int f[16]; // hilbert order
  int m[16]; // hilbert order: only 4 for luma, but 16 for U/V (to match f) */
} superblock_t;

typedef ogg_int16_t    quant_table[64];
typedef quant_table    quant_tables[64]; /* [zigzag][qi] */

#include "enquant.h"

typedef struct oc_mode_scheme_chooser oc_mode_scheme_chooser;

struct oc_mode_scheme_chooser{
  /*Pointers to the a list containing the index of each mode in the mode
    alphabet used by each scheme.
    The first entry points to the dynamic scheme0_ranks, while the remaining
    7 point to the constant entries stored in OC_MODE_SCHEMES.*/
  const unsigned char *mode_ranks[8];
  /*The ranks for each mode when coded with scheme 0.
    These are optimized so that the more frequent modes have lower ranks.*/
  unsigned char        scheme0_ranks[OC_NMODES];
  /*The list of modes, sorted in descending order of frequency, that
    corresponds to the ranks above.*/
  unsigned char        scheme0_list[OC_NMODES];
  /*The number of times each mode has been chosen so far.*/
  int                  mode_counts[OC_NMODES];
  /*The list of mode coding schemes, sorted in ascending order of bit cost.*/
  unsigned char        scheme_list[8];
  /*The number of bits used by each mode coding scheme.*/
  int                  scheme_bits[8];
};

void oc_mode_scheme_chooser_init(oc_mode_scheme_chooser *_chooser);

typedef struct oc_rc_state oc_rc_state;

struct oc_rc_state{
  ogg_int64_t bits_per_frame;
  ogg_int64_t fullness;
  ogg_int64_t target;
  ogg_int64_t max;
  ogg_int64_t log_npixels;
  unsigned    exp[2];
  ogg_int64_t log_scale[2];
  ogg_int64_t log_qtarget;
  int         buf_delay;
};

/* Encoder (Compressor) instance -- installed in a theora_state */
struct CP_INSTANCE {
  /*This structure must be first.
    It contains entry points accessed by the decoder library's API wrapper, and
     is the only assumption that library makes about our internal format.*/
  oc_state_dispatch_vtable  dispatch_vtbl;

  theora_info               info;

  /* ogg bitpacker for use in packet coding, other API state */
  oggpack_buffer           *oggbuffer;
  /*The number of duplicates to produce for the next frame.*/
  int                       dup_count;
  /*The number of duplicates remaining to be emitted for the current frame.*/
  int                       nqueued_dups;

  unsigned char            *frame;
  unsigned char            *recon;
  unsigned char            *golden;
  unsigned char            *lastrecon;
  ogg_uint32_t              frame_size;

  /*Superblock, macroblock and fragment Information.*/
  unsigned char            *frag_coded;
  ogg_uint32_t             *frag_buffer_index;
  ogg_int16_t              *frag_dc;
  ogg_int16_t              *frag_dc_tmp;

  macroblock_t             *macro;
  superblock_t             *super[3];

  ogg_uint32_t              frag_h[3];
  ogg_uint32_t              frag_v[3];
  ogg_uint32_t              frag_n[3];
  ogg_uint32_t              frag_total;

  ogg_uint32_t              macro_h;
  ogg_uint32_t              macro_v;
  ogg_uint32_t              macro_total;
  
  ogg_uint32_t              super_h[3];
  ogg_uint32_t              super_v[3];
  ogg_uint32_t              super_n[3];
  ogg_uint32_t              super_total;

  /*Stride of image and recon planes, accounting for borders.*/
  ogg_uint32_t              stride[3];
  /*Data offset of first coded pixel in plane.*/
  ogg_uint32_t              offset[3];

  /*********************************************************************/
  /* state and stats */

  int                       HeadersWritten;
  ogg_uint32_t              LastKeyFrame;
  ogg_int64_t               CurrentFrame;
  unsigned char             FrameType;
  int                       readyflag;
  int                       packetflag;
  int                       doneflag;
  int                       first_inter_frame;

  /*Indexed via [key/inter][dc/ac][luma/chroma].*/
  int                       huffchoice[2][2][2];

  ogg_uint32_t              dc_bits[2][DC_HUFF_CHOICES];
  ogg_uint32_t              ac1_bits[2][AC_HUFF_CHOICES];
  ogg_uint32_t              acN_bits[2][AC_HUFF_CHOICES];
  /*Count of bits used by MV coding mode 0.*/
  ogg_uint32_t              MVBits_0;
  /*Count of bits used by MV coding mode 1.*/
  ogg_uint32_t              MVBits_1;
  oc_mode_scheme_chooser    chooser;

  /*********************************************************************/
  /* Token Buffers */
  int                      *fr_partial;
  unsigned char            *fr_partial_bits;
  int                      *fr_full;
  unsigned char            *fr_full_bits;
  ogg_int16_t              *fr_block;
  unsigned char            *fr_block_bits;
  int                       fr_partial_count;
  int                       fr_full_count;
  int                       fr_block_count;


  int                       stack_offset;
  unsigned char            *dct_token_storage;
  ogg_uint16_t             *dct_token_eb_storage;
  unsigned char            *dct_token[64];
  ogg_uint16_t             *dct_token_eb[64];

  ogg_int32_t               dct_token_count[64];
  ogg_int32_t               dct_token_ycount[64];

  int                       eob_run[64];
  int                       eob_pre[64];
  int                       eob_ypre[64];

  /********************************************************************/
  /* Fragment SAD->bitrate estimation tracking metrics */
  long                      rho_count[65]; 

#if defined(OC_COLLECT_METRICS)
  long                      rho_postop;
  int                      *frag_mbi;
  int                      *frag_sad;
  int                      *frag_ssd;
  int                      *dct_token_frag_storage;
  int                      *dct_token_frag[64];
  int                      *dct_eob_fi_storage;
  int                      *dct_eob_fi_stack[64];
  int                       dct_eob_fi_count[64];
  ogg_int64_t               dist_dist[3][8];
  ogg_int64_t               dist_bits[3][8];
#endif

  /********************************************************************/
  /* Setup */
  int                     keyframe_granule_shift;
  int                     lambda;
  int                     BaseQ;
  int                     MinQ;
  int                     GoldenFrameEnabled;
  int                     InterPrediction;
  int                     MotionCompensation;

  /* hufftables and quant setup ****************************************/

  th_huff_code            huff_codes[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS];

  th_quant_info           quant_info;
  quant_tables            quant_tables[2][3];
  oc_iquant_tables        iquant_tables[2][3];
  /*An "average" quantizer for each quantizer type (INTRA or INTER) and QI
     value.
    This is used to paramterize the rate control decisions.
    They are kept in the log domain to simplify later processing.
    Keep in mind these are DCT domain quantizers, and so are scaled by an
     additional factor of 4 from the pixel domain.*/
  ogg_int64_t             log_qavg[2][64];
  /*The buffer state used to drive rate control.*/
  oc_rc_state             rc;
  /*Table for encoder acceleration functions.*/
  oc_enc_opt_vtable       opt_vtable;
};

extern void ReconRefFrames (CP_INSTANCE *cpi);

typedef struct {
  int coeff;
  int count; /* -1 indicates no token, ie, midst of an EOB run */
  int chroma;
  int pre;
  int run;
#if defined(OC_COLLECT_METRICS)
  int runstack;
#endif
} token_checkpoint_t;

extern void tokenlog_commit(CP_INSTANCE *cpi, 
			    token_checkpoint_t *stack, 
			    int n);
extern void tokenlog_rollback(CP_INSTANCE *cpi, 
			      token_checkpoint_t *stack,
			      int n);
extern void dct_tokenize_init (CP_INSTANCE *cpi);
extern int dct_tokenize_AC (CP_INSTANCE *cpi, 
			    const int fi, 
			    ogg_int16_t *dct, 
			    const ogg_int16_t *dequant, 
			    const ogg_int16_t *origdct, 
			    const int chroma, 
			    token_checkpoint_t **stack,int _acmin);
extern void dct_tokenize_finish (CP_INSTANCE *cpi);
extern void dct_tokenize_mark_ac_chroma (CP_INSTANCE *cpi);

extern void InitQTables( CP_INSTANCE *cpi );

extern void WriteFrameHeader( CP_INSTANCE *cpi) ;

extern void EncodeData(CP_INSTANCE *cpi);

extern void oc_mcenc_start(CP_INSTANCE *cpi,
			   mc_state *mcenc);

extern void oc_mcenc_search(CP_INSTANCE *cpi, 
			    mc_state *_mcenc,
			    int _mbi,
			    int _goldenp,
			    oc_mv _bmvs[4],
			    int *best_err,
			    int best_block_err[4]);

extern void oc_mcenc_refine1mv(CP_INSTANCE *cpi, 
			      int _mbi,
			      int _goldenp,
			      int err);

extern void oc_mcenc_refine4mv(CP_INSTANCE *cpi, 
			      int _mbi,
			      int err[4]);

extern int PickModes(CP_INSTANCE *cpi, int recode);

extern void InitFrameInfo(CP_INSTANCE *cpi);

extern void ClearFrameInfo (CP_INSTANCE *cpi);

typedef struct {
  ogg_uint16_t sb_partial_count;
  ogg_uint16_t sb_full_count;

  signed char sb_partial_last;
  signed char sb_full_last;
  signed char b_last;
  signed char b_count;
  signed char b_pend;

  char sb_partial_break;
  char sb_full_break;
  char sb_partial;
  char sb_coded;

  int cost;
} fr_state_t;

extern void fr_clear(CP_INSTANCE *cpi, fr_state_t *fr);
extern void fr_skipblock(CP_INSTANCE *cpi, fr_state_t *fr);
extern void fr_codeblock(CP_INSTANCE *cpi, fr_state_t *fr);
extern void fr_finishsb(CP_INSTANCE *cpi, fr_state_t *fr);
extern void fr_write(CP_INSTANCE *cpi, fr_state_t *fr);

extern int fr_cost1(fr_state_t *fr);
extern int fr_cost4(fr_state_t *pre, fr_state_t *post);

#if defined(OC_COLLECT_METRICS)
extern void ModeMetrics(CP_INSTANCE *cpi);
extern void oc_enc_mode_metrics_dump(CP_INSTANCE *cpi);
#endif

/*Encoder-specific accelerated functions.*/
void oc_enc_frag_sub(const CP_INSTANCE *_cpi,ogg_int16_t _diff[64],
 const unsigned char *_src,const unsigned char *_ref,int _ystride);
void oc_enc_frag_sub_128(const CP_INSTANCE *_cpi,ogg_int16_t _diff[64],
 const unsigned char *_src,int _ystride);
unsigned oc_enc_frag_sad(const CP_INSTANCE *_cpi,const unsigned char *_src,
 const unsigned char *_ref,int _ystride);
unsigned oc_enc_frag_sad_thresh(const CP_INSTANCE *_cpi,
 const unsigned char *_src,const unsigned char *_ref,int _ystride,
 unsigned _thresh);
unsigned oc_enc_frag_sad2_thresh(const CP_INSTANCE *_cpi,
 const unsigned char *_src,const unsigned char *_ref1,
 const unsigned char *_ref2,int _ystride,unsigned _thresh);
unsigned oc_enc_frag_satd_thresh(const CP_INSTANCE *_cpi,
 const unsigned char *_src,const unsigned char *_ref,int _ystride,
 unsigned _thresh);
unsigned oc_enc_frag_satd2_thresh(const CP_INSTANCE *_cpi,
 const unsigned char *_src,const unsigned char *_ref1,
 const unsigned char *_ref2,int _ystride,unsigned _thresh);
unsigned oc_enc_frag_intra_satd(const CP_INSTANCE *_cpi,
 const unsigned char *_src,int _ystride);
void oc_enc_frag_copy(const CP_INSTANCE *_cpi,unsigned char *_dst,
 const unsigned char *_src,int _ystride);
void oc_enc_frag_copy2(const CP_INSTANCE *_cpi,unsigned char *_dst,
 const unsigned char *_src1,const unsigned char *_src2,int _ystride);
void oc_enc_frag_recon_intra(const CP_INSTANCE *_cpi,
 unsigned char *_dst,int _ystride,const ogg_int16_t _residue[64]);
void oc_enc_frag_recon_inter(const CP_INSTANCE *_cpi,unsigned char *_dst,
 const unsigned char *_src,int _ystride,const ogg_int16_t _residue[64]);
void oc_enc_fdct8x8(const CP_INSTANCE *_cpi,ogg_int16_t _y[64],
 const ogg_int16_t _x[64]);
void oc_enc_dequant_idct8x8(const CP_INSTANCE *_cpi,ogg_int16_t _y[64],
 const ogg_int16_t _x[64],int _last_zzi,int _ncoefs,
 ogg_uint16_t _dc_quant,const ogg_uint16_t _ac_quant[64]);
void oc_enc_loop_filter(CP_INSTANCE *_cpi,int _flimit);
void oc_enc_restore_fpu(const CP_INSTANCE *_cpi);

/*Default pure-C implementations.*/
void oc_enc_vtable_init_c(CP_INSTANCE *_cpi);

void oc_enc_frag_sub_c(ogg_int16_t _diff[64],
 const unsigned char *_src,const unsigned char *_ref,int _ystride);
void oc_enc_frag_sub_128_c(ogg_int16_t _diff[64],
 const unsigned char *_src,int _ystride);
void oc_enc_frag_copy2_c(unsigned char *_dst,
 const unsigned char *_src1,const unsigned char *_src2,int _ystride);
unsigned oc_enc_frag_sad_c(const unsigned char *_src,
 const unsigned char *_ref,int _ystride);
unsigned oc_enc_frag_sad_thresh_c(const unsigned char *_src,
 const unsigned char *_ref,int _ystride,unsigned _thresh);
unsigned oc_enc_frag_sad2_thresh_c(const unsigned char *_src,
 const unsigned char *_ref1,const unsigned char *_ref2,int _ystride,
 unsigned _thresh);
unsigned oc_enc_frag_satd_thresh_c(const unsigned char *_src,
 const unsigned char *_ref,int _ystride,unsigned _thresh);
unsigned oc_enc_frag_satd2_thresh_c(const unsigned char *_src,
 const unsigned char *_ref1,const unsigned char *_ref2,int _ystride,
 unsigned _thresh);
unsigned oc_enc_frag_intra_satd_c(const unsigned char *_src,int _ystride);
void oc_enc_fdct8x8_c(ogg_int16_t _y[64],const ogg_int16_t _x[64]);
void oc_enc_loop_filter_c(CP_INSTANCE *_cpi,int _flimit);
void oc_enc_restore_fpu_c(void);

#endif /* ENCODER_INTERNAL_H */
