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
	// なぜか使うと遅くなっている気がする。
	Bitboard& operator = (const Bitboard& b) { _mm_store_si128(&this->m_, b.m_); return *this; }
	Bitboard(const Bitboard& bb) { _mm_store_si128(&this->m_, bb.m_); }
#endif
	// コンストラクタ
	Bitboard() {}

	Bitboard(const uint64_t v0, const uint64_t v1)
	{
		this->b_[0] = v0;
		this->b_[1] = v1;
	}

	// getter:indexは0か1
	uint64_t b(const int index) const {
		assert(index == 0 || index == 1);
		return b_[index];
	}

	// setter:indexは0か1 さもなくばassert
	void set(const int index, const uint64_t val)
	{
		assert(index == 0 || index == 1);
		b_[index] = val;
	}

	// 指定した位置に1を立てる
	void set(const Square sq)
	{
		assert(sq >= 0 && sq <= SQUARE_MAX);
		*this |= mask(sq);
	}

	// ゼロクリアする
	void clear() { b_[0] = b_[1] = 0; }

	//指定したビットだけクリア
	void clear(const Square sq) { *this &= ~mask(sq); }

	// 演算子群
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

	//どちらか一つでもビットがたっていれば真
	bool isTrue() const
	{
#ifdef HAVE_SSE4
		return !(_mm_testz_si128(this->m_, _mm_set1_epi8(static_cast<char>(0xffu))));
#else
		return (b_[0] || b_[1]);
#endif
	}

	// コードが醜くなるけど仕方がないとのこと。SSEと非SSEとの互換性のため
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

	// sqのbitがたっていればfalse
	bool isSet(const Square sq) const
	{
		assert(isInSquare(sq));
		return !this->andIsFlase(mask(sq));
	}

	// sqのbitがたっていなければtrue
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

	// 立っているビットの数を返す。
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

// 何ビット目がたっているのかを返す。見つけたビットは0にする。
inline Square Bitboard::firstOne01()
{
	int index = Bitop::bsf64<true>(b(0));

	// b_[0]に1が見つかれば、位置をそのまま返す。
	if (index != -1)
	{
		this->andEqualNot(mask(static_cast<Square>(index)));

		return static_cast<Square>(index);
	}

	index = Bitop::bsf64<false>(b(1));

	// Bitboardがすべて0ではないことを前提としているので、ここでindexが-1になるはずはない。
	assert(index != -1);

	this->andEqualNot(mask(static_cast<Square>(index + 64)));

	return static_cast<Square>(index + 64);
}

// 何ビット目がたっているのかを返す。見つけたビットは0にする。右側のフィールドから探す。
inline Square Bitboard::firstOne10()
{
	int index = Bitop::bsf64<true>(b(1));

	if (index != -1)
	{
		this->andEqualNot(mask(static_cast<Square>(index + 64)));

		return static_cast<Square>(index + 64);
	}

	index = Bitop::bsf64<false>(b(0));

	// Bitboardがすべて0ではないことを前提としているので、ここでindexが-1になるはずはない。
	assert(index != -1);

	this->andEqualNot(mask(static_cast<Square>(index)));

	return static_cast<Square>(index);
}

// 左側を探す。
inline Square Bitboard::leftOne()
{
	assert(b(0));
	int index = Bitop::bsf64<false>(b(0));
	this->andEqualNot(mask(static_cast<Square>(index)));
	return static_cast<Square>(index);
}

// 右側を探す。
inline Square Bitboard::rightOne()
{
	assert(b(1));
	int index = Bitop::bsf64<false>(b(1));
	this->andEqualNot(mask(static_cast<Square>(index + 64)));
	return static_cast<Square>(index + 64);
}