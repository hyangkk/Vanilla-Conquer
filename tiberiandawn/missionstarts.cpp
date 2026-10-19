// Generate native, frame-zero mission saves using the game's normal loader/serializer.
#include "function.h"
#include "common/rawfile.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <cctype>

bool Generate_Mission_Starts(const char* directory)
{
    const std::string dir(directory);
    std::ifstream input(dir+"/missions.txt");
    std::vector<std::string> roots;
    std::set<std::string> unique;
    std::string root;
    while (std::getline(input,root)) {
        if (!root.empty() && root.back()=='\r') root.pop_back();
        if (root.size()!=7 || root.substr(0,2)!="SC" || std::string("GBJ").find(root[2])==std::string::npos ||
            !std::isdigit(static_cast<unsigned char>(root[3])) || !std::isdigit(static_cast<unsigned char>(root[4])) || root.substr(3,2)=="00" ||
            std::string("EW").find(root[5])==std::string::npos || std::string("ABCD").find(root[6])==std::string::npos ||
            !unique.insert(root).second) return false;
        roots.push_back(root);
    }
    if (roots.empty() || roots.size()>800) return false;
    // Preflight every output before any write. Existing files are never replaced.
    for (size_t i=0;i<roots.size();++i) {
        char filename[32]; std::snprintf(filename,sizeof(filename),"/savegame.%03d",100+int(i));
        if (RawFileClass((dir+filename).c_str()).Is_Available()) return false;
    }
    if (RawFileClass((dir+"/verified.tsv").c_str()).Is_Available()) return false;
    std::ofstream report(dir+"/verified.tsv");
    if (!report) return false;
    report << "save\tmission\tframe\tcredits\tunits\tinfantry\tbuildings\taircraft\n";
    for (size_t i=0;i<roots.size();++i) {
        root=roots[i];
        GameToPlay=GAME_NORMAL; GameActive=true; Frame=0;
        PlayerWins=PlayerLoses=false; DoList.Init(); OutList.Init();
        Special.Init(); Special.IsJurassic=(root[2]=='J'); AreThingiesEnabled=Special.IsJurassic;
        Scen.Scenario=(root[3]-'0')*10+root[4]-'0';
        ScenPlayer=root[2]=='G' ? SCEN_PLAYER_GDI : root[2]=='B' ? SCEN_PLAYER_NOD : SCEN_PLAYER_JP;
        ScenDir=root[5]=='E' ? SCEN_DIR_EAST : SCEN_DIR_WEST;
        ScenVar=ScenarioVarType(root[6]-'A'); Whom=root[2]=='B' ? HOUSE_BAD : HOUSE_GOOD;
        Scen.Difficulty=Scen.CDifficulty=DIFF_NORMAL; Scen.CarryOverMoney=0;
        std::snprintf(Scen.ScenarioName,sizeof(Scen.ScenarioName),"%s",root.c_str());
        RequiredCD=root[2]=='G' ? 0 : 1;
        if (!Force_CD_Available(RequiredCD)) return false;
        std::string ini=root+".INI", bin=root+".BIN";
        if (!CCFileClass(ini.c_str()).Is_Available() || !CCFileClass(bin.c_str()).Is_Available()) return false;
        if (!Read_Scenario(Scen.ScenarioName) || !PlayerPtr || Frame!=0) return false;
        Options.Set();
        const auto faction=PlayerPtr->Class->House;
        const int credits=PlayerPtr->Credits, units=Units.Count(), infantry=Infantry.Count();
        const int buildings=Buildings.Count(), aircraft=Aircraft.Count();
        char filename[32],description[DESCRIP_MAX];
        std::snprintf(filename,sizeof(filename),"savegame.%03d",100+int(i));
        std::snprintf(description,sizeof(description),"START %s %02d %c%c",root[2]=='G'?"GDI":root[2]=='B'?"NOD":"BONUS",
                      Scen.Scenario,root[5],root[6]);
        const std::string path=dir+"/"+filename;
        if (!Save_Game(path.c_str(),description)) return false;
        // Validate a cold load, not a load that inherits this scenario's mode flags.
        Special.Init(); AreThingiesEnabled=false;
        ScenPlayer=SCEN_PLAYER_GDI; // Do not inherit Jurassic identity from the exported mission.
        if (!Load_Game(path.c_str()) || !PlayerPtr || Frame!=0) return false;
        if (bool(Special.IsJurassic)!=(root[2]=='J') || (root[2]=='J' && !AreThingiesEnabled)) {
            std::fprintf(stderr,"Cold-load mode mismatch: %s Jurassic=%d assets=%d\n",root.c_str(),
                         int(Special.IsJurassic),int(AreThingiesEnabled));
            return false;
        }
        if (root!=Scen.ScenarioName || PlayerPtr->Class->House!=faction || PlayerPtr->Credits!=credits ||
            Units.Count()!=units || Infantry.Count()!=infantry || Buildings.Count()!=buildings || Aircraft.Count()!=aircraft)
            return false;
        report << filename << '\t' << root << '\t' << Frame << '\t' << credits << '\t' << units << '\t'
               << infantry << '\t' << buildings << '\t' << aircraft << '\n';
        report.flush();
        std::printf("Verified %s %s frame=%d\n",filename,root.c_str(),Frame); std::fflush(stdout);
    }
    return report.good();
}
