#include "GUIapp.hpp"
#include "Utility.hpp"

int main(int argc,char *argv[]){
    GUIapp *guiapp = new GUIapp();
    guiapp->main_loop();
    SAFE_DELETE(guiapp);
}
