/*
 * Register all the formats and protocols
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "avformat.h"
#include "rtp.h"
#include "rdt.h"
#include "url.h"
#include "version.h"

#define REGISTER_MUXER(X, x)                                            \
    {                                                                   \
        extern AVOutputFormat ff_##x##_muxer;                           \
        if (CONFIG_##X##_MUXER)                                         \
            av_register_output_format(&ff_##x##_muxer);                 \
    }

#define REGISTER_DEMUXER(X, x)                                          \
    {                                                                   \
        extern AVInputFormat ff_##x##_demuxer;                          \
        if (CONFIG_##X##_DEMUXER)                                       \
            av_register_input_format(&ff_##x##_demuxer);                \
    }

#define REGISTER_MUXDEMUX(X, x) REGISTER_MUXER(X, x); REGISTER_DEMUXER(X, x)

#define REGISTER_PROTOCOL(X, x)                                         \
    {                                                                   \
        extern URLProtocol ff_##x##_protocol;                           \
        if (CONFIG_##X##_PROTOCOL)                                      \
            ffurl_register_protocol(&ff_##x##_protocol);                \
    }

void av_register_all(void)
{
    static int initialized;

    if (initialized)
        return;
    initialized = 1;

    avcodec_register_all();

#if defined(_MSC_VER) && (_MSC_VER < 1600)
    /* muxers */
#if (CONFIG_A64_MUXER == 1)
    REGISTER_MUXER   (A64,              a64);
#endif
#if (CONFIG_AC3_MUXER == 1)
    REGISTER_MUXER   (AC3,              ac3);
#endif
#if (CONFIG_ADTS_MUXER == 1)
    REGISTER_MUXER   (ADTS,             adts);
#endif
#if (CONFIG_ADX_MUXER == 1)
    REGISTER_MUXER   (ADX,              adx);
#endif
#if (CONFIG_AIFF_MUXER == 1)
    REGISTER_MUXER   (AIFF,             aiff);
#endif
#if (CONFIG_AMR_MUXER == 1)
    REGISTER_MUXER   (AMR,              amr);
#endif
#if (CONFIG_ASF_MUXER == 1)
    REGISTER_MUXER   (ASF,              asf);
#endif
#if (CONFIG_ASS_MUXER == 1)
    REGISTER_MUXER   (ASS,              ass);
#endif
#if (CONFIG_AST_MUXER == 1)
    REGISTER_MUXER   (AST,              ast);
#endif
#if (CONFIG_ASF_STREAM_MUXER == 1)
    REGISTER_MUXER   (ASF_STREAM,       asf_stream);
#endif
#if (CONFIG_AU_MUXER == 1)
    REGISTER_MUXER   (AU,               au);
#endif
#if (CONFIG_AVI_MUXER == 1)
    REGISTER_MUXER   (AVI,              avi);
#endif
#if (CONFIG_AVM2_MUXER == 1)
    REGISTER_MUXER   (AVM2,             avm2);
#endif
#if (CONFIG_BIT_MUXER == 1)
    REGISTER_MUXER   (BIT,              bit);
#endif
#if (CONFIG_CAF_MUXER == 1)
    REGISTER_MUXER   (CAF,              caf);
#endif
#if (CONFIG_CAF_MUXER == 1)
    REGISTER_MUXER   (CAVSVIDEO,        cavsvideo);
#endif
#if (CONFIG_CRC_MUXER == 1)
    REGISTER_MUXER   (CRC,              crc);
#endif
#if (CONFIG_DATA_MUXER == 1)
    REGISTER_MUXER   (DATA,             data);
#endif
#if (CONFIG_DAUD_MUXER == 1)
    REGISTER_MUXER   (DAUD,             daud);
#endif
#if (CONFIG_DIRAC_MUXER == 1)
    REGISTER_MUXER   (DIRAC,            dirac);
#endif
#if (CONFIG_DNXHD_MUXER == 1)
    REGISTER_MUXER   (DNXHD,            dnxhd);
#endif
#if (CONFIG_DTS_MUXER == 1)
    REGISTER_MUXER   (DTS,              dts);
#endif
#if (CONFIG_DV_MUXER == 1)
    REGISTER_MUXER   (DV,               dv);
#endif
#if (CONFIG_EAC3_MUXER == 1)
    REGISTER_MUXER   (EAC3,             eac3);
#endif
#if (CONFIG_F4V_MUXER == 1)
    REGISTER_MUXER   (F4V,              f4v);
#endif
#if (CONFIG_FFM_MUXER == 1)
    REGISTER_MUXER   (FFM,              ffm);
#endif
#if (CONFIG_FFMETADATA_MUXER == 1)
    REGISTER_MUXER   (FFMETADATA,       ffmetadata);
#endif
#if (CONFIG_FILMSTRIP_MUXER == 1)
    REGISTER_MUXER   (FILMSTRIP,        filmstrip);
#endif
#if (CONFIG_FLAC_MUXER == 1)
    REGISTER_MUXER   (FLAC,             flac);
#endif
#if (CONFIG_FLV_MUXER == 1)
    REGISTER_MUXER   (FLV,              flv);
#endif
#if (CONFIG_FRAMECRC_MUXER == 1)
    REGISTER_MUXER   (FRAMECRC,         framecrc);
#endif
#if (CONFIG_FRAMEMD5_MUXER == 1)
    REGISTER_MUXER   (FRAMEMD5,         framemd5);
#endif
#if (CONFIG_G722_MUXER == 1)
    REGISTER_MUXER   (G722,             g722);
#endif
#if (CONFIG_G723_1_MUXER == 1)
    REGISTER_MUXER   (G723_1,           g723_1);
#endif
#if (CONFIG_GIF_MUXER == 1)
    REGISTER_MUXER   (GIF,              gif);
#endif
#if (CONFIG_GXF_MUXER == 1)
    REGISTER_MUXER   (GXF,              gxf);
#endif
#if (CONFIG_H261_MUXER == 1)
    REGISTER_MUXER   (H261,             h261);
#endif
#if (CONFIG_H263_MUXER == 1)
    REGISTER_MUXER   (H263,             h263);
#endif
#if (CONFIG_H264_MUXER == 1)
    REGISTER_MUXER   (H264,             h264);
#endif
#if (CONFIG_HDS_MUXER == 1)
    REGISTER_MUXER   (HDS,              hds);
#endif
#if (CONFIG_HEVC_MUXER == 1)
    REGISTER_MUXER   (HEVC,             hevc);
#endif
#if (CONFIG_HLS_MUXER == 1)
    REGISTER_MUXER   (HLS,              hls);
#endif
#if (CONFIG_ICO_MUXER == 1)
    REGISTER_MUXER   (ICO,              ico);
#endif
#if (CONFIG_ILBC_MUXER == 1)
    REGISTER_MUXER   (ILBC,             ilbc);
#endif
#if (CONFIG_IMAGE2_MUXER == 1)
    REGISTER_MUXER   (IMAGE2,           image2);
#endif
#if (CONFIG_IMAGE2PIPE_MUXER == 1)
    REGISTER_MUXER   (IMAGE2PIPE,       image2pipe);
#endif
#if (CONFIG_IPOD_MUXER == 1)
    REGISTER_MUXER   (IPOD,             ipod);
#endif
#if (CONFIG_IRCAM_MUXER == 1)
    REGISTER_MUXER   (IRCAM,            ircam);
#endif
#if (CONFIG_ISMV_MUXER == 1)
    REGISTER_MUXER   (ISMV,             ismv);
#endif
#if (CONFIG_IVF_MUXER == 1)
    REGISTER_MUXER   (IVF,              ivf);
#endif
#if (CONFIG_JACOSUB_MUXER == 1)
    REGISTER_MUXER   (JACOSUB,          jacosub);
#endif
#if (CONFIG_LATM_MUXER == 1)
    REGISTER_MUXER   (LATM,             latm);
#endif
#if (CONFIG_LRC_MUXER == 1)
    REGISTER_MUXER   (LRC,              lrc);
#endif
#if (CONFIG_M4V_MUXER == 1)
    REGISTER_MUXER   (M4V,              m4v);
#endif
#if (CONFIG_MD5_MUXER == 1)
    REGISTER_MUXER   (MD5,              md5);
#endif
#if (CONFIG_MATROSKA_MUXER == 1)
    REGISTER_MUXER   (MATROSKA,         matroska);
#endif
#if (CONFIG_MATROSKA_AUDIO_MUXER == 1)
    REGISTER_MUXER   (MATROSKA_AUDIO,   matroska_audio);
#endif
#if (CONFIG_MICRODVD_MUXER == 1)
    REGISTER_MUXER   (MICRODVD,         microdvd);
#endif
#if (CONFIG_MJPEG_MUXER == 1)
    REGISTER_MUXER   (MJPEG,            mjpeg);
#endif
#if (CONFIG_MLP_MUXER == 1)
    REGISTER_MUXER   (MLP,              mlp);
#endif
#if (CONFIG_MMF_MUXER == 1)
    REGISTER_MUXER   (MMF,              mmf);
#endif
#if (CONFIG_MOV_MUXER == 1)
    REGISTER_MUXER   (MOV,              mov);
#endif
#if (CONFIG_MP2_MUXER == 1)
    REGISTER_MUXER   (MP2,              mp2);
#endif
#if (CONFIG_MP3_MUXER == 1)
    REGISTER_MUXER   (MP3,              mp3);
#endif
#if (CONFIG_MP4_MUXER == 1)
    REGISTER_MUXER   (MP4,              mp4);
#endif
#if (CONFIG_MPEG1SYSTEM_MUXER == 1)
    REGISTER_MUXER   (MPEG1SYSTEM,      mpeg1system);
#endif
#if (CONFIG_MPEG1VCD_MUXER == 1)
    REGISTER_MUXER   (MPEG1VCD,         mpeg1vcd);
#endif
#if (CONFIG_MPEG1VIDEO_MUXER == 1)
    REGISTER_MUXER   (MPEG1VIDEO,       mpeg1video);
#endif
#if (CONFIG_MPEG2DVD_MUXER == 1)
    REGISTER_MUXER   (MPEG2DVD,         mpeg2dvd);
#endif
#if (CONFIG_MPEG2SVCD_MUXER == 1)
    REGISTER_MUXER   (MPEG2SVCD,        mpeg2svcd);
#endif
#if (CONFIG_MPEG2VIDEO_MUXER == 1)
    REGISTER_MUXER   (MPEG2VIDEO,       mpeg2video);
#endif
#if (CONFIG_MPEG2VOB_MUXER == 1)
    REGISTER_MUXER   (MPEG2VOB,         mpeg2vob);
#endif
#if (CONFIG_MPEGTS_MUXER == 1)
    REGISTER_MUXER   (MPEGTS,           mpegts);
#endif
#if (CONFIG_MPJPEG_MUXER == 1)
    REGISTER_MUXER   (MPJPEG,           mpjpeg);
#endif
#if (CONFIG_MXF_MUXER == 1)
    REGISTER_MUXER   (MXF,              mxf);
#endif
#if (CONFIG_MXF_D10_MUXER == 1)
    REGISTER_MUXER   (MXF_D10,          mxf_d10);
#endif
#if (CONFIG_NULL_MUXER == 1)
    REGISTER_MUXER   (NULL,             null);
#endif
#if (CONFIG_NUT_MUXER == 1)
    REGISTER_MUXER   (NUT,              nut);
#endif
#if (CONFIG_OGA_MUXER == 1)
    REGISTER_MUXER   (OGA,              oga);
#endif
#if (CONFIG_OGG_MUXER == 1)
    REGISTER_MUXER   (OGG,              ogg);
#endif
#if (CONFIG_OMA_MUXER == 1)
    REGISTER_MUXER   (OMA,              oma);
#endif
#if (CONFIG_OPUS_MUXER == 1)
    REGISTER_MUXER   (OPUS,             opus);
#endif
#if (CONFIG_PCM_ALAW_MUXER == 1)
    REGISTER_MUXER   (PCM_ALAW,         pcm_alaw);
#endif
#if (CONFIG_PCM_MULAW_MUXER == 1)
    REGISTER_MUXER   (PCM_MULAW,        pcm_mulaw);
#endif
#if (CONFIG_PCM_F64BE_MUXER == 1)
    REGISTER_MUXER   (PCM_F64BE,        pcm_f64be);
#endif
#if (CONFIG_PCM_F64LE_MUXER == 1)
    REGISTER_MUXER   (PCM_F64LE,        pcm_f64le);
#endif
#if (CONFIG_PCM_F32BE_MUXER == 1)
    REGISTER_MUXER   (PCM_F32BE,        pcm_f32be);
#endif
#if (CONFIG_PCM_F32LE_MUXER == 1)
    REGISTER_MUXER   (PCM_F32LE,        pcm_f32le);
#endif
#if (CONFIG_PCM_S32BE_MUXER == 1)
    REGISTER_MUXER   (PCM_S32BE,        pcm_s32be);
#endif
#if (CONFIG_PCM_S32LE_MUXER == 1)
    REGISTER_MUXER   (PCM_S32LE,        pcm_s32le);
#endif
#if (CONFIG_PCM_S24BE_MUXER == 1)
    REGISTER_MUXER   (PCM_S24BE,        pcm_s24be);
#endif
#if (CONFIG_PCM_S24LE_MUXER == 1)
    REGISTER_MUXER   (PCM_S24LE,        pcm_s24le);
#endif
#if (CONFIG_PCM_S16BE_MUXER == 1)
    REGISTER_MUXER   (PCM_S16BE,        pcm_s16be);
#endif
#if (CONFIG_PCM_S16LE_MUXER == 1)
    REGISTER_MUXER   (PCM_S16LE,        pcm_s16le);
#endif
#if (CONFIG_PCM_S8_MUXER == 1)
    REGISTER_MUXER   (PCM_S8,           pcm_s8);
#endif
#if (CONFIG_PCM_U32BE_MUXER == 1)
    REGISTER_MUXER   (PCM_U32BE,        pcm_u32be);
#endif
#if (CONFIG_PCM_U32LE_MUXER == 1)
    REGISTER_MUXER   (PCM_U32LE,        pcm_u32le);
#endif
#if (CONFIG_PCM_U24BE_MUXER == 1)
    REGISTER_MUXER   (PCM_U24BE,        pcm_u24be);
#endif
#if (CONFIG_PCM_U24LE_MUXER == 1)
    REGISTER_MUXER   (PCM_U24LE,        pcm_u24le);
#endif
#if (CONFIG_PCM_U16BE_MUXER == 1)
    REGISTER_MUXER   (PCM_U16BE,        pcm_u16be);
#endif
#if (CONFIG_PCM_U16LE_MUXER == 1)
    REGISTER_MUXER   (PCM_U16LE,        pcm_u16le);
#endif
#if (CONFIG_PCM_U8_MUXER == 1)
    REGISTER_MUXER   (PCM_U8,           pcm_u8);
#endif
#if (CONFIG_PSP_MUXER == 1)
    REGISTER_MUXER   (PSP,              psp);
#endif
#if (CONFIG_RAWVIDEO_MUXER == 1)
    REGISTER_MUXER   (RAWVIDEO,         rawvideo);
#endif
#if (CONFIG_RM_MUXER == 1)
    REGISTER_MUXER   (RM,               rm);
#endif
#if (CONFIG_ROQ_MUXER == 1)
    REGISTER_MUXER   (ROQ,              roq);
#endif
#if (CONFIG_RSO_MUXER == 1)
    REGISTER_MUXER   (RSO,              rso);
#endif
#if (CONFIG_RTP_MUXER == 1)
    REGISTER_MUXER   (RTP,              rtp);
#endif
#if (CONFIG_RTSP_MUXER == 1)
    REGISTER_MUXER   (RTSP,             rtsp);
#endif
#if (CONFIG_SAP_MUXER == 1)
    REGISTER_MUXER   (SAP,              sap);
#endif
#if (CONFIG_SEGMENT_MUXER == 1)
    REGISTER_MUXER   (SEGMENT,          segment);
#endif
#if (CONFIG_SEGMENT_MUXER == 1)
    REGISTER_MUXER   (SEGMENT,          stream_segment);
#endif
#if (CONFIG_SMJPEG_MUXER == 1)
    REGISTER_MUXER   (SMJPEG,           smjpeg);
#endif
#if (CONFIG_SMOOTHSTREAMING_MUXER == 1)
    REGISTER_MUXER   (SMOOTHSTREAMING,  smoothstreaming);
#endif
#if (CONFIG_SOX_MUXER == 1)
    REGISTER_MUXER   (SOX,              sox);
#endif
#if (CONFIG_SPX_MUXER == 1)
    REGISTER_MUXER   (SPX,              spx);
#endif
#if (CONFIG_SPDIF_MUXER == 1)
    REGISTER_MUXER   (SPDIF,            spdif);
#endif
#if (CONFIG_SRT_MUXER == 1)
    REGISTER_MUXER   (SRT,              srt);
#endif
#if (CONFIG_SWF_MUXER == 1)
    REGISTER_MUXER   (SWF,              swf);
#endif
#if (CONFIG_TEE_MUXER == 1)
    REGISTER_MUXER   (TEE,              tee);
#endif
#if (CONFIG_TG2_MUXER == 1)
    REGISTER_MUXER   (TG2,              tg2);
#endif
#if (CONFIG_TGP_MUXER == 1)
    REGISTER_MUXER   (TGP,              tgp);
#endif
#if (CONFIG_MKVTIMESTAMP_V2_MUXER == 1)
    REGISTER_MUXER   (MKVTIMESTAMP_V2,  mkvtimestamp_v2);
#endif
#if (CONFIG_TRUEHD_MUXER == 1)
    REGISTER_MUXER   (TRUEHD,           truehd);
#endif
#if (CONFIG_UNCODEDFRAMECRC_MUXER == 1)
    REGISTER_MUXER   (UNCODEDFRAMECRC,  uncodedframecrc);
#endif
#if (CONFIG_VC1_MUXER == 1)
    REGISTER_MUXER   (VC1,              vc1);
#endif
#if (CONFIG_VC1T_MUXER == 1)
    REGISTER_MUXER   (VC1T,             vc1t);
#endif
#if (CONFIG_VOC_MUXER == 1)
    REGISTER_MUXER   (VOC,              voc);
#endif
#if (CONFIG_W64_MUXER == 1)
    REGISTER_MUXER   (W64,              w64);
#endif
#if (CONFIG_WAV_MUXER == 1)
    REGISTER_MUXER   (WAV,              wav);
#endif
#if (CONFIG_WEBM_MUXER == 1)
    REGISTER_MUXER   (WEBM,             webm);
#endif
#if (CONFIG_WEBM_DASH_MANIFEST_MUXER == 1)
    REGISTER_MUXER   (WEBM_DASH_MANIFEST, webm_dash_manifest);
#endif
#if (CONFIG_WEBVTT_MUXER == 1)
    REGISTER_MUXER   (WEBVTT,           webvtt);
#endif
#if (CONFIG_WTV_MUXER == 1)
    REGISTER_MUXER   (WTV,              wtv);
#endif
#if (CONFIG_WV_MUXER == 1)
    REGISTER_MUXER   (WV,               wv);
#endif
#if (CONFIG_YUV4MPEGPIPE_MUXER == 1)
    REGISTER_MUXER   (YUV4MPEGPIPE,     yuv4mpegpipe);
#endif
#if (CONFIG_LIBNUT_MUXER == 1)
    REGISTER_MUXER   (LIBNUT,           libnut);
#endif

    /* demuxers */
#if (CONFIG_AAC_DEMUXER == 1)
    REGISTER_DEMUXER (AAC,              aac);
#endif
#if (CONFIG_AC3_DEMUXER == 1)
    REGISTER_DEMUXER (AC3,              ac3);
#endif
#if (CONFIG_ACT_DEMUXER == 1)
    REGISTER_DEMUXER (ACT,              act);
#endif
#if (CONFIG_ADF_DEMUXER == 1)
    REGISTER_DEMUXER (ADF,              adf);
#endif
#if (CONFIG_ADP_DEMUXER == 1)
    REGISTER_DEMUXER (ADP,              adp);
#endif
#if (CONFIG_ADX_DEMUXER == 1)
    REGISTER_DEMUXER(ADX,              adx);
#endif
#if (CONFIG_AEA_DEMUXER == 1)
    REGISTER_DEMUXER (AEA,              aea);
#endif
#if (CONFIG_AFC_DEMUXER == 1)
    REGISTER_DEMUXER (AFC,              afc);
#endif
#if (CONFIG_AIFF_DEMUXER == 1)
    REGISTER_DEMUXER (AIFF,             aiff);
#endif
#if (CONFIG_AMR_DEMUXER == 1)
    REGISTER_DEMUXER (AMR,              amr);
#endif
#if (CONFIG_ANM_DEMUXER == 1)
    REGISTER_DEMUXER (ANM,              anm);
#endif
#if (CONFIG_APC_DEMUXER == 1)
    REGISTER_DEMUXER (APC,              apc);
#endif
#if (CONFIG_APE_DEMUXER == 1)
    REGISTER_DEMUXER (APE,              ape);
#endif
#if (CONFIG_AQTITLE_DEMUXER == 1)
    REGISTER_DEMUXER (AQTITLE,          aqtitle);
#endif
#if (CONFIG_ASF_DEMUXER == 1)
    REGISTER_DEMUXER (ASF,              asf);
#endif
#if (CONFIG_ASS_DEMUXER == 1)
    REGISTER_DEMUXER (ASS,              ass);
#endif
#if (CONFIG_AST_DEMUXER == 1)
    REGISTER_DEMUXER (AST,              ast);
#endif
#if (CONFIG_AU_DEMUXER == 1)
    REGISTER_DEMUXER (AU,               au);
#endif
#if (CONFIG_AVI_DEMUXER == 1)
    REGISTER_DEMUXER (AVI,              avi);
#endif
#if (CONFIG_AVISYNTH_DEMUXER == 1)
    REGISTER_DEMUXER (AVISYNTH,         avisynth);
#endif
#if (CONFIG_AVR_DEMUXER == 1)
    REGISTER_DEMUXER (AVR,              avr);
#endif
#if (CONFIG_AVS_DEMUXER == 1)
    REGISTER_DEMUXER (AVS,              avs);
#endif
#if (CONFIG_BETHSOFTVID_DEMUXER == 1)
    REGISTER_DEMUXER (BETHSOFTVID,      bethsoftvid);
#endif
#if (CONFIG_BFI_DEMUXER == 1)
    REGISTER_DEMUXER (BFI,              bfi);
#endif
#if (CONFIG_BINTEXT_DEMUXER == 1)
    REGISTER_DEMUXER (BINTEXT,          bintext);
#endif
#if (CONFIG_BINK_DEMUXER == 1)
    REGISTER_DEMUXER (BINK,             bink);
#endif
#if (CONFIG_BIT_DEMUXER == 1)
    REGISTER_DEMUXER (BIT,              bit);
#endif
#if (CONFIG_BMV_DEMUXER == 1)
    REGISTER_DEMUXER (BMV,              bmv);
#endif
#if (CONFIG_BRSTM_DEMUXER == 1)
    REGISTER_DEMUXER (BRSTM,            brstm);
#endif
#if (CONFIG_BOA_DEMUXER == 1)
    REGISTER_DEMUXER (BOA,              boa);
#endif
#if (CONFIG_C93_DEMUXER == 1)
    REGISTER_DEMUXER (C93,              c93);
#endif
#if (CONFIG_CAF_DEMUXER == 1)
    REGISTER_DEMUXER (CAF,              caf);
#endif
#if (CONFIG_CAVSVIDEO_DEMUXER == 1)
    REGISTER_DEMUXER (CAVSVIDEO,        cavsvideo);
#endif
#if (CONFIG_CDG_DEMUXER == 1)
    REGISTER_DEMUXER (CDG,              cdg);
#endif
#if (CONFIG_CDXL_DEMUXER == 1)
    REGISTER_DEMUXER (CDXL,             cdxl);
#endif
#if (CONFIG_CINE_DEMUXER == 1)
    REGISTER_DEMUXER (CINE,             cine);
#endif
#if (CONFIG_CONCAT_DEMUXER == 1)
    REGISTER_DEMUXER (CONCAT,           concat);
#endif
#if (CONFIG_DATA_DEMUXER == 1)
    REGISTER_DEMUXER (DATA,             data);
#endif
#if (CONFIG_DAUD_DEMUXER == 1)
    REGISTER_DEMUXER (DAUD,             daud);
#endif
#if (CONFIG_DFA_DEMUXER == 1)
    REGISTER_DEMUXER (DFA,              dfa);
#endif
#if (CONFIG_DIRAC_DEMUXER == 1)
    REGISTER_DEMUXER (DIRAC,            dirac);
#endif
#if (CONFIG_DNXHD_DEMUXER == 1)
    REGISTER_DEMUXER (DNXHD,            dnxhd);
#endif
#if (CONFIG_DSF_DEMUXER == 1)
    REGISTER_DEMUXER (DSF,              dsf);
#endif
#if (CONFIG_DSICIN_DEMUXER == 1)
    REGISTER_DEMUXER (DSICIN,           dsicin);
#endif
#if (CONFIG_DTS_DEMUXER == 1)
    REGISTER_DEMUXER (DTS,              dts);
#endif
#if (CONFIG_DTSHD_DEMUXER == 1)
    REGISTER_DEMUXER (DTSHD,            dtshd);
#endif
#if (CONFIG_DV_DEMUXER == 1)
    REGISTER_DEMUXER (DV,               dv);
#endif
#if (CONFIG_DXA_DEMUXER == 1)
    REGISTER_DEMUXER (DXA,              dxa);
#endif
#if (CONFIG_EA_DEMUXER == 1)
    REGISTER_DEMUXER (EA,               ea);
#endif
#if (CONFIG_EA_CDATA_DEMUXER == 1)
    REGISTER_DEMUXER (EA_CDATA,         ea_cdata);
#endif
#if (CONFIG_EAC3_DEMUXER == 1)
    REGISTER_DEMUXER (EAC3,             eac3);
#endif
#if (CONFIG_EPAF_DEMUXER == 1)
    REGISTER_DEMUXER (EPAF,             epaf);
#endif
#if (CONFIG_FFM_DEMUXER == 1)
    REGISTER_DEMUXER (FFM,              ffm);
#endif
#if (CONFIG_FFMETADATA_DEMUXER == 1)
    REGISTER_DEMUXER (FFMETADATA,       ffmetadata);
#endif
#if (CONFIG_FILMSTRIP_DEMUXER == 1)
    REGISTER_DEMUXER (FILMSTRIP,        filmstrip);
#endif
#if (CONFIG_FLAC_DEMUXER == 1)
    REGISTER_DEMUXER (FLAC,             flac);
#endif
#if (CONFIG_FLIC_DEMUXER == 1)
    REGISTER_DEMUXER (FLIC,             flic);
#endif
#if (CONFIG_FLV_DEMUXER == 1)
    REGISTER_DEMUXER (FLV,              flv);
#endif
#if (CONFIG_LIVE_FLV_DEMUXER == 1)
    REGISTER_DEMUXER (LIVE_FLV,         live_flv);
#endif
#if (CONFIG_FOURXM_DEMUXER == 1)
    REGISTER_DEMUXER (FOURXM,           fourxm);
#endif
#if (CONFIG_FRM_DEMUXER == 1)
    REGISTER_DEMUXER (FRM,              frm);
#endif
#if (CONFIG_G722_DEMUXER == 1)
    REGISTER_DEMUXER (G722,             g722);
#endif
#if (CONFIG_G723_1_DEMUXER == 1)
    REGISTER_DEMUXER (G723_1,           g723_1);
#endif
#if (CONFIG_G729_DEMUXER == 1)
    REGISTER_DEMUXER (G729,             g729);
#endif
#if (CONFIG_GIF_DEMUXER == 1)
    REGISTER_DEMUXER (GIF,              gif);
#endif
#if (CONFIG_GSM_DEMUXER == 1)
    REGISTER_DEMUXER (GSM,              gsm);
#endif
#if (CONFIG_GXF_DEMUXER == 1)
    REGISTER_DEMUXER (GXF,              gxf);
#endif
#if (CONFIG_H261_DEMUXER == 1)
    REGISTER_DEMUXER (H261,             h261);
#endif
#if (CONFIG_H263_DEMUXER == 1)
    REGISTER_DEMUXER (H263,             h263);
#endif
#if (CONFIG_H264_DEMUXER == 1)
    REGISTER_DEMUXER (H264,             h264);
#endif
#if (CONFIG_HEVC_DEMUXER == 1)
    REGISTER_DEMUXER (HEVC,             hevc);
#endif
#if (CONFIG_HLS_DEMUXER == 1)
    REGISTER_DEMUXER (HLS,              hls);
#endif
#if (CONFIG_HNM_DEMUXER == 1)
    REGISTER_DEMUXER (HNM,              hnm);
#endif
#if (CONFIG_ICO_DEMUXER == 1)
    REGISTER_DEMUXER (ICO,              ico);
#endif
#if (CONFIG_IDCIN_DEMUXER == 1)
    REGISTER_DEMUXER (IDCIN,            idcin);
#endif
#if (CONFIG_IDF_DEMUXER == 1)
    REGISTER_DEMUXER (IDF,              idf);
#endif
#if (CONFIG_IFF_DEMUXER == 1)
    REGISTER_DEMUXER (IFF,              iff);
#endif
#if (CONFIG_ILBC_DEMUXER == 1)
    REGISTER_DEMUXER (ILBC,             ilbc);
#endif
#if (CONFIG_IMAGE2_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE2,           image2);
#endif
#if (CONFIG_IMAGE2PIPE_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE2PIPE,       image2pipe);
#endif
#if (CONFIG_IMAGE2_ALIAS_PIX_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE2_ALIAS_PIX, image2_alias_pix);
#endif
#if (CONFIG_IMAGE2_BRENDER_PIX_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE2_BRENDER_PIX, image2_brender_pix);
#endif
#if (CONFIG_INGENIENT_DEMUXER == 1)
    REGISTER_DEMUXER (INGENIENT,        ingenient);
#endif
#if (CONFIG_IPMOVIE_DEMUXER == 1)
    REGISTER_DEMUXER (IPMOVIE,          ipmovie);
#endif
#if (CONFIG_IRCAM_DEMUXER == 1)
    REGISTER_DEMUXER (IRCAM,            ircam);
#endif
#if (CONFIG_ISS_DEMUXER == 1)
    REGISTER_DEMUXER (ISS,              iss);
#endif
#if (CONFIG_IV8_DEMUXER == 1)
    REGISTER_DEMUXER (IV8,              iv8);
#endif
#if (CONFIG_IVF_DEMUXER == 1)
    REGISTER_DEMUXER (IVF,              ivf);
#endif
#if (CONFIG_JACOSUB_DEMUXER == 1)
    REGISTER_DEMUXER (JACOSUB,          jacosub);
#endif
#if (CONFIG_JV_DEMUXER == 1)
    REGISTER_DEMUXER (JV,               jv);
#endif
#if (CONFIG_LATM_DEMUXER == 1)
    REGISTER_DEMUXER (LATM,             latm);
#endif
#if (CONFIG_LMLM4_DEMUXER == 1)
    REGISTER_DEMUXER (LMLM4,            lmlm4);
#endif
#if (CONFIG_LOAS_DEMUXER == 1)
    REGISTER_DEMUXER (LOAS,             loas);
#endif
#if (CONFIG_LRC_DEMUXER == 1)
    REGISTER_DEMUXER (LRC,              lrc);
#endif
#if (CONFIG_LVF_DEMUXER == 1)
    REGISTER_DEMUXER (LVF,              lvf);
#endif
#if (CONFIG_LXF_DEMUXER == 1)
    REGISTER_DEMUXER (LXF,              lxf);
#endif
#if (CONFIG_M4V_DEMUXER == 1)
    REGISTER_DEMUXER (M4V,              m4v);
#endif
#if (CONFIG_MATROSKA_DEMUXER == 1)
    REGISTER_DEMUXER (MATROSKA,         matroska);
#endif
#if (CONFIG_MGSTS_DEMUXER == 1)
    REGISTER_DEMUXER (MGSTS,            mgsts);
#endif
#if (CONFIG_MICRODVD_DEMUXER == 1)
    REGISTER_DEMUXER (MICRODVD,         microdvd);
#endif
#if (CONFIG_MJPEG_DEMUXER == 1)
    REGISTER_DEMUXER (MJPEG,            mjpeg);
#endif
#if (CONFIG_MLP_DEMUXER == 1)
    REGISTER_DEMUXER (MLP,              mlp);
#endif
#if (CONFIG_MLV_DEMUXER == 1)
    REGISTER_DEMUXER (MLV,              mlv);
#endif
#if (CONFIG_MM_DEMUXER == 1)
    REGISTER_DEMUXER (MM,               mm);
#endif
#if (CONFIG_MMF_DEMUXER == 1)
    REGISTER_DEMUXER (MMF,              mmf);
#endif
#if (CONFIG_MOV_DEMUXER == 1)
    REGISTER_DEMUXER (MOV,              mov);
#endif
#if (CONFIG_MP3_DEMUXER == 1)
    REGISTER_DEMUXER (MP3,              mp3);
#endif
#if (CONFIG_MPC_DEMUXER == 1)
    REGISTER_DEMUXER (MPC,              mpc);
#endif
#if (CONFIG_MPC8_DEMUXER == 1)
    REGISTER_DEMUXER (MPC8,             mpc8);
#endif
#if (CONFIG_MPEGPS_DEMUXER == 1)
    REGISTER_DEMUXER (MPEGPS,           mpegps);
#endif
#if (CONFIG_MPEGTS_DEMUXER == 1)
    REGISTER_DEMUXER (MPEGTS,           mpegts);
#endif
#if (CONFIG_MPEGTSRAW_DEMUXER == 1)
    REGISTER_DEMUXER (MPEGTSRAW,        mpegtsraw);
#endif
#if (CONFIG_MPEGVIDEO_DEMUXER == 1)
    REGISTER_DEMUXER (MPEGVIDEO,        mpegvideo);
#endif
#if (CONFIG_MPL2_DEMUXER == 1)
    REGISTER_DEMUXER (MPL2,             mpl2);
#endif
#if (CONFIG_MPSUB_DEMUXER == 1)
    REGISTER_DEMUXER (MPSUB,            mpsub);
#endif
#if (CONFIG_MSNWC_TCP_DEMUXER == 1)
    REGISTER_DEMUXER (MSNWC_TCP,        msnwc_tcp);
#endif
#if (CONFIG_MTV_DEMUXER == 1)
    REGISTER_DEMUXER (MTV,              mtv);
#endif
#if (CONFIG_MV_DEMUXER == 1)
    REGISTER_DEMUXER (MV,               mv);
#endif
#if (CONFIG_MVI_DEMUXER == 1)
    REGISTER_DEMUXER (MVI,              mvi);
#endif
#if (CONFIG_MXF_DEMUXER == 1)
    REGISTER_DEMUXER (MXF,              mxf);
#endif
#if (CONFIG_MXG_DEMUXER == 1)
    REGISTER_DEMUXER (MXG,              mxg);
#endif
#if (CONFIG_NC_DEMUXER == 1)
    REGISTER_DEMUXER (NC,               nc);
#endif
#if (CONFIG_NISTSPHERE_DEMUXER == 1)
    REGISTER_DEMUXER (NISTSPHERE,       nistsphere);
#endif
#if (CONFIG_NSV_DEMUXER == 1)
    REGISTER_DEMUXER (NSV,              nsv);
#endif
#if (CONFIG_NUT_DEMUXER == 1)
    REGISTER_DEMUXER (NUT,              nut);
#endif
#if (CONFIG_NUV_DEMUXER == 1)
    REGISTER_DEMUXER (NUV,              nuv);
#endif
#if (CONFIG_OGG_DEMUXER == 1)
    REGISTER_DEMUXER (OGG,              ogg);
#endif
#if (CONFIG_OMA_DEMUXER == 1)
    REGISTER_DEMUXER (OMA,              oma);
#endif
#if (CONFIG_PAF_DEMUXER == 1)
    REGISTER_DEMUXER (PAF,              paf);
#endif
#if (CONFIG_PCM_ALAW_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_ALAW,         pcm_alaw);
#endif
#if (CONFIG_PCM_MULAW_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_MULAW,        pcm_mulaw);
#endif
#if (CONFIG_PCM_F64BE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_F64BE,        pcm_f64be);
#endif
#if (CONFIG_PCM_F64LE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_F64LE,        pcm_f64le);
#endif
#if (CONFIG_PCM_F32BE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_F32BE,        pcm_f32be);
#endif
#if (CONFIG_PCM_F32LE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_F32LE,        pcm_f32le);
#endif
#if (CONFIG_PCM_S32BE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_S32BE,        pcm_s32be);
#endif
#if (CONFIG_PCM_S32LE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_S32LE,        pcm_s32le);
#endif
#if (CONFIG_PCM_S24BE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_S24BE,        pcm_s24be);
#endif
#if (CONFIG_PCM_S24LE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_S24LE,        pcm_s24le);
#endif
#if (CONFIG_PCM_S16BE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_S16BE,        pcm_s16be);
#endif
#if (CONFIG_PCM_S16LE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_S16LE,        pcm_s16le);
#endif
#if (CONFIG_PCM_S8_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_S8,           pcm_s8);
#endif
#if (CONFIG_PCM_U32BE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_U32BE,        pcm_u32be);
#endif
#if (CONFIG_PCM_U32LE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_U32LE,        pcm_u32le);
#endif
#if (CONFIG_PCM_U24BE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_U24BE,        pcm_u24be);
#endif
#if (CONFIG_PCM_U24LE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_U24LE,        pcm_u24le);
#endif
#if (CONFIG_PCM_U16BE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_U16BE,        pcm_u16be);
#endif
#if (CONFIG_PCM_U16LE_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_U16LE,        pcm_u16le);
#endif
#if (CONFIG_PCM_U8_DEMUXER == 1)
    REGISTER_DEMUXER (PCM_U8,           pcm_u8);
#endif
#if (CONFIG_PJS_DEMUXER == 1)
    REGISTER_DEMUXER (PJS,              pjs);
#endif
#if (CONFIG_PMP_DEMUXER == 1)
    REGISTER_DEMUXER (PMP,              pmp);
#endif
#if (CONFIG_PVA_DEMUXER == 1)
    REGISTER_DEMUXER (PVA,              pva);
#endif
#if (CONFIG_PVF_DEMUXER == 1)
    REGISTER_DEMUXER (PVF,              pvf);
#endif
#if (CONFIG_QCP_DEMUXER == 1)
    REGISTER_DEMUXER (QCP,              qcp);
#endif
#if (CONFIG_R3D_DEMUXER == 1)
    REGISTER_DEMUXER (R3D,              r3d);
#endif
#if (CONFIG_RAWVIDEO_DEMUXER == 1)
    REGISTER_DEMUXER (RAWVIDEO,         rawvideo);
#endif
#if (CONFIG_REALTEXT_DEMUXER == 1)
    REGISTER_DEMUXER (REALTEXT,         realtext);
#endif
#if (CONFIG_REDSPARK_DEMUXER == 1)
    REGISTER_DEMUXER (REDSPARK,         redspark);
#endif
#if (CONFIG_RL2_DEMUXER == 1)
    REGISTER_DEMUXER (RL2,              rl2);
#endif
#if (CONFIG_RM_DEMUXER == 1)
    REGISTER_DEMUXER (RM,               rm);
#endif
#if (CONFIG_ROQ_DEMUXER == 1)
    REGISTER_DEMUXER (ROQ,              roq);
#endif
#if (CONFIG_RPL_DEMUXER == 1)
    REGISTER_DEMUXER (RPL,              rpl);
#endif
#if (CONFIG_RSD_DEMUXER == 1)
    REGISTER_DEMUXER (RSD,              rsd);
#endif
#if (CONFIG_RSO_DEMUXER == 1)
    REGISTER_DEMUXER (RSO,              rso);
#endif
#if (CONFIG_RTP_DEMUXER == 1)
    REGISTER_DEMUXER (RTP,              rtp);
#endif
#if (CONFIG_RTSP_DEMUXER == 1)
    REGISTER_DEMUXER (RTSP,             rtsp);
#endif
#if (CONFIG_SAMI_DEMUXER == 1)
    REGISTER_DEMUXER (SAMI,             sami);
#endif
#if (CONFIG_SAP_DEMUXER == 1)
    REGISTER_DEMUXER (SAP,              sap);
#endif
#if (CONFIG_SBG_DEMUXER == 1)
    REGISTER_DEMUXER (SBG,              sbg);
#endif
#if (CONFIG_SDP_DEMUXER == 1)
    REGISTER_DEMUXER (SDP,              sdp);
#endif
#if (CONFIG_SDR2_DEMUXER == 1)
    REGISTER_DEMUXER (SDR2,             sdr2);
#endif
#if CONFIG_RTPDEC
    ff_register_rtp_dynamic_payload_handlers();
    ff_register_rdt_dynamic_payload_handlers();
#endif
#if (CONFIG_SEGAFILM_DEMUXER == 1)
    REGISTER_DEMUXER (SEGAFILM,         segafilm);
#endif
#if (CONFIG_SHORTEN_DEMUXER == 1)
    REGISTER_DEMUXER (SHORTEN,          shorten);
#endif
#if (CONFIG_SIFF_DEMUXER == 1)
    REGISTER_DEMUXER (SIFF,             siff);
#endif
#if (CONFIG_SLN_DEMUXER == 1)
    REGISTER_DEMUXER (SLN,              sln);
#endif
#if (CONFIG_SMACKER_DEMUXER == 1)
    REGISTER_DEMUXER (SMACKER,          smacker);
#endif
#if (CONFIG_SMJPEG_DEMUXER == 1)
    REGISTER_DEMUXER (SMJPEG,           smjpeg);
#endif
#if (CONFIG_SMUSH_DEMUXER == 1)
    REGISTER_DEMUXER (SMUSH,            smush);
#endif
#if (CONFIG_SOL_DEMUXER == 1)
    REGISTER_DEMUXER (SOL,              sol);
#endif
#if (CONFIG_SOX_DEMUXER == 1)
    REGISTER_DEMUXER (SOX,              sox);
#endif
#if (CONFIG_SPDIF_DEMUXER == 1)
    REGISTER_DEMUXER (SPDIF,            spdif);
#endif
#if (CONFIG_SRT_DEMUXER == 1)
    REGISTER_DEMUXER (SRT,              srt);
#endif
#if (CONFIG_STR_DEMUXER == 1)
    REGISTER_DEMUXER (STR,              str);
#endif
#if (CONFIG_SUBVIEWER1_DEMUXER == 1)
    REGISTER_DEMUXER (SUBVIEWER1,       subviewer1);
#endif
#if (CONFIG_SUBVIEWER_DEMUXER == 1)
    REGISTER_DEMUXER (SUBVIEWER,        subviewer);
#endif
#if (CONFIG_SWF_DEMUXER == 1)
    REGISTER_DEMUXER (SWF,              swf);
#endif
#if (CONFIG_TAK_DEMUXER == 1)
    REGISTER_DEMUXER (TAK,              tak);
#endif
#if (CONFIG_TEDCAPTIONS_DEMUXER == 1)
    REGISTER_DEMUXER (TEDCAPTIONS,      tedcaptions);
#endif
#if (CONFIG_THP_DEMUXER == 1)
    REGISTER_DEMUXER (THP,              thp);
#endif
#if (CONFIG_TIERTEXSEQ_DEMUXER == 1)
    REGISTER_DEMUXER (TIERTEXSEQ,       tiertexseq);
#endif
#if (CONFIG_TMV_DEMUXER == 1)
    REGISTER_DEMUXER (TMV,              tmv);
#endif
#if (CONFIG_TRUEHD_DEMUXER == 1)
    REGISTER_DEMUXER (TRUEHD,           truehd);
#endif
#if (CONFIG_TTA_DEMUXER == 1)
    REGISTER_DEMUXER (TTA,              tta);
#endif
#if (CONFIG_TXD_DEMUXER == 1)
    REGISTER_DEMUXER (TXD,              txd);
#endif
#if (CONFIG_TTY_DEMUXER == 1)
    REGISTER_DEMUXER (TTY,              tty);
#endif
#if (CONFIG_VC1_DEMUXER == 1)
    REGISTER_DEMUXER (VC1,              vc1);
#endif
#if (CONFIG_VC1T_DEMUXER == 1)
    REGISTER_DEMUXER (VC1T,             vc1t);
#endif
#if (CONFIG_VIVO_DEMUXER == 1)
    REGISTER_DEMUXER (VIVO,             vivo);
#endif
#if (CONFIG_VMD_DEMUXER == 1)
    REGISTER_DEMUXER (VMD,              vmd);
#endif
#if (CONFIG_VOBSUB_DEMUXER == 1)
    REGISTER_DEMUXER (VOBSUB,           vobsub);
#endif
#if (CONFIG_VOC_DEMUXER == 1)
    REGISTER_DEMUXER (VOC,              voc);
#endif
#if (CONFIG_VPLAYER_DEMUXER == 1)
    REGISTER_DEMUXER (VPLAYER,          vplayer);
#endif
#if (CONFIG_VQF_DEMUXER == 1)
    REGISTER_DEMUXER (VQF,              vqf);
#endif
#if (CONFIG_W64_DEMUXER == 1)
    REGISTER_DEMUXER (W64,              w64);
#endif
#if (CONFIG_WAV_DEMUXER == 1)
    REGISTER_DEMUXER (WAV,              wav);
#endif
#if (CONFIG_WC3_DEMUXER == 1)
    REGISTER_DEMUXER (WC3,              wc3);
#endif
#if (CONFIG_WEBM_DASH_MANIFEST_DEMUXER == 1)
    REGISTER_DEMUXER (WEBM_DASH_MANIFEST, webm_dash_manifest);
#endif
#if (CONFIG_WEBVTT_DEMUXER == 1)
    REGISTER_DEMUXER (WEBVTT,           webvtt);
#endif
#if (CONFIG_WSAUD_DEMUXER == 1)
    REGISTER_DEMUXER (WSAUD,            wsaud);
#endif
#if (CONFIG_WSVQA_DEMUXER == 1)
    REGISTER_DEMUXER (WSVQA,            wsvqa);
#endif
#if (CONFIG_WTV_DEMUXER == 1)
    REGISTER_DEMUXER (WTV,              wtv);
#endif
#if (CONFIG_WV_DEMUXER == 1)
    REGISTER_DEMUXER (WV,               wv);
#endif
#if (CONFIG_XA_DEMUXER == 1)
    REGISTER_DEMUXER (XA,               xa);
#endif
#if (CONFIG_XBIN_DEMUXER == 1)
    REGISTER_DEMUXER (XBIN,             xbin);
#endif
#if (CONFIG_XMV_DEMUXER == 1)
    REGISTER_DEMUXER (XMV,              xmv);
#endif
#if (CONFIG_XWMA_DEMUXER == 1)
    REGISTER_DEMUXER (XWMA,             xwma);
#endif
#if (CONFIG_YOP_DEMUXER == 1)
    REGISTER_DEMUXER (YOP,              yop);
#endif
#if (CONFIG_YUV4MPEGPIPE_DEMUXER == 1)
    REGISTER_DEMUXER (YUV4MPEGPIPE,     yuv4mpegpipe);
#endif

    /* image demuxers */
#if (CONFIG_IMAGE_BMP_PIPE_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE_BMP_PIPE,        image_bmp_pipe);
#endif
#if (CONFIG_IMAGE_DPX_PIPE_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE_DPX_PIPE,        image_dpx_pipe);
#endif
#if (CONFIG_IMAGE_EXR_PIPE_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE_EXR_PIPE,        image_exr_pipe);
#endif
#if (CONFIG_IMAGE_J2K_PIPE_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE_J2K_PIPE,        image_j2k_pipe);
#endif
#if (CONFIG_IMAGE_JPEGLS_PIPE_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE_JPEGLS_PIPE,     image_jpegls_pipe);
#endif
#if (CONFIG_IMAGE_PICTOR_PIPE_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE_PICTOR_PIPE,     image_pictor_pipe);
#endif
#if (CONFIG_IMAGE_PNG_PIPE_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE_PNG_PIPE,        image_png_pipe);
#endif
#if (CONFIG_IMAGE_SGI_PIPE_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE_SGI_PIPE,        image_sgi_pipe);
#endif
#if (CONFIG_IMAGE_SUNRAST_PIPE_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE_SUNRAST_PIPE,    image_sunrast_pipe);
#endif
#if (CONFIG_IMAGE_TIFF_PIPE_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE_TIFF_PIPE,       image_tiff_pipe);
#endif
#if (CONFIG_IMAGE_WEBP_PIPE_DEMUXER == 1)
    REGISTER_DEMUXER (IMAGE_WEBP_PIPE,       image_webp_pipe);
#endif

    /* external libraries */
#if (CONFIG_LIBGME_DEMUXER == 1)
    REGISTER_DEMUXER (LIBGME,           libgme);
#endif
#if (CONFIG_LIBMODPLUG_DEMUXER == 1)
    REGISTER_DEMUXER (LIBMODPLUG,       libmodplug);
#endif
#if (CONFIG_LIBNUT_DEMUXER == 1)
    REGISTER_DEMUXER (LIBNUT,           libnut);
#endif
#if (CONFIG_LIBQUVI_DEMUXER == 1)
    REGISTER_DEMUXER (LIBQUVI,          libquvi);
#endif

    /* protocols */
#if (CONFIG_BLURAY_PROTOCOL == 1)
    REGISTER_PROTOCOL(BLURAY,           bluray);
#endif
#if (CONFIG_CACHE_PROTOCOL == 1)
    REGISTER_PROTOCOL(CACHE,            cache);
#endif
#if (CONFIG_CONCAT_PROTOCOL == 1)
    REGISTER_PROTOCOL(CONCAT,           concat);
#endif
#if (CONFIG_CRYPTO_PROTOCOL == 1)
    REGISTER_PROTOCOL(CRYPTO,           crypto);
#endif
#if (CONFIG_DATA_PROTOCOL == 1)
    REGISTER_PROTOCOL(DATA,             data);
#endif
#if (CONFIG_FFRTMPCRYPT_PROTOCOL == 1)
    REGISTER_PROTOCOL(FFRTMPCRYPT,      ffrtmpcrypt);
#endif
#if (CONFIG_FFRTMPHTTP_PROTOCOL == 1)
    REGISTER_PROTOCOL(FFRTMPHTTP,       ffrtmphttp);
#endif
#if (CONFIG_FILE_PROTOCOL == 1)
    REGISTER_PROTOCOL(FILE,             file);
#endif
#if (CONFIG_FTP_PROTOCOL == 1)
    REGISTER_PROTOCOL(FTP,              ftp);
#endif
#if (CONFIG_GOPHER_PROTOCOL == 1)
    REGISTER_PROTOCOL(GOPHER,           gopher);
#endif
#if (CONFIG_HLS_PROTOCOL == 1)
    REGISTER_PROTOCOL(HLS,              hls);
#endif
#if (CONFIG_HTTP_PROTOCOL == 1)
    REGISTER_PROTOCOL(HTTP,             http);
#endif
#if (CONFIG_HTTPPROXY_PROTOCOL == 1)
    REGISTER_PROTOCOL(HTTPPROXY,        httpproxy);
#endif
#if (CONFIG_HTTPS_PROTOCOL == 1)
    REGISTER_PROTOCOL(HTTPS,            https);
#endif
#if (CONFIG_ICECAST_PROTOCOL == 1)
    REGISTER_PROTOCOL(ICECAST,          icecast);
#endif
#if (CONFIG_MMSH_PROTOCOL == 1)
    REGISTER_PROTOCOL(MMSH,             mmsh);
#endif
#if (CONFIG_MMST_PROTOCOL == 1)
    REGISTER_PROTOCOL(MMST,             mmst);
#endif
#if (CONFIG_MD5_PROTOCOL == 1)
    REGISTER_PROTOCOL(MD5,              md5);
#endif
#if (CONFIG_PIPE_PROTOCOL == 1)
    REGISTER_PROTOCOL(PIPE,             pipe);
#endif
#if (CONFIG_RTMP_PROTOCOL == 1)
    REGISTER_PROTOCOL(RTMP,             rtmp);
#endif
#if (CONFIG_RTMPE_PROTOCOL == 1)
    REGISTER_PROTOCOL(RTMPE,            rtmpe);
#endif
#if (CONFIG_RTMPS_PROTOCOL == 1)
    REGISTER_PROTOCOL(RTMPS,            rtmps);
#endif
#if (CONFIG_RTMPT_PROTOCOL == 1)
    REGISTER_PROTOCOL(RTMPT,            rtmpt);
#endif
#if (CONFIG_RTMPTE_PROTOCOL == 1)
    REGISTER_PROTOCOL(RTMPTE,           rtmpte);
#endif
#if (CONFIG_RTMPTS_PROTOCOL == 1)
    REGISTER_PROTOCOL(RTMPTS,           rtmpts);
#endif
#if (CONFIG_RTP_PROTOCOL == 1)
    REGISTER_PROTOCOL(RTP,              rtp);
#endif
#if (CONFIG_SCTP_PROTOCOL == 1)
    REGISTER_PROTOCOL(SCTP,             sctp);
#endif
#if (CONFIG_SRTP_PROTOCOL == 1)
    REGISTER_PROTOCOL(SRTP,             srtp);
#endif
#if (CONFIG_SUBFILE_PROTOCOL == 1)
    REGISTER_PROTOCOL(SUBFILE,          subfile);
#endif
#if (CONFIG_TCP_PROTOCOL == 1)
    REGISTER_PROTOCOL(TCP,              tcp);
#endif
#if (CONFIG_TLS_PROTOCOL == 1)
    REGISTER_PROTOCOL(TLS,              tls);
#endif
#if (CONFIG_UDP_PROTOCOL == 1)
    REGISTER_PROTOCOL(UDP,              udp);
#endif
#if (CONFIG_UNIX_PROTOCOL == 1)
    REGISTER_PROTOCOL(UNIX,             unix);
#endif
#if (CONFIG_LIBRTMP_PROTOCOL == 1)
    REGISTER_PROTOCOL(LIBRTMP,          librtmp);
#endif
#if (CONFIG_LIBRTMPE_PROTOCOL == 1)
    REGISTER_PROTOCOL(LIBRTMPE,         librtmpe);
#endif
#if (CONFIG_LIBRTMPS_PROTOCOL == 1)
    REGISTER_PROTOCOL(LIBRTMPS,         librtmps);
#endif
#if (CONFIG_LIBRTMPT_PROTOCOL == 1)
    REGISTER_PROTOCOL(LIBRTMPT,         librtmpt);
#endif
#if (CONFIG_LIBRTMPTE_PROTOCOL == 1)
    REGISTER_PROTOCOL(LIBRTMPTE,        librtmpte);
#endif
#if (CONFIG_LIBSSH_PROTOCOL == 1)
    REGISTER_PROTOCOL(LIBSSH,           libssh);
#endif
#if (CONFIG_LIBSMBCLIENT_PROTOCOL == 1)
    REGISTER_PROTOCOL(LIBSMBCLIENT,     libsmbclient);
#endif
#else
    /* (de)muxers */
    REGISTER_MUXER   (A64,              a64);
    REGISTER_DEMUXER (AAC,              aac);
    REGISTER_MUXDEMUX(AC3,              ac3);
    REGISTER_DEMUXER (ACT,              act);
    REGISTER_DEMUXER (ADF,              adf);
    REGISTER_DEMUXER (ADP,              adp);
    REGISTER_MUXER   (ADTS,             adts);
    REGISTER_MUXDEMUX(ADX,              adx);
    REGISTER_DEMUXER (AEA,              aea);
    REGISTER_DEMUXER (AFC,              afc);
    REGISTER_MUXDEMUX(AIFF,             aiff);
    REGISTER_MUXDEMUX(AMR,              amr);
    REGISTER_DEMUXER (ANM,              anm);
    REGISTER_DEMUXER (APC,              apc);
    REGISTER_DEMUXER (APE,              ape);
    REGISTER_DEMUXER (AQTITLE,          aqtitle);
    REGISTER_MUXDEMUX(ASF,              asf);
    REGISTER_MUXDEMUX(ASS,              ass);
    REGISTER_MUXDEMUX(AST,              ast);
    REGISTER_MUXER   (ASF_STREAM,       asf_stream);
    REGISTER_MUXDEMUX(AU,               au);
    REGISTER_MUXDEMUX(AVI,              avi);
    REGISTER_DEMUXER (AVISYNTH,         avisynth);
    REGISTER_MUXER   (AVM2,             avm2);
    REGISTER_DEMUXER (AVR,              avr);
    REGISTER_DEMUXER (AVS,              avs);
    REGISTER_DEMUXER (BETHSOFTVID,      bethsoftvid);
    REGISTER_DEMUXER (BFI,              bfi);
    REGISTER_DEMUXER (BINTEXT,          bintext);
    REGISTER_DEMUXER (BINK,             bink);
    REGISTER_MUXDEMUX(BIT,              bit);
    REGISTER_DEMUXER (BMV,              bmv);
    REGISTER_DEMUXER (BRSTM,            brstm);
    REGISTER_DEMUXER (BOA,              boa);
    REGISTER_DEMUXER (C93,              c93);
    REGISTER_MUXDEMUX(CAF,              caf);
    REGISTER_MUXDEMUX(CAVSVIDEO,        cavsvideo);
    REGISTER_DEMUXER (CDG,              cdg);
    REGISTER_DEMUXER (CDXL,             cdxl);
    REGISTER_DEMUXER (CINE,             cine);
    REGISTER_DEMUXER (CONCAT,           concat);
    REGISTER_MUXER   (CRC,              crc);
    REGISTER_MUXDEMUX(DATA,             data);
    REGISTER_MUXDEMUX(DAUD,             daud);
    REGISTER_DEMUXER (DFA,              dfa);
    REGISTER_MUXDEMUX(DIRAC,            dirac);
    REGISTER_MUXDEMUX(DNXHD,            dnxhd);
    REGISTER_DEMUXER (DSF,              dsf);
    REGISTER_DEMUXER (DSICIN,           dsicin);
    REGISTER_MUXDEMUX(DTS,              dts);
    REGISTER_DEMUXER (DTSHD,            dtshd);
    REGISTER_MUXDEMUX(DV,               dv);
    REGISTER_DEMUXER (DXA,              dxa);
    REGISTER_DEMUXER (EA,               ea);
    REGISTER_DEMUXER (EA_CDATA,         ea_cdata);
    REGISTER_MUXDEMUX(EAC3,             eac3);
    REGISTER_DEMUXER (EPAF,             epaf);
    REGISTER_MUXER   (F4V,              f4v);
    REGISTER_MUXDEMUX(FFM,              ffm);
    REGISTER_MUXDEMUX(FFMETADATA,       ffmetadata);
    REGISTER_MUXDEMUX(FILMSTRIP,        filmstrip);
    REGISTER_MUXDEMUX(FLAC,             flac);
    REGISTER_DEMUXER (FLIC,             flic);
    REGISTER_MUXDEMUX(FLV,              flv);
    REGISTER_DEMUXER (LIVE_FLV,         live_flv);
    REGISTER_DEMUXER (FOURXM,           fourxm);
    REGISTER_MUXER   (FRAMECRC,         framecrc);
    REGISTER_MUXER   (FRAMEMD5,         framemd5);
    REGISTER_DEMUXER (FRM,              frm);
    REGISTER_MUXDEMUX(G722,             g722);
    REGISTER_MUXDEMUX(G723_1,           g723_1);
    REGISTER_DEMUXER (G729,             g729);
    REGISTER_MUXDEMUX(GIF,              gif);
    REGISTER_DEMUXER (GSM,              gsm);
    REGISTER_MUXDEMUX(GXF,              gxf);
    REGISTER_MUXDEMUX(H261,             h261);
    REGISTER_MUXDEMUX(H263,             h263);
    REGISTER_MUXDEMUX(H264,             h264);
    REGISTER_MUXER   (HDS,              hds);
    REGISTER_MUXDEMUX(HEVC,             hevc);
    REGISTER_MUXDEMUX(HLS,              hls);
    REGISTER_DEMUXER (HNM,              hnm);
    REGISTER_MUXDEMUX(ICO,              ico);
    REGISTER_DEMUXER (IDCIN,            idcin);
    REGISTER_DEMUXER (IDF,              idf);
    REGISTER_DEMUXER (IFF,              iff);
    REGISTER_MUXDEMUX(ILBC,             ilbc);
    REGISTER_MUXDEMUX(IMAGE2,           image2);
    REGISTER_MUXDEMUX(IMAGE2PIPE,       image2pipe);
    REGISTER_DEMUXER (IMAGE2_ALIAS_PIX, image2_alias_pix);
    REGISTER_DEMUXER (IMAGE2_BRENDER_PIX, image2_brender_pix);
    REGISTER_DEMUXER (INGENIENT,        ingenient);
    REGISTER_DEMUXER (IPMOVIE,          ipmovie);
    REGISTER_MUXER   (IPOD,             ipod);
    REGISTER_MUXDEMUX(IRCAM,            ircam);
    REGISTER_MUXER   (ISMV,             ismv);
    REGISTER_DEMUXER (ISS,              iss);
    REGISTER_DEMUXER (IV8,              iv8);
    REGISTER_MUXDEMUX(IVF,              ivf);
    REGISTER_MUXDEMUX(JACOSUB,          jacosub);
    REGISTER_DEMUXER (JV,               jv);
    REGISTER_MUXDEMUX(LATM,             latm);
    REGISTER_DEMUXER (LMLM4,            lmlm4);
    REGISTER_DEMUXER (LOAS,             loas);
    REGISTER_MUXDEMUX(LRC,              lrc);
    REGISTER_DEMUXER (LVF,              lvf);
    REGISTER_DEMUXER (LXF,              lxf);
    REGISTER_MUXDEMUX(M4V,              m4v);
    REGISTER_MUXER   (MD5,              md5);
    REGISTER_MUXDEMUX(MATROSKA,         matroska);
    REGISTER_MUXER   (MATROSKA_AUDIO,   matroska_audio);
    REGISTER_DEMUXER (MGSTS,            mgsts);
    REGISTER_MUXDEMUX(MICRODVD,         microdvd);
    REGISTER_MUXDEMUX(MJPEG,            mjpeg);
    REGISTER_MUXDEMUX(MLP,              mlp);
    REGISTER_DEMUXER (MLV,              mlv);
    REGISTER_DEMUXER (MM,               mm);
    REGISTER_MUXDEMUX(MMF,              mmf);
    REGISTER_MUXDEMUX(MOV,              mov);
    REGISTER_MUXER   (MP2,              mp2);
    REGISTER_MUXDEMUX(MP3,              mp3);
    REGISTER_MUXER   (MP4,              mp4);
    REGISTER_DEMUXER (MPC,              mpc);
    REGISTER_DEMUXER (MPC8,             mpc8);
    REGISTER_MUXER   (MPEG1SYSTEM,      mpeg1system);
    REGISTER_MUXER   (MPEG1VCD,         mpeg1vcd);
    REGISTER_MUXER   (MPEG1VIDEO,       mpeg1video);
    REGISTER_MUXER   (MPEG2DVD,         mpeg2dvd);
    REGISTER_MUXER   (MPEG2SVCD,        mpeg2svcd);
    REGISTER_MUXER   (MPEG2VIDEO,       mpeg2video);
    REGISTER_MUXER   (MPEG2VOB,         mpeg2vob);
    REGISTER_DEMUXER (MPEGPS,           mpegps);
    REGISTER_MUXDEMUX(MPEGTS,           mpegts);
    REGISTER_DEMUXER (MPEGTSRAW,        mpegtsraw);
    REGISTER_DEMUXER (MPEGVIDEO,        mpegvideo);
    REGISTER_MUXER   (MPJPEG,           mpjpeg);
    REGISTER_DEMUXER (MPL2,             mpl2);
    REGISTER_DEMUXER (MPSUB,            mpsub);
    REGISTER_DEMUXER (MSNWC_TCP,        msnwc_tcp);
    REGISTER_DEMUXER (MTV,              mtv);
    REGISTER_DEMUXER (MV,               mv);
    REGISTER_DEMUXER (MVI,              mvi);
    REGISTER_MUXDEMUX(MXF,              mxf);
    REGISTER_MUXER   (MXF_D10,          mxf_d10);
    REGISTER_DEMUXER (MXG,              mxg);
    REGISTER_DEMUXER (NC,               nc);
    REGISTER_DEMUXER (NISTSPHERE,       nistsphere);
    REGISTER_DEMUXER (NSV,              nsv);
    REGISTER_MUXER   (NULL,             null);
    REGISTER_MUXDEMUX(NUT,              nut);
    REGISTER_DEMUXER (NUV,              nuv);
    REGISTER_MUXER   (OGA,              oga);
    REGISTER_MUXDEMUX(OGG,              ogg);
    REGISTER_MUXDEMUX(OMA,              oma);
    REGISTER_MUXER   (OPUS,             opus);
    REGISTER_DEMUXER (PAF,              paf);
    REGISTER_MUXDEMUX(PCM_ALAW,         pcm_alaw);
    REGISTER_MUXDEMUX(PCM_MULAW,        pcm_mulaw);
    REGISTER_MUXDEMUX(PCM_F64BE,        pcm_f64be);
    REGISTER_MUXDEMUX(PCM_F64LE,        pcm_f64le);
    REGISTER_MUXDEMUX(PCM_F32BE,        pcm_f32be);
    REGISTER_MUXDEMUX(PCM_F32LE,        pcm_f32le);
    REGISTER_MUXDEMUX(PCM_S32BE,        pcm_s32be);
    REGISTER_MUXDEMUX(PCM_S32LE,        pcm_s32le);
    REGISTER_MUXDEMUX(PCM_S24BE,        pcm_s24be);
    REGISTER_MUXDEMUX(PCM_S24LE,        pcm_s24le);
    REGISTER_MUXDEMUX(PCM_S16BE,        pcm_s16be);
    REGISTER_MUXDEMUX(PCM_S16LE,        pcm_s16le);
    REGISTER_MUXDEMUX(PCM_S8,           pcm_s8);
    REGISTER_MUXDEMUX(PCM_U32BE,        pcm_u32be);
    REGISTER_MUXDEMUX(PCM_U32LE,        pcm_u32le);
    REGISTER_MUXDEMUX(PCM_U24BE,        pcm_u24be);
    REGISTER_MUXDEMUX(PCM_U24LE,        pcm_u24le);
    REGISTER_MUXDEMUX(PCM_U16BE,        pcm_u16be);
    REGISTER_MUXDEMUX(PCM_U16LE,        pcm_u16le);
    REGISTER_MUXDEMUX(PCM_U8,           pcm_u8);
    REGISTER_DEMUXER (PJS,              pjs);
    REGISTER_DEMUXER (PMP,              pmp);
    REGISTER_MUXER   (PSP,              psp);
    REGISTER_DEMUXER (PVA,              pva);
    REGISTER_DEMUXER (PVF,              pvf);
    REGISTER_DEMUXER (QCP,              qcp);
    REGISTER_DEMUXER (R3D,              r3d);
    REGISTER_MUXDEMUX(RAWVIDEO,         rawvideo);
    REGISTER_DEMUXER (REALTEXT,         realtext);
    REGISTER_DEMUXER (REDSPARK,         redspark);
    REGISTER_DEMUXER (RL2,              rl2);
    REGISTER_MUXDEMUX(RM,               rm);
    REGISTER_MUXDEMUX(ROQ,              roq);
    REGISTER_DEMUXER (RPL,              rpl);
    REGISTER_DEMUXER (RSD,              rsd);
    REGISTER_MUXDEMUX(RSO,              rso);
    REGISTER_MUXDEMUX(RTP,              rtp);
    REGISTER_MUXDEMUX(RTSP,             rtsp);
    REGISTER_DEMUXER (SAMI,             sami);
    REGISTER_MUXDEMUX(SAP,              sap);
    REGISTER_DEMUXER (SBG,              sbg);
    REGISTER_DEMUXER (SDP,              sdp);
    REGISTER_DEMUXER (SDR2,             sdr2);
#if CONFIG_RTPDEC
    ff_register_rtp_dynamic_payload_handlers();
    ff_register_rdt_dynamic_payload_handlers();
#endif
    REGISTER_DEMUXER (SEGAFILM,         segafilm);
    REGISTER_MUXER   (SEGMENT,          segment);
    REGISTER_MUXER   (SEGMENT,          stream_segment);
    REGISTER_DEMUXER (SHORTEN,          shorten);
    REGISTER_DEMUXER (SIFF,             siff);
    REGISTER_DEMUXER (SLN,              sln);
    REGISTER_DEMUXER (SMACKER,          smacker);
    REGISTER_MUXDEMUX(SMJPEG,           smjpeg);
    REGISTER_MUXER   (SMOOTHSTREAMING,  smoothstreaming);
    REGISTER_DEMUXER (SMUSH,            smush);
    REGISTER_DEMUXER (SOL,              sol);
    REGISTER_MUXDEMUX(SOX,              sox);
    REGISTER_MUXER   (SPX,              spx);
    REGISTER_MUXDEMUX(SPDIF,            spdif);
    REGISTER_MUXDEMUX(SRT,              srt);
    REGISTER_DEMUXER (STR,              str);
    REGISTER_DEMUXER (SUBVIEWER1,       subviewer1);
    REGISTER_DEMUXER (SUBVIEWER,        subviewer);
    REGISTER_MUXDEMUX(SWF,              swf);
    REGISTER_DEMUXER (TAK,              tak);
    REGISTER_MUXER   (TEE,              tee);
    REGISTER_DEMUXER (TEDCAPTIONS,      tedcaptions);
    REGISTER_MUXER   (TG2,              tg2);
    REGISTER_MUXER   (TGP,              tgp);
    REGISTER_DEMUXER (THP,              thp);
    REGISTER_DEMUXER (TIERTEXSEQ,       tiertexseq);
    REGISTER_MUXER   (MKVTIMESTAMP_V2,  mkvtimestamp_v2);
    REGISTER_DEMUXER (TMV,              tmv);
    REGISTER_MUXDEMUX(TRUEHD,           truehd);
    REGISTER_DEMUXER (TTA,              tta);
    REGISTER_DEMUXER (TXD,              txd);
    REGISTER_DEMUXER (TTY,              tty);
    REGISTER_MUXER   (UNCODEDFRAMECRC,  uncodedframecrc);
    REGISTER_MUXDEMUX(VC1,              vc1);
    REGISTER_MUXDEMUX(VC1T,             vc1t);
    REGISTER_DEMUXER (VIVO,             vivo);
    REGISTER_DEMUXER (VMD,              vmd);
    REGISTER_DEMUXER (VOBSUB,           vobsub);
    REGISTER_MUXDEMUX(VOC,              voc);
    REGISTER_DEMUXER (VPLAYER,          vplayer);
    REGISTER_DEMUXER (VQF,              vqf);
    REGISTER_MUXDEMUX(W64,              w64);
    REGISTER_MUXDEMUX(WAV,              wav);
    REGISTER_DEMUXER (WC3,              wc3);
    REGISTER_MUXER   (WEBM,             webm);
    REGISTER_MUXDEMUX(WEBM_DASH_MANIFEST, webm_dash_manifest);
    REGISTER_MUXDEMUX(WEBVTT,           webvtt);
    REGISTER_DEMUXER (WSAUD,            wsaud);
    REGISTER_DEMUXER (WSVQA,            wsvqa);
    REGISTER_MUXDEMUX(WTV,              wtv);
    REGISTER_MUXDEMUX(WV,               wv);
    REGISTER_DEMUXER (XA,               xa);
    REGISTER_DEMUXER (XBIN,             xbin);
    REGISTER_DEMUXER (XMV,              xmv);
    REGISTER_DEMUXER (XWMA,             xwma);
    REGISTER_DEMUXER (YOP,              yop);
    REGISTER_MUXDEMUX(YUV4MPEGPIPE,     yuv4mpegpipe);

    /* image demuxers */
    REGISTER_DEMUXER (IMAGE_BMP_PIPE,        image_bmp_pipe);
    REGISTER_DEMUXER (IMAGE_DPX_PIPE,        image_dpx_pipe);
    REGISTER_DEMUXER (IMAGE_EXR_PIPE,        image_exr_pipe);
    REGISTER_DEMUXER (IMAGE_J2K_PIPE,        image_j2k_pipe);
    REGISTER_DEMUXER (IMAGE_JPEGLS_PIPE,     image_jpegls_pipe);
    REGISTER_DEMUXER (IMAGE_PICTOR_PIPE,     image_pictor_pipe);
    REGISTER_DEMUXER (IMAGE_PNG_PIPE,        image_png_pipe);
    REGISTER_DEMUXER (IMAGE_SGI_PIPE,        image_sgi_pipe);
    REGISTER_DEMUXER (IMAGE_SUNRAST_PIPE,    image_sunrast_pipe);
    REGISTER_DEMUXER (IMAGE_TIFF_PIPE,       image_tiff_pipe);
    REGISTER_DEMUXER (IMAGE_WEBP_PIPE,       image_webp_pipe);


    /* protocols */
    REGISTER_PROTOCOL(BLURAY,           bluray);
    REGISTER_PROTOCOL(CACHE,            cache);
    REGISTER_PROTOCOL(CONCAT,           concat);
    REGISTER_PROTOCOL(CRYPTO,           crypto);
    REGISTER_PROTOCOL(DATA,             data);
    REGISTER_PROTOCOL(FFRTMPCRYPT,      ffrtmpcrypt);
    REGISTER_PROTOCOL(FFRTMPHTTP,       ffrtmphttp);
    REGISTER_PROTOCOL(FILE,             file);
    REGISTER_PROTOCOL(FTP,              ftp);
    REGISTER_PROTOCOL(GOPHER,           gopher);
    REGISTER_PROTOCOL(HLS,              hls);
    REGISTER_PROTOCOL(HTTP,             http);
    REGISTER_PROTOCOL(HTTPPROXY,        httpproxy);
    REGISTER_PROTOCOL(HTTPS,            https);
    REGISTER_PROTOCOL(ICECAST,          icecast);
    REGISTER_PROTOCOL(MMSH,             mmsh);
    REGISTER_PROTOCOL(MMST,             mmst);
    REGISTER_PROTOCOL(MD5,              md5);
    REGISTER_PROTOCOL(PIPE,             pipe);
    REGISTER_PROTOCOL(RTMP,             rtmp);
    REGISTER_PROTOCOL(RTMPE,            rtmpe);
    REGISTER_PROTOCOL(RTMPS,            rtmps);
    REGISTER_PROTOCOL(RTMPT,            rtmpt);
    REGISTER_PROTOCOL(RTMPTE,           rtmpte);
    REGISTER_PROTOCOL(RTMPTS,           rtmpts);
    REGISTER_PROTOCOL(RTP,              rtp);
    REGISTER_PROTOCOL(SCTP,             sctp);
    REGISTER_PROTOCOL(SRTP,             srtp);
    REGISTER_PROTOCOL(SUBFILE,          subfile);
    REGISTER_PROTOCOL(TCP,              tcp);
    REGISTER_PROTOCOL(TLS,              tls);
    REGISTER_PROTOCOL(UDP,              udp);
    REGISTER_PROTOCOL(UNIX,             unix);

    /* external libraries */
    REGISTER_DEMUXER (LIBGME,           libgme);
    REGISTER_DEMUXER (LIBMODPLUG,       libmodplug);
    REGISTER_MUXDEMUX(LIBNUT,           libnut);
    REGISTER_DEMUXER (LIBQUVI,          libquvi);
    REGISTER_PROTOCOL(LIBRTMP,          librtmp);
    REGISTER_PROTOCOL(LIBRTMPE,         librtmpe);
    REGISTER_PROTOCOL(LIBRTMPS,         librtmps);
    REGISTER_PROTOCOL(LIBRTMPT,         librtmpt);
    REGISTER_PROTOCOL(LIBRTMPTE,        librtmpte);
    REGISTER_PROTOCOL(LIBSSH,           libssh);
    REGISTER_PROTOCOL(LIBSMBCLIENT,     libsmbclient);
#endif
}
