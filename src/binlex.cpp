#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <capstone/capstone.h>
#if defined(__linux__) || defined(__APPLE__)
#include <sys/time.h>
#include <signal.h>
#elif _WIN32
#include <windows.h>
#endif
#include "args.h"
#include "pe.h"
#include "raw.h"
#include "cil.h"
#include "pe-dotnet.h"
#include "blelf.h"
#include "auto.h"
#include "disassembler.h"

#ifdef _WIN32
#pragma comment(lib, "capstone")
#pragma comment(lib, "binlex")
#endif
using namespace binlex;

void timeout_handler(int signum) {
    (void)signum;
    fprintf(stderr, "[x] execution timeout\n");
    exit(0);
}

#if defined(__linux__) || defined(__APPLE__)
void start_timeout(time_t seconds){
    struct itimerval timer;
    timer.it_value.tv_sec = seconds;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer (ITIMER_VIRTUAL, &timer, 0);
    struct sigaction sa;
    memset(&sa, 0, sizeof (sa));
    sa.sa_handler = &timeout_handler;
    sigaction(SIGVTALRM, &sa, 0);
}
#endif

int main(int argc, char **argv){
    g_args.parse(argc, argv);
    if (g_args.options.debug == true){
        LIEF::logging::set_level(LIEF::logging::LOGGING_LEVEL::LOG_DEBUG);
    } else {
        LIEF::logging::disable();
    }
    if (g_args.options.timeout > 0){
        #if defined(__linux__) || defined(__APPLE__)
        start_timeout(g_args.options.timeout);
        #endif
    }
    if (g_args.options.mode.empty() == true){
        g_args.print_help();
        return EXIT_FAILURE;
    }
    if (g_args.options.mode == "auto" &&
        g_args.options.io_type == ARGS_IO_TYPE_FILE){
        AutoLex autolex;
        return autolex.ProcessFile(g_args.options.input);
        return 0;
    }
    if (g_args.options.mode == "elf:x86_64" &&
        g_args.options.io_type == ARGS_IO_TYPE_FILE){
        ELF elf64;
        elf64.SetArchitecture(BINARY_ARCH_X86_64, BINARY_MODE_64);
        if (elf64.ReadFile(g_args.options.input) == false){
            return EXIT_FAILURE;
        }
        PRINT_DEBUG("[binlex.cpp] number of total executable sections = %u\n", elf64.total_exec_sections);
        Disassembler disassembler(elf64);
        disassembler.Disassemble();
        disassembler.WriteTraits();
        return EXIT_SUCCESS;
    }
    if (g_args.options.mode == "elf:x86" &&
        g_args.options.io_type == ARGS_IO_TYPE_FILE){
        ELF elf32;
        elf32.SetArchitecture(BINARY_ARCH_X86, BINARY_MODE_32);
        if (elf32.ReadFile(g_args.options.input) == false){
            return EXIT_FAILURE;
        }
        PRINT_DEBUG("[binlex.cpp] number of total executable sections = %u\n", elf32.total_exec_sections);
        Disassembler disassembler(elf32);
        disassembler.Disassemble();
        disassembler.WriteTraits();
        return EXIT_SUCCESS;
    }
    if (g_args.options.mode == "pe:cil" &&
        g_args.options.io_type == ARGS_IO_TYPE_FILE){
        // TODO: This should be valid for both x86-86 and x86-64
        // we need to do this more generic
        DOTNET pe;
        pe.SetArchitecture(BINARY_ARCH_X86, BINARY_MODE_CIL);
        if (pe.ReadFile(g_args.options.input) == false) return 1;
        CILDisassembler disassembler(pe);
        PRINT_DEBUG("[binlex.cpp] analyzing %lu sections for CIL byte code.\n", pe._sections.size());
        int si = 0;
        for (auto section : pe._sections) {
            if (section.offset == 0) continue;
            if (disassembler.Disassemble(section.data, section.size, si) == false){
                    continue;
            }
            si++;
        }
        disassembler.WriteTraits();
        return EXIT_SUCCESS;
    }
    if (g_args.options.mode == "pe:cil64" &&
        g_args.options.io_type == ARGS_IO_TYPE_FILE){
        // TODO: This should be valid for both x86-86 and x86-64
        // we need to do this more generic
        DOTNET pe64;
        pe64.SetArchitecture(BINARY_ARCH_X86_64, BINARY_MODE_CIL);
        if (pe64.ReadFile(g_args.options.input) == false) return 1;
        CILDisassembler disassembler(pe64);
        PRINT_DEBUG("[binlex.cpp] analyzing %lu sections for CIL byte code.\n", pe64._sections.size());
        int si = 0;
        for (auto section : pe64._sections) {
            if (section.offset == 0) continue;
            if (disassembler.Disassemble(section.data, section.size, si) == false){
                    continue;
            }
            si++;
        }
        disassembler.WriteTraits();
        return EXIT_SUCCESS;
    }
    if (g_args.options.mode == "pe:x86" &&
        g_args.options.io_type == ARGS_IO_TYPE_FILE){
        PE pe32;
        pe32.SetArchitecture(BINARY_ARCH_X86, BINARY_MODE_32);
        if (pe32.ReadFile(g_args.options.input) == false){
            return EXIT_FAILURE;
        }
        PRINT_DEBUG("[binlex.cpp] number of total sections = %u\n", pe32.total_exec_sections);
        Disassembler disassembler(pe32);
        disassembler.Disassemble();
        disassembler.WriteTraits();
        return EXIT_SUCCESS;
    }
    if (g_args.options.mode == "pe:x86_64" &&
        g_args.options.io_type == ARGS_IO_TYPE_FILE){
        PE pe64;
        pe64.SetArchitecture(BINARY_ARCH_X86_64, BINARY_MODE_64);
        if (pe64.ReadFile(g_args.options.input) == false){
            return EXIT_FAILURE;
        }
        PRINT_DEBUG("[binlex.cpp] number of total executable sections = %u\n", pe64.total_exec_sections);
        Disassembler disassembler(pe64);
        disassembler.Disassemble();
        disassembler.WriteTraits();
        return EXIT_SUCCESS;
    }
    if (g_args.options.mode == "raw:x86" &&
        g_args.options.io_type == ARGS_IO_TYPE_FILE){
        Raw rawx86;
        rawx86.SetArchitecture(BINARY_ARCH_X86, BINARY_MODE_32);
        if (rawx86.ReadFile(g_args.options.input) == false){
            return EXIT_FAILURE;
        }
        Disassembler disassembler(rawx86);
        disassembler.Disassemble();
        disassembler.WriteTraits();
        return EXIT_SUCCESS;
    }
    if (g_args.options.mode == "raw:x86_64" &&
        g_args.options.io_type == ARGS_IO_TYPE_FILE){
        Raw rawx86_64;
        rawx86_64.SetArchitecture(BINARY_ARCH_X86_64, BINARY_MODE_64);
        if (rawx86_64.ReadFile(g_args.options.input) == false){
            return EXIT_FAILURE;
        }
        Disassembler disassembler(rawx86_64);
        disassembler.Disassemble();
        disassembler.WriteTraits();
        return EXIT_SUCCESS;
    }
    if (g_args.options.mode == "raw:cil" &&
        g_args.options.io_type == ARGS_IO_TYPE_FILE){
        Raw rawcil;
        rawcil.SetArchitecture(BINARY_ARCH_X86, BINARY_MODE_CIL);
        if (rawcil.ReadFile(g_args.options.input) == false){
            return EXIT_FAILURE;
        }
        CILDisassembler disassembler(rawcil);
        disassembler.Disassemble(rawcil.sections[0].data, rawcil.sections[0].size, 0);
        disassembler.WriteTraits();
        return EXIT_SUCCESS;
    }

    g_args.print_help();
    return EXIT_FAILURE;
}
