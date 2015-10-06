#ifndef __UTILITY_HPP__
#define __UTILITY_HPP__

#define SAFE_DELETE(x) if(x) delete x,x = 0
#define EXCEPT_NORMAL_BEGIN try{
#define EXCEPT_NORMAL_END }catch(char *error){ printf("[**ERROR**] %s",error); abort(); }


#endif
