#pragma once

// Some functions here are prepend with 'hoc_' but they are unrelated to hoc itself.
#include <cstddef>

/* check return from malloc */
void* hoc_Emalloc(std::size_t n);
void* hoc_Ecalloc(std::size_t n, std::size_t size);
void* hoc_Erealloc(void* ptr, std::size_t size);

void hoc_malchk(void);

void* emalloc(std::size_t n);
void* ecalloc(std::size_t n, std::size_t size);
void* erealloc(void* ptr, std::size_t size);

void* nrn_cacheline_alloc(void** memptr, std::size_t size);
void* nrn_cacheline_calloc(void** memptr, std::size_t nmemb, std::size_t size);
