#pragma once
#include "platform.h"
#include <cassert>
#include "puyo.h"
#include "bitop.h"

//#undef HAVE_SSE4
//#undef HAVE_SSE2
class Bitboard;

class Bitboard
{

public:
#if defined (HAVE_SSE2) || defined(HAVE_SSE4)
	// �Ȃ����g���ƒx���Ȃ��Ă���C������B
	Bitboard& operator = (const Bitboard& b) { _mm_store_si128(&this->m_, b.m_); return *this; }
	Bitboard(const Bitboard& bb) { _mm_store_si128(&this->m_, bb.m_); }
#endif
	// �R���X�g���N�^
	Bitboard() {}

	Bitboard(const uint64_t v0, const uint64_t v1)
	{
		this->b_[0] = v0;
		this->b_[1] = v1;
	}

	// getter:index��0��1
	uint64_t b(const int index) const {
		assert(index == 0 || index == 1);
		return b_[index];
	}

	// setter:index��0��1 �����Ȃ���assert
	void set(const int index, const uint64_t val)
	{
		assert(index == 0 || index == 1);
		b_[index] = val;
	}

	// �w�肵���ʒu��1�𗧂Ă�
	void set(const Square sq)
	{
		assert(sq >= 0 && sq <= SQUARE_MAX);
		*this |= mask(sq);
	}

	// �[���N���A����
	void clear() { b_[0] = b_[1] = 0; }

	//�w�肵���r�b�g�����N���A
	void clear(const Square sq) { *this &= ~mask(sq); }

	// ���Z�q�Q
	Bitboard operator ~ () const {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		Bitboard tmp;
		_mm_store_si128(&tmp.m_, _mm_andnot_si128(this->m_, _mm_set1_epi8(static_cast<char>(0xffu))));
		return tmp;
#else
		return Bitboard(~this->b(0), ~this->b(1));
#endif
	}

	Bitboard operator &= (const Bitboard& rhs)
	{
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_and_si128(this->m_, rhs.m_));
#else
		this->b_[0] &= rhs.b(0);
		this->b_[1] &= rhs.b(1);
#endif
		return *this;
	}

	Bitboard operator |= (const Bitboard& rhs)
	{
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_or_si128(this->m_, rhs.m_));
#else
		this->b_[0] |= rhs.b(0);
		this->b_[1] |= rhs.b(1);
#endif
		return *this;
	}

	Bitboard operator ^= (const Bitboard& rhs)
	{
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_xor_si128(this->m_, rhs.m_));
#else
		this->b_[0] ^= rhs.b(0);
		this->b_[1] ^= rhs.b(1);
#endif
		return *this;
	}

	Bitboard operator <<= (const int i)
	{
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_slli_epi64(this->m_, i));
#else
		this->b_[0] <<= i;
		this->b_[1] <<= i;
#endif
		return *this;
	}

	Bitboard operator >>= (const int i)
	{
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_srli_epi64(this->m_, i));
#else
		this->b_[0] >>= i;
		this->b_[1] >>= i;
#endif
		return *this;
	}

	Bitboard operator & (const Bitboard& rhs) const { return Bitboard(*this) &= rhs; }
	Bitboard operator | (const Bitboard& rhs) const { return Bitboard(*this) |= rhs; }
	Bitboard operator ^ (const Bitboard& rhs) const { return Bitboard(*this) ^= rhs; }
	Bitboard operator << (const int i) const { return Bitboard(*this) <<= i; }
	Bitboard operator >> (const int i) const { return Bitboard(*this) >>= i; }

	bool operator == (const Bitboard& rhs) const
	{
#ifdef HAVE_SSE4
		return (_mm_testc_si128(_mm_cmpeq_epi8(this->m_, rhs.m_), _mm_set1_epi8(static_cast<char>(0xffu))) ? true : false);
#else
		return (this->b(0) == rhs.b(0)) && (this->b(1) == rhs.b(1));
#endif
	}

	bool operator != (const Bitboard& rhs) const { return !(*this == rhs); }

	//�ǂ��炩��ł��r�b�g�������Ă���ΐ^
	bool isTrue() const
	{
#ifdef HAVE_SSE4
		return !(_mm_testz_si128(this->m_, _mm_set1_epi8(static_cast<char>(0xffu))));
#else
		return (b_[0] || b_[1]);
#endif
	}

	// �R�[�h���X���Ȃ邯�ǎd�����Ȃ��Ƃ̂��ƁBSSE�Ɣ�SSE�Ƃ̌݊����̂���
	bool andIsFlase(const Bitboard& bb) const
	{
#ifdef HAVE_SSE4
		return _mm_testz_si128(this->m_, bb.m_);
#else
		return !(*this & bb).isTrue();
#endif
	}

	Bitboard andEqualNot(const Bitboard& bb)
	{
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_andnot_si128(bb.m_, this->m_));
#else
		return (*this) &= ~bb;
#endif
		return *this;
	}

	Bitboard notThisAnd(const Bitboard& bb)
	{
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		Bitboard temp;
		_mm_store_si128(&temp.m_, _mm_andnot_si128(this->m_, bb.m_));
		return temp;
#else
		return ~(*this) & bb;
#endif
	}

	// sq��bit�������Ă����false
	bool isSet(const Square sq) const
	{
		assert(isInSquare(sq));
		return !this->andIsFlase(mask(sq));
	}

	// sq��bit�������Ă��Ȃ����true
	bool isNotSet(const Square sq) const
	{
		assert(isInSquare(sq));
		return this->andIsFlase(mask(sq));
	}

	void xorBit(const Square sq) { (*this) ^= mask(sq); }
	void xorBit(const Square sq1, const Square sq2) { (*this) ^= (mask(sq1) | mask(sq2)); }

	Square firstOne01();
	Square firstOne10();
	Square leftOne();
	Square rightOne();
	static Bitboard mask(const Square sq) { return BB_MASK[sq]; };

	// �����Ă���r�b�g�̐���Ԃ��B
	int count() const { return Bitop::popCount(b(0)) + Bitop::popCount(b(1)); }

	Bitboard line(const int x) const { return *this & LINE_MASK[x];}

private:

#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
	union
	{
		uint64_t b_[2];
		__m128i m_;
	};
#else
	uint64_t b_[2];
#endif

	static const Bitboard BB_MASK[SQUARE_MAX];
	static const Bitboard LINE_MASK[8];
};

// ���r�b�g�ڂ������Ă���̂���Ԃ��B�������r�b�g��0�ɂ���B
inline Square Bitboard::firstOne01()
{
	int index = Bitop::bsf64<true>(b(0));

	// b_[0]��1��������΁A�ʒu�����̂܂ܕԂ��B
	if (index != -1)
	{
		this->andEqualNot(mask(static_cast<Square>(index)));

		return static_cast<Square>(index);
	}

	index = Bitop::bsf64<false>(b(1));

	// Bitboard�����ׂ�0�ł͂Ȃ����Ƃ�O��Ƃ��Ă���̂ŁA������index��-1�ɂȂ�͂��͂Ȃ��B
	assert(index != -1);

	this->andEqualNot(mask(static_cast<Square>(index + 64)));

	return static_cast<Square>(index + 64);
}

// ���r�b�g�ڂ������Ă���̂���Ԃ��B�������r�b�g��0�ɂ���B�E���̃t�B�[���h����T���B
inline Square Bitboard::firstOne10()
{
	int index = Bitop::bsf64<true>(b(1));

	if (index != -1)
	{
		this->andEqualNot(mask(static_cast<Square>(index + 64)));

		return static_cast<Square>(index + 64);
	}

	index = Bitop::bsf64<false>(b(0));

	// Bitboard�����ׂ�0�ł͂Ȃ����Ƃ�O��Ƃ��Ă���̂ŁA������index��-1�ɂȂ�͂��͂Ȃ��B
	assert(index != -1);

	this->andEqualNot(mask(static_cast<Square>(index)));

	return static_cast<Square>(index);
}

// ������T���B
inline Square Bitboard::leftOne()
{
	assert(b(0));
	int index = Bitop::bsf64<false>(b(0));
	this->andEqualNot(mask(static_cast<Square>(index)));
	return static_cast<Square>(index);
}

// �E����T���B
inline Square Bitboard::rightOne()
{
	assert(b(1));
	int index = Bitop::bsf64<false>(b(1));
	this->andEqualNot(mask(static_cast<Square>(index + 64)));
	return static_cast<Square>(index + 64);
}