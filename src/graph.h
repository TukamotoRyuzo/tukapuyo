#pragma once
#include"DxLib.h"

struct Vector
{
	int x;
	int y;
};

// �摜�Ɋւ���N���X
class Graph
{
public:
	Graph(){};
	Graph(int handle, bool flag);// �摜�n���h������C���X�^���X�����B
	void init(int handle, bool flag);// �R���X�g���N�^�Ɠ���
	void load(const TCHAR *FileName , bool flag);// ���[�h
	
	void setPosition(Vector pos);// ���W�̐ݒ�(Vector�o�[�W����)
	void setPosition(int x , int y);// ���W�̐ݒ�i�e���l�w��o�[�W�����j

	void draw(float BlendParam = 100.0f);// �`��
	int handle() const ;// �摜�n���h���̎擾
	Vector getUpperLeft() const ;// ����̍��W���擾
	Vector getLowerRight() const ;// �E���̍��W���擾
	Vector size() const ;// �摜�̃T�C�Y���擾
private:
	int handle_;// �摜�n���h��
	Vector pos_;// ���W
	Vector size_;// �T�C�Y
	bool flag_;// ���߂����邩�̃t���O
};

