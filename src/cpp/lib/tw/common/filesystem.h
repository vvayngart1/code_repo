#pragma once

#include <tw/common/exception.h>
#include <tw/log/defs.h>

#include <boost/filesystem.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <iterator>
#include <vector>
#include <algorithm>

namespace tw {
namespace common {
    
class Filesystem {
public:
    typedef std::vector<std::string> TFilesList;
    typedef boost::filesystem::path TPath;
    typedef std::vector<TPath> TPaths;
    
public:
    static TFilesList getDirFiles(const std::string& path) {
        TFilesList list;
        Exception exception;
        try {
            if ( exists_dir(path) ) {
                TPath p(path);
                TPaths v;
                std::copy(boost::filesystem::directory_iterator(p), boost::filesystem::directory_iterator(), std::back_inserter(v));
                
                for ( size_t i = 0; i < v.size(); ++i ) {
                    list.push_back(v[i].string());
                }
            }
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
        return list;
    }
    
    static bool exists(const std::string& fileName) {
        try {
            if ( !boost::filesystem::exists(fileName) ) 
                return false;

            if ( !boost::filesystem::is_regular_file(fileName) ) 
                return false;
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
            return false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            return false;
        }

        return true;
    }
    
    static bool exists_dir(const std::string& fileName) {
        try {
            if ( !boost::filesystem::exists(fileName) ) 
                return false;

            if ( !boost::filesystem::is_directory(fileName) ) 
                return false;
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
            return false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            return false;
        }

        return true;
    }
    
    
    static bool create_dir(const std::string& path) {
        try {
            if ( exists_dir(path) )
                return true;
            
            boost::filesystem::path p(path);
            return boost::filesystem::create_directory(p);
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
            return false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            return false;
        }
        
        return true;       
    }
    
    static bool remove(const std::string& fileName) {
        try {
            if ( !boost::filesystem::remove(fileName) )
                return false;
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
            return false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            return false;
        }

        return true;
    }
    
    static bool swap(const std::string& fileName) {
        static const uint32_t MAX_SWAPS = 10000;
        
        Exception exception;
        try {
            if ( !Filesystem::exists(fileName) ) {
                exception << "File doesn't exist or is not a regular file: " << fileName;
                throw(exception);
            }
            
            boost::gregorian::date d(boost::gregorian::day_clock::local_day());            
            std::string fileNameOfBackup = fileName + "." + boost::gregorian::to_iso_string(d) + ".";
            uint32_t index = 1;
            for ( ; index < MAX_SWAPS; ++index ) {
                if ( !boost::filesystem::exists(fileNameOfBackup+boost::lexical_cast<std::string>(index)) ) {
                    fileNameOfBackup += boost::lexical_cast<std::string>(index);
                    break;
                }
            }
            
            if ( MAX_SWAPS == index ) {
                exception << "Exceeded MAX_SWAPS for: " << fileName;
                throw(exception);
            }
            
            boost::filesystem::rename(fileName, fileNameOfBackup);
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
            return false;
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
            return false;
        }

        return true;
    }
    
    static std::string parent_path(const std::string& fileName) {
        std::string result;        
        try {
           boost::filesystem::path p(fileName);
           result = p.parent_path().string();
            
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
        
        return result;       
    }
    
    static std::string filename(const std::string& fileName) {
        std::string result;        
        try {
           boost::filesystem::path p(fileName);
           result = p.filename().string();
            
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
        
        return result;       
    }
};

} // namespace common
} // namespace tw




