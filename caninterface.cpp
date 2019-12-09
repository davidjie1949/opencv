#include "caninterface.h"

//CanInterface::CanInterface(int canBus, string baudRate)
//    :m_canBus(canBus), m_baudRate(baudRate)
//{
//}

//bool CanInterface::connect(){
//    if(this->m_canBus != 1 && this->m_canBus !=2){
//        cout << "Error: Can Bus " << this->m_canBus << "not available" << endl;
//       return false;
//    }

//    string availableBaud[] = { "100k", "10k", "125k", "1M", "20k", "250k", "33k", "47k", "500k", "50k", "5k", "800k", "83k", "95k" };
//    int baudConst[] = { PCAN_BAUD_100K, PCAN_BAUD_10K, PCAN_BAUD_125K, PCAN_BAUD_1M, PCAN_BAUD_20K, PCAN_BAUD_250K, PCAN_BAUD_33K, PCAN_BAUD_47K, PCAN_BAUD_500K, PCAN_BAUD_50K, PCAN_BAUD_5K, PCAN_BAUD_800K, PCAN_BAUD_83K, PCAN_BAUD_95K };

//    int selectedBaud = -1;
//    if (selectedBaud != -1){
//        for (int i = 0; i < 14; i++){
//            if(availableBaud[i] == this->m_baudRate){
//                selectedBaud = baudConst[i];
//                break;
//            }
//        }
//    }else{
//        cout << "Error: Baud rate " << this->m_baudRate << " not available!" << endl;
//        return false;
//    }

//    TPCANStatus result;

//    if(this->m_canBus == 1){
//        result = CAN_Initialize(PCAN_USBBUS1, static_cast<TPCANBaudrate>(selectedBaud));
//    }else if (this->m_canBus == 2){
//        result = CAN_Initialize(PCAN_USBBUS2, static_cast<TPCANBaudrate>(selectedBaud));
//    }else{
//        result = PCAN_ERROR_REGTEST;
//        cout << "Please connect CAN Channel 1 or CAN Channel 2" << endl;
//    }

//    if(result == PCAN_ERROR_OK){
//        return true;
//    }else{
//        char strMsg[256];
//        CAN_GetErrorText(result, 0, strMsg);
//        cout << strMsg << endl;
//        return false;
//    }
//}

//bool CanInterface::read(vector<int>& output, int& timestamp){
//    vector<int> bytes;
//    TPCANMsg msg;
//    TPCANTimestamp ts;
//    TPCANStatus result;
//    result = CAN_Read(PCAN_USBBUS1, &msg, &ts);
//    if (result != PCAN_ERROR_QRCVEMPTY){
//        for(unsigned i = 0; i < msg.LEN; i++){
//            bytes.push_back(msg.DATA[i]);
//        }
//        output = bytes;
//        timestamp = ts.micros;
//        return true;
//    }else{
//        cout << "Receive queue is empty" << endl;
//        return false;
//    }
//}

//bool CanInterface::write(unsigned int ID, vector<int> bytes){
//    TPCANStatus result;
//    TPCANMsg msg;
//    msg.ID = static_cast<unsigned long>(ID);
//    msg.MSGTYPE = PCAN_MESSAGE_STANDARD;
//    msg.LEN = static_cast<unsigned char>(bytes.size());
//    for (unsigned i = 0; i < msg.LEN; i++){
//        msg.DATA[i] = static_cast<unsigned char>(bytes[i]);
//    }
//    if(this->m_canBus == 1){
//        result = CAN_Write(PCAN_USBBUS1, &msg);
//    }else if(this->m_canBus == 2){
//        result = CAN_Write(PCAN_USBBUS2, &msg);
//    }else{
//        result = PCAN_ERROR_REGTEST;
//        cout << "Please connect CAN Channel 1 or CAN Channel 2" << endl;
//    }

//    if(result == PCAN_ERROR_OK){
//        return true;
//    }else{
//        char strMsg[256];
//        CAN_GetErrorText(result, 0, strMsg);
//        cout << strMsg << endl;
//        return false;
//    }

//}
