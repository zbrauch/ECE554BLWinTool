#include "TinyBLTool.h"


char comPort[20] = "\\\\.\\COMX\0\0";


uint8_t HexToBinary(char *hexFilePath, uint8_t *binaryOut, uint32_t *byteCount, uint32_t *startAddr) {
    *byteCount = 0;
    *startAddr = 0;
    
    FILE *hexFile = fopen(hexFilePath, "rb");
    if(hexFile == NULL) {
        printf("ERROR: Failed to open HEX file!!!\r\n");
        return -1;
    }
    

    //(*byteCount)++;
    
    uint8_t inChar;
    uint32_t offsetAddr = 0;
    while(!feof(hexFile)) {
        fread(&inChar, 1, 1, hexFile);
        if(inChar == ':') { //HEX SOP
            uint8_t hexCmdBuf[100] = {};
            hexCmdBuf[0] = ':';
            uint8_t i;
            for(i = 1; hexCmdBuf[i-1] != '\n' && hexCmdBuf[i-1] != '\r'; i++) {
                fread(hexCmdBuf+i, 1, 1, hexFile);
            }
            hexCmdBuf[i] = 0;
            printf("Hex Command: %s\r\n", hexCmdBuf);


        }
    }


    fclose(hexFile);
    return 0;
}


int main(int argc, char *argv[]) {
    if(argc < 3) {
        printf("Please specify COM port (\"COMX\"), and RAMcode hex file\r\n");
        return 1;
    }

    //process RAMCode into binary block to be sent
    uint32_t startAddr;
    uint32_t byteCount;
    uint8_t *binaryRam = (uint8_t*)malloc(30000000);
    HexToBinary(argv[2], binaryRam, &byteCount, &startAddr);

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

    uint32_t msgCount = 1489;

    printf("Sending Init\r\n");
    uint8_t inoutBuf[0xff];
    inoutBuf[0] = 'I';
    *((uint32_t*)(inoutBuf+1)) = msgCount;
    *((uint32_t*)(inoutBuf+5)) = startAddr;
    inoutBuf[9] = 'E';
    serial.writeBytes(inoutBuf,10);
    uint8_t count = serial.readBytes(inoutBuf, 0xFF, 1, 0);
    inoutBuf[count] = 0;
    printf("Received: %s\r\n", inoutBuf);

    /*uint32_t mismatchCount = 0;
    for(uint64_t i = 0x200000000; i < 0x200000000+30000; i++) {
        printf("Sending string: %s\r\n", std::to_string(i).c_str());
        //serial.writeString(argv[2]);
        //serial.writeBytes(argv[2],strlen(argv[2]));
        serial.writeBytes(std::to_string(i).c_str(), strlen(std::to_string(i).c_str()));

        uint8_t inbuf[0xFF];
        uint8_t count = serial.readBytes(inbuf, 0xFF, 1, 0);
        inbuf[count] = 0;

        printf("Received: %s\r\n", inbuf);

        if(strcmp(std::to_string(i).c_str(),(char*)inbuf) != 0) {
            printf("!!!!!!!!!!!!!STRING MISMATCH!!!!!!!!!!!!\r\n");
            mismatchCount++;
        }
    }
    printf("Total Mismatches: %d\r\n", mismatchCount);*/

    serial.closeDevice();

    return 0;
}