enum PackageCommand
{
    C2R_CMD,            //controller -> remote, ����ִ��cmd����
    C2R_SCREEN,         //controller -> remote, ������ȡ��Ļ����
    C2R_SCREENCMD,      //controller -> remote, ����������Ļָ��
    C2R_FILETRAVELS,    //controller -> remote, ������ȡָ���ļ����µ��ļ�
    C2R_FILEUPLOAD,

    R2C_SCREEN           //remote -> controller, ������Ļ����
}; 


enum PointCommand
{
    POINT_MOVE,           //�������ƶ�
    POINT_LBUTTONDOWN,    //�������������
    POINT_LBUTTONUP,      //����������ɿ�
    POINT_LBUTTONDBDOWN,  //���������˫��
    POINT_RBUTTONDOWN,    //�������Ҽ�����
    POINT_RBUTTONUP,      //�������Ҽ��ɿ�
    POINT_RBUTTONDBDOWN,  //�������Ҽ�˫��
    POINT_MBUTTONDOWN,    //�������м�����
    POINT_MBUTTONUP,      //�������м��ɿ�
    POINT_MBUTTONDBDOWN,  //�������м�˫��
};

#pragma pack(push)
#pragma pack(1)
//����1����������
//����2�����ݵĳ���
struct PackageHeader
{
    PackageCommand m_command; //��������
    DWORD m_dwDataLen; //�������ݵĳ���
    //CHAR m_data[1];
};

//��Ļ����
//��Ⱥͳ���
struct ScreenData
{
    DWORD m_dwWidth;
    DWORD m_dwHeight;
    //BYTE m_data[1];
};

struct ScreenPoint
{
    DOUBLE m_pointRateX;
    DOUBLE m_pointRateY;
    CHAR m_pointCommand;
};
#pragma pack(pop)