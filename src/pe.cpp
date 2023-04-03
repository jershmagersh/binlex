#include "pe.h"

using namespace binlex;
using namespace LIEF::PE;

PE::PE(){
    total_exec_sections = 0;
    for (int i = 0; i < BINARY_MAX_SECTIONS; i++){
        sections[i].offset = 0;
        sections[i].size = 0;
        sections[i].data = NULL;
    }
}

bool PE::ReadVector(const std::vector<uint8_t> &data){
    if (binary = Parser::parse(data)){
        binary_type = BINARY_TYPE_PE;
        if (binary_arch == BINARY_ARCH_UNKNOWN ||
            binary_mode == BINARY_MODE_UNKNOWN){
            if (IsDotNet() == true){
                binary_arch = BINARY_ARCH_X86;
                binary_mode = BINARY_MODE_CIL;
            } else {
                switch(binary->header().machine()){
                    case MACHINE_TYPES::IMAGE_FILE_MACHINE_I386:
                        SetArchitecture(BINARY_ARCH_X86, BINARY_MODE_32);
                        g_args.options.mode = "pe:x86";
                        break;
                    case MACHINE_TYPES::IMAGE_FILE_MACHINE_AMD64:
                        SetArchitecture(BINARY_ARCH_X86, BINARY_MODE_64);
                        g_args.options.mode = "pe:x86_64";
                        break;
                    default:
                        binary_arch = BINARY_ARCH_UNKNOWN;
                        binary_mode = BINARY_MODE_UNKNOWN;
                        return false;
                }
            }
        }
        CalculateFileHashes(data);
        return ParseSections();
    }
    return false;
}

bool PE::IsDotNet(){
    try {
        auto imports = binary->imports();
        for(Import i : imports) {
            if (i.name() == "mscorelib.dll") {
                if(binary->data_directory(DATA_DIRECTORY::CLR_RUNTIME_HEADER).RVA() > 0) {
                    PRINT_DEBUG("Detected .NET\n");
                    return true;
                }
            }
            if (i.name() == "mscoree.dll") {
                if(binary->data_directory(DATA_DIRECTORY::CLR_RUNTIME_HEADER).RVA() > 0) {
                    PRINT_DEBUG("Detected .NET\n");
                    return true;
                }
            }
        }
        PRINT_DEBUG("Did not detect .NET\n");
        return false;
    } catch(LIEF::bad_format const&) {
        PRINT_DEBUG("An error occurred while trying to detect .NET.\n")
        return false;
    }
}

bool PE::HasLimitations(){
    if(binary->has_imports()){
        auto imports = binary->imports();
        for(Import i : imports){
            if(i.name() == "MSVBVM60.DLL"){
                return true;
            }
        }
    }
    return false;
}

bool PE::ParseSections(){
    uint32_t index = 0;
    Binary::it_sections local_sections = binary->sections();
    for (auto it = local_sections.begin(); it != local_sections.end(); it++){
        if (it->characteristics() & (uint32_t)SECTION_CHARACTERISTICS::IMAGE_SCN_MEM_EXECUTE){
            vector<uint8_t> data = binary->get_content_from_virtual_address(it->virtual_address(), it->sizeof_raw_data());
            if (data.size() == 0) {
                continue;
            }
            sections[index].offset = it->offset();
            sections[index].size = it->sizeof_raw_data();
            sections[index].data = malloc(sections[index].size);
            memset(sections[index].data, 0, sections[index].size);
            memcpy(sections[index].data, &data[0], sections[index].size);
            // Add exports to the function list
            if (binary->has_exports()){
                Export exports = binary->get_export();
                Export::it_entries export_entries = exports.entries();
                for (auto j = export_entries.begin(); j != export_entries.end(); j++){
                    PRINT_DEBUG("PE Export offset: 0x%x\n", (int)binary->rva_to_offset(j->address()));
                    uint64_t tmp_offset = binary->rva_to_offset(j->address());
                    if (tmp_offset > sections[index].offset &&
                        tmp_offset < sections[index].offset + sections[index].size){
                        sections[index].functions.insert(tmp_offset-sections[index].offset);
                    }
                }
            }
            // Add entrypoint to the function list
            uint64_t entrypoint_offset = binary->va_to_offset(binary->entrypoint());
            PRINT_DEBUG("PE Entrypoint offset: 0x%x\n", (int)entrypoint_offset);
            if (entrypoint_offset > sections[index].offset && entrypoint_offset < sections[index].offset + sections[index].size){
                sections[index].functions.insert(entrypoint_offset-sections[index].offset);
            }
            index++;
            if (BINARY_MAX_SECTIONS == index){
                fprintf(stderr, "[x] malformed binary, too many executable sections\n");
                return false;
            }
        }
    }
    total_exec_sections = index + 1;
    return true;
}

PE::~PE(){
    for (uint32_t i = 0; i < total_exec_sections; i++){
        sections[i].offset = 0;
        sections[i].size = 0;
        free(sections[i].data);
        sections[i].functions.clear();
    }
}
