#ifndef swap_endian_h
#define swap_endian_h

#include <algorithm>
#include <iterator>
#include <cstring>
#include <cstddef>
#include <stdint.h>

#ifdef SWAP_ENDIAN_ASSERT
#include <cassert>
#endif

#if !defined(SWAP_ENDIAN_DISABLE_ASM) && (defined(__AVX2__) || defined(__SSSE3__))
#include <immintrin.h>
#endif

#if !defined(SWAP_ENDIAN_MAX_UNROLL)
#define SWAP_ENDIAN_MAX_UNROLL 8
#endif

namespace endian {

    namespace impl {
        template <typename I>
        struct is_pointer { enum { value=false }; };

        template <typename V>
        struct is_pointer<V *> { enum { value=true }; };

        struct not_implemented { static void eval(...); };

        /** Check to ensure a class is not derived from not_implemented */
        template <typename C>
        struct is_implemented {
            typedef char no;
            typedef double yes;
            static no check(not_implemented *);
            static yes check(...);
            enum { value=sizeof(check((C *)0))==sizeof(yes) };
        };


        template <size_t K,size_t Unroll,bool Aligned>
        struct swap_endian_basic: not_implemented {};

        template <size_t K,bool Aligned>
        struct swap_endian_basic<K,1,Aligned> {
            static void eval(unsigned char *d) {
                std::reverse(d,d+K);
            }
        };

        template <size_t K,size_t Unroll,bool Aligned>
        struct swap_endian_fast: not_implemented {};

        /** Reverse bytes within values of fixed size in buffer
         *
         * Basic overload does one item at a time, using std::reverse.
         * Specialisations provide faster implementations for particular
         * item sizes and unrolls.
         *
         * If a _fast version is implemented (e.g., architecture specific)
         * use that in preference.
         *
         * If Align is true, we can assume that d is aligned to a multiple
         * of K*Unroll bytes.
         */
        template <size_t K,size_t Unroll,bool Aligned=false>
        struct swap_endian {
            static void eval(unsigned char *d) {
#ifdef SWAP_ENDIAN_ASSERT
                assert(!Aligned || (uintptr_t)d%(K*Unroll)==0);
#endif
                if (is_implemented<swap_endian_fast<K,Unroll,Aligned> >::value)
                    swap_endian_fast<K,Unroll,Aligned>::eval(d);
                else if (is_implemented<swap_endian_basic<K,Unroll,Aligned> >::value)
                    swap_endian_basic<K,Unroll,Aligned>::eval(d);
                else {
                    swap_endian<K,Unroll/2,Aligned>::eval(d);
                    swap_endian<K,Unroll/2,Aligned>::eval(d+K*(Unroll/2));
                    if (Unroll%1)
                        swap_endian<K,1,Aligned>::eval(d+K*(Unroll-1));
                }
            }
        };

        template <size_t Unroll,bool Aligned>
        struct swap_endian<1,Unroll,Aligned> {
            static void eval(unsigned char *) {}
        };
        
        // specialise swap_endian_basic for integer data sizes

        template <bool Aligned>
        struct swap_endian_basic<2,1,Aligned> {
            static void eval(unsigned char *d) {
                uint16_t v;
                memcpy(&v,d,2);
                v=(uint16_t)((v>>8u)|(v<<8u));
                memcpy(d,&v,2);
            }
        };

        template <bool Aligned>
        struct swap_endian_basic<4,1,Aligned> {
            static void eval(unsigned char *d) {
                uint32_t v;
                memcpy(&v,d,4);
                v=(v>>24) | ((v>>8) & 0x0000ff00ul) | ((v<<8) & 0x00ff0000ul) | (v<<24);
                memcpy(d,&v,4);
            }
        };

        template <bool Aligned>
        struct swap_endian_basic<8,1,Aligned> {
            static void eval(unsigned char *d) {
                uint64_t v;
                memcpy(&v,d,8);
                v=(v>>56) | 
                     ((v<<40) & 0x00FF000000000000ull) |
                     ((v<<24) & 0x0000FF0000000000ull) |
                     ((v<<8)  & 0x000000FF00000000ull) |
                     ((v>>8)  & 0x00000000FF000000ull) |
                     ((v>>24) & 0x0000000000FF0000ull) |
                     ((v>>40) & 0x000000000000FF00ull) |
                     (v<<56);
                memcpy(d,&v,8);
            }
        };

#if !defined(SWAP_ENDIAN_DISABLE_ASM) && defined(__PPC64__) 
        /* generic methods very slow on bgq */

        template <bool Aligned>
        struct swap_endian_fast<4,1,Aligned> {
            static void eval(unsigned char *d) {
                struct chunk_t { unsigned char x[4]; } &u=*(chunk_t *)(void *)d;

                uint32_t v;
                asm("lwz    %[v],%[ldata]         \n\t"
                    "stwbrx %[v],0,%[sdata]       \n\t"
                  : [v]"+&r"(v), "+o"(u)
                  : [ldata]"o"(u), [sdata]"r"(d)
                  : ); 
            }
        };

        template <>
        struct swap_endian_fast<4,2,true> {
            static void eval(unsigned char *d) {
                struct chunk_t { unsigned char x[8]; } &u=*(chunk_t *)(void *)d;

                uint64_t v;
                asm("ld     %[v],%[ldata]         \n\t"
                    "stwbrx %[v],%[word],%[sdata] \n\t"
                    "srd    %[v],%[v],%[shift]    \n\t"
                    "stwbrx %[v],0,%[sdata]       \n\t"
                  : [v]"+&r"(v), "+o"(u)
                  : [ldata]"o"(u), [sdata]"r"(d), [word]"b"(4), [shift]"b"(32)
                  : ); 
            }
        };

        template <>
        struct swap_endian_fast<8,1,true> {
            static void eval(unsigned char *d) {
                struct chunk_t { unsigned char x[8]; } &u=*(chunk_t *)(void *)d;

                uint64_t v;
                asm("ld     %[v],%[ldata]         \n\t"
                    "stwbrx %[v],0,%[sdata]       \n\t"
                    "srd    %[v],%[v],%[shift]    \n\t"
                    "stwbrx %[v],%[word],%[sdata] \n\t"
                  : [v]"+&r"(v), "+o"(u)
                  : [ldata]"o"(u), [sdata]"r"(d), [word]"b"(4), [shift]"b"(32)
                  : ); 
            }
        };
#endif

#if !defined(SWAP_ENDIAN_DISABLE_ASM) && defined(__SSSE3__)
        template <>
        struct swap_endian_fast<2,8,true> {
            static void eval(unsigned char *d) {
                __m128i permute=_mm_setr_epi8(1,0,3,2,5,4,7,6,9,8,11,10,13,12,15,14);
                __m128i x=_mm_load_si128((__m128i *)d);
                x=_mm_shuffle_epi8(x,permute);
                _mm_store_si128((__m128i *)d,x);
            }
        };

        template <>
        struct swap_endian_fast<4,4,true> {
            static void eval(unsigned char *d) {
                __m128i permute=_mm_setr_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12);
                __m128i x=_mm_load_si128((__m128i *)d);
                x=_mm_shuffle_epi8(x,permute);
                _mm_store_si128((__m128i *)d,x);
            }
        };

        template <>
        struct swap_endian_fast<8,2,true> {
            static void eval(unsigned char *d) {
                __m128i permute=_mm_setr_epi8(7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8);
                __m128i x=_mm_load_si128((__m128i *)d);
                x=_mm_shuffle_epi8(x,permute);
                _mm_store_si128((__m128i *)d,x);
            }
        };
#endif

#if !defined(SWAP_ENDIAN_DISABLE_ASM) && defined(__AVX2__)
        // Modern implementations suffer little or no penalty from unaligned load.
        template <bool Aligned>
        struct swap_endian_fast<4,8,Aligned> {
            static void eval(unsigned char *d) {
                __m256i permute=_mm256_setr_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12,
                                                 19,18,17,16,23,22,21,20,27,26,25,24,31,30,29,28);
                __m256i x=_mm256_loadu_si256((__m256i *)d);
                x=_mm256_shuffle_epi8(x,permute);
                _mm256_storeu_si256((__m256i *)d,x);
            }
        };

        template <bool Aligned>
        struct swap_endian_fast<8,4,Aligned> {
            static void eval(unsigned char *d) {
                __m256i permute=_mm256_setr_epi8(7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,
                                                 23,22,21,20,19,18,17,16,31,30,29,28,27,26,25,24);
                __m256i x=_mm256_loadu_si256((__m256i *)d);
                x=_mm256_shuffle_epi8(x,permute);
                _mm256_storeu_si256((__m256i *)d,x);
            }
        };

#endif

        template <typename V>
        void swap_endian_unroll(V *b,V *e) {
            static const size_t n_unroll=SWAP_ENDIAN_MAX_UNROLL;
            if (e<=b) return;

            size_t n=e-b;
            bool aligned_vsize=((uintptr_t)b%sizeof(V)==0);

            if (n>=n_unroll) {
                if (!aligned_vsize) {
                    /* No guarantees on alignment for swap_endian: elements
                       are not aligned to multiple of the element size.
                       This can happen with doubles on some 32-bit ABIs. */

                    while (n>=n_unroll) {
                        swap_endian<sizeof(V),n_unroll,false>::eval((unsigned char *)b);
                        b+=n_unroll;
                        n-=n_unroll;
                    }
                }
                else {
                    /* process elements singly until we get to a n_unroll*sizeof(V)
                       boundary, and then do an alignment-guaranteed swap_endian. */

                    size_t off_align_count=(size_t)((uintptr_t)b%(sizeof(V)*n_unroll))/sizeof(V);
                    if (off_align_count>0) {
                        while (off_align_count++<n_unroll) {
                            swap_endian<sizeof(V),1,true>::eval((unsigned char *)b++);
                            --n;
                        }
                    }
                    
                    /* b should now be n_unroll*sizeof(V) aligned. */
#ifdef SWAP_ENDIAN_ASSERT
                    assert((uintptr_t)b%(sizeof(V)*n_unroll)==0);
#endif
                    while (n>=n_unroll) {
                        swap_endian<sizeof(V),n_unroll,true>::eval((unsigned char *)b);
                        b+=n_unroll;
                        n-=n_unroll;
                    }
                }
            }

            if (aligned_vsize) {
                while (n-->0) swap_endian<sizeof(V),1,true>::eval((unsigned char *)b++);
            }
            else {
                while (n-->0) swap_endian<sizeof(V),1,false>::eval((unsigned char *)b++);
            }
        }
    }

    /** Reverse the endianness of a value in-place.
     *
     * /tparam T value type
     * /param v value to byte-reorder
     */
    template <typename T>
    T &swap_endian(T &v) {
        impl::swap_endian<sizeof(T),1>::eval((unsigned char *)&v);
        return v;
    }

    /** Reverse the endianness of the values in the given range.
     *
     * /tparam I iterator type
     * /param b iterator representing the beginning of the range.
     * /param e iterator representing the end of the range.
     *
     * All values in the iterator range [b,e) are byte-reversed.
     */
    template <typename I>
    void swap_endian_range(I b,I e) {
        if (impl::is_pointer<I>::value)
            impl::swap_endian_unroll(b,e);
        else {
            while (b!=e) swap_endian(*b++);
        }
    }

} // namespace endian

#endif // ifndef swap_endian_h
