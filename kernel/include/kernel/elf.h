#ifndef __ELF_H
#define __ELF_H

#include <types.h>

/* elf data types */

typedef uint16_t elf32_half;
typedef uint32_t elf32_off;
typedef uint32_t elf32_addr;
typedef uint32_t elf32_word;
typedef int32_t elf32_sword;

#define N_ELF_IDENTIFICATION 16

typedef struct {
  uint8_t identification[N_ELF_IDENTIFICATION]; 
  /* obj file type */
  elf32_half type;
  /* required arch */
  elf32_half machine;
  /* file version */
  elf32_word version;
  /* virtual entry point address */
  elf32_addr entry;
  /* program header table file offset */
  elf32_off p_header_off;
  /* section header table file offset */
  elf32_off s_header_off;
  /* processor-specific flags */
  elf32_word flags;
  /* ELF header size */
  elf32_half header_size;
  /* size of one entry in program header table */
  elf32_half p_header_entry_size;
  /* number of entries in program header table */
  elf32_half p_header_num_entries;
  /* size of one entry in section header table */
  elf32_half s_header_entry_size;
  /* number of entries in section header table */
  elf32_half s_header_num_entries;
  /* section header table index of string table */
  elf32_half s_header_str_index;
} elf32_header_t;

/* elf32_header_t.identification meanings */

typedef enum {
  /* 0x7F */
  ELF32_IDENTIFICATION_INDEX_MAG0      = 0,
  /* 'E' */
  ELF32_IDENTIFICATION_INDEX_MAG1      = 1,
  /* 'L' */
  ELF32_IDENTIFICATION_INDEX_MAG2      = 2,
  /* 'F' */
  ELF32_IDENTIFICATION_INDEX_MAG3      = 3,
  /* file class */
  ELF32_IDENTIFICATION_INDEX_CLASS     = 4,
  /* encoding of processor-specific data */
  ELF32_IDENTIFICATION_INDEX_DATA      = 5,
  /* must be elf32_header_t.version */
  ELF32_IDENTIFICATION_INDEX_VERSION   = 6,
  /* beginning of unused bytes */
  ELF32_IDENTIFICATION_INDEX_PAD_START = 7,
  /* size of elf header */ 
  ELF32_IDENTIFICATION_INDEX_SIZE      = sizeof(uint8_t[N_ELF_IDENTIFICATION])
} e_elf32_identification_index;

typedef enum {
  ELF32_IDENTIFICATION_CLASS_NONE = 0,
  ELF32_IDENTIFICATION_CLASS_32   = 1
} e_elf32_identification_class;

typedef enum {
  /* invalid encoding */
  ELF32_IDENTIFICATION_DATA_NONE = 0,
  /* little endian */
  ELF32_IDENTIFICATION_DATA_2LSB = 1,
  /* big endian */
  ELF32_IDENTIFICATION_DATA_2MSB = 2
} e_elf32_idenfification_data;

/* elf32_header_t.type meanings */
typedef enum {
  /* no file type */
  ELF32_TYPE_NONE = 0,
  /* relocatable file */
  ELF32_TYPE_REL  = 1,
  /* executable file */
  ELF32_TYPE_EXEC = 2,
  /* shared object file */
  ELF32_TYPE_DYN  = 3,
  /* core file */
  ELF32_TYPE_CORE = 4,
  /* processor-specific */
  ELF32_TYPE_LOW_PROC = 0xFF00,
  /* processor-specific */
  ELF32_TYPE_HI_PROC  = 0xFFFF
} e_elf32_type;

/* elf32_header_t.machine meanings */
typedef enum {
  /* no machine */
  ELF32_MACHINE_NONE = 0,
  /* intel architecture */
  ELF32_MACHINE_386  = 3,
  /* intel 80860 */
  ELF32_MACHINE_860  = 7,
} e_elf32_machine;

/* elf32_header_t.version meanings */
typedef enum {
  ELF32_VERSION_NONE = 0,
  ELF32_VERSION_CURR = 1
} e_elf32_version;

typedef enum {
  /* undefined, missing, or irrelevant section reference */
  ELF32_S_HEADER_SPECIAL_UNDEFINED = 0x0000,
  /* lower bound of reserved section headers */
  ELF32_S_HEADER_SPECIAL_LORESERVE = 0xFF00,
  /* lower bound of reserved for processor-specific semantics */
  ELF32_S_HEADER_SPECIAL_LO_PROCES = 0xFF00,
  /* higher bound of reserver  for processor-specific semantics */
  ELF32_S_HEADER_SPECIAL_HI_PROCES = 0xFF1F,
  /* spcifies absolute values for corresponding reference */
  ELF32_S_HEADER_SPECIAL_ABSOLUTES = 0xFFF1,
  /* common symbols (extern C veriables) */
  ELF32_S_HEADER_SPECIAL_COMMONSYM = 0xFFF2,
  /* upper bound of reserved section headers */
  ELF32_S_HEADER_SPECIAL_HIRESERVE = 0xFFFF
} e_elf32_section_header_special;

typedef struct {
  /**
   * name of the section
   * index into the section header string table section
   */
  elf32_word name;
  /* categorizes the secion's contents & semantics */
  elf32_word type;
  /* 1-bit flags that describe miscellaneous attributes */
  elf32_word flags;
  /* address at which sections first byte should reside in the process */
  elf32_addr address;
  /* offset to the section */
  elf32_off  offset;
  /* section size in bytes */
  elf32_word size;
  /**
   * IF type = ELF32_S_HEADER_TYPE_DYNAMIC:
   *  the s_header index of the string table used by
   *  entries in the section
   * IF type = ELF32_S_HEADER_TYPE_SYMBOL_HASH_TABLE:
   *  the s_header index of the symbol table to which
   *  the hash table applies
   * IF type = ELF32_S_HEADER_TYPE_RELOCATION_<ANY>:
   *  the s_header index of the associated symbol
   *  table
   * ELSE:
   *  operating-system specific
   */
  elf32_word link;
  /**
   * IF type = ELF32_S_HEADER_TYPE_RELOCATION_<ANY>:
   *  the s_header index of the section to which the
   *  relocation applies
   * ELSE:
   *  operating-system specific
   */
  elf32_word info;
  /* alignment of the section */
  elf32_word address_alignment;
  /* the size of each entry in an entry in a section if that section contains a table of entries */
  elf32_word entry_size;
} elf32_s_header_t;

typedef enum {
  /* is inactive */
  ELF32_S_HEADER_TYPE_NULL = 0,
  /* information defined by program, only for program */
  ELF32_S_HEADER_TYPE_PROG = 1,
  /* holds a symbol table */
  ELF32_S_HEADER_TYPE_SYSTEM_TABLE = 2,
  /* holds a string table */
  ELF32_S_HEADER_TYPE_STRING_TABLE = 3,
  /* relocation entries with explicit addends */
  ELF32_S_HEADER_TYPE_RELOCATION_EXPLICIT = 4,
  /* holds a symbol hash table */
  ELF32_S_HEADER_TYPE_SYMBOL_HASH_TABLE = 5,
  /* information for dynamic linking */
  ELF32_S_HEADER_TYPE_DYNAMIC = 6,
  /* information that marks the file in some way */
  ELF32_S_HEADER_TYPE_NOTE = 7,
  /* occupies NO space in the file but otherwise resembles ELF32_S_HEADER_TYPE_PROG */
  ELF32_S_HEADER_TYPE_NOBITS = 8,
  /* relocation entries without explicit addends */
  ELF32_S_HEADER_TYPE_RELOCATION_IMPLICIT = 9,
  /* reserved */
  ELF32_S_HEADER_TYPE_SHLIB = 10,
  /* holds a symbol table */
  ELF32_S_HEADER_TYPE_DYNAMIC_SYMB = 11,
  /* begin of range reserved for processor-specific semantics */
  ELF32_S_HEADER_TYPE_PROCESSOR_LOW = 0x70000000,
  /* end of range reserved for processor-specific smantics*/
  ELF32_S_HEADER_TYPE_PROCESSOR_HIGH = 0x7fffffff,
  /* begin of range reserved for applications */
  ELF32_S_HEADER_TYPE_USER_LOW = 0x80000000, 
  /* end of range used for applications */
  ELF32_S_HEADER_TYPE_USER_HIGH = 0xffffffff
} e_elf32_section_header_type;

typedef enum {
  /* contains data that should be writeable during execution */
  ELF32_S_HEADER_FLAG_WRITE         = 0x1,
  /* occupies memory during process execution */
  ELF32_S_HEADER_FLAG_ALLOC         = 0x2,
  /* contains executable machine instructions */
  ELF32_S_HEADER_FLAG_EXECUTABLE    = 0x4,
  /* all bits in this mask are reserved */
  ELF32_S_HEADER_FLAG_RESERVED_BITS = 0xf0000000,
} e_elf32_section_header_flags;

typedef struct {
  /* index into symbol string table */
  elf32_word name;
  /* value of associated symbol */
  elf32_addr value;
  /* size of symbol */
  elf32_word size;
  /* symbol type & binding attibutes */
  uint8_t    info;
  /* reserved - 0 */
  uint8_t    other;
  /* index for related s_header table */
  elf32_half s_header_index;
} elf32_sym_table_entry_t;

#define ELF32_SYM_TABLE_ENTRY_BIND(i) ((i) >> 4)
#define ELF32_SYM_TABLE_ENTRY_TYPE(i) ((i) & 0xF)
#define ELF32_SYM_TABLE_ENTRY_INFO(b, t) (((b) << 4) + ((t) & 0xF))

typedef enum {
  /* local symbols are not visible outside the object file */
  ELF32_SYM_TABLE_ENTRY_BIND_LOCAL   = 0,
  /* global symbols ARE visible outside the object file */
  ELF32_SYM_TABLE_ENTRY_BIND_GLOBAL  = 1,
  /* resemble global symbols, but their definitions have lower precedence */
  ELF32_SYM_TABLE_ENTRY_BIND_WEAK    = 2,
  /* begin of reserved for processor-specific semantics */
  ELF32_SYM_TABLE_ENTRY_BIND_LO_PROC = 13,
  /* end of reserved for processor-specific semantics */
  ELF32_SYM_TABLE_ENTRY_BIND_HI_PROC = 15
} e_elf32_sym_table_entry_bind;

typedef enum {
  /* type is not specified */
  ELF32_SYM_TABLE_ENTRY_TYPE_NOTYPE = 0,
  /* data object; variable, array, etc. */
  ELF32_SYM_TABLE_ENTRY_TYPE_OBJECT = 1,
  /* function / other executable code */
  ELF32_SYM_TABLE_ENTRY_TYPE_FUNC   = 2,
  /* associated with a section */
  ELF32_SYM_TABLE_ENTRY_TYPE_SECTION = 3,
  /* symbols name give the name of the source file */
  ELF32_SYM_TABLE_ENTRY_TYPE_FILE = 4,
  /* begin of reserved for processor-specific semantics */
  ELF32_SYM_TABLE_ENTRY_TYPE_LO_PROC = 13,
  /* end of reserved for processor-specific semantics */
  ELF32_SYM_TABLE_ENTRY_TYPE_HI_PROC = 15
} e_elf32_sym_table_entry_type;

typedef struct {
  /* location at which to apply the relocation action */
  elf32_addr offset;
  /* symbol table index & type of relocation */
  elf32_word info;
} elf32_relocation_t;


typedef struct {
  /* location at which to apply the relocation action */
  elf32_addr  offset;
  /* symbol table index & type of relocation */
  elf32_word  info;
  /* constant addend to compute value stored into relocatable field */
  elf32_sword addend;
} elf32_relocation_add_t;

#define ELF32_RELOCATION_INFO_SYM_TABLE_INDEX(i) ((i) >> 8)
#define ELF32_RELOCATION_INFO_RELOCATION_TYPE(i)\
  ((unsigned char) i)
#define ELF32_RELOCATION_INFO_DEF(index, type)\
  (((index) << 8) + (unsigned char)(type))

typedef struct {
  /* what kind of segment this array element describes */
  elf32_word type;
  /* offset where the segment resides */
  elf32_off  offset;
  /* virtual address of first byte of segment */
  elf32_addr vaddr;
  /* not relevant */
  elf32_addr paddr;
  /* number of bytes in file image of segment */
  elf32_word file_size;
  /* number of bytes in memory image of segment */
  elf32_word mem_size;
  /* flags */
  elf32_word flags;
  /* alignent of segment when loaded */
  elf32_word alignment;
} elf32_p_header_t;

typedef enum {
  ELF32_P_HEADER_TYPE_NULL = 0,
  ELF32_P_HEADER_TYPE_LOADABLE_SEG = 1,
  ELF32_P_HEADER_TYPE_DYNAMIC = 2,
  ELF32_P_HEADER_TYPE_INTERPRETER = 3,
  ELF32_P_HEADER_TYPE_NOTE = 4,
  ELF32_P_HEADER_TYPE_RESERVED = 5,
  ELF32_P_HEADER_TYPE_P_HEADER = 6,
  ELF32_P_HEADER_TYPE_LO_PROC = 0x70000000,
  ELF32_P_HEADER_TYPE_HI_PROC = 0x7fffffff
} e_elf32_p_header_type;

/* exe.c */
typedef struct t_Process t_Process;
void *elf_load(t_Process *process, const void *file_buff);
bool elf_validate_magic(elf32_header_t *header);
bool elf_validate_supported(elf32_header_t *header);

void elf_load_segments(t_Process *process, elf32_header_t *header);
#endif

