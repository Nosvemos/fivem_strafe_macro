#include <fstream>
#include <string>
void LogMsg(const std::string& m) {
    std::ofstream out("debug.log", std::ios::app);
    out << m << std::endl;
}
