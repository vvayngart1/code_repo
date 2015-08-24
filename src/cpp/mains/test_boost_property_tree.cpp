#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <string>
#include <set>
#include <vector>
#include <map>

#include <exception>
#include <iostream>
#include <fstream>

struct debug_settings
{
    std::string m_file;               // log filename
    int m_level;                      // debug level
    std::set<std::string> m_modules;  // modules where logging is enabled
    void load(const std::string &filename);
    void save(const std::string &filename);
};

void debug_settings::load(const std::string &filename)
{

    // Create empty property tree object
    using boost::property_tree::ptree;
    ptree pt;

    // Load XML file and put its contents in property tree. 
    // No namespace qualification is needed, because of Koenig 
    // lookup on the second argument. If reading fails, exception
    // is thrown.
    read_xml(filename, pt);

    // Get filename and store it in m_file variable. Note that 
    // we specify a path to the value using notation where keys 
    // are separated with dots (different separator may be used 
    // if keys themselves contain dots). If debug.filename key is 
    // not found, exception is thrown.
    m_file = pt.get<std::string>("debug.filename");
    
    // Get debug level and store it in m_level variable. This is 
    // another version of get method: if debug.level key is not 
    // found, it will return default value (specified by second 
    // parameter) instead of throwing. Type of the value extracted 
    // is determined by type of second parameter, so we can simply 
    // write get(...) instead of get<int>(...).
    m_level = pt.get("debug.level", 0);

    // Iterate over debug.modules section and store all found 
    // modules in m_modules set. get_child() function returns a 
    // reference to child at specified path; if there is no such 
    // child, it throws. Property tree iterator can be used in 
    // the same way as standard container iterator. Category 
    // is bidirectional_iterator.
    BOOST_FOREACH(ptree::value_type &v, pt.get_child("debug.modules"))
        m_modules.insert(v.second.data());

}

void debug_settings::save(const std::string &filename)
{
    // Create an empty property tree object
   using boost::property_tree::ptree;
   ptree pt;

   // Put log filename in property tree
   pt.put("debug.filename", m_file);

   // Put debug level in property tree
   pt.put("debug.level", m_level+1);

   // Iterate over the modules in the set and put them in the
   // property tree. Note that the put function places the new
   // key at the end of the list of keys. This is fine most of
   // the time. If you want to place an item at some other place
   // (i.e. at the front or somewhere in the middle), this can
   // be achieved using a combination of the insert and put_own
   // functions.
   BOOST_FOREACH(const std::string &name, m_modules)
      pt.add("debug.modules.module", name);

   // Write the property tree to the XML file.
   write_xml(filename, pt);
}

void test_channel_configuration() 
{
   using boost::property_tree::ptree;
   ptree pt;
   
   read_xml("/opt/tradework/onix/config/pf/config.xml", pt);   
   BOOST_FOREACH(ptree::value_type &ch, pt.get_child("configuration") ) {
       if ( ch.first == "channel" ) {
            int channel_id = ch.second.get<int>("<xmlattr>.id");
            std::string label = ch.second.get<std::string>("<xmlattr>.label");
            if ( boost::algorithm::ends_with(label, "Futures")) {
                BOOST_FOREACH(ptree::value_type &p, ch.second.get_child("products")) {
                    if ( p.first == "product" ) {
                        std::string product_code = p.second.get<std::string>("<xmlattr>.code");
                        std::cout << product_code << ":" << channel_id << std::endl;
                    }
                }                
            }
       }
   }
}

int main()
{
    try
    {
        debug_settings ds;
        std::string filename;
        std::string filename_out;
        
        filename = "./debug_settings.xml";
        filename_out = "./debug_settings_out.xml";
        
        {
            std::ofstream f;
            f.open(filename.c_str());
            f << "<debug>" << "\n"
                 <<   "\t<filename>debug.log</filename>" << "\n"
                 <<   "\t<level>2</level>" << "\n"
                 <<   "\t<modules>" << "\n"
                 <<       "\t\t<module>Admin</module>" << "\n"
                 <<       "\t\t<module>Finance</module>" << "\n"
                 <<       "\t\t<module>HR</module>" << "\n"
                 <<   "\t</modules>" << "\n"
                 <<  "</debug>";
            f.close();
        }        
                
        
        ds.load(filename);
        ds.save(filename_out);
        
        std::cout << "Success\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }
    return 0;
}
