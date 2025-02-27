#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DLList_Init(list) \
  (list)->next = (list)->prev = (list)

#define DLList_Detach(item)              \
  do {                                   \
    if (item) {                          \
      (item)->prev->next = (item)->next; \
      (item)->next->prev = (item)->prev; \
      (item)->next       = nullptr;      \
      (item)->prev       = nullptr;      \
    }                                    \
  } while (0)

#define DLList_Prepend(list, item)       \
  do {                                   \
    if (item) {                          \
      if ((item)->next)                  \
        DLList_Detach(item);             \
      (item)->next       = (list)->next; \
      (item)->prev       = (list);       \
      (item)->next->prev = (item);       \
      (item)->prev->next = (item);       \
    }                                    \
  } while (0)

#define DLList_Append(list, item)        \
  do {                                   \
    if (item) {                          \
      if ((item)->next)                  \
        DLList_Detach(item);             \
      (item)->next       = (list);       \
      (item)->prev       = (list)->prev; \
      (item)->next->prev = (item);       \
      (item)->prev->next = (item);       \
    }                                    \
  } while (0)

#define DLList_ForEach(it_name, list) \
  auto *it_name = (list)->next;       \
  (it_name) != (list);                \
  (it_name) = (it_name)->next

struct Str8
{
  char *data;
  int   size;
};

struct Str8List
{
  Str8      str8;
  Str8List *next;
  Str8List *prev;
};

#define STR8(str8)                 \
  {                                \
    (char *)str8, sizeof(str8) - 1 \
  }

struct LoadedFile
{
  Str8     buffer;      // The contents of the file loaded from disk
  Str8     sub_dir;     // The sub directory that the file orginated from
  Str8     name;        // The file name
  Str8     hash_define; // The hash define, if any, to make the file conditionally loaded
};

static Str8 Str8_Copy(Str8 src)
{
  Str8 result = Str8{(char *)malloc(src.size + 1), src.size};
  memcpy(result.data, src.data, src.size);
  result.data[src.size] = 0;
  return result;
}

static void Str8_ListAppendRef(Str8List *list, Str8 str8)
{
  if (str8.size) {
    Str8List *link = (Str8List *)calloc(1, sizeof(Str8List));
    link->str8     = str8;
    DLList_Append(list, link);
  }
}

static void Str8_ListAppendCopy(Str8List *list, Str8 str8)
{
  if (str8.size) {
    Str8 copy = Str8_Copy(str8);
    Str8_ListAppendRef(list, copy);
  }
}

static void Str8_ListPrependRef(Str8List *list, Str8 str8)
{
  if (str8.size) {
    Str8List *link = (Str8List *)calloc(1, sizeof(Str8List));
    link->str8     = str8;
    DLList_Prepend(list, link);
  }
}

static void Str8_ListPrependCopy(Str8List *list, Str8 str8)
{
  if (str8.size) {
    Str8 copy = Str8_Copy(str8);
    Str8_ListPrependRef(list, copy);
  }
}

static Str8 Str8_ListBuild(Str8List *list)
{
  int total_size = 0;
  for (DLList_ForEach(entry, list))
    total_size += entry->str8.size;

  char *str8   = (char *)malloc(total_size + 1);
  char *writer = str8;
  for (DLList_ForEach(entry, list)) {
    memcpy(writer, entry->str8.data, entry->str8.size);
    writer += entry->str8.size;
  }

  writer[0] = 0;

  Str8 result = {str8, total_size};
  return result;
}

static bool Str8_Equals(Str8 lhs, Str8 rhs)
{
  bool result = lhs.size == rhs.size && (memcmp(lhs.data, rhs.data, lhs.size) == 0);
  return result;
}

static Str8 Str8_FindThenInsert(Str8 src, Str8 find, Str8 insert)
{
  // NOTE: We leak the str8 list memory but don't care because this is
  // a meta tool that lives for a couple of seconds and kills itself.
  Str8List str8_list = {};
  size_t count = 0;
  DLList_Init(&str8_list);

  int src_end = src.size - find.size;
  int head    = 0;
  for (int tail = head; tail <= src_end; tail++) {
    Str8 check = Str8{src.data + tail, find.size};
    if (!Str8_Equals(check, find))
      continue;

    Str8 range = Str8{src.data + head, (tail - head)};
    Str8_ListAppendRef(&str8_list, range);
    Str8_ListAppendRef(&str8_list, insert);
    Str8_ListAppendRef(&str8_list, check);
    head = tail + find.size;
    tail = head - 1; // NOTE: -1 because for loop post increment
    count++;
  }

  Str8 result = {};
  if (count == 0) {
    result = Str8_Copy(src); // NOTE: No insertion possible, so we just do a full-copy
  } else {
    Str8 remainder = Str8{src.data + head, src.size - head};
    Str8_ListAppendRef(&str8_list, remainder);
    result = Str8_ListBuild(&str8_list);
  }

  return result;
}

static Str8 Str8_Replace(Str8 src, Str8 find, Str8 replace)
{
  // NOTE: We leak the str8 list memory but don't care because this is
  // a meta tool that lives for a couple of seconds and kills itself.
  Str8List str8_list = {};
  size_t count = 0;
  DLList_Init(&str8_list);

  int src_end = src.size - find.size;
  int head    = 0;
  for (int tail = head; tail <= src_end; tail++) {
    Str8 check = Str8{src.data + tail, find.size};
    if (!Str8_Equals(check, find))
      continue;

    Str8 range = Str8{src.data + head, (tail - head)};
    Str8_ListAppendRef(&str8_list, range);
    Str8_ListAppendRef(&str8_list, replace);
    head = tail + find.size;
    tail = head - 1; // NOTE: -1 because for loop post increment
    count++;
  }

  Str8 result = {};
  if (count == 0) {
    result = Str8_Copy(src); // NOTE: No insertment possible, so we just do a full-copy
  } else {
    Str8 remainder = Str8{src.data + head, src.size - head};
    Str8_ListAppendRef(&str8_list, remainder);
    result = Str8_ListBuild(&str8_list);
  }

  return result;
}


static char *ReadEntireFile(char const *file_path, int *file_size)
{
  char *result      = nullptr;
  FILE *file_handle = fopen(file_path, "rb");
  if (file_handle) {
    fseek(file_handle, 0, SEEK_END);
    *file_size = ftell(file_handle);
    rewind(file_handle);

    result             = (char *)malloc(*file_size + 1);
    result[*file_size] = 0;
    if (result) {
      if (fread(result, (size_t)(*file_size), 1, file_handle) != 1) {
        free(result);
        result = nullptr;
      }
    }
    fclose(file_handle);
  }

  return result;
}

static bool LoadFiles(LoadedFile *files, int file_count, char const *root_dir)
{
  bool result = true;
  for (int file_index = 0; file_index < file_count; file_index++) {
    LoadedFile *file = files + file_index;
    char        file_path[2048];
    if (file->sub_dir.size)
      snprintf(file_path, sizeof(file_path), "%s/%.*s/%.*s", root_dir, file->sub_dir.size, file->sub_dir.data, file->name.size, file->name.data);
    else
      snprintf(file_path, sizeof(file_path), "%s/%.*s", root_dir, file->name.size, file->name.data);

    file->buffer.data = ReadEntireFile(file_path, &file->buffer.size);
    if (!file->buffer.data || file->buffer.size == 0) {
      fprintf(stderr, "Failed to load file [file=%s]\n", file_path);
      result = false;
    }
  }

  return result;
}

void AddSourceFileComment(Str8List *str8_list, LoadedFile const *file)
{
  Str8_ListAppendRef(str8_list, STR8("////////////////////////////////////////////////////////////////////////////////\n"
                                     "// File: "));
  if (file->sub_dir.size) {
    Str8_ListAppendRef(str8_list, file->sub_dir);
    Str8_ListAppendRef(str8_list, STR8("/"));
  }

  Str8_ListAppendRef(str8_list, file->name);
  Str8_ListAppendRef(str8_list, STR8("\n"));
}

void EmitFile(LoadedFile const *file, Str8List *str8_list)
{
  char macro_name_buf[256];
  Str8 macro_name = {};
  macro_name.data = macro_name_buf;
  macro_name.size = snprintf(macro_name_buf, sizeof(macro_name_buf), "%.*s", file->hash_define.size, file->hash_define.data);
  AddSourceFileComment(str8_list, file);

  // NOTE: Begin the conditional macro
  if (macro_name.size) {
    Str8_ListAppendRef(str8_list, STR8("#if defined("));
    Str8_ListAppendCopy(str8_list, macro_name);
    Str8_ListAppendRef(str8_list, STR8(")\n"));
  }

  // NOTE: Write file
  Str8_ListAppendRef(str8_list, file->buffer);

  // NOTE: End the conditional macro
  if (macro_name.size) {
    Str8_ListAppendRef(str8_list, STR8("#endif //"));
    Str8_ListAppendCopy(str8_list, macro_name);
    Str8_ListAppendRef(str8_list, STR8("\n\n"));
  }
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("Usage: bt_secp256k1_metaprogram <path/to/secp256k1/directory>\n");
    return -1;
  }

  // NOTE: Load files into memory
  LoadedFile intro_files[] = {
      {STR8(""), STR8(""), STR8("COPYING"), STR8("")},
  };

  LoadedFile header_files[] = {
      {STR8(""), STR8("include"), STR8("secp256k1.h"),              STR8("")},
      {STR8(""), STR8("include"), STR8("secp256k1_preallocated.h"), STR8("")},
  };

  LoadedFile extra_module_header_files[] = {
      {STR8(""), STR8("include"), STR8("secp256k1_ecdh.h"),       STR8("ENABLE_MODULE_ECDH")      },
      {STR8(""), STR8("include"), STR8("secp256k1_recovery.h"),   STR8("ENABLE_MODULE_RECOVERY")  },
      {STR8(""), STR8("include"), STR8("secp256k1_extrakeys.h"),  STR8("ENABLE_MODULE_EXTRAKEYS") },
      {STR8(""), STR8("include"), STR8("secp256k1_schnorrsig.h"), STR8("ENABLE_MODULE_SCHNORRSIG")},
      {STR8(""), STR8("include"), STR8("secp256k1_musig.h"),      STR8("ENABLE_MODULE_MUSIG")     },
      {STR8(""), STR8("include"), STR8("secp256k1_ellswift.h"),   STR8("ENABLE_MODULE_ELLSWIFT")  },
  };

  LoadedFile private_header_files[] = {
      {STR8(""), STR8("src"), STR8("checkmem.h"),               STR8("")                        },
      {STR8(""), STR8("src"), STR8("util.h"),                   STR8("")                        },
      {STR8(""), STR8("src"), STR8("field_10x26.h"),            STR8("SECP256K1_WIDEMUL_INT64") },
      {STR8(""), STR8("src"), STR8("scalar_8x32.h"),            STR8("SECP256K1_WIDEMUL_INT64") },
      {STR8(""), STR8("src"), STR8("field_5x52.h"),             STR8("SECP256K1_WIDEMUL_INT128")},
      {STR8(""), STR8("src"), STR8("scalar_4x64.h"),            STR8("SECP256K1_WIDEMUL_INT128")},
      {STR8(""), STR8("src"), STR8("modinv64.h"),               STR8("SECP256K1_WIDEMUL_INT128")},
      {STR8(""), STR8("src"), STR8("hash.h"),                   STR8("")                        },
      {STR8(""), STR8("src"), STR8("modinv32.h"),               STR8("")                        },
      {STR8(""), STR8("src"), STR8("assumptions.h"),            STR8("")                        },
      {STR8(""), STR8("src"), STR8("scalar.h"),                 STR8("")                        },
      {STR8(""), STR8("src"), STR8("field.h"),                  STR8("")                        },
      {STR8(""), STR8("src"), STR8("group.h"),                  STR8("")                        },
      {STR8(""), STR8("src"), STR8("ecmult_const.h"),           STR8("")                        },
      {STR8(""), STR8("src"), STR8("ecmult_gen.h"),             STR8("")                        },
      {STR8(""), STR8("src"), STR8("scratch.h"),                STR8("")                        },
      {STR8(""), STR8("src"), STR8("ecmult.h"),                 STR8("")                        },
      {STR8(""), STR8("src"), STR8("precomputed_ecmult.h"),     STR8("")                        },
      {STR8(""), STR8("src"), STR8("precomputed_ecmult_gen.h"), STR8("")                        },
      {STR8(""), STR8("src"), STR8("ecdsa.h"),                  STR8("")                        },
      {STR8(""), STR8("src"), STR8("eckey.h"),                  STR8("")                        },
      {STR8(""), STR8("src"), STR8("selftest.h"),               STR8("")                        },
  };

  LoadedFile private_impl_files[] = {
      {STR8(""), STR8("src"), STR8("field_10x26_impl.h"),       STR8("SECP256K1_WIDEMUL_INT64")},
      {STR8(""), STR8("src"), STR8("scalar_8x32_impl.h"),       STR8("SECP256K1_WIDEMUL_INT64")},
      {STR8(""), STR8("src"), STR8("int128_struct.h"),          STR8("SECP256K1_INT128_STRUCT")},
      {STR8(""), STR8("src"), STR8("int128_native.h"),          STR8("SECP256K1_INT128_NATIVE")},
      {STR8(""), STR8("src"), STR8("int128.h"),                 STR8("")},
      {STR8(""), STR8("src"), STR8("int128_impl.h"),            STR8("")},
      {STR8(""), STR8("src"), STR8("int128_struct_impl.h"),     STR8("SECP256K1_INT128_STRUCT")},
      {STR8(""), STR8("src"), STR8("int128_native_impl.h"),     STR8("SECP256K1_INT128_NATIVE")},
      {STR8(""), STR8("src"), STR8("field_5x52_int128_impl.h"), STR8("SECP256K1_WIDEMUL_INT128")},
      {STR8(""), STR8("src"), STR8("field_5x52_impl.h"),        STR8("SECP256K1_WIDEMUL_INT128")},
      {STR8(""), STR8("src"), STR8("scalar_4x64_impl.h"),       STR8("SECP256K1_WIDEMUL_INT128")},
      {STR8(""), STR8("src"), STR8("modinv64_impl.h"),          STR8("SECP256K1_WIDEMUL_INT128")},
      {STR8(""), STR8("src"), STR8("ecdsa_impl.h"),             STR8("")},
      {STR8(""), STR8("src"), STR8("scalar_impl.h"),            STR8("")},
      {STR8(""), STR8("src"), STR8("eckey_impl.h"),             STR8("")},
      {STR8(""), STR8("src"), STR8("group_impl.h"),             STR8("")},
      {STR8(""), STR8("src"), STR8("field_impl.h"),             STR8("")},
      {STR8(""), STR8("src"), STR8("ecmult_impl.h"),            STR8("")},
      {STR8(""), STR8("src"), STR8("ecmult_const_impl.h"),      STR8("")},
      {STR8(""), STR8("src"), STR8("ecmult_gen_impl.h"),        STR8("")},
      {STR8(""), STR8("src"), STR8("precomputed_ecmult.c"),     STR8("")},
      {STR8(""), STR8("src"), STR8("precomputed_ecmult_gen.c"), STR8("")},
      {STR8(""), STR8("src"), STR8("hash_impl.h"),              STR8("")},
      {STR8(""), STR8("src"), STR8("modinv32_impl.h"),          STR8("")},
      {STR8(""), STR8("src"), STR8("scratch_impl.h"),           STR8("")},
      {STR8(""), STR8("src"), STR8("hsort.h"),                  STR8("")},
      {STR8(""), STR8("src"), STR8("hsort_impl.h"),             STR8("")},
      {STR8(""), STR8("src"), STR8("secp256k1.c"),              STR8("")},
  };

  LoadedFile private_extra_module_impl_files[] = {
      {STR8(""), STR8("src/modules/ecdh"),       STR8("main_impl.h"), STR8("ENABLE_MODULE_ECDH")      },
      {STR8(""), STR8("src/modules/recovery"),   STR8("main_impl.h"), STR8("ENABLE_MODULE_RECOVERY")  },
      {STR8(""), STR8("src/modules/extrakeys"),  STR8("main_impl.h"), STR8("ENABLE_MODULE_EXTRAKEYS") },
      {STR8(""), STR8("src/modules/schnorrsig"), STR8("main_impl.h"), STR8("ENABLE_MODULE_SCHNORRSIG")},
      {STR8(""), STR8("src/modules/musig"),      STR8("main_impl.h"), STR8("ENABLE_MODULE_MUSIG")},
      {STR8(""), STR8("src/modules/ellswift"),   STR8("main_impl.h"), STR8("ENABLE_MODULE_ELLSWIFT")},
  };

#define ARRAY_COUNT(array) sizeof(array) / sizeof((array)[0])
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
    for (LoadedFile &file : private_header_files) {
      if (Str8_Equals(file.name, STR8("util.h"))) {
        // NOTE: Fix implicit cast from void to unsigned char C++ error
        file.buffer = Str8_Replace(
            file.buffer,
            STR8("const unsigned char *p1 = s1, *p2 = s2;"),
            STR8("const unsigned char *p1 = (const unsigned char *)s1, *p2 = (const unsigned char *)s2;"));
        break;
      }
    }

    for (LoadedFile &file : private_impl_files) {
      if (Str8_Equals(file.name, STR8("secp256k1.c"))) {
        // NOTE: Patch out module includes, we insert them manually after
        // the main implementation file
        file.buffer = Str8_FindThenInsert(file.buffer, STR8("# include \"modules"), STR8("// "));
        break;
      }
      if (Str8_Equals(file.name, STR8("int128.h"))) {
        file.buffer = Str8_FindThenInsert(file.buffer, STR8("#    include \"int128_struct.h\""), STR8("// "));
        file.buffer = Str8_FindThenInsert(file.buffer, STR8("#    include \"int128_native.h\""), STR8("// "));
      }
      if (Str8_Equals(file.name, STR8("int128_impl.h"))) {
        file.buffer = Str8_FindThenInsert(file.buffer, STR8("#    include \"int128_struct_impl.h\""), STR8("// "));
        file.buffer = Str8_FindThenInsert(file.buffer, STR8("#    include \"int128_native_impl.h\""), STR8("// "));
      }
      if (Str8_Equals(file.name, STR8("hsort_impl.h"))) {
        file.buffer = Str8_Replace(file.buffer,
                                   STR8("secp256k1_heap_down(ptr"),
                                   STR8("secp256k1_heap_down((unsigned char *)ptr"));
        file.buffer = Str8_Replace(file.buffer,
                                   STR8("secp256k1_heap_swap(ptr"),
                                   STR8("secp256k1_heap_swap((unsigned char *)ptr"));
      }
    }

    for (LoadedFile &file : private_extra_module_impl_files) {
      // NOTE: modules all use relative headers, so patch them out early.
      file.buffer = Str8_FindThenInsert(file.buffer, STR8("#include"), STR8("// "));

      if (Str8_Equals(file.sub_dir, STR8("src/modules/schnorrsig")) && Str8_Equals(file.name, STR8("main_impl.h"))) {
        // NOTE: Fix implicit cast from void to unsigned char C++ error
        file.buffer = Str8_Replace(file.buffer,
                                   STR8("secp256k1_sha256_write(&sha, data, 32)"),
                                   STR8("secp256k1_sha256_write(&sha, (const unsigned char *)data, 32)"));

        // NOTE: C++ requires that a str8 array initialised by a literal has a space for the null-terminator
        file.buffer = Str8_Replace(file.buffer,
                                   STR8("unsigned char bip340_algo[13] ="),
                                   STR8("unsigned char bip340_algo[13 + 1] ="));

        // NOTE: Code that relied on sizeof(bip340_algo) must be adjusted to account for the null-terminator now
        file.buffer = Str8_Replace(file.buffer,
                                   STR8("sizeof(bip340_algo)"),
                                   STR8("(sizeof(bip340_algo) - 1)"));
      }

      if (Str8_Equals(file.sub_dir, STR8("src/modules/ellswift")) && Str8_Equals(file.name, STR8("main_impl.h"))) {
        // NOTE: Fix implicit cast from void to unsigned char C++ error
        file.buffer = Str8_Replace(file.buffer,
                                   STR8("secp256k1_sha256_write(&sha, data, 64)"),
                                   STR8("secp256k1_sha256_write(&sha, (const unsigned char *)data, 64)"));

      }
    }
  }

  // NOTE: Code generate
  char build_name_buf[1024];
  Str8 build_name = {};
  build_name.data = build_name_buf;
  build_name.size = snprintf(build_name_buf, sizeof(build_name_buf), "bt_secp256k1.h");

  // NOTE: Append all the files into one contiguous buffer
  Str8 buffer = {};
  {
    Str8List str8_list = {};
    DLList_Init(&str8_list);

    // NOTE: Intro files
    // TODO(doyle): Add no precomputed table macro
    Str8_ListAppendRef(&str8_list, STR8("////////////////////////////////////////////////////////////////////////////////\n"
                                        "// NOTE: Overview\n"
                                        "// A machine generated single-header file of bitcoin-core/libsecp256k1 that is\n"
                                        "// compilable in C/C++ with minimal effort.\n"
                                        "//\n"
                                        "// Define the following desired macros in one and only one C/C++ file to enable\n"
                                        "// the implementation in that translation unit.\n"
                                        "//\n"
                                        "//   - BT_SECP256K1_IMPLEMENTATION\n"
                                        "//     Enable the single file library implementation in that translation unit\n"
                                        "//\n"
                                        "//   - BT_SECP256K1_DISABLE_WARNING_PRAGMAS\n"
                                        "//     Disable the extra compiler pragmas in this file that suppress some\n"
                                        "//     compiler warnings generated by the library for C++ compilers\n"
                                        "//\n"
                                        "//   - SECP256K1_CPLUSPLUS_TEST_OVERRIDE\n"
                                        "//     Officially, secp256k1 is not approved to be compiled with C++.\n"
                                        "//     This warning can be overriden by defining this macro.\n"
                                        "//\n"
                                        "//   - ECMULT_GEN_PREC_BITS <N>\n"
                                        "//     By default ECMULT_GEN_PREC_BITS is set to 4 if not defined. This is the\n"
                                        "//     default recommended value you'd get from building the library with\n"
                                        "//     \"auto\" settings.\n"
                                        "//\n"
                                        "//   - ECMULT_WINDOW_SIZE <N>\n"
                                        "//     By default ECMULT_WINDOW_SIZE is set to 15 if not defined. This is the\n"
                                        "//     default recommended value you'd get from building the library with\n"
                                        "//     \"auto\" settings.\n"
                                        "//\n"
                                        "//   - USE_ASM_X86_64\n"
                                        "//     Use x86_64 ASM implementations of functions (recommended if supported)\n"
                                        "//\n"
                                        "//   - USE_FORCE_WIDEMUL_INT128_STRUCT\n"
                                        "//   - USE_FORCE_WIDEMUL_INT128\n"
                                        "//   - USE_FORCE_WIDEMUL_INT64\n"
                                        "//     Choose how 128 integer math is done (with a struct, natively or using\n"
                                        "//     This is optional, omit to automatically use the best supported method.\n"
                                        "//\n"
                                        "// Define the following desired macros before the header file\n"
                                        "//\n"
                                        "//   - ENABLE_MODULE_ECDH\n"
                                        "//   - ENABLE_MODULE_RECOVERY\n"
                                        "//   - ENABLE_MODULE_EXTRAKEYS\n"
                                        "//   - ENABLE_MODULE_SCHNORRSIG\n"
                                        "//   - ENABLE_MODULE_MUSIG\n"
                                        "//   - ENABLE_MODULE_ELLSWIFT\n"
                                        "//\n"
                                        "//   Additional modules that are available in secp256k1 can be enabled by using\n"
                                        "//   these macros which ensure the additional API's are not pre-processed out\n"
                                        "//   of the file.\n"
                                        "//\n"
                                        "////////////////////////////////////////////////////////////////////////////////\n"
                                        "// NOTE: License\n"));

    Str8_ListAppendRef(&str8_list, STR8("/*\n"));
    for (LoadedFile const &file : intro_files)
      Str8_ListAppendRef(&str8_list, file.buffer);
    Str8_ListAppendRef(&str8_list, STR8("*/\n"));

    Str8_ListAppendRef(&str8_list, STR8("////////////////////////////////////////////////////////////////////////////////\n"
                                        "// NOTE: Example\n"
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
                                        "#define ENABLE_MODULE_MUSIG\n"
                                        "#define ENABLE_MODULE_ELLSWIFT\n"
                                        "#define BT_SECP256K1_IMPLEMENTATION\n"
                                        "#include \""));

    Str8_ListAppendRef(&str8_list, build_name);

    Str8_ListAppendRef(&str8_list, STR8("\"\n"
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
                                        "////////////////////////////////////////////////////////////////////////////////\n"
                                        "// NOTE: Header Start\n"));

    // NOTE: Header defines
    Str8_ListAppendRef(&str8_list, STR8("#if !defined(BT_SECP256K1_H)\n"));
    Str8_ListAppendRef(&str8_list, STR8("#define BT_SECP256K1_H\n\n"));

    Str8_ListAppendRef(&str8_list, STR8("#if !defined(SECP256K1_BUILD)\n"));
    Str8_ListAppendRef(&str8_list, STR8("    #define SECP256K1_BUILD\n"));
    Str8_ListAppendRef(&str8_list, STR8("#endif\n\n"));

    // NOTE: Header files
    for (LoadedFile const &file : header_files) {
      EmitFile(&file, &str8_list);
      Str8_ListAppendRef(&str8_list, STR8("\n"));
    }

    // NOTE: Extra Module Header files
    for (LoadedFile const &file : extra_module_header_files)
      EmitFile(&file, &str8_list);

    Str8_ListAppendRef(&str8_list, STR8("#endif // BT_SECP256K1_H\n"));

    // NOTE: Add implementation defines
    Str8_ListAppendRef(&str8_list, STR8("\n"
                                        "/////////////////////////////////////////////////////////////////////////////////\n"
                                        "// NOTE: Implementation Start\n"
                                        "#if defined(BT_SECP256K1_IMPLEMENTATION)\n"));

    {
      Str8_ListAppendRef(&str8_list, STR8("#if !defined(ECMULT_GEN_PREC_BITS)\n"));
      Str8_ListAppendRef(&str8_list, STR8("    #define ECMULT_GEN_PREC_BITS 4\n"));
      Str8_ListAppendRef(&str8_list, STR8("#endif\n\n"));
    }

    {
      Str8_ListAppendRef(&str8_list, STR8("#if !defined(ECMULT_WINDOW_SIZE)\n"));
      Str8_ListAppendRef(&str8_list, STR8("    #define ECMULT_WINDOW_SIZE 15\n"));
      Str8_ListAppendRef(&str8_list, STR8("#endif\n\n"));
    }

    Str8_ListAppendRef(&str8_list, STR8("#if !defined(BT_SECP256K1_DISABLE_WARNING_PRAGMAS)\n"
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
                                        "\n"));

    // NOTE: Append implementation Files
    LoadedFile field_5x52_file  = {};
    LoadedFile field_10x26_file = {};
    for (LoadedFile &file : private_header_files) {
      bool is_field_files = Str8_Equals(file.name, STR8("field_5x52.h")) || Str8_Equals(file.name, STR8("field_10x26.h"));
      if (is_field_files) {
        // NOTE: We substitue these directly into "field.h" because "field.h"
        // defines a macro before including these header files to conditionally
        // change some values in it
        if (Str8_Equals(file.name, STR8("field_5x52.h")))
          field_5x52_file = file;
        else
          field_10x26_file = file;
      }

      if (Str8_Equals(file.name, STR8("field.h"))) {
        Str8List list = {};
        DLList_Init(&list);
        AddSourceFileComment(&list, &field_5x52_file);
        Str8_ListAppendRef(&list, field_5x52_file.buffer);
        Str8 field_5x52_file_buffer = Str8_ListBuild(&list);

        list = {};
        DLList_Init(&list);
        AddSourceFileComment(&list, &field_10x26_file);
        Str8_ListAppendRef(&list, field_10x26_file.buffer);
        Str8 field_10x26_file_buffer = Str8_ListBuild(&list);

        file.buffer = Str8_Replace(file.buffer, STR8("#include \"field_5x52.h\""), field_5x52_file_buffer);
        file.buffer = Str8_Replace(file.buffer, STR8("#include \"field_10x26.h\""), field_10x26_file_buffer);
      }

      if (!is_field_files)
        EmitFile(&file, &str8_list);
    }

    for (LoadedFile const &file : private_impl_files)
      EmitFile(&file, &str8_list);

    for (LoadedFile const &file : private_extra_module_impl_files)
      EmitFile(&file, &str8_list);

    Str8_ListAppendRef(&str8_list, STR8("\n"
                                        "#if !defined(BT_SECP256K1_DISABLE_WARNING_PRAGMAS)\n"
                                        "    #if defined(__clang__)\n"
                                        "        #pragma clang diagnostic pop\n"
                                        "    #elif defined(_MSC_VER)\n"
                                        "        #pragma warning(pop)\n"
                                        "    #endif\n"
                                        "#endif // !defined(BT_SECP256K1_DISABLE_WARNING_PRAGMAS)\n"
                                        "\n"
                                        "#endif // BT_SECP256K1_IMPLEMENTATION\n"));

    buffer = Str8_ListBuild(&str8_list);
  }

  // NOTE: Do str8 inserts and comment out header include directives
  {
    char old_directive_buf[1024];
    char new_directive_buf[1024];
    Str8 old_directive = Str8{old_directive_buf, 0};
    Str8 new_directive = Str8{new_directive_buf, 0};

    struct LoadedFileList
    {
      LoadedFile *data;
      int         size;
    };

    LoadedFileList file_list[] = {
        {header_files,                    ARRAY_COUNT(header_files)                   },
        {extra_module_header_files,       ARRAY_COUNT(extra_module_header_files)      },
        {private_header_files,            ARRAY_COUNT(private_header_files)           },
        {private_impl_files,              ARRAY_COUNT(private_impl_files)             },
        {private_extra_module_impl_files, ARRAY_COUNT(private_extra_module_impl_files)},
    };

    for (LoadedFileList const &list : file_list) {
      for (int file_index = 0; file_index < list.size; file_index++) {
        LoadedFile const &file = list.data[file_index];
        // NOTE: Comment out the #includes for the file since
        // we're producing a single header file.
        old_directive.size = snprintf(old_directive_buf, sizeof(old_directive_buf), "#include \"%.*s\"", file.name.size, file.name.data);
        buffer             = Str8_FindThenInsert(buffer, old_directive, STR8("// "));
      }
    }

    // NOTE: secp256k1 uses relative headers, so patch them out
    buffer = Str8_FindThenInsert(buffer, STR8("#include \"../include/secp256k1"), STR8("// "));

    // NOTE: Allow adding a static context ontop of the single file
    // header library by commenting the #include directive in the source
    // code out. This allows the user to #include their own generated
    // static context before including this library.
    buffer = Str8_FindThenInsert(buffer, STR8("#include \"ecmult_static_context.h\""), STR8("// "));
  }

  // NOTE: Misc patches to source code
  {
    // NOTE: Delete any Windows style new-lines if there were any
    buffer = Str8_Replace(buffer, STR8("\r"), STR8(""));
  }

  // NOTE: Output file
  {
    FILE *file_handle = fopen(build_name.data, "w+b");
    if (!file_handle) {
      fprintf(stderr, "Failed to write to file, unable to run metaprogram [file=%s]\n", build_name.data);
      return -1;
    }

    fprintf(file_handle, "%.*s", buffer.size, buffer.data);
    fclose(file_handle);
    printf("File generated to %s\n", build_name.data);
  }

  return 0;
}
