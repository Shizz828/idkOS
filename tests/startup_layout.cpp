#include "../src/ui/StartupScreen.h"
int main(){
    spark::drawStartupScreen();spark::presentStartupLogo();
    static_assert(sizeof(spark::BootLogoPixels)/2==unsigned(spark::BootLogoW*spark::BootLogoH),"logo size");
}
