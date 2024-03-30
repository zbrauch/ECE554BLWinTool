#include "TinyBLTool.h"


char comPort[20] = "\\\\.\\COMX\0\0";

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Please specify COM port (\"COMX\")\r\n");
        return 1;
    }

    //copy over COM port num string
    comPort[7] = argv[1][3];
    if(argv[1][4] != 0)
        comPort[8] = argv[1][4];

    serialib serial;


    printf("Attempting to connect to %s\r\n", comPort);
    char errorOpening = serial.openDevice(comPort, 57600);
    if(errorOpening != 1) {
        printf("Failed to connect. Error: %d\r\n", errorOpening);
        return -1;
    }
    else {
        printf("Successfully connected\r\n");
    }

    uint32_t mismatchCount = 0;
    for(uint32_t i = 0xF0000000; i < 0xF0000000+10000; i++) {
        //printf("Sending string: %s\r\n", std::to_string(i).c_str());
        //serial.writeString(argv[2]);
        //serial.writeBytes(argv[2],strlen(argv[2]));
        serial.writeBytes(std::to_string(i).c_str(), strlen(std::to_string(i).c_str()));

        uint8_t inbuf[0xFF];
        uint8_t count = serial.readBytes(inbuf, 0xFF, 1, 0);
        inbuf[count] = 0;

        //printf("Received: %s\r\n", inbuf);

        if(strcmp(std::to_string(i).c_str(),(char*)inbuf) != 0) {
            printf("!!!!!!!!!!!!!STRING MISMATCH!!!!!!!!!!!!\r\n");
            mismatchCount++;
        }
    }
    printf("Total Mismatches: %d\r\n", mismatchCount);

    serial.closeDevice();

    return 0;
}