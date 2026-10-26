#include "Core.hpp"
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace lumen {
namespace {
std::uint32_t checksum(const std::string& text) {
    std::uint32_t h=2166136261u;
    for(unsigned char c:text) {h^=c;h*=16777619u;} return h;
}
#ifdef _WIN32
// Environment values from getenv() use the Windows ANSI code page, whereas
// MinGW's filesystem::path(char*) expects UTF-8. Keep paths native UTF-16
// instead, including usernames outside the active system code page.
std::wstring environmentPath(const wchar_t* name) {
    DWORD size=GetEnvironmentVariableW(name,nullptr,0);
    for(int attempt=0;size>0 && attempt<3;++attempt) {
        std::wstring value(size,L'\0');
        const DWORD copied=GetEnvironmentVariableW(name,value.data(),size);
        if(copied==0) return {};
        if(copied<size) {value.resize(copied);return value;}
        size=copied; // Retry if another thread enlarged the value meanwhile.
    }
    return {};
}
#endif
}
std::filesystem::path defaultSavePath() {
#ifdef _WIN32
    const auto custom=environmentPath(L"LUMENFALL_SAVE_DIR");
    if(!custom.empty()) return std::filesystem::path(custom)/L"save.dat";
    const auto local=environmentPath(L"LOCALAPPDATA");
    if(!local.empty()) return std::filesystem::path(local)/L"Lumenfall"/L"save.dat";
#else
    if(const char* custom=std::getenv("LUMENFALL_SAVE_DIR");custom && *custom)
        return std::filesystem::path(custom)/"save.dat";
#if defined(__APPLE__)
    if(const char* home=std::getenv("HOME")) return std::filesystem::path(home)/"Library"/"Application Support"/"Lumenfall"/"save.dat";
#else
    if(const char* data=std::getenv("XDG_DATA_HOME")) return std::filesystem::path(data)/"lumenfall"/"save.dat";
    if(const char* home=std::getenv("HOME")) return std::filesystem::path(home)/".local"/"share"/"lumenfall"/"save.dat";
#endif
#endif
    return "lumenfall-save.dat";
}
bool saveProgress(const std::filesystem::path& path,const Progress& p,std::string& error) {
    try {
        if(!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
        std::ostringstream body;
        body << std::setprecision(9) << p.abilities << ' ' << p.shards << ' ' << p.barriers << ' '
             << p.visited << ' ' << p.checkpointRoom << ' ' << p.checkpoint.x << ' ' << p.checkpoint.y
             << ' ' << int(p.bossDefeated) << ' ' << p.playSeconds << ' ' << p.deaths << '\n';
        auto temp=path; temp+=".tmp";
        {
            std::ofstream f(temp,std::ios::binary|std::ios::trunc);
            f << "LUMENFALL 1\n" << checksum(body.str()) << '\n' << body.str();
            f.flush();
            if(!f) {error="Cannot write save file";return false;}
        }
        // Keep a backup while replacing on platforms where rename cannot overwrite.
        auto backup=path; backup+=".bak";
        std::error_code ec;
        std::filesystem::remove(backup,ec); ec.clear();
        const bool existed=std::filesystem::exists(path);
        if(existed) std::filesystem::rename(path,backup);
        try { std::filesystem::rename(temp,path); }
        catch(...) {
            if(existed) std::filesystem::rename(backup,path,ec);
            throw;
        }
        error.clear(); return true;
    } catch(const std::exception& e) {error=e.what();return false;}
}
bool loadProgress(const std::filesystem::path& path,Progress& out,std::string& error) try {
    std::ifstream f(path,std::ios::binary);
    if(!f) {error="No saved journey";return false;}
    std::string header,line,body;
    std::getline(f,header); std::getline(f,line);
    body.assign(std::istreambuf_iterator<char>(f),{});
    if(header!="LUMENFALL 1" || body.size()>1024) {error="Invalid save format";return false;}
    std::uint32_t expected=0; std::istringstream hs(line);
    if(!(hs>>expected) || expected!=checksum(body)) {error="Save checksum mismatch";return false;}
    Progress p; int defeated=0; std::istringstream in(body);
    if(!(in>>p.abilities>>p.shards>>p.barriers>>p.visited>>p.checkpointRoom
        >>p.checkpoint.x>>p.checkpoint.y>>defeated>>p.playSeconds>>p.deaths)) {error="Incomplete save";return false;}
    if((p.abilities&~7u) || (p.shards&~255u) || (p.barriers&~1u) || (p.visited&~255u)
        || p.checkpointRoom<0 || p.checkpointRoom>=8 || !std::isfinite(p.checkpoint.x)
        || !std::isfinite(p.checkpoint.y) || !std::isfinite(p.playSeconds) || p.playSeconds<0
        || p.deaths<0 || defeated<0 || defeated>1 || (p.has(Wings)&&!p.has(Dash))
        || (p.has(Bell)&&!p.has(Dash|Wings)) || (defeated&&!p.has(Bell))) {
        error="Invalid save values";return false;
    }
    bool validCheckpoint=false;
    const auto world=makeWorld();
    for(const auto cp:world[p.checkpointRoom].checkpoints)
        if(distance(p.checkpoint,{cp.x-10,cp.y-HeroH})<1.f) validCheckpoint=true;
    if(!validCheckpoint) {error="Unknown checkpoint";return false;}
    p.bossDefeated=defeated!=0; out=p; error.clear(); return true;
} catch(const std::exception& e) {error=e.what();return false;}
} // namespace lumen
