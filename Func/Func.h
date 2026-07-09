#ifndef FUNC_H
#define FUNC_H
#define MAX(a,b) (a)>(b)?(a):(b)
#define MIN(a,b) (a)>(b)?(b):(a)
#define SWAP(a,b) do{\
   typeof(a) temp=(a);\
   (a)=(b);\
   (b)=temp;\
}while(0)
#define ArrSize(a) sizeof(a)/sizeof(a[0])
#define my_offset_of(Struct,member) ((size_t)&((Struct*)0)->member) 
#define my_container_of(Struct,member,node) ({\
   const typeof(((Struct*)0)->member)* __ptr=(node);\
    (Struct*)((char*)(__ptr)-my_offset_of(Struct,member));\
})
#endif