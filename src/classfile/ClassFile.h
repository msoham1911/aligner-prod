#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Constant-pool builder
//
// The JVM constant pool is 1-indexed. Internally we store entries in a
// 0-based vector; the public index is internal_index + 1.
// All add*() methods are idempotent: calling them twice with the same
// arguments returns the same index.
// ─────────────────────────────────────────────────────────────────────────────

class ConstantPool {
public:
    uint16_t addUtf8      (const std::string& text);
    uint16_t addClass     (const std::string& className);           // binary name
    uint16_t addNameAndType(const std::string& name,
                            const std::string& descriptor);
    uint16_t addFieldref  (const std::string& className,
                            const std::string& name,
                            const std::string& descriptor);
    uint16_t addMethodref (const std::string& className,
                            const std::string& name,
                            const std::string& descriptor);
    uint16_t addInteger   (int32_t value);

    /// count() == number_of_entries + 1  (the JVM constant_pool_count field)
    uint16_t count() const noexcept;

    /// Append raw constant-pool bytes to out.
    void serialize(std::vector<uint8_t>& out) const;

private:
    struct Entry {
        enum class Tag : uint8_t {
            Utf8        = 1,
            Integer     = 3,
            Class       = 7,
            Fieldref    = 9,
            Methodref   = 10,
            NameAndType = 12,
        };
        Tag         tag{Tag::Utf8};
        std::string utf8;              // Utf8
        uint16_t    i1{0}, i2{0};     // Class / Fieldref / Methodref / NameAndType
        int32_t     integer{0};        // Integer
    };

    std::vector<Entry>              entries_;

    // Deduplication caches
    std::map<std::string, uint16_t> utf8Map_;
    std::map<std::string, uint16_t> classMap_;
    std::map<std::string, uint16_t> natMap_;    // "name:desc"
    std::map<std::string, uint16_t> fieldMap_;  // "class.name:desc"
    std::map<std::string, uint16_t> methodMap_; // "class.name:desc"
    std::map<int32_t,     uint16_t> intMap_;

    /// Append an entry and return its 1-based index.
    uint16_t push(Entry e);
};

// ─────────────────────────────────────────────────────────────────────────────
// Bytecode builder — accumulates JVM instructions for a single method.
// Stack depth is tracked to compute max_stack.
// ─────────────────────────────────────────────────────────────────────────────

class BytecodeBuilder {
public:
    // ── Integer push (selects most compact encoding) ──────────────────────────
    void pushInt(int32_t v);

    // ── Local variable access ─────────────────────────────────────────────────
    void istore(int slot);
    void iload (int slot);

    // ── Field and method ──────────────────────────────────────────────────────
    void getstatic     (uint16_t cpIndex);
    void invokevirtual (uint16_t cpIndex, int popArgs, bool pushResult);

    // ── Arithmetic ────────────────────────────────────────────────────────────
    void iadd();
    void isub();
    void imul();
    void idiv();

    // ── Control ───────────────────────────────────────────────────────────────
    void returnVoid();

    // ── Inform the builder about local slots not covered by istore/iload ──────
    void requireLocals(int count);

    // ── Accessors for ClassFileWriter ─────────────────────────────────────────
    const std::vector<uint8_t>& code()      const noexcept { return code_;      }
    uint16_t                    maxStack()   const noexcept { return static_cast<uint16_t>(maxStack_);  }
    uint16_t                    maxLocals()  const noexcept { return static_cast<uint16_t>(maxLocals_); }

private:
    std::vector<uint8_t> code_;
    int                  currentStack_{0};
    int                  maxStack_{0};
    int                  maxLocals_{0};

    void u1(uint8_t v)   { code_.push_back(v); }
    void u2(uint16_t v)  { code_.push_back(v >> 8); code_.push_back(v & 0xFF); }

    void push(int n = 1) { currentStack_ += n; if (currentStack_ > maxStack_) maxStack_ = currentStack_; }
    void pop (int n = 1) { currentStack_ -= n; }
    void trackLocal(int slot) { if (slot + 1 > maxLocals_) maxLocals_ = slot + 1; }
};

// ─────────────────────────────────────────────────────────────────────────────
// ClassFileWriter — serialises the assembled class to a byte vector.
// ─────────────────────────────────────────────────────────────────────────────

struct MethodInfo {
    uint16_t accessFlags;
    uint16_t nameIndex;
    uint16_t descriptorIndex;
    uint16_t codeAttrNameIndex; // index of "Code" Utf8 in CP
    uint16_t maxStack;
    uint16_t maxLocals;
    std::vector<uint8_t> code;
};

class ClassFileWriter {
public:
    /// Write a complete class file into out.
    static void write(std::vector<uint8_t>&    out,
                      const ConstantPool&       cp,
                      uint16_t                  thisClass,
                      uint16_t                  superClass,
                      const std::vector<MethodInfo>& methods);
private:
    static void u1(std::vector<uint8_t>& b, uint8_t  v);
    static void u2(std::vector<uint8_t>& b, uint16_t v);
    static void u4(std::vector<uint8_t>& b, uint32_t v);
};
