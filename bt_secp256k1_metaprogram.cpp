#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_SCOPE static

FILE_SCOPE char *ReadEntireFile(char const *file_path, int *file_size)
{
    char *result      = nullptr;
    FILE *file_handle = fopen(file_path, "rb");
    if (file_handle)
    {
        fseek(file_handle, 0, SEEK_END);
        *file_size = ftell(file_handle);
        rewind(file_handle);

        result             = (char *)malloc(*file_size + 1);
        result[*file_size] = 0;
        if (result)
        {
            if (fread(result, (size_t)(*file_size), 1, file_handle) != 1)
            {
                free(result);
                result = nullptr;
            }
        }
        fclose(file_handle);
    }

    return result;
}

enum FileFlag
{
    FileFlag_Normal        = 1 << 0,
    FileFlag_Mul64Support  = 1 << 1,
    FileFlag_Mul128Support = 1 << 2,
    FileFlag_AsmX86_64     = 1 << 3,
    FileFlag_AsmInt128     = 1 << 4,
};

struct String     { char *str; int size; };
struct StringList { String string; StringList *next; };
#define STRING(string) {string, sizeof(string) - 1}

struct LoadedFile
{
    unsigned  flag;              // Corresponds to FileFlag enum
    String    buffer;            // The contents of the file loaded from disk
    String    sub_dir;           // The sub directory that the file orginated from
    String    name;              // The file name
    String    module_macro_name; // For secp256k1 modules, the name of the C macro used to identify the file
};

FILE_SCOPE bool LoadFiles(LoadedFile *files, int file_count, char const *root_dir)
{
    bool result = true;
    for (int file_index = 0; file_index < file_count; file_index++)
    {
        LoadedFile *file = files + file_index;
        char file_path[2048];
        if (file->sub_dir.size)
        {
            snprintf(file_path, sizeof(file_path), "%s/%.*s/%.*s", root_dir, file->sub_dir.size, file->sub_dir.str, file->name.size, file->name.str);
        }
        else
        {
            snprintf(file_path, sizeof(file_path), "%s/%.*s", root_dir, file->name.size, file->name.str);
        }

        file->buffer.str = ReadEntireFile(file_path, &file->buffer.size);
        if (!file->buffer.str || file->buffer.size == 0)
        {
            fprintf(stderr, "Failed to load file [file=%s]\n", file_path);
            result = false;
        }
    }

    return result;
}

FILE_SCOPE String StringCopy(String src)
{
    String result = String{(char *)malloc(src.size + 1), src.size};
    memcpy(result.str, src.str, src.size);
    result.str[src.size] = 0;
    return result;
}

FILE_SCOPE StringList *StringListAppendRef(StringList *list, String string)
{
    StringList *result = list;
    if (string.str && string.size)
    {
        if (list->string.str)
        {
            list->next = (StringList *)malloc(sizeof(StringList));
            result     = list->next;
            *result    = {};
        }
        result->string = string;
    }
    return result;
}

FILE_SCOPE StringList *StringListAppendCopy(StringList *list, String string)
{
    StringList *result = list;
    if (string.str && string.size)
    {
        String copy = StringCopy(string);
        result      = StringListAppendRef(result, copy);
    }
    return result;
}

FILE_SCOPE String StringListBuild(StringList *list)
{
    int total_size = 0;
    for (StringList *entry = list; entry; entry = entry->next)
        total_size += entry->string.size;

    char *string = (char *)malloc(total_size + 1);
    char *writer = string;
    for (StringList *entry = list; entry; entry = entry->next)
    {
        memcpy(writer, entry->string.str, entry->string.size);
        writer += entry->string.size;
    }

    writer[0] = 0;

    String result = {string, total_size};
    return result;
}

FILE_SCOPE bool StringEquals(String lhs, String rhs)
{
    bool result = lhs.size == rhs.size && (memcmp(lhs.str, rhs.str, lhs.size) == 0);
    return result;
}

FILE_SCOPE String StringFindThenInsert(String src, String find, String insert)
{
    // NOTE: We leak the string list memory but don't care because this is
    // a meta tool that lives for a couple of seconds and kills itself.

    StringList  list_head = {};
    StringList *list_tail = &list_head;

    int        src_end     = src.size - find.size;
    int        head        = 0;
    for (int tail = head; tail <= src_end; tail++)
    {
        String check = String{src.str + tail, find.size};
        if (!StringEquals(check, find))
            continue;

        String range = String{src.str + head, (tail - head)};
        list_tail    = StringListAppendRef(list_tail, range);
        list_tail    = StringListAppendRef(list_tail, insert);
        list_tail    = StringListAppendRef(list_tail, check);
        head         = tail + find.size;
        tail         = head - 1; ; // NOTE: -1 because for loop post increment
    }

    String result = {};
    if (list_head.string.size == 0)
    {
        // NOTE: No insertment possible, so we just do a full-copy
        result = StringCopy(src);
    }
    else
    {
        String remainder = String{src.str + head, src.size - head};
        list_tail        = StringListAppendRef(list_tail, remainder);
        result           = StringListBuild(&list_head);
    }

    return result;
}

FILE_SCOPE String StringReplace(String src, String find, String replace)
{
    // NOTE: We leak the string list memory but don't care because this is
    // a meta tool that lives for a couple of seconds and kills itself.

    StringList  list_head = {};
    StringList *list_tail = &list_head;

    int        src_end     = src.size - find.size;
    int        head        = 0;
    for (int tail = head; tail <= src_end; tail++)
    {
        String check = String{src.str + tail, find.size};
        if (!StringEquals(check, find))
            continue;

        String range = String{src.str + head, (tail - head)};
        list_tail    = StringListAppendRef(list_tail, range);
        list_tail    = StringListAppendRef(list_tail, replace);
        head         = tail + find.size;
        tail         = head - 1; ; // NOTE: -1 because for loop post increment
    }

    String result = {};
    if (list_head.string.size == 0)
    {
        // NOTE: No insertment possible, so we just do a full-copy
        result = StringCopy(src);
    }
    else
    {
        String remainder = String{src.str + head, src.size - head};
        list_tail        = StringListAppendRef(list_tail, remainder);
        result           = StringListBuild(&list_head);
    }

    return result;
}

StringList *AddSourceFileComment(StringList *tail, LoadedFile const *file)
{
    StringList *result = StringListAppendRef(tail, STRING(
    "// -----------------------------------------------------------------------------\n"
    "// File: "
    ));
    if (file->sub_dir.size)
    {
        result = StringListAppendRef(result, file->sub_dir);
        result = StringListAppendRef(result, STRING("/"));
    }

    result = StringListAppendRef(result, file->name);
    result = StringListAppendRef(result, STRING("\n"));
    result = StringListAppendRef(result, STRING("// -----------------------------------------------------------------------------\n"));

    return result;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: bt_secp256k1_metaprogram <path/to/secp256k1/directory>\n");
        return -1;
    }

    // NOTE: Load files into memory
    LoadedFile intro_files[] = {
        {FileFlag_Normal, nullptr, 0, STRING(""), STRING("COPYING"), STRING("")},
    };

    LoadedFile header_files[] = {
        {FileFlag_Normal, nullptr, 0, STRING("include"), STRING("secp256k1.h"), STRING("")},
        {FileFlag_Normal, nullptr, 0, STRING("include"), STRING("secp256k1_preallocated.h"), STRING("")},
    };

    LoadedFile extra_module_header_files[] = {
        {FileFlag_Normal, nullptr, 0, STRING("include"), STRING("secp256k1_ecdh.h"), STRING("ECDH")},
        {FileFlag_Normal, nullptr, 0, STRING("include"), STRING("secp256k1_recovery.h"), STRING("RECOVERY")},
        {FileFlag_Normal, nullptr, 0, STRING("include"), STRING("secp256k1_extrakeys.h"), STRING("EXTRAKEYS")},
        {FileFlag_Normal, nullptr, 0, STRING("include"), STRING("secp256k1_schnorrsig.h"), STRING("SCHNORRSIG")},
    };

    LoadedFile private_header_files[] = {
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("util.h"), STRING("")},
        {FileFlag_Mul64Support,  nullptr, 0, STRING("src"), STRING("field_10x26.h"), STRING("")},
        {FileFlag_Mul64Support,  nullptr, 0, STRING("src"), STRING("scalar_8x32.h"), STRING("")},
        {FileFlag_Mul128Support, nullptr, 0, STRING("src"), STRING("field_5x52.h"), STRING("")},
        {FileFlag_Mul128Support, nullptr, 0, STRING("src"), STRING("scalar_4x64.h"), STRING("")},
        {FileFlag_Mul128Support, nullptr, 0, STRING("src"), STRING("modinv64.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("hash.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("modinv32.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("assumptions.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("scalar.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("field.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("group.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("ecmult_const.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("ecmult_gen.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("scratch.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("ecmult.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("precomputed_ecmult.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("precomputed_ecmult_gen.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("ecdsa.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("eckey.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("selftest.h"), STRING("")},
    };

    LoadedFile private_impl_files[] = {
        {FileFlag_Mul64Support,  nullptr, 0, STRING("src"), STRING("field_10x26_impl.h"), STRING("")},
        {FileFlag_Mul64Support,  nullptr, 0, STRING("src"), STRING("scalar_8x32_impl.h"), STRING("")},
        {FileFlag_AsmInt128,     nullptr, 0, STRING("src"), STRING("field_5x52_int128_impl.h"), STRING("")},
        {FileFlag_AsmX86_64,     nullptr, 0, STRING("src"), STRING("field_5x52_asm_impl.h"), STRING("")},
        {FileFlag_Mul128Support, nullptr, 0, STRING("src"), STRING("field_5x52_impl.h"), STRING("")},
        {FileFlag_Mul128Support, nullptr, 0, STRING("src"), STRING("scalar_4x64_impl.h"), STRING("")},
        {FileFlag_Mul128Support, nullptr, 0, STRING("src"), STRING("modinv64_impl.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("ecdsa_impl.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("eckey_impl.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("group_impl.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("field_impl.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("scalar_impl.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("ecmult_impl.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("ecmult_const_impl.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("ecmult_gen_impl.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("precomputed_ecmult.c"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("precomputed_ecmult_gen.c"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("hash_impl.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("modinv32_impl.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("scratch_impl.h"), STRING("")},
        {FileFlag_Normal,        nullptr, 0, STRING("src"), STRING("secp256k1.c"), STRING("")},
    };

    LoadedFile private_extra_module_impl_files[] = {
        {FileFlag_Normal, nullptr, 0, STRING("src/modules/ecdh"), STRING("main_impl.h"), STRING("ECDH")},
        {FileFlag_Normal, nullptr, 0, STRING("src/modules/recovery"), STRING("main_impl.h"), STRING("RECOVERY")},
        {FileFlag_Normal, nullptr, 0, STRING("src/modules/extrakeys"), STRING("main_impl.h"), STRING("EXTRAKEYS")},
        {FileFlag_Normal, nullptr, 0, STRING("src/modules/schnorrsig"), STRING("main_impl.h"), STRING("SCHNORRSIG")},
    };

    #define ARRAY_COUNT(array) sizeof(array)/sizeof((array)[0])
    char const *root_dir     = argv[1];
    bool        files_loaded = true;
    files_loaded &= LoadFiles(intro_files, ARRAY_COUNT(intro_files), root_dir);
    files_loaded &= LoadFiles(header_files, ARRAY_COUNT(header_files), root_dir);
    files_loaded &= LoadFiles(extra_module_header_files, ARRAY_COUNT(extra_module_header_files), root_dir);
    files_loaded &= LoadFiles(private_header_files, ARRAY_COUNT(private_header_files), root_dir);
    files_loaded &= LoadFiles(private_impl_files, ARRAY_COUNT(private_impl_files), root_dir);
    files_loaded &= LoadFiles(private_extra_module_impl_files, ARRAY_COUNT(private_extra_module_impl_files), root_dir);

    if (!files_loaded)
        return -1;

    // NOTE: Patch problematic source code before code generation
    {
        for (LoadedFile &file : private_header_files)
        {
            if (StringEquals(file.name, STRING("util.h")))
            {
                // NOTE: Fix implicit cast from void to unsigned char C++ error
                file.buffer = StringReplace(
                    file.buffer,
                    STRING("const unsigned char *p1 = s1, *p2 = s2;"),
                    STRING("const unsigned char *p1 = (const unsigned char *)s1, *p2 = (const unsigned char *)s2;"));
                break;
            }
        }

        for (LoadedFile &file : private_impl_files)
        {
            if (StringEquals(file.name, STRING("secp256k1.c")))
            {
                // NOTE: Patch out module includes, we insert them manually after
                // the main implementation file
                file.buffer = StringFindThenInsert(file.buffer, STRING("# include \"modules"), STRING("// "));
                break;
            }
        }

        for (LoadedFile &file : private_extra_module_impl_files)
        {
            // NOTE: modules all use relative headers, so patch them out early.
            file.buffer = StringFindThenInsert(file.buffer, STRING("#include"), STRING("// "));

            if (StringEquals(file.sub_dir, STRING("src/modules/schnorrsig")) && StringEquals(file.name, STRING("main_impl.h")))
            {
                // NOTE: Fix implicit cast from void to unsigned char C++ error
                file.buffer = StringReplace(file.buffer,
                                            STRING("secp256k1_sha256_write(&sha, data, 32)"),
                                            STRING("secp256k1_sha256_write(&sha, (const unsigned char *)data, 32)"));

                // NOTE: C++ requires that a string array initialised by a literal has a space for the null-terminator
                file.buffer = StringReplace(file.buffer,
                                            STRING("unsigned char bip340_algo[13] ="),
                                            STRING("unsigned char bip340_algo[13 + 1] ="));

                // NOTE: Code that relied on sizeof(bip340_algo) must be adjusted to account for the null-terminator now
                file.buffer = StringReplace(file.buffer,
                                            STRING("sizeof(bip340_algo)"),
                                            STRING("(sizeof(bip340_algo) - 1)"));
            }
        }
    }

    unsigned const BUILD_FLAGS[] = {
        FileFlag_Normal | FileFlag_Mul64Support,
        FileFlag_Normal | FileFlag_Mul128Support | FileFlag_AsmInt128,
        FileFlag_Normal | FileFlag_Mul128Support | FileFlag_AsmX86_64,
    };

    // NOTE: Code generate
    for (int build_index = 0; build_index < ARRAY_COUNT(BUILD_FLAGS); build_index++)
    {
        unsigned const build_flags = BUILD_FLAGS[build_index];

        char build_name_buf[1024];
        String build_name = {};
        build_name.str    = build_name_buf;
        build_name.size   = snprintf(build_name_buf,
                                     sizeof(build_name_buf),
                                     "bt_secp256k1_i%s%s.h",
                                     (build_flags & FileFlag_Mul64Support) ? "64" : "128",
                                     (build_flags & FileFlag_AsmX86_64)    ? "_x86_64_asm" : "");

        // NOTE: Append all the files into one contiguous buffer
        String buffer = {};
        {
            StringList  list_head = {};
            StringList *list_tail = &list_head;

            // -------------------------------------------------------------------------------------
            // Intro files
            // -------------------------------------------------------------------------------------
            // TODO(doyle): Add no precomputed table macro
            list_tail = StringListAppendRef(list_tail, STRING(
            "// -----------------------------------------------------------------------------\n"
            "// NOTE: Overview\n"
            "// -----------------------------------------------------------------------------\n"
            "// A machine generated single-header file of bitcoin-core/libsecp256k1 that is\n"
            "// compilable in C/C++ with minimal effort.\n"
            "//\n"
            "// Define the following desired macros in one and only one C/C++ file to enable\n"
            "// the implementation in that translation unit.\n"
            "//   - BT_SECP256K1_IMPLEMENTATION\n"
            "//   Enable the single file library implementation in that translation unit\n"
            "//\n"
            "//   - BT_SECP256K1_DISABLE_WARNING_PRAGMAS\n"
            "//   Disable the extra compiler pragmas in this file that suppress some\n"
            "//   compiler warnings generated by the library for C++ compilers\n"
            "//\n"
            "//   - ECMULT_GEN_PREC_BITS <N>\n"
            "//   By default ECMULT_GEN_PREC_BITS is set to 4 if not defined. This is the\n"
            "//   default recommended value you'd get from building the library with\n"
            "//   \"auto\" settings.\n"
            "//\n"
            "//   - ECMULT_WINDOW_SIZE <N>\n"
            "//   By default ECMULT_WINDOW_SIZE is set to 15 if not defined. This is the\n"
            "//   default recommended value you'd get from building the library with\n"
            "//   \"auto\" settings.\n"
            "//\n"
            "// Define the following desired macros before the header file\n"
            "//   - ENABLE_MODULE_ECDH\n"
            "//   - ENABLE_MODULE_RECOVERY\n"
            "//   - ENABLE_MODULE_EXTRAKEYS\n"
            "//   - ENABLE_MODULE_SCHNORRSIG\n"
            "//   Additional modules that are available in secp256k1 can be enabled by using\n"
            "//   these macros which ensure the additional API's are not pre-processed out\n"
            "//   of the file.\n"
            "// -----------------------------------------------------------------------------\n"
            "// NOTE: License\n"
            "// -----------------------------------------------------------------------------\n"
            ));

            list_tail = StringListAppendRef(list_tail, STRING("/*\n"));
            for (LoadedFile const &file : intro_files)
            {
                if (build_flags & file.flag)
                    list_tail = StringListAppendRef(list_tail, file.buffer);
            }
            list_tail = StringListAppendRef(list_tail, STRING("*/\n"));

            list_tail = StringListAppendRef(list_tail, STRING(
            "// -----------------------------------------------------------------------------\n"
            "// NOTE: Example\n"
            "// -----------------------------------------------------------------------------\n"
            "#if 0\n"
            "// NOTE: The 2 following defines are optional! We provide a default value if\n"
            "// these are not defined before the implementation\n"
            "#define ECMULT_GEN_PREC_BITS 4\n"
            "#define ECMULT_WINDOW_SIZE 15\n"
            "// NOTE: The module defines are optional! They expose optional secp256k1 modules API\n"
            "#define ENABLE_MODULE_ECDH\n"
            "#define ENABLE_MODULE_RECOVERY\n"
            "#define ENABLE_MODULE_EXTRAKEYS\n"
            "#define ENABLE_MODULE_SCHNORRSIG\n"
            "#define BT_SECP256K1_IMPLEMENTATION\n"
            "#include \""
            ));

            list_tail = StringListAppendRef(list_tail, build_name);

            list_tail = StringListAppendRef(list_tail, STRING("\"\n"
            "#include <stdio.h>\n"
            "\n"
            "int main(int argc, char **argv)\n"
            "{\n"
            "    (void)argc; (void)argv;\n"
            "\n"
            "    // NOTE: Initialise the library\n"
            "    secp256k1_context *lib_context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);\n"
            "\n"
            "    // NOTE: Generate a public key from a hard-coded secret key\n"
            "    unsigned char const skey[32 + 1] = \"\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\"\n"
            "                                       \"\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\"\n"
            "                                       \"\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\"\n"
            "                                       \"\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x01\";\n"
            "    secp256k1_pubkey pkey;\n"
            "\n"
            "    if (secp256k1_ec_pubkey_create(lib_context, &pkey, skey) == 1)\n"
            "    {\n"
            "\n"
            "        // NOTE: Serialize the key\n"
            "        unsigned char pkey_serialized[33];\n"
            "        size_t pkey_serialized_size = sizeof(pkey_serialized);\n"
            "        secp256k1_ec_pubkey_serialize(lib_context, pkey_serialized, &pkey_serialized_size, &pkey, SECP256K1_EC_COMPRESSED);\n"
            "\n"
            "        // NOTE: Dump the secret key to stdout\n"
            "        fprintf(stdout, \"Secret Key [key=0x\");\n"
            "        for (size_t index = 0; index < sizeof(skey); index++)\n"
            "        {\n"
            "            fprintf(stdout, \"%x\", (skey[index] >> 4) & 0xF);\n"
            "            fprintf(stdout, \"%x\", (skey[index] >> 0) & 0xF);\n"
            "        }\n"
            "        fprintf(stdout, \"]\\n\");\n"
            "\n"
            "        // NOTE: Dump the serialized public key to stdout\n"
            "        fprintf(stdout, \"Public Key [key=0x\");\n"
            "        for (size_t index = 0; index < pkey_serialized_size; index++)\n"
            "        {\n"
            "            fprintf(stdout, \"%x\", (pkey_serialized[index] >> 4) & 0xF);\n"
            "            fprintf(stdout, \"%x\", (pkey_serialized[index] >> 0) & 0xF);\n"
            "        }\n"
            "        fprintf(stdout, \"]\\n\");\n"
            "    }\n"
            "    else\n"
            "    {\n"
            "        fprintf(stderr, \"Failed to generate public key\");\n"
            "    }\n"
            "\n"
            "    return 0;\n"
            "}\n"
            "#endif\n"
            "\n"
            "// -----------------------------------------------------------------------------\n"
            "// NOTE: Header Start\n"
            "// -----------------------------------------------------------------------------\n"
            ));

            // -------------------------------------------------------------------------------------
            // Header defines
            // -------------------------------------------------------------------------------------
            list_tail = StringListAppendRef(list_tail, STRING("#if !defined(BT_SECP256K1_H)\n"));
            list_tail = StringListAppendRef(list_tail, STRING("#define BT_SECP256K1_H\n\n"));

            list_tail = StringListAppendRef(list_tail, STRING("#if !defined(SECP256K1_BUILD)\n"));
            list_tail = StringListAppendRef(list_tail, STRING("    #define SECP256K1_BUILD\n"));
            list_tail = StringListAppendRef(list_tail, STRING("#endif\n\n"));

            // -------------------------------------------------------------------------------------
            // Header files
            // -------------------------------------------------------------------------------------
            for (LoadedFile const &file : header_files)
            {
                if (build_flags & file.flag)
                {
                    list_tail = AddSourceFileComment(list_tail, &file);
                    list_tail = StringListAppendRef(list_tail, file.buffer);
                    list_tail = StringListAppendRef(list_tail, STRING("\n"));
                }
            }

            // -------------------------------------------------------------------------------------
            // Extra Module Header files
            // -------------------------------------------------------------------------------------
            for (LoadedFile const &file : extra_module_header_files)
            {
                if (build_flags & file.flag)
                {
                    // NOTE: These are optional, conditionally defined based on
                    // the ENABLE_MODULE_* macro
                    char macro_name_buf[256];

                    String macro_name = {};
                    macro_name.str    = macro_name_buf;
                    macro_name.size   = snprintf(macro_name_buf,
                                                 sizeof(macro_name_buf),
                                                 "ENABLE_MODULE_%.*s",
                                                 file.module_macro_name.size,
                                                 file.module_macro_name.str);

                    list_tail = AddSourceFileComment(list_tail, &file);

                    // NOTE: Begin the conditional macro
                    list_tail = StringListAppendRef(list_tail, STRING("#if defined("));
                    list_tail = StringListAppendCopy(list_tail, macro_name);
                    list_tail = StringListAppendRef(list_tail, STRING(")\n"));

                    // NOTE: Write file
                    list_tail = StringListAppendRef(list_tail, file.buffer);

                    // NOTE: End the conditional macro
                    list_tail = StringListAppendRef(list_tail, STRING("#endif //"));
                    list_tail = StringListAppendCopy(list_tail, macro_name);
                    list_tail = StringListAppendRef(list_tail, STRING("\n\n"));
                }
            }

            list_tail = StringListAppendRef(list_tail, STRING("#endif // BT_SECP256K1_H\n"));

            // -------------------------------------------------------------------------------------
            // Add implementation defines
            // -------------------------------------------------------------------------------------
            list_tail = StringListAppendRef(list_tail, STRING(
            "\n"
            "// -----------------------------------------------------------------------------\n"
            "// NOTE: Implementation Start\n"
            "// -----------------------------------------------------------------------------\n"
            "#if defined(BT_SECP256K1_IMPLEMENTATION)\n"
            ));

            {
                list_tail = StringListAppendRef(list_tail, STRING("#if !defined(ECMULT_GEN_PREC_BITS)\n"));
                list_tail = StringListAppendRef(list_tail, STRING("    #define ECMULT_GEN_PREC_BITS 4\n"));
                list_tail = StringListAppendRef(list_tail, STRING("#endif\n\n"));
            }

            {
                list_tail = StringListAppendRef(list_tail, STRING("#if !defined(ECMULT_WINDOW_SIZE)\n"));
                list_tail = StringListAppendRef(list_tail, STRING("    #define ECMULT_WINDOW_SIZE 15\n"));
                list_tail = StringListAppendRef(list_tail, STRING("#endif\n\n"));
            }

            if (build_flags & FileFlag_Mul64Support)
            {
                list_tail = StringListAppendRef(list_tail, STRING("#if !defined(USE_FORCE_WIDEMUL_INT64)\n"));
                list_tail = StringListAppendRef(list_tail, STRING("    #define USE_FORCE_WIDEMUL_INT64\n"));
                list_tail = StringListAppendRef(list_tail, STRING("#endif\n\n"));
            }

            if (build_flags & FileFlag_Mul128Support)
            {
                list_tail = StringListAppendRef(list_tail, STRING("#if !defined(USE_FORCE_WIDEMUL_INT128)\n"));
                list_tail = StringListAppendRef(list_tail, STRING("    #define USE_FORCE_WIDEMUL_INT128\n"));
                list_tail = StringListAppendRef(list_tail, STRING("#endif\n\n"));
            }

            if (build_flags & FileFlag_AsmX86_64)
            {
                list_tail = StringListAppendRef(list_tail, STRING("#if !defined(USE_ASM_X86_64)\n"));
                list_tail = StringListAppendRef(list_tail, STRING("    #define USE_ASM_X86_64\n"));
                list_tail = StringListAppendRef(list_tail, STRING("#endif\n\n"));
            }

            list_tail = StringListAppendRef(list_tail, STRING(
                "#if !defined(BT_SECP256K1_DISABLE_WARNING_PRAGMAS)\n"
                "    #if defined(__clang__)\n"
                "        #pragma clang diagnostic push\n"
                "        #pragma clang diagnostic ignored \"-Wunused-function\"\n"
                "        #pragma clang diagnostic ignored \"-Wmissing-field-initializers\"\n"
                "    #elif defined(_MSC_VER)\n"
                "        #pragma warning(push)\n"
                "        #pragma warning(disable: 4146)\n"
                "        #pragma warning(disable: 4310)\n"
                "        #pragma warning(disable: 4244)\n"
                "        #pragma warning(disable: 4319)\n"
                "        #pragma warning(disable: 4267)\n"
                "        #pragma warning(disable: 4334)\n"
                "        #pragma warning(disable: 4127)\n"
                "        #pragma warning(disable: 4505)\n"
                "        #pragma warning(disable: 4706)\n"
                "    #endif\n"
                "#endif // !defined(BT_SECP256K1_DISABLE_WARNING_PRAGMAS)\n"
                "\n"
            ));

            // -------------------------------------------------------------------------------------
            // Append implementation Files
            // -------------------------------------------------------------------------------------
            for (LoadedFile const &file : private_header_files)
            {
                if (build_flags & file.flag)
                {
                    list_tail = AddSourceFileComment(list_tail, &file);
                    list_tail = StringListAppendRef(list_tail, file.buffer);
                }
            }

            for (LoadedFile const &file : private_impl_files)
            {
                if (build_flags & file.flag)
                {
                    list_tail = AddSourceFileComment(list_tail, &file);
                    list_tail = StringListAppendRef(list_tail, file.buffer);
                }
            }

            for (LoadedFile const &file : private_extra_module_impl_files)
            {
                if (build_flags & file.flag)
                {
                    // NOTE: These are optional, conditionally defined based on
                    // the ENABLE_MODULE_* macro
                    char macro_name_buf[256];

                    String macro_name = {};
                    macro_name.str    = macro_name_buf;
                    macro_name.size   = snprintf(macro_name_buf,
                                                 sizeof(macro_name_buf),
                                                 "ENABLE_MODULE_%.*s",
                                                 file.module_macro_name.size,
                                                 file.module_macro_name.str);

                    list_tail = AddSourceFileComment(list_tail, &file);

                    // NOTE: Begin the conditional macro
                    list_tail = StringListAppendRef(list_tail, STRING("#if defined("));
                    list_tail = StringListAppendCopy(list_tail, macro_name);
                    list_tail = StringListAppendRef(list_tail, STRING(")\n"));

                    // NOTE: Write file
                    list_tail = StringListAppendRef(list_tail, file.buffer);

                    // NOTE: End the conditional macro
                    list_tail = StringListAppendRef(list_tail, STRING("#endif //"));
                    list_tail = StringListAppendCopy(list_tail, macro_name);
                    list_tail = StringListAppendRef(list_tail, STRING("\n\n"));
                }
            }

            list_tail = StringListAppendRef(list_tail, STRING(
                "\n"
                "#if !defined(BT_SECP256K1_DISABLE_WARNING_PRAGMAS)\n"
                "    #if defined(__clang__)\n"
                "        #pragma clang diagnostic pop\n"
                "    #elif defined(_MSC_VER)\n"
                "        #pragma warning(pop)\n"
                "    #endif\n"
                "#endif // !defined(BT_SECP256K1_DISABLE_WARNING_PRAGMAS)\n"
                "\n"
                "#endif // BT_SECP256K1_IMPLEMENTATION\n"
            ));

            buffer = StringListBuild(&list_head);
        }

        // NOTE: Do string inserts and comment out header include directives
        {
            char   old_directive_buf[1024];
            char   new_directive_buf[1024];
            String old_directive = String{old_directive_buf, 0};
            String new_directive = String{new_directive_buf, 0};

            struct LoadedFileList
            {
                LoadedFile *data;
                int         size;
            };

            LoadedFileList file_list[] = {
                {header_files, ARRAY_COUNT(header_files)},
                {extra_module_header_files, ARRAY_COUNT(extra_module_header_files)},
                {private_header_files, ARRAY_COUNT(private_header_files)},
                {private_impl_files, ARRAY_COUNT(private_impl_files)},
                {private_extra_module_impl_files, ARRAY_COUNT(private_extra_module_impl_files)},
            };

            for (LoadedFileList const &list : file_list)
            {
                for (int file_index = 0; file_index < list.size; file_index++)
                {
                    LoadedFile const &file = list.data[file_index];
                    if (build_flags & file.flag)
                    {
                        // NOTE: Comment out the #includes for the file since
                        // we're producing a single header file.
                        old_directive.size = snprintf(old_directive_buf, sizeof(old_directive_buf), "#include \"%.*s\"", file.name.size, file.name.str);
                        buffer = StringFindThenInsert(buffer, old_directive, STRING("// "));
                    }
                }
            }

            // NOTE: secp256k1 uses relative headers, so patch them out
            buffer = StringFindThenInsert(buffer, STRING("#include \"../include/secp256k1"), STRING("// "));

            // NOTE: Allow adding a static context ontop of the single file
            // header library by commenting the #include directive in the source
            // code out. This allows the user to #include their own generated
            // static context before including this library.
            buffer = StringFindThenInsert(buffer, STRING("#include \"ecmult_static_context.h\""), STRING("// "));

        }

        // NOTE: Misc patches to source code
        {
            // NOTE: Delete any Windows style new-lines if there were any
            buffer = StringReplace(buffer, STRING("\r"), STRING(""));
        }

        // NOTE: Output file
        {
            FILE *file_handle = fopen(build_name.str, "w+b");
            if (!file_handle)
            {
                fprintf(stderr, "Failed to write to file, unable to run metaprogram [file=%s]\n", build_name.str);
                return -1;
            }

            fwrite(buffer.str, buffer.size, 1, file_handle);
            fclose(file_handle);

            printf("File generated to %s\n", build_name.str);
        }
    }

    return 0;
}
