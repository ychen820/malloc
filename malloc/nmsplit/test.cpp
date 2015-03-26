#include <cstddef>
#include <vector>
#include <sys/mman.h>
#include <sys/time.h>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <random>
#include <locale.h>
#include <locale>
#include <iostream>
#  define MAP_ANONYMOUS MAP_ANON
using namespace std;



size_t current_mmapped, max_mmapped, total_mmap_calls;
size_t current_allocated;


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
extern "C" int
cs550_munmap_wrapper(void *vp, size_t sz) {
    int rv;
    #ifndef CS550_DEBUG
        rv = munmap(vp, sz);
    #else
        free(vp);
        rv = 0;
    #endif
    current_mmapped -= sz;
    return rv;
}
#pragma GCC diagnostic pop

extern "C" void cs550_set_chunk_size(size_t);

// From http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static inline uint64_t
round_up_pow2(uint64_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
}

#include "utility.h"

#ifdef CS550_DEBUG
    extern "C" {
        // These will have been redefined to have different names.
        void *malloc(const size_t request_sz);
        void *calloc(size_t nmemb, size_t size);
        void free(void *vp);
        void *realloc(void *vp, size_t sz);
    }
#endif

extern "C" size_t iteration_index = 0;
extern "C" void *
cs550_mmap_wrapper(size_t sz) {
    #ifndef CS550_DEBUG
        void *vp = mmap(0, sz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    #else
        void *vp = malloc(sz);
    #endif
    current_mmapped += sz;
    max_mmapped = max(max_mmapped, current_mmapped);
    total_mmap_calls++;
     cs550_print("Mmap called for %lu, total allocated is %lu. at index%lu\n", sz, current_allocated,iteration_index);
    return vp;
}

class BlockInfo {
    public:
        BlockInfo() : m_ptr(nullptr), m_size(0), m_value(0) {}
        ~BlockInfo() { free(); }
        explicit operator bool() const { return m_ptr != nullptr; }
        void malloc(size_t max_size);
        void realloc(size_t max_size);
        void calloc(size_t max_size);
        void free();
        bool check() const { return check(m_ptr, m_value, m_size); }
        void set();
    private:
        static size_t rand(size_t max) {
            return uniform_int_distribution<size_t>(1, max)(rand_eng);
        }
       static bool check(const void *const begin, const int value, const size_t sz);
    private:
        void *m_ptr;
        size_t m_size;
        int m_value;
    private:
        static default_random_engine rand_eng;
};
default_random_engine BlockInfo::rand_eng;

void
BlockInfo::malloc(size_t max_size) {
    assert(m_ptr == nullptr);
    m_size = rand(max_size);
    m_ptr = ::malloc(m_size);
    set();
    current_allocated += m_size;
}

void
BlockInfo::realloc(size_t max_size) {
    size_t new_sz = rand(max_size);
    m_ptr = ::realloc(m_ptr, new_sz);
    check(m_ptr, m_value, min(m_size, new_sz));
    current_allocated += new_sz - m_size;
    m_size = new_sz;
    // fprintf(stderr,  "memset at 0x%p for %d.\n", m_ptr, size);
    set();
}
void
BlockInfo::calloc(size_t max_size) {
    const size_t n_elems = rand(max_size), elem_sz = rand(max_size/n_elems);
    m_size = n_elems*elem_sz;
    assert(m_size > 0 && m_size <= max_size);
    m_ptr = ::calloc(n_elems, elem_sz);
    set();
    current_allocated += m_size;
}
void
BlockInfo::free() {
    ::free(m_ptr);
    m_ptr = nullptr;
    current_allocated -= m_size;
}
void
BlockInfo::set() {
    m_value = uniform_int_distribution<int>(0, 100)(rand_eng);
    memset(m_ptr, m_value, m_size);
}

bool
BlockInfo::check(const void *const begin, const int value, const size_t sz) {
    if (begin != nullptr) {
        for (const char *p = (const char *) begin, *const end = p + sz; p < end; p++) {
            if (*p != value) {
                fprintf(stderr,
                 "Byte at position %'td in block at address 0x%p incorrect in iteration %'zu,\n"
                 "    value is %'d but should have been %'d.\n",
                 end - p,  begin, iteration_index, *p, value);
                return false;
            }
        }
    }
    return true;
}

class CommaNumPunct : public numpunct<char> {
    protected:
        virtual char do_thousands_sep() const {
            return ',';
        }

        virtual string do_grouping() const {
            return "\03";
        }
};



int
main(int argc, char **argv) {

    // This creates a new locale based on the current application default
    // (which is either the one given on startup, but can be overriden with
    // locale::global) - then extends it with an extra facet that
    // controls numeric output.
    locale comma_locale(locale(), new CommaNumPunct());

    // Use our new locale.
    cout.imbue(comma_locale);
    cerr.imbue(comma_locale);

    setlocale(LC_NUMERIC, "en_US.UTF-8");

    int rv;

    if (argc < 4) {
        printf("Usage: ./a.out n_blocks max_block_size n_iterations\n");
        printf("    n_blocks is the maximum number of blocks that will be allocated at any one time.\n");
        printf("    max_block_size is the maximum allocation size.\n");
        printf("    n_iterations is the number of iterations.\n");
        exit(1);
    }

    size_t n_blocks = atoi(argv[1]), max_block_size = atoi(argv[2]), n_iterations = atoi(argv[3]);
    printf("Doing %'zu iterations, while allocating up to %'zu blocks of up to %'zu bytes each.\n",
     n_iterations, n_blocks, max_block_size);

    vector<BlockInfo> blocks(n_blocks);
    default_random_engine eng;
    eng.seed(12345);

    // Use a smaller initial chunk size than what was specified in the assignment.
    cs550_set_chunk_size(16*1024*1024);
    struct timeval start, end;
    rv = gettimeofday(&start, NULL); assert(rv == 0);
    // Note that iteration_index is a global variable.
    for (iteration_index = 0; iteration_index < n_iterations; iteration_index++) {

        BlockInfo &bi(blocks.at(rand()%blocks.size()));

        // 1 out of 1000 times change the chunk size.
        if (uniform_real_distribution<float>()(eng) < .001) {
            cs550_set_chunk_size(round_up_pow2(uniform_int_distribution<size_t>(10000, 16*1024*1024)(eng)));
        }

        if (bi) {
            bi.check();
            if (rand()%2) {
                bi.free();
            } else {
                bi.realloc(max_block_size);
            }
        } else {
            if (rand()%2) {
               bi.malloc(max_block_size);
            } else {
              bi.calloc(max_block_size);
            }
        }
    }
    // This indicates that the iterations are done.
    iteration_index = numeric_limits<size_t>::max();

    rv = gettimeofday(&end, NULL); assert(rv == 0);
    double interval = (end.tv_sec + end.tv_usec/1000000.0) - (start.tv_sec + start.tv_usec/1000000.0);
    printf("Execution time is %'f seconds.\n", interval);
    printf("Mmapped memory (or malloc'ed, if compiled with CS550_DEBUG defined) at exit is %'zu.\n", current_mmapped);
    printf("Maximum mmapped memory at any point during the execution is %'zu.\n", max_mmapped);
    printf("Number of mmap() calls is %'zu.\n", total_mmap_calls);
}
