#ifndef CANINTERFACE_H
#define CANINTERFACE_H

#include <iostream>
#include <vector>
#include "windows.h"
//#include "pcanbasic.h"

using namespace std;

class CanInterface{
public:
    //constructor
    CanInterface(int canBus, string baudRate);
    bool connect();
    bool read(vector<int>& output, int& timestamp);
    bool write(unsigned int ID, vector<int> bytes);

private:
    int m_canBus;
    string m_baudRate;
};

#endif // CANINTERFACE_H
