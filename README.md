# Memory-Manager
Implement a memory manager in C++, whose features include initializing, tracking, allocating, and deallocating sections of memory. Integrate the memory manager into the console-based testing program provided.

## DESCRIPTION
###### MemoryManager(unsigned wordSize, std::function<int(int, void*)> allocator)
- Constructor. Takes arugment to object's class variables.

###### ~MemoryManager()
- Destructor. Calls shutdown() on termination.

###### void initialize(size_t sizeInWords)
- Calls shutdown() at start to clean up memory , stores sizeInWords via argument, and assign ptr to new memory address.

###### void shutdown()
- Clear memory list data-structure delete the ptr's memory address and assign it to NULL.

###### void* allocate(size_t sizeInBytes)
- Calculate sizeInWords then pass in to allocation algorithm with getList's list to calculate the proper offset, if there is enough memory and the size is valid, adjust the memory list and return the pointer to its memory address.

###### void free(void* address)
- Calculates the proper offset for the memory block that is asked to remove in the memory list. Find the item that has the offset and set its boolean allocation status to false then merge the neighbor elements if they are holes.

###### void setAllocator(std::function<int(int, void*)> allocator)
- Sets the objects allocator variable to new allocation algorithm.

###### int dumpMemoryMap(char* filename)
- Uses POSIX calls like open(), write(), and close() to write hole list to text file using given filename. Stores the proper hole list and return 0 on success, -1 on fail.

###### void* getList()
- Iterates through memory list and store the numberOfHoles as its first element and every holes' offset and length following it.

###### void* getBitmap()
- Taken the information from getList() to create a little-Endian bit-stream of bits to visually represent the memory with 1s being used word and 0s being free. Split the bits into bytes and converting them to decimal and stores the decimal values to array that contain size of bitMap at the start of it using low/high bytes.

###### unsigned getWordSize()
- Returns this->wordSize.

###### void* getMemoryStart()
- Returns main memory address.

###### unsigned getMemoryLimit()
- Returns size of main memory block.

###### int bestFit(int sizeInWords, void* list)
- Gets list of holes and finds the smallest hole that can store the given size of memory, return its offset.

###### int worstFit(int sizeInWords, void* list)
- Gets list of holes and finds the biggest hole there is and return its offset.

## TESTING
Created the executable following the step given from the spec, executed the executable via Valgrind, to both test the functionality of the program as well as checking the memory leaks.

## BUGS
Wrong data type for getList array resulting 0s between each element during testing. Replacing the datatype from int to uint16_t solved the issue

## LINK
[Youtube Walkthrough](https://youtu.be/iCEJiC6qYlg)

## REFERENCES/CITATIONS
https://stackoverflow.com/questions/2245193/why-does-open-create-my-file-with-the-wrong-permissions
https://opendsa-server.cs.vt.edu/embed/firstFitAV

## AUTHOR
Dilimulati Diliyaer
