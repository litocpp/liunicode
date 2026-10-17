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

auto mapping(const Storage&     storage,
             unsigned           table_id,
             unsigned long long target,
             bool               exact = true) -> Mapping {
    const auto& table = storage.tables[table_id];
    unsigned    first = table.index, last = first + table.count;
    while (first < last) {
        auto middle = first + (last - first) / 2;
        if (storage.index[middle].key <= target)
            first = middle + 1;
        else
            last = middle;
    }
    if (first == table.index) return {};
    auto               position = storage.index[first - 1].offset;
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

auto contains(const Storage& storage, unsigned table, unsigned cp) -> bool {
    if (cp > 0x10ffff) return false;
    auto entry = mapping(storage, table, cp, false);
    return entry.size && cp - entry.key <= entry.values[0];
}

auto scalar(const Storage& storage, unsigned table, unsigned cp, unsigned fallback) -> unsigned {
    auto entry = mapping(storage, table, cp);
    return entry.size ? entry.values[0] : fallback;
}

} // namespace liunicode::tables
