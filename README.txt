I have tested my program with all given test cases and I believe that it works correctly.

In the development of this program, I mainly used the c++ reference manual for outside help, 
as well as a bit of googling for error codes and bugs. The pages I used are linked below.

When developing this program, I decided that the simplest way for me to implement it was the 
following. I decided to make a fully associative cache as my underlying structure, and then
make a vector of those caches to represent different cache lines for direct-mapped/associative
caches. The LRU cache takes in its size, associativity, and blocksize, and uses that to calculate
the number of lines that will be in the total cache. Then I use that as the size of the vector.
I did it this way because the other way I was going to do it was to have an array of dynamic size 
as one of the class variables to represent the different lines of the cache, but that's illegal. 
To hold the data of each line's cache, I used a hash set because that has very quick searching for 
a collection of unique unordered elements, and that's what our fully associative cache is. When 
checking if something is in the cache, I check if it's in the set and if not, I add it. To keep 
track of the least recently used elements, I used a doubly linked list to be able to add and remove 
elements very quickly, especially at the two ends. The head of the list is the most recently used 
element and the tail is the least recently used, so when caching a new element to a full cache, I 
add it to the front of the list and remove the tail of the list, and remove the list's tail from the 
set as well. The accessCache function returns a boolean representing a cache hit(true) or miss(false). 
It also adds to the cache upon a miss. The function that I wrote to add something to the cache is 
private because an add is automatically triggered upon an access resulting in a miss. Therefore, there 
is no need for the addToCache function to be publicly accessible. This is important because now it's 
guaranteed that addToCache will only be called for an address that's not yet in the cache, which 
eliminates the need for another check within the addToCache function. 



https://stackoverflow.com/questions/5803150/list-iterator-access-violation
https://stackoverflow.com/questions/33052397/c-access-violation-reading-location-while-erasing-from-list
https://www.geeksforgeeks.org/passing-vector-function-cpp/