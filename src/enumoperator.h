#pragma once

// +,-,*�ȂǕW���I��operator��W���I�ȕ��@�Œ�`���邽�߂̃}�N��
// enum�Œ�`����Ă���^�ɑ΂��ėp����B
#define ENABLE_SAFE_OPERATORS_ON(T)\
	inline T  operator+ (const T d1, const T d2) { return T(int(d1) + int(d2)); }\
	inline T  operator+ (const T d1, const int d2) { return T(int(d1) + int(d2)); }\
	inline T  operator| (const T d1, const T d2) { return T(int(d1) | int(d2)); }\
	inline T  operator^ (const T d1, const T d2) { return T(int(d1) ^ int(d2)); }\
	inline T  operator- (const T d1, const T d2) { return T(int(d1) - int(d2)); }\
	inline T  operator- (const T d1, const int d2) { return T(int(d1) - int(d2)); }\
	inline T  operator* (int i, const T d) { return T(i * int(d)); }\
	inline T  operator& (int i, const T d) { return T(i & int(d)); }\
	inline T  operator* (const T d, int i) { return T(int(d) * i); }\
	inline T  operator* (const T d, const T i) { return T(int(d) * int(i)); }\
	inline T  operator- (const T d) { return T(-int(d)); }\
	inline T& operator+=(T& d1, const T d2) { return d1 = d1 + d2; }\
	inline T& operator-=(T& d1, const T d2) { return d1 = d1 - d2; }\
	inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }\
	inline T& operator^=(T& d, int i) { return d = T(int(d) ^ i); }\
	inline T& operator|=(T& d, int i) { return d = T(int(d) | i); }\
	inline T& operator&=(T& d, int i) { return d = T(int(d) & i); }\
	inline T& operator += (T& d1, const int d2) { return d1 = d1 + d2; }\

// ENABLE_OPERATORS_ON(T)�̂ق��͏�L��ENABLE_SAFE_OPERATORS_ON(T)�ɉ����āA++,--,/,/=���T�|�[�g���Ă���B
// �قƂ��int�^���R�ł���SCORE,DEPTH�Ȃǂɑ΂��ėp����B
#define ENABLE_OPERATORS_ON(T) ENABLE_SAFE_OPERATORS_ON(T)\
	inline T& operator++(T& d) { return d = T(int(d) + 1); }\
	inline T& operator--(T& d) { return d = T(int(d) - 1); }\
	inline T  operator/(const T d, int i) { return T(int(d) / i); }\
	inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }