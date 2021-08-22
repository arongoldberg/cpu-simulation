
#ifndef COMPARCHPROJ3_LRUCACHE_H
#define COMPARCHPROJ3_LRUCACHE_H

#include <list>
#include <unordered_set>

/**
 * Fully associative LRU cache
 */
class LRUCache{
public:
    //default constructor
    LRUCache(){
        size = 0;
        blocksize = 0;
        associativity = 0;
        lines = 0;
    }
    //constructor
    LRUCache(int size, int associativity, int blocksize){
        this->size = size;
        this->blocksize = blocksize;
        this->associativity = associativity;
        lines = size/blocksize/associativity;
    }
    
    /**
     * lineNumber(address)
     * 
     * This function takes in an address and returns which line of the cache that address will be
     * stored on. It calculates this by dividing the address by the blocksize to get the blockID
     * and then the line number is calculated as blockID % lines.
     * 
     * @param address - An int representing an address in memory
     * @return Returns the line number of the cache that will hold the given address
     */
    int lineNumber(int address) const{
        int blockID = address/blocksize;
        return blockID % lines;
    }
    
    //Getter function for the number of lines in the cache
    int numLines() const{
        return lines;
    }

    /**
     * accessCache(address)
     * 
     * This function takes in a memory address and returns a boolean representing whether or not
     * the address was already in the cache. If the address was not in the cache, it adds it to 
     * the cache. If the cache is full before adding, it will remove the least recently used 
     * memory address. If the address is already present in the cache, it will just rearrange the
     * list representing the least recently used memory addresses to reflect this access.
     * 
     * @param address - An int representing an address in memory
     * @return Returns true if the address results in a cache hit and false if not
     */
    bool accessCache(int address){
        int blockID = address/blocksize;

        bool isInCache = cacheSet.count(blockID);

        if(isInCache){
            //rearrange list by removing the block that we're looking for and putting
            //it back in the front of the list
            lastUsed.remove(blockID);
            lastUsed.push_front(blockID);
        }else{//not in cache yet
            putInCache(address);
        }

        return isInCache;
    }

private:
    std::list<int> lastUsed;
    int blocksize;
    int size;
    int lines;
    size_t associativity;
    std::unordered_set<int> cacheSet;

    /**
     * putInCache(address)
     * 
     * This function adds an address to the cache. If the cache is full before adding, it will remove
     * the least recently used element. After adding to the cache, it updates the list representing the
     * least recently used elements.
     * 
     * @param address - An int representing an address in memory
     */
    void putInCache(int address){
        int blockID = address/blocksize;

        //If the cache is full, remove the least recently used block, which is stored at the tail of lastUsed
        if(cacheSet.size() >= associativity) {
            cacheSet.erase(lastUsed.back());
            lastUsed.pop_back();
        }

        //Add block to cache set and put it in front of lastUsed list, marking it as the most recently used
        cacheSet.insert(blockID);
        lastUsed.push_front(blockID);
    }

};


#endif //COMPARCHPROJ3_LRUCACHE_H
