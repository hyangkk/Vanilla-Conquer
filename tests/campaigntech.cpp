#include "tiberiandawn/campaigntech.h"
#include <cstdio>
#include <cstdlib>
void Check(bool ok) { if (!ok) std::exit(1); }
int main() {
    using namespace CampaignTech;
    Check(Enabled(true,true,true,true,true));
    Check(!Enabled(false,true,true,true,true));
    Check(!Enabled(true,false,true,true,true));
    Check(!Enabled(true,true,false,true,true));
    Check(!Enabled(true,true,true,false,true));
    Check(!Enabled(true,true,true,true,false));
    Check(Eligible(true,true,true));
    Check(!Eligible(true,false,true));
    Check(!Eligible(true,true,false));
    Check(!Eligible(false,true,true));
    std::puts("campaign tech scope and non-production exclusions passed");
}
