#pragma once

#include <boost/shared_ptr.hpp>

#define FWD_DECL(TYPENAME, CLASSNAME) \
TYPENAME CLASSNAME;\
typedef boost::shared_ptr< CLASSNAME > CLASSNAME##Ptr;\
typedef boost::shared_ptr< const CLASSNAME > CLASSNAME##ConstPtr;

#define FWD(CLASSNAME) FWD_DECL(class, CLASSNAME)

