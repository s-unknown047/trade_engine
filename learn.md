we use uint64_t, uint32_t because u means unsigned and int64_t means int type with 64 bits it is machine independent 
means int is machine dependent same for long but uint64_t means it should be exactly 64 bits in every system


Fan out -
this is a when a process is divided into sub process using and then each task run parallel 

Fan-out refers to the architectural pattern where a single request, event, or task is distributed to multiple downstream services or workers simultaneously.

In simple terms: One input → many parallel executions

fan in - 
when result of several process is aggegated into one result that is fan out

Fan-in is the reverse of fan-out. It refers to collecting, aggregating, or merging results from multiple services or workers into a single outcome.

In simple terms: Many outputs → one combined result


Varient: is a datatype similar to the union in c we can save multiple type of data in it which inculde primitive,no premitive (struct, class) and even other varient but the main advantage is it keep track of active datatype and thus have type safety 
how union works is it take the size of max size datatype and at a time hold a single type value which can be accessed and if other value is stored it overwrites previous value.

varient syntax: varient<int, double, float> v;
v = 123;
holds_alternative() Check if the given type of data is stored inside the variant at the given moment in time.

holds_alternative<int>(v) if true 
get<int>(v);

index()	Returns the index of the type of data stored in the variant.
if we have at index one string 
v.emplace<1>("ghi")

batching is the process in which is all the process nrt proces are aggrigated and feed to the cpu 



DI is a design patter used to implement IOC inversion of control. It allows a program to remove hard-coded dependencies by "injecting" them (passing them in) from the outside, typically through a constructor or a setter.


file handiling 
#include <fstream>
ifstream used of reading data from file
ofstream used for writing data to files

creating and opening of file using ofstream 
ofstream outFile("name.txt");
outFile.is_open() if file is open or not 

outFile.close();

ifstream inFile("name.txt");

inFile.is_open()
string line
getline(inFile, line)

ios::in	Opens for reading (default for ifstream)
ios::out	Opens for writing (default for ofstream)
ios::app	Appends new output to the end of the file
ios::trunc	Deletes the contents of the file if it exists before opening
ios::binary	Opens the file in binary mode for raw data operations
ios::ate	Opens the file and moves the read/write control to the end

Stream Buffer is a middleman btw the high-level i/o and low-level physical device 

Think of it as a temporary warehouse. When you use std::cout << "Hello", you aren't writing directly to the screen; you are putting bytes into a buffer. Once that buffer is full or "flushed," the data is sent to the OS in one big chunk to save time.

stream buffer have three pointers pbase start of buffer 
epptr end of write buffer
pptr the put pointer where the next character will be 


vector store three things in the stack the a pointer to the start of vector,
a pointer to the end of vector 
a pointer to the end of capacity
vector.data() return pointer to the start;

chrono library

Date and time 
this is library to handle time and duration

duration ex std::chrono::second(5)
time point ex std::clock::now()
Clock the source of time

system_clock : system wide real time wall clocl

steady_clock : monotonically increasing clock

high_resolution_clock : clock with the smallest tick period


Today i learned about memory reordering in detail

what is memory ordering this is a process performed by the compiler and os to optimise your code it does not matter in a single threaded code but for a multi threaded code it can create problem where the application need to read and write to a shared memory simultaneously

three type of ordering in atomic variable 

Sequentially Consistent: This is the most strict and straightforward ordering model. It guarantees that all the threads will agree on the same order of events, meaning that they will establish a single total modification order of all atomic operations that are tagged with memory_order_seq_cst.
this means that no reordering how we write the code it is excepted to run in the same way. 


Relaxed: This ordering poses no constraints on the CPU and the compiler. The only guarantee with this ordering is that the operations will be carried out atomically. This ordering applies to operations tagged with memory_order_relaxed.
means it can reorder as it want most efficient and performance but can have different results.


Acquire/Release: 
Acquire: When you perform a load with memory_order_acquire, you are telling the system:

    The Barrier: No memory reads or writes that appear after this load in the code can be reordered to happen before it.

Release: When you perform a store with memory_order_release, you are telling the system:

    The Barrier: No memory reads or writes that appear before this store in the code can be reordered to happen after it.


    // Vector in cpp

    vectors in static alloction store three variables [pointer to the first memory in dynamic memory], size, capacity,

    vectors are stored in heap in a continious memory location if that location is filled then it get reallocated to different memory location in heap first allocate the newcapity worth of continious memory block then copy the previous elements  

    // why we are having  LUT index based hashing int (system id ) -> int (order id ) and unordered_map<int, int> in order_gateway.cpp file order_id -> system_id 

    becasuse system_id are generated sequencly while order_id can be random in LUT (Look up table) sytemid -> orderid which is cache friendly as it is stored sequencly 

    while order id is random so if use LUT table approach that can not be cache friendy as one order is at one place while next order at differ index far away so not a optimal approch