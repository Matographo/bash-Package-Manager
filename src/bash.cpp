#include "bash.h"

Bash::Bash() {
}

Bash::~Bash() {
}

int Bash::install(std::string package) {
    Package pkg = this->toolkit->parsePackage(package);
    PackagePaths pkgPath = this->toolkit->getPackagePaths(this->progLang, pkg.name, pkg.version);
    this->toolkit->installOwnDatabase(this->progLang, this->gitRepo);
    
    if (std::filesystem::exists(pkgPath.packageVersionPath)) {
        return 0;
    }
    
    if(!std::filesystem::exists(pkgPath.packageBasePath)) {
        std::filesystem::create_directories(pkgPath.packageBasePath);
    }

    if(!std::filesystem::exists(pkgPath.packageVersionPath)) {
        std::filesystem::create_directories(pkgPath.packageVersionPath);
    }
    
    std::string repo = this->toolkit->getRepoFromDatabase(this->progLang, pkg.name);

    if(repo == "") {
        std::cout << "No package found with name " << pkg.name << std::endl;
        return 1;
    }
    
    Downloader downloader;

    if(downloader.downloadGit(repo, pkgPath.packageRawPath)) {
        std::cout << "Failed to download package " << pkg.name << std::endl;
        return 1;
    }
    
    return this->createNewVersion(&pkg, &pkgPath);
}

int Bash::install(std::vector<std::string> packages) {
    int result = 0;
    for(std::string package : packages) {
        result += this->install(package);
    }
    return result;
}

int Bash::uninstall(std::string package) {
    Package pkg = this->toolkit->parsePackage(package);
    PackagePaths pkgPath = this->toolkit->getPackagePaths(this->progLang, pkg.name, pkg.version);

    return this->toolkit->uninstallPackage(this->progLang, pkg.name, pkg.version);
}

int Bash::uninstall(std::vector<std::string> packages) {
    return this->toolkit->uinistallAllPackages(this->progLang);
}

int Bash::update(std::string package) {
    Package pkg = this->toolkit->parsePackage(package);

    return this->toolkit->updatePackage(this->progLang, pkg.name);
}

int Bash::update(std::vector<std::string> packages) {
    int result = 0;
    for(std::string package : packages) {
        result += this->update(package);
    }
    return result;
}

int Bash::update() {
    return this->toolkit->updateAllPackages(this->progLang);
}

int Bash::search(std::string package) {
    this->toolkit->getPackagePaths(this->progLang, package, "");
    this->toolkit->installOwnDatabase(this->progLang, this->gitRepo);
    return this->toolkit->searchPackageDatabase(this->progLang, package);
}

int Bash::list() {
    return this->toolkit->listInstalledPackages(this->progLang);
}

int Bash::info(std::string package) {
    return this->toolkit->infoPackage(this->progLang, package);
}

void Bash::setToolkit(PackageManagerToolkit * toolkit) {
    this->toolkit = toolkit;
    this->createSetup();
}

int Bash::createNewVersion(Package * pkg, PackagePaths * pkgPath) {
    std::string gitCommand = "git -C " + pkgPath->packageRawPath + " checkout ";
    
    if(pkg->isHash) gitCommand += pkg->version;
    else gitCommand += "tags/" + pkg->version;
    
    if(system(gitCommand.c_str())) {
        std::cout << "Failed to checkout version " << pkg->version << " of package " << pkg->name << std::endl;
        return 1;
    }
    
    for(auto & p : std::filesystem::recursive_directory_iterator(pkgPath->packageRawPath)) {
        if(p.is_regular_file() && (p.path().extension() == ".sh" || p.path().extension() == ".bash" || p.path().filename() == "README.md")) {
            std::filesystem::copy(p, pkgPath->packageVersionPath / p.path().filename());
        }
    }
    return 0;
}

int Bash::createSetup() {
    PackagePaths paths = this->toolkit->getPackagePaths(this->progLang, "", "");
    
    if(!std::filesystem::exists(paths.languagePackagePath)) {
        std::filesystem::create_directories(paths.languagePackagePath);
    }
    
    std::string libraryPath = paths.languagePackagePath + "/library.sh";
    
    if(!std::filesystem::exists(libraryPath)) {
        std::ofstream libraryFile(libraryPath);
        libraryFile << this->getBashLibrary();
        libraryFile.close();
    }
    
    std::string shell = getenv("SHELL");
    std::string shellrc = std::string(getenv("HOME")) + "/." + shell + "rc";
    
    std::string command = "source " + libraryPath;
    
    std::string commandCheck = "grep \"" + command + "\" " + shellrc;
    
    if(!system(commandCheck.c_str())) {
        std::ofstream shellrcFile(shellrc, std::ios_base::app);
        shellrcFile << std::endl << command;
        shellrcFile.close();
    }
    
    return 0;
}

std::string Bash::getBashLibrary() {
    std::stringstream library;
    
    library << "#!/bin/bash" << std::endl;
    library << "import() {" << std::endl;
    library << "    MAIN_PATH=~/.nxpm/packages/bash" << std::endl;
    library << "    LIBRARY=$1" << std::endl;
    library << "    VERSION=$2" << std::endl;
    library << "    FILES=(\"$3\")" << std::endl;
    library << "" << std::endl;
    library << "    if [ -z \"$LIBRARY\" ]; then" << std::endl;
    library << "        echo \"Library name is required\"" << std::endl;
    library << "        exit 1" << std::endl;
    library << "    elif [ -z \"$VERSION\" ]; then" << std::endl;
    library << "        VERSION_PATH=$MAIN_PATH/$LIBRARY" << std::endl;
    library << "        VERSIONS=$(ls -d $VERSION_PATH/* | grep -Eo '[0-9]+\\.[0-9]+\\.[0-9]+' | sort -V)" << std::endl;
    library << "        VERSION=$(echo \"$VERSIONS\" | tail -n 1)" << std::endl;
    library << "    fi" << std::endl;
    library << "" << std::endl;
    library << "    if [ ${#FILES[@]} -eq 0 ]; then" << std::endl;
    library << "        FILES=($MAIN_PATH/$LIBRARY/$VERSION/*.sh)" << std::endl;
    library << "    else" << std::endl;
    library << "        for i in ${!FILES[@]}; do" << std::endl;
    library << "            FILES[i]=$MAIN_PATH/$LIBRARY/$VERSION/${FILES[$i]}" << std::endl;
    library << "        done" << std::endl;
    library << "    fi" << std::endl;
    library << "" << std::endl;
    library << "    for file in $FILES; do" << std::endl;
    library << "        if [ ! -f $file ]; then" << std::endl;
    library << "            echo \"Library not found\"" << std::endl;
    library << "            exit 1" << std::endl;
    library << "        fi" << std::endl;
    library << "        source $file" << std::endl;
    library << "    done" << std::endl;
    library << "}" << std::endl;
    
    return library.str();
}


extern "C" PackageManager * create() {
    return new Bash();
}

extern "C" void destroy(PackageManager * pm) {
    delete pm;
}