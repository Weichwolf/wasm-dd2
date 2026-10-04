/* Execute the unchanged original numeric text parser or its actual C body.
 * Explicit source bytes, starting positions and output guards are compared. */
#include "pe_fixture.h"
#ifdef DD2_ORIGINAL_PRINT_NUMBER
#define Parse ((int (*)(int *,int,int))(uintptr_t)0x4222b4)
#else
int FUN_004222b4(int *,int,int);
#define Parse FUN_004222b4
#endif

static const char *numbers[] = {
    "0/","1/","-1/","255/","256/","511/","-256/","-257/",
    "2147483647/","-2147483648/","999999999/","1234567890/",
    "0000000000/","12345678901/","-1234567890/","12,34,56/",
    "-17,0,255/","/","","--1/","+1/"," 1/","999abc/",
    "0Xff/","12.34/","-0/","9876543210/"
};

static void snapshot(FILE *out) {
    require(fwrite((void *)0x900000,1,96,out)==96,"numeric source and guards");
    require(fwrite((void *)0x900100,1,32,out)==32,"numeric destination guards");
}

int main(int argc, char **argv) {
    unsigned input, position, i, description[3], count=0;
    const unsigned starts[] = {0,3,9};
    char *text=(char *)0x900010;
    int result;
    FILE *out;
    require(argc==3,"original executable and output required");
    map_original(argv[1]);
    out=fopen(argv[2],"wb");
    require(out!=NULL,"open numeric parser output");
    for (input=0; input<256+sizeof(numbers)/sizeof(numbers[0]); input++)
    for (position=0; position<3; position++) {
        memset((void *)0x900000,0xa5,96);
        memset(text,0,64);
        for (i=0; i<starts[position]; i++) text[i]='X';
        if (input<256) {
            text[starts[position]]=(char)input;
            text[starts[position]+1]='1';
            text[starts[position]+2]='7';
            text[starts[position]+3]='/';
        } else {
            strcpy(text+starts[position],numbers[input-256]);
        }
        for (i=0; i<32; i++)
            *(unsigned char *)(uintptr_t)(0x900100+i)=(unsigned char)(i*17+23);
        description[0]=count++;
        description[1]=input;
        description[2]=starts[position];
        require(fwrite(description,4,3,out)==3,"numeric parser descriptor");
        snapshot(out);
        result=Parse((int *)0x900110,(int)(uintptr_t)text,starts[position]);
        snapshot(out);
        require(fwrite(&result,4,1,out)==1,"actual returned text cursor");
    }
    require(count==849,"all numeric parser cases executed");
    require(fclose(out)==0,"close numeric parser output");
    return 0;
}
