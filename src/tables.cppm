module liunicode:tables;
namespace liunicode::tables
{
struct Table {
    unsigned end, index, count, width;
};
struct Checkpoint {
    unsigned long long key;
    unsigned           offset;
};
struct Alias {
    unsigned name, property;
};
struct Storage {
    const char*       data;
    const Checkpoint* index;
    const Table*      tables;
};
struct Mapping {
    unsigned long long key {};
    unsigned           size {};
    unsigned           values[32] {};
};

auto byte(const char* data, unsigned& position) -> unsigned {
    return static_cast<unsigned char>(data[position++]);
}

auto integer(const char* data, unsigned& position) -> unsigned long long {
    unsigned long long value {};
    for (unsigned shift {};; shift += 7) {
        auto part = byte(data, position);
        value |= static_cast<unsigned long long>(part & 127) << shift;
        if (part < 128) return value;
    }
}

auto checkpoint(const Storage& storage, const Table& table, unsigned long long target)
    -> const Checkpoint* {
    unsigned first = table.index, last = first + table.count;
    while (first < last) {
        auto middle = first + (last - first) / 2;
        if (storage.index[middle].key <= target)
            first = middle + 1;
        else
            last = middle;
    }
    return first == table.index ? nullptr : &storage.index[first - 1];
}

auto mapping(const Storage&     storage,
             unsigned           table_id,
             unsigned long long target,
             bool               exact = true) -> Mapping {
    const auto& table = storage.tables[table_id];
    auto        block = checkpoint(storage, table, target);
    if (! block) return {};
    auto               position = block->offset;
    unsigned long long key {};
    Mapping            result;
    for (unsigned row {}; row < 32 && position < table.end; ++row) {
        // The first key in each indexed block is absolute; the rest are deltas.
        auto delta = integer(storage.data, position);
        key        = row ? key + delta : delta;
        if (key > target) break;
        Mapping current;
        current.key  = key;
        current.size = table.width ? table.width : byte(storage.data, position);
        for (unsigned i {}; i < current.size; ++i)
            current.values[i] = static_cast<unsigned>(integer(storage.data, position));
        if (key == target) return current;
        if (! exact) result = current;
    }
    return result;
}

auto contains(const Storage& storage, unsigned table_id, unsigned cp) -> bool {
    if (cp > 0x10ffff) return false;
    const auto& table = storage.tables[table_id];
    auto        block = checkpoint(storage, table, cp);
    if (! block) return false;
    auto     position = block->offset;
    unsigned key {};
    for (unsigned row {}; row < 32 && position < table.end; ++row) {
        auto delta = static_cast<unsigned>(integer(storage.data, position));
        key        = row ? key + delta : delta;
        if (key > cp) return false;
        // Membership rows store only a range length after the key.
        auto length = static_cast<unsigned>(integer(storage.data, position));
        if (cp - key <= length) return true;
    }
    return false;
}

auto scalar(const Storage& storage, unsigned table, unsigned cp, unsigned fallback) -> unsigned {
    auto entry = mapping(storage, table, cp);
    return entry.size ? entry.values[0] : fallback;
}

} // namespace liunicode::tables
