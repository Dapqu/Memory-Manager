#include "MemoryManager.h"

#include <utility>
using namespace std;

// Constructor; sets native word size (in bytes, for alignment) and default allocator for finding a memory hole.
MemoryManager::MemoryManager(unsigned wordSize, function<int(int, void *)> allocator) {
    this->wordSize = wordSize;
    this->allocator = move(allocator);
    this->ptr = nullptr;
}

// Releases all memory allocated by this object without leaking memory.
MemoryManager::~MemoryManager() {
    shutdown();
}

// Instantiates block of requested size, no larger than 65536 words; cleans up previous block if applicable.
void MemoryManager::initialize(size_t sizeInWords) {
    shutdown();
    if (sizeInWords <= 65535) {
        this->sizeInWords = sizeInWords;
        this->ptr = new char[this->wordSize * this->sizeInWords];

        struct Block b1 = {0, sizeInWords, false};
        this->memory.push_back(b1);
    }
}

// Releases memory block acquired during initialization, if any. This should only include memory created for
// long term use not those that returned such as getList() or getBitmap() as whatever is calling those should
// delete it instead.
void MemoryManager::shutdown() {
    this->memory.clear();
    delete[] this->ptr;
    this->ptr = nullptr;
}

// Allocates a memory using the allocator function. If no memory is available or size is invalid, returns *nullptr*.
void *MemoryManager::allocate(size_t sizeInBytes) {
    size_t sizeInWords = ceil(sizeInBytes/this->wordSize);

    void *getListPtr = getList();
    int offsetToAllocate = this->allocator(sizeInWords, getListPtr);
    delete[] (uint16_t *) getListPtr;

    if (offsetToAllocate == -1) {
        return nullptr;
    }

    int i = 0;
    for (auto &block : memory) {
        if (block.offset == offsetToAllocate) {
            break;
        }
        i++;
    }

    auto it = memory.begin();
    advance(it, i);
    struct Block newBlock = {offsetToAllocate, sizeInWords, true};
    memory.insert(it, newBlock);
    it->offset += sizeInWords;
    it->length -= sizeInWords;

    // Move to the hole's new position
    if (it->length <= 0) {
        memory.erase(it);
    }

    return (void *)(this->ptr + offsetToAllocate * this->wordSize);
}

// Frees the memory block within the memory manager so that it can be reused.
void MemoryManager::free(void *address) {
    int offsetToRemove = ((char*) address - this->ptr) / this->wordSize;

    int i = 0;
    for (auto &block : memory) {
        if (block.offset == offsetToRemove) {
            break;
        }
        i++;
    }
    auto newHole = memory.begin();
    auto beforeNewHole = memory.begin();
    auto afterNewHole = memory.begin();
    advance(newHole, i);
    // Change it to hole.
    newHole->allocated = false;
    // If there's already a hole behind the new hole.
    // Makes sure the newHole is the last element of the list, so we can move the iterator to the next element without going out of bounce.
    if (newHole != memory.end()) {
        advance(afterNewHole, i + 1);
        if (!afterNewHole->allocated) {
            // Update the new hole length.
            newHole->length += afterNewHole->length;
            // Remove the hole that is behind the new hole.
            memory.erase(afterNewHole);
        }
    }
    // If there's already a hole before the new hole.
    // Makes sure the newHole is the first element of the list, so we can move the iterator to the previous element without going out of bounce.
    if (newHole != memory.begin()) {
        advance(beforeNewHole, i - 1);
        if (!beforeNewHole->allocated) {
            // Update the new hole offset and length.
            newHole->offset -= beforeNewHole->length;
            newHole->length += beforeNewHole->length;
            // Remove the hole that is before the new hole.
            memory.erase(beforeNewHole);
        }
    }
}

// Changes the allocation algorithm to identifying the memory hole to use for allocation.
void MemoryManager::setAllocator(function<int(int, void *)> allocator) {
    this->allocator = move(allocator);
}

// Uses standard POSIX calls to write hole list to filename as text, returning -1 on error and 0 if successful.
// Format: "[START, LENGTH] - [START, LENGTH] ...", e.g., "[0, 10] - [12, 2] - [20, 6]"
int MemoryManager::dumpMemoryMap(char *filename) {
    auto *list = (uint16_t *) getList();
    string output;

    if (list != nullptr) {
        for (int i = 1; i < list[0] * 2 + 1; i += 2) {
            output += "[";
            output += to_string(list[i]);
            output += ", ";
            output += std::to_string(list[i + 1]);
            // If is the last item
            if (i + 1 == list[0] * 2) {
                output += "]";
            }
            else {
                output += "] - ";
            }
        }
    }

    // Delete the list pointer to prevent memory leak.
    delete[] (uint16_t *) list;

    int newFile = open(filename, O_CREAT | O_WRONLY, 0666);
    if (newFile != - 1) {
        ssize_t newFileWritten = write(newFile, output.c_str(), output.size());
        if (newFileWritten != -1) {
            return close(newFile);
        }
        else {
            cout << "The write() is fked: " << newFileWritten << endl;
            return newFileWritten;
        }
    }
    else {
        cout << "The open() is fked: " << newFile << endl;
        return newFile;
    }
}

// Returns an array of information (in decimal) about holes for use by the allocator function (little-Endian).
// Offset and length are in words. If no memory has been allocated, the function should return a NULL pointer.
void *MemoryManager::getList() {
    if (memory.empty()) {
        return nullptr;
    }

    int numberOfHoles = 0;

    for (auto &block : memory) {
        if (!block.allocated) {
            numberOfHoles++;
        }
    }

    auto *arr = new uint16_t[1 + 2 * numberOfHoles];

    arr[0] = numberOfHoles;
    int j = 1;
    for (auto &block : memory) {
        if (!block.allocated) {
            arr[j++] = block.offset;
            arr[j++] = block.length;
        }
    }

    return arr;
}

// Returns a bit-stream of bits in terms of an array representing whether words are used (1) or free (0). The
// first two bytes are the size of the bitmap (little-Endian); the rest is the bitmap, word-wise.
void *MemoryManager::getBitmap() {
    vector<char> bits;
    for (auto &block : memory) {
        if (block.allocated) {
            for (int i = 0; i < block.length; i++) {
                bits.push_back('1');
            }
        }
        else {
            for (int i = 0; i < block.length; i++) {
                bits.push_back('0');
            }
        }
    }

    int extraSpace = 0;

    if (this->sizeInWords % 8 != 0) {
        extraSpace = 8 - (this->sizeInWords % 8);
    }

    for (int i = 0; i < extraSpace; i++) {
        bits.push_back('0');
    }

    reverse(bits.begin(), bits.end());

    int bitMapSize = ceil(bits.size() / 8.0f);

    vector<string> bytes;
    for (int i = 0; i < bitMapSize; i++) {
        string temp;
        for (int j = 0; j < 8; j++) {
            temp += bits.at(j + (8 * i));
        }
        bytes.push_back(temp);
    }

    vector<int> bytesInDecimal;
    bytesInDecimal.reserve(bytes.size());
    for (auto &byte : bytes) {
        bytesInDecimal.push_back(stoi(byte, nullptr, 2));
    }

    reverse((bytesInDecimal.begin()), bytesInDecimal.end());

    auto *arr = new uint8_t[2 + bytesInDecimal.size()];

    int i = 0;

    stringstream ss;
    ss << hex << bitMapSize;
    string hexBitMapSize (ss.str());
    if (hexBitMapSize.size() == 1) {
        hexBitMapSize = "000" + hexBitMapSize;
    }
    else if (hexBitMapSize.size() == 2) {
        hexBitMapSize = "00" + hexBitMapSize;
    }
    else if (hexBitMapSize.size() == 3) {
        hexBitMapSize = "0" + hexBitMapSize;
    }

    int lowByte = stoi(hexBitMapSize.substr(2, 2), nullptr, 16);
    int highByte = stoi(hexBitMapSize.substr(0, 2), nullptr, 16);

    arr[i++] = lowByte;
    arr[i++] = highByte;

    for (auto &bytes : bytesInDecimal) {
        arr[i++] = bytes;
    }

    return arr;
}

// Returns the word size used for alignment.
unsigned MemoryManager::getWordSize() {
    return this->wordSize;
}

// Returns the byte-wise memory address of the beginning of the memory block.
void *MemoryManager::getMemoryStart() {
    return this->ptr;
}

// Returns the byte limit of the current memory block.
unsigned MemoryManager::getMemoryLimit() {
    return (this->wordSize * this->sizeInWords);
}

// Returns word offset of hole selected by the best fit memory allocation algorithm, and -1 if there is no fit.
int bestFit(int sizeInWords, void *list) {
    auto *listPtr = (uint16_t *) list;

    int minHoleThatFits = INT_MAX;
    int offset = -1;
    for (int i = 2; i < listPtr[0] * 2 + 1; i += 2) {
        if (listPtr[i] < minHoleThatFits && listPtr[i] >= sizeInWords) {
            minHoleThatFits = listPtr[i];
            offset = listPtr[i - 1];
        }
    }

    return offset;
}

// Returns word offset of hole selected by the worst fit memory allocation algorithm, and -1 if there is no fit.
int worstFit(int sizeInWords, void *list) {
    auto *listPtr = (uint16_t *) list;

    int maxHole = 0;
    int offset = -1;
    for (int i = 2; i < listPtr[0] * 2 + 1; i += 2) {
        if (listPtr[i] > maxHole && listPtr[i] >= sizeInWords) {
            maxHole = listPtr[i];
            offset = listPtr[i - 1];
        }
    }

    return offset;
}