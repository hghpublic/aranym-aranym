/* acconfig.h
   This file is in the public domain.

   Descriptive text for the C preprocessor macros that
   the distributed Autoconf macros can define.
   No software package will use all of them; autoheader copies the ones
   your configure.in uses into your configuration header file templates.

   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  Although this order
   can split up related entries, it makes it easier to check whether
   a given entry is in the file.

   Leave the following blank line there!!  Autoheader needs it.  */


/* Define to 'off_t' if <sys/types.h> doesn't define. */
#undef loff_t

/* Define if you have the atanh function. */
#undef HAVE_ATANH

/* Define if using "mon". */
#undef ENABLE_MON

/* Define if using native 68k mode. */
#undef ENABLE_NATIVE_M68K

/* Define if your system support GNU SOURCEs (vasprintf function) */
#undef HAVE_GNU_SOURCE

/* Define if needed */
#undef _XOPEN_SOURCE

/* Define if using some debugger */
#undef DEBUGGER

/* Define if using New Debugger */
#undef NEWDEBUG

/* Define if using full MMU */
#undef FULLMMU

/* Define if using DSP */
#undef DSP_EMULATION

/* Define if DSP disassembly */
#undef DSP_DISASM

/* Define if you want Timer C to be emulated using SDL Timers */
#undef USE_TIMERS

/* Define if you want to try out a different memory check */
#undef ARAM_PAGE_CHECK

/* Define if you want FastRAM of fixed size */
#undef FixedSizeFastRAM

/* Define if you want fixed position of VideoRAM */
#undef FIXED_VIDEORAM

/* Define if you want direct truecolor output */
#undef DIRECT_TRUECOLOR

/* Define if you want accelerated blitter output */
#undef BLITTER_MEMMOVE

/* Define if you want even more accelerated blitter output */
#undef BLITTER_SDLBLIT

/* Define if you want TV conf GUI and have TV lib */
#undef HAVE_TVISION

/* Define if your system has a working vm_allocate()-based memory allocator */
#undef HAVE_MACH_VM

/* Define if your system has a working mmap()-based memory allocator */
#undef HAVE_MMAP_VM

/* Define if <sys/mman.h> defines MAP_ANON and mmap()'ing with MAP_ANON works */
#undef HAVE_MMAP_ANON

/* Define if <sys/mman.h> defines MAP_ANONYMOUS and mmap()'ing with MAP_ANONYMOUS works */
#undef HAVE_MMAP_ANONYMOUS

/* Define if you want to use full history */
#undef FULL_HISTORY

/* Define if you want to use direct access to host's fs */
#undef EXTFS_SUPPORT

/* Define if you want to build SDL GUI */
#undef SDL_GUI

/* Define if <getopt.h> knows getopt_long */
#undef HAVE_GETOPT_H

/* Define if you want to use FreeBSD's imitation of O_SYNC */
#undef HAVE_O_FSYNC

/* Define if you want to use ethernet card emulation */
#undef ETHERNET_SUPPORT

/* Define if you don't want to use check memory boundary */
#undef NOCHECKBOUNDARY

/* Define if you want protect end of memory */
#undef RAMENDNEEDED


/* Leave that blank line there!!  Autoheader needs it.
   If you're adding to this file, keep in mind:
   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  */
