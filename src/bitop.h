#pragma once

#include "platform.h"
#include <intrin.h>

namespace Bitop
{
	// 1�ɂȂ��Ă���bit�����ʂ���scan���ĉ�bit�ڂŌ�����������Ԃ��B������0��n���Ȃ��悤�ɒ��ӂ���
	// ����́A32�r�b�g�p
	inline int bsf32(unsigned long mask)
	{
		unsigned long index;
		_BitScanForward(&index, mask);
		return index;
	}

	// 64bit�̂����ALSB�̈ʒu��Ԃ��B0��n���ƁA-1��Ԃ��B
	// Check��true�ɂ��ēn����_BitScanForwawd64�̖߂�l�`�F�b�N������B
	// mask��0�łȂ����Ƃ��킩���Ă���Ȃ�false�œn�����ق��������B
	template <bool Check> inline int bsf64(uint64_t mask)
	{
#if defined(USE_BSF)
		unsigned long index;

		if (Check)
		{
			if (!_BitScanForward64(&index, mask)) { return -1; };
		}
		else
		{
			assert(mask != 0);
			_BitScanForward64(&index, mask);
		}
		return index;
#else
		static const int BitTable[64] =
		{
			-1, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
			51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
			26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
			58, 20, 37, 17, 36, 8
		};
		const uint64_t tmp = mask ^ (mask - 1);
		const uint32_t old = static_cast<uint32_t>((tmp & 0xffffffff) ^ (tmp >> 32));
		return BitTable[(old * 0x783a9b23) >> 26];

#endif
	}
	// ����x���R�[�h�Ȃ̂Ō�ŏ��������邱�ƁB
	inline int bsr64(const uint64_t b) 
	{
		for (int i = 63; 0 <= i; --i) {
			if (b >> i) {
				return i;
			}
		}
		return 0;
	}

	//64bit�̂����A�����Ă���r�b�g�̐���Ԃ��B(popcnt�̑���)
	inline int popCount(uint64_t v) 
	{
#if defined(USE_POPCNT)
		return static_cast<int>(_mm_popcnt_u64(v));
#else
		uint64_t count = (v & 0x5555555555555555ULL) + ((v >> 1) & 0x5555555555555555ULL);
		count = (count & 0x3333333333333333ULL) + ((count >> 2) & 0x3333333333333333ULL);
		count = (count & 0x0f0f0f0f0f0f0f0fULL) + ((count >> 4) & 0x0f0f0f0f0f0f0f0fULL);
		count = (count & 0x00ff00ff00ff00ffULL) + ((count >> 8) & 0x00ff00ff00ff00ffULL);
		count = (count & 0x0000ffff0000ffffULL) + ((count >> 16) & 0x0000ffff0000ffffULL);
		return (int)((count & 0x00000000ffffffffULL) + ((count >> 32) & 0x00000000ffffffffULL));
#endif
	}
}// namespace Bitop


