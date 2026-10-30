// Offline native integration probe. Never install this entry point.
#include "function.h"
#include "campaigncredits.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
static int failures;
static void Check(const char* label, bool ok) { std::printf("%s %s\n",ok?"PASS":"FAIL",label);std::fflush(stdout);if(!ok)++failures; }
struct Start {int slot;std::string root;};
static bool Fresh(const std::string& root,int carryover=0) {
    GameToPlay=GAME_NORMAL;GameActive=true;Frame=0;PlayerWins=PlayerLoses=false;DoList.Init();OutList.Init();
    Special.Init();Special.IsJurassic=root[2]=='J';AreThingiesEnabled=Special.IsJurassic;
    Scen.Scenario=(root[3]-'0')*10+root[4]-'0';ScenPlayer=root[2]=='G'?SCEN_PLAYER_GDI:root[2]=='B'?SCEN_PLAYER_NOD:SCEN_PLAYER_JP;
    ScenDir=root[5]=='E'?SCEN_DIR_EAST:SCEN_DIR_WEST;ScenVar=ScenarioVarType(root[6]-'A');Whom=root[2]=='B'?HOUSE_BAD:HOUSE_GOOD;
    Scen.Difficulty=Scen.CDifficulty=DIFF_NORMAL;Scen.CarryOverMoney=carryover;
    std::snprintf(Scen.ScenarioName,sizeof(Scen.ScenarioName),"%s",root.c_str());
    RequiredCD=root[2]=='G'?0:1;
    return Force_CD_Available(RequiredCD)&&Read_Scenario(Scen.ScenarioName)&&PlayerPtr&&Frame==0;
}
bool Generate_Mission_Starts(const char* directory) {
    std::string dir(directory);std::ifstream list(dir+"/starts.tsv");std::vector<Start> starts;
    int slot;std::string root;while(list>>slot>>root)starts.push_back({slot,root});
    Check("all 81 base, bonus and expansion starts supplied",starts.size()==81);
    std::ofstream report(dir+"/credits.tsv");report<<"slot\tmission\toriginal\tloaded\n";
    for(auto const& start:starts) {
        if(!Fresh(start.root)){Check("fresh native scenario loads",false);return false;}
        CCFileClass file((start.root+".INI").c_str());CCINIClass ini;
        Check("scenario INI is readable",ini.Load(file,false));
        int original=ini.Get_Int(PlayerPtr->Class->IniName,"Credits",0)*100;
        Check("fresh start retains original scenario credits",PlayerPtr->Available_Money()==original&&PlayerPtr->InitialCredits==original);
        char path[512];std::snprintf(path,sizeof(path),"%s/original/savegame.%03d",dir.c_str(),start.slot);
        if(!Load_Game(path)){Check("legacy start save loads",false);return false;}
        Check("legacy frame-zero start restores original credits",Frame==0&&PlayerPtr->Credits==original&&PlayerPtr->Tiberium==0&&PlayerPtr->InitialCredits==original);
        Check("displayed starting funds match restored balance",PlayerPtr->VisibleCredits.Credits==original&&PlayerPtr->VisibleCredits.Current==original);
        Check("scenario identity preserved",start.root==Scen.ScenarioName);
        int units=Units.Count(),infantry=Infantry.Count(),buildings=Buildings.Count();
        std::string roundtrip=dir+"/roundtrip.sav";
        Check("save restored start",Save_Game(roundtrip.c_str(),"Original opening funds"));
        Check("load restored start",Load_Game(roundtrip.c_str()));
        Check("restoring again is idempotent and keeps armies",PlayerPtr->Available_Money()==original&&Units.Count()==units&&Infantry.Count()==infantry&&Buildings.Count()==buildings);
        report<<start.slot<<'\t'<<start.root<<'\t'<<original<<'\t'<<PlayerPtr->Credits<<'\n';
        std::printf("MISSION %s native=%d loaded=%d\n",start.root.c_str(),original,PlayerPtr->Credits);
    }
    Check("fresh carry-over scenario starts",Fresh("SCG02EA",1234));int carryBalance=PlayerPtr->Available_Money();
    Check("native carry-over remains part of starting balance",carryBalance>0&&Scen.CarryOverMoney==1234);
    Check("save carry-over opening",Save_Game((dir+"/carryover.sav").c_str(),"Original carryover"));
    Check("reload carry-over opening",Load_Game((dir+"/carryover.sav").c_str()));
    Check("carry-over is neither lost nor counted twice",PlayerPtr->Available_Money()==carryBalance);
    Check("existing progress loads",Load_Game((dir+"/progress.sav").c_str()));
    Check("progress fixture is not frame zero",Frame>0);
    PlayerPtr->Credits=1234;PlayerPtr->Tiberium=56;PlayerPtr->InitialCredits=999;
    Check("save ongoing balance",Save_Game((dir+"/progress-roundtrip.sav").c_str(),"Keep ongoing funds"));
    Check("load ongoing balance",Load_Game((dir+"/progress-roundtrip.sav").c_str()));
    Check("ongoing money and initial-accounting are unchanged",PlayerPtr->Credits==1234&&PlayerPtr->Tiberium==56&&PlayerPtr->InitialCredits==999);
    auto balance=PlayerPtr->Available_Money();Restore_Campaign_Starting_Credits();
    Check("explicit helper also ignores ongoing games",PlayerPtr->Available_Money()==balance);
    std::printf("TOTAL FAILURES %d\n",failures);return failures==0;
}
