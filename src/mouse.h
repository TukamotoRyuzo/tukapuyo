#pragma once
#include"DxLib.h"
#include"vector.h"
#include"graph.h"

enum MouseID
{
	MOUSE_LEFT, MOUSE_RIGHT, MOUSE_CENTOR,
};

// �}�E�X�Ɋւ���N���X
class Mouse{
private:
    Vector pos;// ���W
    int button[8];// �{�^���������Ă��鎞��
    int wheel;// �z�C�[��
    Vector before , after;// �}�E�X��������������

public:
	
    void SetPermanent();// ���Ԍo�߂ŕω�����ϐ��̐ݒ�
    void SetDispFlag( bool DispFlag );// �}�E�X�J�[�\���̕\���ݒ�t���O�̃Z�b�g
    void SetPoint( Vector pos );// �}�E�X�J�[�\���̈ʒu���Z�b�g����
    void SetPoint( int x , int y );// �}�E�X�J�[�\���̈ʒu���Z�b�g����

    Vector GetPoint();// �}�E�X�J�[�\���̈ʒu���擾����
    int GetInput(int num);// �}�E�X�J�[�\���̃{�^���������Ă��鎞�Ԃ𓾂�
    int GetWheelRotVol();// �}�E�X�z�C�[���̉�]�ʂ𓾂�
    bool JudgeCollisionGraph(Graph graph);// �摜�Ƃ̓����蔻��
    bool JudgeMouseMove();// �}�E�X�����������ǂ�������

};
