#include "headers/processParser.h"
#include <vector>
#include <string>
#include <iostream>
#include <map>


int main() {
    ProcessParser parser;

   std::string json = parser.getProcessorSpecs();

   std::string json1 = parser.getMemoryInfo();

   std::string json2 = parser.getCurrentCpuMhz();

   std::string json3 = parser.getKernelVersion();

   std::string json4 = parser.getMountsInfo();

   std::string json5 = parser.getUptime();


   std::string json6 = parser.getLoadAvg();

    std::string json7 = parser.getUptime();

    std::string json8 = parser.getNetworkStats();


    std::string json9 = parser.getSwapInfo();



   std::cout << json << std::endl;

   std::cout << json1 << std::endl;

    std::cout << json2 << std::endl;

    std::cout << json3 << std::endl;

    std::cout << json4 << std::endl;

    std::cout << json5 << std::endl;

    std::cout << json6 << std::endl;

    std::cout << json7 << std::endl;

    std::cout << json8 << std::endl;

    std::cout << json9 << std::endl;
}