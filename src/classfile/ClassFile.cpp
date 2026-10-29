#include "classfile/ClassFile.h"
#include "parser/Diagnostic.h"
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// ConstantPool
// ─────────────────────────────────────────────────────────────────────────────

uint16_t ConstantPool::push(Entry e) {
    uint16_t idx = static_cast<uint16_t>(entries_.size()) + 1u; // 1-based
    entries_.push_back(std::move(e));
    return idx;
}

uint16_t ConstantPool::addUtf8(const std::string& text) {
    auto [it, inserted] = utf8Map_.emplace(text, 0u);
    if (!inserted) return it->second;
    Entry e;
    e.tag  = Entry::Tag::Utf8;
    e.utf8 = text;
    it->second = push(std::move(e));
    return it->second;
}

uint16_t ConstantPool::addClass(const std::string& className) {
    auto [it, inserted] = classMap_.emplace(className, 0u);
    if (!inserted) return it->second;
    uint16_t nameIdx = addUtf8(className);
    Entry e;
    e.tag = Entry::Tag::Class;
    e.i1  = nameIdx;
    it->second = push(std::move(e));
    return it->second;
}

uint16_t ConstantPool::addNameAndType(const std::string& name,
                                       const std::string& descriptor) {
    std::string key = name + ":" + descriptor;
    auto [it, inserted] = natMap_.emplace(key, 0u);
    if (!inserted) return it->second;
    uint16_t nameIdx = addUtf8(name);
    uint16_t descIdx = addUtf8(descriptor);
    Entry e;
    e.tag = Entry::Tag::NameAndType;
    e.i1  = nameIdx;
    e.i2  = descIdx;
    it->second = push(std::move(e));
    return it->second;
}

uint16_t ConstantPool::addFieldref(const std::string& className,
                                    const std::string& name,
                                    const std::string& descriptor) {
    std::string key = className + "." + name + ":" + descriptor;
    auto [it, inserted] = fieldMap_.emplace(key, 0u);
    if (!inserted) return it->second;
    uint16_t classIdx = addClass(className);
    uint16_t natIdx   = addNameAndType(name, descriptor);
    Entry e;
    e.tag = Entry::Tag::Fieldref;
    e.i1  = classIdx;
    e.i2  = natIdx;
    it->second = push(std::move(e));
    return it->second;
}

uint16_t ConstantPool::addMethodref(const std::string& className,
                                     const std::string& name,
                                     const std::string& descriptor) {
    std::string key = className + "." + name + ":" + descriptor;
    auto [it, inserted] = methodMap_.emplace(key, 0u);
    if (!inserted) return it->second;
    uint16_t classIdx = addClass(className);
    uint16_t natIdx   = addNameAndType(name, descriptor);
    Entry e;
    e.tag = Entry::Tag::Methodref;
    e.i1  = classIdx;
    e.i2  = natIdx;
    it->second = push(std::move(e));
    return it->second;
}

uint16_t ConstantPool::addInteger(int32_t value) {
    auto [it, inserted] = intMap_.emplace(value, 0u);
    if (!inserted) return it->second;
    Entry e;
    e.tag     = Entry::Tag::Integer;
    e.integer = value;
    it->second = push(std::move(e));
    return it->second;
}

uint16_t ConstantPool::count() const noexcept {
    return static_cast<uint16_t>(entries_.size()) + 1u;
}

void ConstantPool::serialize(std::vector<uint8_t>& out) const {
    for (const auto& e : entries_) {
        switch (e.tag) {
            case Entry::Tag::Utf8: {
                out.push_back(1); // CONSTANT_Utf8
                uint16_t len = static_cast<uint16_t>(e.utf8.size());
                out.push_back(len >> 8);
                out.push_back(len & 0xFF);
                out.insert(out.end(), e.utf8.begin(), e.utf8.end());
                break;
            }
            case Entry::Tag::Integer:
                out.push_back(3); // CONSTANT_Integer
                out.push_back((e.integer >> 24) & 0xFF);
                out.push_back((e.integer >> 16) & 0xFF);
                out.push_back((e.integer >>  8) & 0xFF);
                out.push_back( e.integer        & 0xFF);
                break;
            case Entry::Tag::Class:
                out.push_back(7); // CONSTANT_Class
                out.push_back(e.i1 >> 8);
                out.push_back(e.i1 & 0xFF);
                break;
            case Entry::Tag::Fieldref:
                out.push_back(9); // CONSTANT_Fieldref
                out.push_back(e.i1 >> 8); out.push_back(e.i1 & 0xFF);
                out.push_back(e.i2 >> 8); out.push_back(e.i2 & 0xFF);
                break;
            case Entry::Tag::Methodref:
                out.push_back(10); // CONSTANT_Methodref
                out.push_back(e.i1 >> 8); out.push_back(e.i1 & 0xFF);
                out.push_back(e.i2 >> 8); out.push_back(e.i2 & 0xFF);
                break;
            case Entry::Tag::NameAndType:
                out.push_back(12); // CONSTANT_NameAndType
                out.push_back(e.i1 >> 8); out.push_back(e.i1 & 0xFF);
                out.push_back(e.i2 >> 8); out.push_back(e.i2 & 0xFF);
                break;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// BytecodeBuilder
// ─────────────────────────────────────────────────────────────────────────────

void BytecodeBuilder::requireLocals(int count) {
    if (count > maxLocals_) maxLocals_ = count;
}

void BytecodeBuilder::pushInt(int32_t v) {
    if (v >= -1 && v <= 5) {
        // iconst_m1 (0x02) through iconst_5 (0x08)
        u1(static_cast<uint8_t>(0x03 + v)); // iconst_0=0x03, v=-1 → 0x02
    } else if (v >= -128 && v <= 127) {
        u1(0x10); // bipush
        u1(static_cast<uint8_t>(static_cast<int8_t>(v)));
    } else if (v >= -32768 && v <= 32767) {
        u1(0x11); // sipush
        u1(static_cast<uint8_t>((v >> 8) & 0xFF));
        u1(static_cast<uint8_t>(v & 0xFF));
    } else {
        // ldc not yet implemented — flag as unimplemented
        throw CodeGenError("integer constant " + std::to_string(v) +
                            " requires ldc (not yet implemented)");
    }
    push(1);
}

void BytecodeBuilder::istore(int slot) {
    if (slot <= 3) {
        u1(static_cast<uint8_t>(0x3B + slot)); // istore_0..istore_3
    } else {
        u1(0x36); // istore <index>
        u1(static_cast<uint8_t>(slot));
    }
    pop(1);
    trackLocal(slot);
}

void BytecodeBuilder::iload(int slot) {
    if (slot <= 3) {
        u1(static_cast<uint8_t>(0x1A + slot)); // iload_0..iload_3
    } else {
        u1(0x15); // iload <index>
        u1(static_cast<uint8_t>(slot));
    }
    push(1);
    trackLocal(slot);
}

void BytecodeBuilder::getstatic(uint16_t cpIndex) {
    u1(0xB2); // getstatic
    u2(cpIndex);
    push(1); // pushes the field value (reference)
}

void BytecodeBuilder::invokevirtual(uint16_t cpIndex, int popArgs, bool pushResult) {
    u1(0xB6); // invokevirtual
    u2(cpIndex);
    pop(1 + popArgs);  // objectref + args
    if (pushResult) push(1);
}

void BytecodeBuilder::iadd() { u1(0x60); pop(1); } // pops 2, pushes 1 → net -1
void BytecodeBuilder::isub() { u1(0x64); pop(1); }
void BytecodeBuilder::imul() { u1(0x68); pop(1); }
void BytecodeBuilder::idiv() { u1(0x6C); pop(1); }

void BytecodeBuilder::returnVoid() { u1(0xB1); }

// ─────────────────────────────────────────────────────────────────────────────
// ClassFileWriter — static helpers
// ─────────────────────────────────────────────────────────────────────────────

void ClassFileWriter::u1(std::vector<uint8_t>& b, uint8_t v) {
    b.push_back(v);
}
void ClassFileWriter::u2(std::vector<uint8_t>& b, uint16_t v) {
    b.push_back(v >> 8);
    b.push_back(v & 0xFF);
}
void ClassFileWriter::u4(std::vector<uint8_t>& b, uint32_t v) {
    b.push_back((v >> 24) & 0xFF);
    b.push_back((v >> 16) & 0xFF);
    b.push_back((v >>  8) & 0xFF);
    b.push_back( v        & 0xFF);
}

// ─────────────────────────────────────────────────────────────────────────────
// ClassFileWriter::write — full class-file serialisation
//
// Layout (JVM spec §4):
//   magic u4 | minor u2 | major u2
//   constant_pool_count u2 | constant_pool[count-1]
//   access_flags u2 | this_class u2 | super_class u2
//   interfaces_count u2
//   fields_count u2
//   methods_count u2 | method_info[]
//   attributes_count u2
// ─────────────────────────────────────────────────────────────────────────────

void ClassFileWriter::write(std::vector<uint8_t>&         out,
                             const ConstantPool&            cp,
                             uint16_t                       thisClass,
                             uint16_t                       superClass,
                             const std::vector<MethodInfo>& methods) {
    // Magic
    u4(out, 0xCAFEBABEu);

    // Version: Java 8 (52.0)
    u2(out, 0);   // minor_version
    u2(out, 52);  // major_version

    // Constant pool
    u2(out, cp.count());
    cp.serialize(out);

    // Access flags: ACC_SUPER (0x0020)
    u2(out, 0x0020);

    // this_class / super_class
    u2(out, thisClass);
    u2(out, superClass);

    // Interfaces: none
    u2(out, 0);

    // Fields: none
    u2(out, 0);

    // Methods
    u2(out, static_cast<uint16_t>(methods.size()));
    for (const auto& m : methods) {
        u2(out, m.accessFlags);
        u2(out, m.nameIndex);
        u2(out, m.descriptorIndex);

        // Each method has exactly one attribute: Code
        u2(out, 1); // attributes_count

        // Code attribute
        // attribute_length = 2+2+4+code_length+2+2 = 12 + code_length
        uint32_t attrLen = 12u + static_cast<uint32_t>(m.code.size());

        u2(out, m.codeAttrNameIndex);
        u4(out, attrLen);
        u2(out, m.maxStack);
        u2(out, m.maxLocals);
        u4(out, static_cast<uint32_t>(m.code.size()));
        out.insert(out.end(), m.code.begin(), m.code.end());
        u2(out, 0); // exception_table_length
        u2(out, 0); // Code sub-attributes count
    }

    // Class attributes: none
    u2(out, 0);
}
