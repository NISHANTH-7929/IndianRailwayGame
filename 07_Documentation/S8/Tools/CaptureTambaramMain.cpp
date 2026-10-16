#include <process.h>
#include <cstdio>
int main(){
 const auto code=_spawnl(_P_WAIT,"C:\\Python314\\python.exe","python",
  "07_Documentation/S8/Tools/capture_tambaram_desktop.py",static_cast<const char*>(nullptr));
 if(code==-1){std::perror("Cannot start capture adapter");return 1;}return static_cast<int>(code);
}
