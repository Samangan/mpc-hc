/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2012 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <atlcoll.h>
#include <stdint.h>
#include <a52dec/include/a52.h>
#include <libdca/include/dts.h>
#include "../../../DeCSS/DeCSSInputPin.h"
#include "IMpaDecFilter.h"
#include "MpaDecSettingsWnd.h"
#include "../../../apps/mplayerc/InternalFiltersConfig.h"

#define MPCAudioDecName	L"MPC Audio Decoder"

struct ps2_state_t {
	bool sync;
	double a[2], b[2];
	ps2_state_t() {
		reset();
	}
	void reset() {
		sync = false;
		a[0] = a[1] = b[0] = b[1] = 0;
	}
};

#if defined(REGISTER_FILTER) | INTERNAL_DECODER_FLAC
struct flac_state_t {
	void*   pDecoder;
	HRESULT hr;
};
#endif

struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVCodecParserContext;

class __declspec(uuid("3D446B6F-71DE-4437-BE15-8CE47174340F"))
	CMpaDecFilter
	: public CTransformFilter
	, public IMpaDecFilter
	, public ISpecifyPropertyPages2
{
protected:
	CCritSec m_csReceive;

#if defined(REGISTER_FILTER) | INTERNAL_DECODER_AC3
	a52_state_t*			m_a52_state;
#endif
#if defined(REGISTER_FILTER) | INTERNAL_DECODER_DTS
	dts_state_t*			m_dts_state;
#endif
	ps2_state_t				m_ps2_state;
#if defined(REGISTER_FILTER) | INTERNAL_DECODER_FLAC
	flac_state_t			m_flac;
#endif
	DolbyDigitalMode		m_DolbyDigitalMode;

#if defined(REGISTER_FILTER) | HAS_FFMPEG_AUDIO_DECODERS
	// === FFMpeg variables
	AVCodec*				m_pAVCodec;
	AVCodecContext*			m_pAVCtx;
	AVCodecParserContext*	m_pParser;
	AVFrame*				m_pFrame;
#endif

	CAtlArray<BYTE> m_buff;
	REFERENCE_TIME m_rtStart;
	bool m_fDiscontinuity;

#if defined(REGISTER_FILTER) | INTERNAL_DECODER_LPCM
	HRESULT ProcessLPCM();
	HRESULT ProcessHdmvLPCM(bool bAlignOldBuffer);
#endif
#if defined(REGISTER_FILTER) | INTERNAL_DECODER_AC3
	HRESULT ProcessAC3();
	HRESULT ProcessA52(BYTE* p, int buffsize, int& size, bool& fEnoughData);
#endif
#if defined(REGISTER_FILTER) | INTERNAL_DECODER_DTS
	HRESULT ProcessDTS();
#endif
#if defined(REGISTER_FILTER) | INTERNAL_DECODER_PS2AUDIO
	HRESULT ProcessPS2PCM();
	HRESULT ProcessPS2ADPCM();
#endif
#if defined(REGISTER_FILTER) | INTERNAL_DECODER_FLAC
	HRESULT ProcessFlac();
#endif
#if defined(REGISTER_FILTER) | INTERNAL_DECODER_PCM
	HRESULT ProcessPCMraw();
	HRESULT ProcessPCMintBE();
	HRESULT ProcessPCMintLE();
	HRESULT ProcessPCMfloatBE();
	HRESULT ProcessPCMfloatLE();
#endif

	HRESULT GetDeliveryBuffer(IMediaSample** pSample, BYTE** pData);
	HRESULT Deliver(CAtlArray<float>& pBuff, DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask = 0);
	HRESULT DeliverBitstream(BYTE* pBuff, int size, int sample_rate, int frame_length, BYTE type);
	HRESULT ReconnectOutput(int nSamples, CMediaType& mt);
	CMediaType CreateMediaType(MPCSampleFormat sf, DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask = 0);
	CMediaType CreateMediaTypeSPDIF(DWORD nSamplesPerSec = 48000);

#if defined(REGISTER_FILTER) | INTERNAL_DECODER_FLAC
	void    FlacInitDecoder();
	void    flac_stream_finish();
#endif

#if defined(REGISTER_FILTER) | HAS_FFMPEG_AUDIO_DECODERS
	bool    InitFFmpeg(enum CodecID nCodecId);
	void    ffmpeg_stream_finish();
	HRESULT DeliverFFmpeg(enum CodecID nCodecId, BYTE* p, int samples, int& size);
	HRESULT ProcessFFmpeg(enum CodecID nCodecId);
	static void LogLibAVCodec(void* par,int level,const char *fmt,va_list valist);

	BYTE*   m_pFFBuffer;
	int     m_nFFBufferSize;

	enum CodecID	FindCodec(const GUID subtype);

	struct {
		int flavor;
		int coded_frame_size;
		int sub_packet_h;
		int sub_packet_size;
		unsigned int deint_id;
	} m_raData;

	HRESULT ParseRealAudioHeader(const BYTE *extra, const int extralen);
#endif

protected:
	CCritSec m_csProps;
	MPCSampleFormat m_iSampleFormat;
	int  m_iSpeakerConfig[etlast];
	bool m_fDynamicRangeControl[etlast];

	bool m_bResync;

public:
	CMpaDecFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CMpaDecFilter();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT EndOfStream();
	HRESULT BeginFlush();
	HRESULT EndFlush();
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT Receive(IMediaSample* pIn);

	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);

	HRESULT StartStreaming();
	HRESULT StopStreaming();

	HRESULT SetMediaType(PIN_DIRECTION dir, const CMediaType *pmt);

	// ISpecifyPropertyPages2

	STDMETHODIMP GetPages(CAUUID* pPages);
	STDMETHODIMP CreatePage(const GUID& guid, IPropertyPage** ppPage);

	// IMpaDecFilter

	STDMETHODIMP SetSampleFormat(MPCSampleFormat sf);
	STDMETHODIMP_(MPCSampleFormat) GetSampleFormat();
	STDMETHODIMP SetSpeakerConfig(enctype et, int sc);
	STDMETHODIMP_(int) GetSpeakerConfig(enctype et);
	STDMETHODIMP SetDynamicRangeControl(enctype et, bool fDRC);
	STDMETHODIMP_(bool) GetDynamicRangeControl(enctype et);
	STDMETHODIMP_(DolbyDigitalMode) GetDolbyDigitalMode();

	STDMETHODIMP SaveSettings();

#if defined(REGISTER_FILTER) | INTERNAL_DECODER_FLAC
	void    FlacFillBuffer(BYTE buffer[], size_t *bytes);
	void    FlacDeliverBuffer (unsigned blocksize, const __int32 * const buffer[]);
#endif
};

class CMpaDecInputPin : public CDeCSSInputPin
{
public:
	CMpaDecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName);
};
