#include "TinyBLTool.h"


char comPort[20] = "\\\\.\\COMX\0\0";


uint8_t HexToBinary(char *hexFilePath, uint8_t *binaryOut, uint32_t *byteCount, uint32_t *startAddr, uint32_t *entryAddr) {
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
            //printf("Hex Command: %s\r\n", hexCmdBuf);

            //extract data len
            uint8_t dataLen = AsciiHexToByte(hexCmdBuf+1);
            //extract address
            uint8_t byteBuf[50];
            AsciiHexToBytes(hexCmdBuf+3, 4, byteBuf);
            uint32_t cmdAddr = (((uint32_t)byteBuf[0])<<8) + (uint32_t)byteBuf[1];
            //extract data
            AsciiHexToBytes(hexCmdBuf+9, dataLen<<1, byteBuf);

            //record type
            uint8_t cmdType = AsciiHexToByte(hexCmdBuf+7);
            if(cmdType == 0) { //data
                //printf("Data Command: %s  Addr: %04x\r\n", hexCmdBuf, cmdAddr);
                if(*startAddr == 0)
                    *startAddr = offsetAddr + cmdAddr;
                
                for(uint32_t i = 0; i < dataLen; i++)
                    binaryOut[offsetAddr - *startAddr + cmdAddr + i] = byteBuf[i];

                //inefficient code
                if(*byteCount < (offsetAddr + cmdAddr + (uint32_t)dataLen - *startAddr))
                    *byteCount = offsetAddr + cmdAddr + (uint32_t)dataLen - *startAddr;
            }
            else if(cmdType == 1) { //EOF

            }
            else if (cmdType == 2) { //extended segment address

            }
            else if (cmdType == 3) { //start segment address

            }
            else if (cmdType == 4) { //extended linear address
                //printf("Address Command: %s\r\n", hexCmdBuf);
                offsetAddr = (((uint32_t)byteBuf[0]) << 24) + (((uint32_t)byteBuf[1]) << 16);
            }
            else if (cmdType == 5) { //start linear address
                *entryAddr = (((uint32_t)byteBuf[0]) << 24) + (((uint32_t)byteBuf[1]) << 16)
                                + (((uint32_t)byteBuf[2]) << 8) + (((uint32_t)byteBuf[3]));
            }
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
    uint32_t startAddr = 0;
    uint32_t byteCount = 0;
    uint8_t *binaryRam = (uint8_t*)malloc(30000000);
    uint32_t entryAddr = 0;
    HexToBinary(argv[2], binaryRam, &byteCount, &startAddr, &entryAddr);
    printf("Byte Count: 0x%08x, Start Addr: 0x%08x, Entry Address: 0x%08x\r\n", byteCount, startAddr, entryAddr);

    //temp write binary to file
    FILE *tmpOutFile = fopen("tempbin", "wb");
    fwrite(binaryRam, 1, byteCount, tmpOutFile);
    fclose(tmpOutFile);

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

    uint32_t msgCount = (byteCount>>3) + ((byteCount%8)?1:0);

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

    uint8_t dummyByte;
    printf("Press ENTER to continue\r\n");
    scanf("%c", &dummyByte);


    printf("Sending RAM code...\r\n");
    inoutBuf[0] = 'D';
    inoutBuf[9] = 'E';
    for(uint32_t i = 0; i < byteCount; i+= 8) {
        memcpy(inoutBuf+1, binaryRam+i, 8);
        serial.writeBytes(inoutBuf,10);
        usleep(100);
    }

    printf("Press ENTER to continue\r\n");
    scanf("%c", &dummyByte);

    printf("Sending second init message\r\n");
    inoutBuf[0] = 'I';
    inoutBuf[9] = 'E';
    *((uint32_t*)(inoutBuf+1)) = entryAddr;
    serial.writeBytes(inoutBuf,10);
    count = serial.readBytes(inoutBuf, 0xFF, 1, 0);
    inoutBuf[count] = 0;
    printf("Received: %s\r\n", inoutBuf);


    //continue printing messages
    for(;;) {
        count = serial.readBytes(inoutBuf, 0xFF, 10, 0);
        if(count) {
            inoutBuf[count] = 0;
            printf("Received: %s\r\n", inoutBuf);
        }
    }
    

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
    free(binaryRam);

    return 0;
}