#include "Core.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace lumen;
namespace {
int checks=0;
void require(bool ok,const char* message) {
    ++checks;
    if(!ok) throw std::runtime_error(message);
}

// Restore only the test process's environment, including on assertion failure.
class ScopedEnvironment {
public:
#ifdef _WIN32
    explicit ScopedEnvironment(const wchar_t* key):name(key) {
        std::wstring buffer(32768,L'\0');
        DWORD n=GetEnvironmentVariableW(name.c_str(),buffer.data(),DWORD(buffer.size()));
        if(n>0 && n<buffer.size()) {buffer.resize(n);previous=buffer;}
    }
    void set(const std::filesystem::path& value) {
        require(SetEnvironmentVariableW(name.c_str(),value.c_str())!=0,"Set Unicode environment value");
    }
    void clear() {require(SetEnvironmentVariableW(name.c_str(),nullptr)!=0,"Clear environment override");}
    ~ScopedEnvironment() {SetEnvironmentVariableW(name.c_str(),previous?previous->c_str():nullptr);}
private:
    std::wstring name;
    std::optional<std::wstring> previous;
#else
    explicit ScopedEnvironment(const char* key):name(key) {
        if(const char* value=std::getenv(key)) previous=value;
    }
    void set(const std::filesystem::path& value) {require(setenv(name.c_str(),value.c_str(),1)==0,"Set UTF-8 environment value");}
    ~ScopedEnvironment() {
        if(previous) setenv(name.c_str(),previous->c_str(),1);
        else unsetenv(name.c_str());
    }
private:
    std::string name;
    std::optional<std::string> previous;
#endif
};

struct TestDirectory {
    std::filesystem::path root,originalDirectory;
    TestDirectory() {
        originalDirectory=std::filesystem::current_path();
        const auto id=std::chrono::high_resolution_clock::now().time_since_epoch().count();
        root=std::filesystem::temp_directory_path()/("lumen-path-regression-"+std::to_string(id));
        std::filesystem::create_directories(root);
    }
    ~TestDirectory() {
        std::error_code ec;
        std::filesystem::current_path(originalDirectory,ec);
        std::filesystem::remove_all(root,ec);
    }
};

void roundtrip(const std::filesystem::path& file) {
    std::string error;Progress original,loaded;
    require(saveProgress(file,original,error),"Write save in Unicode directory");
    require(loadProgress(file,loaded,error),"Read save in Unicode directory");
    require(loaded.checkpointRoom==original.checkpointRoom && loaded.abilities==0,"Read correct progress");
    original.abilities=Dash;
    require(saveProgress(file,original,error),"Replace save in Unicode directory");
    require(loadProgress(file,loaded,error) && loaded.abilities==Dash,"Read replaced save");
    auto backup=file;backup+=".bak";
    require(loadProgress(backup,loaded,error) && loaded.abilities==0,"Read previous Unicode-path backup");
    require(!std::filesystem::exists(std::filesystem::path(file).concat(".tmp")),"Temporary save renamed successfully");
}
}

int main() {
    try {
        TestDirectory dir;
#ifdef _WIN32
        ScopedEnvironment custom(L"LUMENFALL_SAVE_DIR");
        ScopedEnvironment local(L"LOCALAPPDATA");
#else
        ScopedEnvironment custom("LUMENFALL_SAVE_DIR");
#endif
        const std::string names[]={"ASCII folder",u8"\u4e2d\u6587\u5b58\u6863",u8"\u65e5\u672c\u8a9e_\u00e9_\U0001f31f"};
        for(const auto& name:names) {
            const auto folder=dir.root/std::filesystem::u8path(name);
            custom.set(folder);
            const auto file=defaultSavePath();
            require(file==folder/"save.dat","Environment path must be preserved exactly");
            roundtrip(file);
        }
        // Also exercise startup from a non-ASCII working directory.
        const auto working=dir.root/std::filesystem::u8path(u8"\u6e38\u620f\u76ee\u5f55");
        std::filesystem::create_directories(working);
        std::filesystem::current_path(working);
        require(defaultSavePath().is_absolute(),"Unicode working directory does not change explicit save location");
#ifdef _WIN32
        // This is the ordinary double-click launch path: no custom override,
        // LOCALAPPDATA contains a non-ASCII Windows username.
        custom.clear();
        const auto profile=dir.root/std::filesystem::u8path(u8"Users/\u5f20\u4e09_\U0001f31f/AppData/Local");
        local.set(profile);
        require(defaultSavePath()==profile/L"Lumenfall"/L"save.dat","Non-ASCII Windows profile is lossless");
        roundtrip(defaultSavePath());
        custom.set(dir.root/"override");
        require(defaultSavePath()==dir.root/"override"/"save.dat","Custom save location takes precedence");
#endif
        Progress unchanged;unchanged.abilities=Dash;
        std::string error;
        require(!loadProgress(dir.root/"does-not-exist.dat",unchanged,error),"Missing save does not terminate startup");
        require(unchanged.abilities==Dash,"Failed read preserves output progress");
        std::cout<<"PASS Unicode save paths: "<<checks<<" checks\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL Unicode save paths: "<<e.what()<<'\n';return 1;}
}
