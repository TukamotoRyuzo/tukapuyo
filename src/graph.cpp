#include"graph.h"

// �R���X�g���N�^: �n���h������C���X�^���X�����
Graph::Graph(int handle, bool flag)
{
	init(handle, flag);	
}

// �R���X�g���N�^�ȊO����ł��������ł���悤�ɁB
void Graph::init(int handle, bool flag)
{
	handle_ = handle;

	// �摜�̃T�C�Y�擾
	int x , y;
	DxLib::GetGraphSize(handle_ , &x , &y);
	size_.x = x; size_.y = y;
	
	flag_ = flag;
}

// �摜�̃��[�h
void Graph::load(const TCHAR *FileName , bool flag)
{
	// �摜�̃��[�h
	handle_ = DxLib::LoadGraph(FileName);

	if (handle_ == -1)
	{
		//printfDx("DAME");
	}
	else
	{
		//printfDx("OK");
	}
	// �摜�̃T�C�Y�擾
	int x, y;
	DxLib::GetGraphSize(handle_ , &x , &y);
	size_.x = x;
	size_.y = y;

	// ���߂����邩�̃t���O�̐ݒ�
	flag_ = flag;

}


// �摜�̍��W��ݒ�iVector�o�[�W�����j
void Graph::setPosition(Vector pos)
{
	pos_.x = pos.x;
	pos_.y = pos.y;
}

// �摜�̍��W��ݒ�i�e���l�w��o�[�W�����j
void Graph::setPosition(int x , int y)
{
	pos_.x = x; pos_.y = y;
}

// �摜�̕`��
void Graph::draw(float BlendParam)
{
	DxLib::SetDrawBlendMode(DX_BLENDMODE_ALPHA , (int)(255 * BlendParam));
	DxLib::DrawGraph((int)pos_.x , (int)pos_.y , handle_ , flag_);
	DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND , 0);
}

// �摜�n���h���̎擾
int Graph::handle() const 
{
	return handle_;
}

// �摜�̍���̍��W���擾
Vector Graph::getUpperLeft() const  
{
	return pos_;
}

// �摜�̉E���̍��W���擾
Vector Graph::getLowerRight() const 
{
	Vector calc;
	calc.x = pos_.x + size_.x;
	calc.y = pos_.y + size_.y;

	return calc;
}

// �摜�̃T�C�Y���擾
Vector Graph::size() const 
{
	return size_;
}