//#include "sqlite/sqlite3ext.h" only to create a SQL extension
#include <stdio.h>
#include "sqlite/sqlite3.h"





int main(void){
    printf("Hello");
    sqlite3 **ppDb;
    sqlite3_open("hola",ppDb);

    return 0;
}