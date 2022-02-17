enum PackageCommand
{
    C2R_CMD,            //controller -> remote, 用来执行cmd命令
    C2R_SCREEN,         //controller -> remote, 用来获取屏幕数据
    C2R_FILETRAVELS,    //controller -> remote, 用来获取指定文件夹下的文件
    C2R_FILEUPLOAD,

    R2C_SCREEN           //remote -> controller, 返回屏幕数据
};

#pragma pack(push)
#pragma pack(1)
struct PackageHeader
{
    PackageCommand m_command; //包的命令
    DWORD m_dwDataLen; //包的数据的长度
    //CHAR m_data[1];
};

struct ScreenData
{
    DWORD m_dwWith;
    DWORD m_dwHeight;
    //BYTE m_data[1];
};
#pragma pack(pop)