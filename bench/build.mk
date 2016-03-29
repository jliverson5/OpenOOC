bench_PROGRAMS := dram longjmp micro siglongjmp sigsegv swapcontext

dram_SOURCES := dram.c

longjmp_SOURCES := longjmp.c

micro_SOURCES  := impl/io.c impl/libc.c impl/sbma.c micro.c
micro_HEADERS  := impl/impl.h

siglongjmp_SOURCES := siglongjmp.c

sigsegv_SOURCES := sigsegv.c

swapcontext_SOURCES := swapcontext.c
