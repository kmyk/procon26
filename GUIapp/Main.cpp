#include <cstdlib>
#include "GUIapp.hpp"
#include "Utility.hpp"

int main(int argc,char *argv[]){
  int game = 1;
  if(argc >= 2){
    game = atoi(argv[1]);
  }
  GUIapp *guiapp = new GUIapp(game);
  guiapp->main_loop();
  SAFE_DELETE(guiapp);
}
