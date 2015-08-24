#include <tw/common/filesystem.h>

int main()
{
    try
    {
        std::string filename = "/home/vlad/tradework_AMRDEV01/logs/spreader/log.txt";
        std::cout << "parent_path of: " << filename << " is: " <<  tw::common::Filesystem::parent_path(filename) << std::endl;
        std::cout << "filename of: " << filename << " is: " <<  tw::common::Filesystem::filename(filename) << std::endl;
    
        std::cout << "parent_path of: " << filename << (tw::common::Filesystem::exists_dir(tw::common::Filesystem::parent_path(filename)) ? " exists" : " doesn't exist") << std::endl;
        std::cout << "parent_path of: " << filename << (tw::common::Filesystem::create_dir(tw::common::Filesystem::parent_path(filename)) ? " created" : " didn't create") << std::endl;
        std::cout << "parent_path of: " << filename << (tw::common::Filesystem::exists_dir(tw::common::Filesystem::parent_path(filename)) ? " exists" : " doesn't exist") << std::endl;
        
        tw::common::Filesystem::remove(tw::common::Filesystem::parent_path(filename));
        std::cout << "parent_path of: " << filename << (tw::common::Filesystem::exists_dir(tw::common::Filesystem::parent_path(filename)) ? " exists" : " doesn't exist") << std::endl;
    
        std::string path = "/home/vlad/tradework_AMRDEV01/logs";
        std::cout << path << " contents: " << "\n";
        tw::common::Filesystem::TFilesList list = tw::common::Filesystem::getDirFiles(path);
        for ( size_t i = 0; i < list.size(); ++i ) {
            std::cout << "\tfile[" << i << "]: " << list[i] << "\n";
        }
    }
    catch (std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }
    return 0;
}
