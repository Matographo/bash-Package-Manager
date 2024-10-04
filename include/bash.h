#ifndef BASH_H
#define BASH_H

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Downloader.h"
#include "sqlite3.h"
#include "PackageManager.h"

class Bash : public PackageManager {
private:
    PackageManagerToolkit * toolkit;
    const std::string progLang = "bash";
    const std::string gitRepo = "Matographo/bash-database";
    

    int createNewVersion(Package * pkg, PackagePaths * pkgPath);
    int createSetup();
    std::string getBashLibrary();

public:
    Bash();
    virtual ~Bash();
    
    virtual int install(std::string package) override;
    virtual int install(std::vector<std::string> packages) override;
    virtual int uninstall(std::string package) override;
    virtual int uninstall(std::vector<std::string> packages) override;
    virtual int update(std::string package) override;
    virtual int update(std::vector<std::string> packages) override;
    virtual int update() override;
    virtual int search(std::string package) override;
    virtual int list() override;
    virtual int info(std::string package) override;
    virtual void setToolkit(PackageManagerToolkit * toolkit) override;
};

#endif
