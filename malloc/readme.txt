Readme:
For the newest test.cpp it exceedes the limitation of the memory location 681MB, but it will work for around 950MB.
My default chunk_size is set to 128MB
When mallocing, the program will firstly search for according freelist that matches the size, then if not exist, it will search for next bigger block and then split, if still not it will call mmap and then split

Yang Chen
ychen207@binghamton.edu