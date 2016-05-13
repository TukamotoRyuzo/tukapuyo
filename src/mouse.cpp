#include"mouse.h"


// ���Ԍo�߂ŕω�����ϐ��̐ݒ�
void Mouse::SetPermanent(){

    // �O�t���[���̍��W���擾
    before = after;

    // �}�E�X�̍��W���擾
    int x , y;
    GetMousePoint( &x , &y );
    pos.x = x; pos.y = y;
   

    // ���̍��W���擾
    after = pos;

    // �{�^����������Ă����瑝���@�{�^���𗣂���0�ɂȂ�
    if( ( GetMouseInput() & MOUSE_INPUT_LEFT ) != 0 )button[0]++;
    else button[0] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_RIGHT ) !=0 )button[1]++;
    else button[1] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_MIDDLE ) !=0 )button[2]++;
    else button[2] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_4 ) !=0 )button[3]++;
    else button[3] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_5 ) !=0 )button[4]++;
    else button[4] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_6 ) !=0 )button[5]++;
    else button[5] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_7 ) !=0 )button[6]++;
    else button[6] = 0;
    if( ( GetMouseInput() & MOUSE_INPUT_8 ) !=0 )button[7]++;
    else button[7] = 0;

    // �z�C�[���̉�]�ʂ𒲂ׂ�
    int rot = GetMouseWheelRotVol();

    if( rot < 0 )wheel--;
    else if( 0 < rot )wheel++;
    else wheel = 0;

}

// �}�E�X�J�[�\���̕\���ݒ�t���O�̃Z�b�g
void Mouse::SetDispFlag( bool DispFlag ){
    SetMouseDispFlag( DispFlag );
}

// �}�E�X�J�[�\���̈ʒu���擾����
Vector Mouse::GetPoint(){
    return pos;
}

// �}�E�X�J�[�\���̈ʒu���Z�b�g����
void Mouse::SetPoint( Vector pos ){
    
    this->pos = pos;

    DxLib::SetMousePoint( (int)this->pos.x , (int)this->pos.y );
}

// �}�E�X�J�[�\���̈ʒu���Z�b�g����
void Mouse::SetPoint( int x , int y ){
    
    this->pos.x = x;
    this->pos.y = y;

    DxLib::SetMousePoint( (int)this->pos.x , (int)this->pos.y );
}

// �}�E�X�J�[�\���̃{�^���������Ă��鎞�Ԃ𓾂�
int Mouse::GetInput( int num )
{
    // �G���[����
    if( num < 0 || 7 < num ){
        printfDx( "�s���ȃ}�E�X�{�^���̐ݒ�\n" );
        return -1;
    }

    return button[ num ];
}

// �}�E�X�z�C�[���̉�]�ʂ𓾂�
int Mouse::GetWheelRotVol(){
    return wheel;
}

// �摜�Ƃ̓����蔻��
bool Mouse::JudgeCollisionGraph( Graph graph ){

    // �}�E�X�̍��W���摜�̍��[�����E
    if( graph.getUpperLeft().x < this->pos.x )
		
        // �}�E�X�̍��W���摜�̉E�[������
        if( this->pos.x < graph.getLowerRight().x )

            // �}�E�X�̍��W���摜�̏�[������
            if( graph.getUpperLeft().y < this->pos.y )

                // �}�E�X�̍��W���摜�̉��[������
                if( this->pos.y < graph.getLowerRight().y )

                    return TRUE;

    // �����ɍ���Ȃ����FALSE
    return FALSE;

}

// �}�E�X�������Ă��邩����
bool Mouse::JudgeMouseMove(){

    // �����Ă�����TRUE
    if( this->before.x != this->after.x || this->before.y != this->after.y )
        return TRUE;
    else return FALSE;

}
