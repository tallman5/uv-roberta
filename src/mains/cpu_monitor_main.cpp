#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>

/*
g++ -o cpu_monitor cpu_monitor_main.cpp
*/


// Function to read the CPU statistics from /proc/stat
void readCpuStat(long long& user, long long& nice, long long& system, long long& idle)
{
    std::ifstream file("/proc/stat");
    std::string line;
    
    if (file.is_open())
    {
        std::getline(file, line);
        std::istringstream iss(line);
        iss.ignore(5);  // Ignore "cpu" string
        
        // Read CPU usage statistics
        iss >> user >> nice >> system >> idle;
    }
    
    file.close();
}

int main()
{
    long long prevUser, prevNice, prevSystem, prevIdle;
    long long user, nice, system, idle;
    
    // Read initial CPU statistics
    readCpuStat(prevUser, prevNice, prevSystem, prevIdle);
    
    while (true)
    {
        // Wait for 1 second
        sleep(1);
        
        // Read current CPU statistics
        readCpuStat(user, nice, system, idle);
        
        // Calculate total and usage percentages
        long long prevTotal = prevUser + prevNice + prevSystem + prevIdle;
        long long total = user + nice + system + idle;
        long long totalDiff = total - prevTotal;
        double usagePercent = (totalDiff - (idle - prevIdle)) * 100.0 / totalDiff;
        
        std::cout << "CPU Usage: " << usagePercent << "%" << std::endl;
        
        // Update previous CPU statistics
        prevUser = user;
        prevNice = nice;
        prevSystem = system;
        prevIdle = idle;
    }
    
    return 0;
}
