#include <functional>
#include <list>
#include <iostream>
#include <cmath>
#include <climits>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

class MemoryManager {
private:
    struct Block {
        int offset;
        size_t length;
        bool allocated;
    };

    list<Block> memory;

    unsigned wordSize;
    function<int(int,void *)> allocator;

    size_t sizeInWords{};

    char* ptr;

public:
    MemoryManager(unsigned wordSize, function<int(int,void *)> allocator);
    ~MemoryManager();

    void initialize(size_t sizeInWords);
    void shutdown();
    void *allocate(size_t sizeInBytes);
    void free(void *address);
    void setAllocator(function<int(int, void *)> allocator);
    int dumpMemoryMap(char *filename);
    void *getList();
    void *getBitmap();
    unsigned getWordSize();
    void *getMemoryStart();
    unsigned getMemoryLimit();
};

int bestFit(int sizeInWords, void *list);
int worstFit(int sizeInWords, void *list);