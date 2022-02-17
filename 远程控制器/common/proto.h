enum PackageCommand
{
    C2R_CMD,            //controller -> remote, 用来执行cmd命令
    C2R_SCREEN,         //controller -> remote, 用来获取屏幕数据
    C2R_SCREENCMD,      //controller -> remote, 用来发送屏幕指令
    C2R_FILETRAVELS,    //controller -> remote, 用来获取指定文件夹下的文件
    C2R_FILEUPLOAD,

    R2C_SCREEN           //remote -> controller, 返回屏幕数据
}; 


enum PointCommand
{
    POINT_MOVE,           //鼠标命令：移动
    POINT_LBUTTONDOWN,    //鼠标命令：左键按下
    POINT_LBUTTONUP,      //鼠标命令：左键松开
    POINT_LBUTTONDBDOWN,  //鼠标命令：左键双击
    POINT_RBUTTONDOWN,    //鼠标命令：右键按下
    POINT_RBUTTONUP,      //鼠标命令：右键松开
    POINT_RBUTTONDBDOWN,  //鼠标命令：右键双击
    POINT_MBUTTONDOWN,    //鼠标命令：中键按下
    POINT_MBUTTONUP,      //鼠标命令：中键松开
    POINT_MBUTTONDBDOWN,  //鼠标命令：中键双击
};

#pragma pack(push)
#pragma pack(1)
//参数1：包的类型
//参数2：数据的长度
struct PackageHeader
{
    PackageCommand m_command; //包的命令
    DWORD m_dwDataLen; //包的数据的长度
    //CHAR m_data[1];
};

//屏幕数据
//宽度和长度
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