//
// Created by Gxin on 2022/11/7.
//

#include "lua_table.h"

#include "gany_lua_impl.h"

#include <algorithm>


LuaTable::LuaTable() = default;

LuaTable::LuaTable(lua_State *L, int idx)
{
    parse(L, idx);
}

LuaTable::LuaTable(const LuaTable &b)
    : mTable(b.mTable)
{
}

LuaTable::LuaTable(LuaTable &&b) noexcept
    : mTable(std::move(b.mTable))
{
}

LuaTable &LuaTable::operator=(const LuaTable &b)
{
    if (this != &b) {
        this->mTable = b.mTable;
    }
    return *this;
}

LuaTable &LuaTable::operator=(LuaTable &&b) noexcept
{
    if (this != &b) {
        this->mTable = std::move(b.mTable);
    }
    return *this;
}

LuaTable LuaTable::fromGAnyObject(const GAny &obj)
{
    if (obj.is<LuaTable>()) {
        return *obj.as<LuaTable>();
    }
    LuaTable table;
    if (obj.isArray()) {
        int32_t index = 1;
        for (int32_t i = 0; i < obj.size(); i++) {
            GAny item = obj[i];
            if (item.isArray() || item.isObject()) {
                table.setItem(index, fromGAnyObject(item));
            } else {
                table.setItem(index, item);
            }
            index++;
        }
    }
    if (obj.isObject()) {
        const auto it = obj.iterator();
        while (it.hasNext()) {
            auto item = it.next();
            if (item.second.isArray() || item.second.isObject()) {
                table.setItem(item.first, fromGAnyObject(item.second));
            } else {
                table.setItem(item.first, item.second);
            }
        }
    }

    return table;
}

GAny LuaTable::getItem(const GAny &key) const
{
    for (const auto &item: mTable) {
        const auto &k = item.first;
        if (compareKey(k, key)) {
            return item.second;
        }
    }
    return GAny::null();
}

void LuaTable::setItem(const GAny &key, const GAny &value)
{
    if (value.isNull() || value.isUndefined()) {
        delItem(key);
        return;
    }

    for (auto it = mTable.begin(); it != mTable.end(); ++it) {
        const auto &k = it->first;
        if (compareKey(k, key)) {
            it->second = value;
            return;
        }
    }
    mTable.emplace_back(key, value);
}

void LuaTable::delItem(const GAny &key)
{
    for (auto it = mTable.begin(); it != mTable.end(); ++it) {
        const auto &k = it->first;
        if (compareKey(k, key)) {
            mTable.erase(it);
            return;
        }
    }
}

std::string LuaTable::toString() const
{
    std::stringstream ss;
    ss << "{";
    for (auto it = mTable.begin(); it != mTable.end(); ++it) {
        if (it != mTable.begin()) {
            ss << ", ";
        }
        const auto &key = it->first;
        const auto &val = it->second;
        ss << "[";
        if (!isNonStringType(key)) {
            ss << "\"";
        }
        ss << key.toString();
        if (!isNonStringType(key)) {
            ss << "\"";
        }
        ss << "]=";
        if (!isNonStringType(val)) {
            ss << "\"" << val.toString() << "\"";
        } else {
            ss << val.toString();
        }
    }
    ss << "}";
    return ss.str();
}

size_t LuaTable::length() const
{
    return mTable.size();
}

void LuaTable::push(lua_State *L) const
{
    lua_newtable(L);
    const int top = lua_gettop(L);
    for (const auto &item: mTable) {
        GAnyLuaImpl::makeGAnyToLuaObject(L, item.first);
        GAnyLuaImpl::makeGAnyToLuaObject(L, item.second);
        lua_settable(L, top);
    }
}

GAny LuaTable::toObject() const
{
    if (isArray()) {
        return toArray();
    }
    GAny obj = GAny::object();
    for (const auto &item: mTable) {
        if (item.first.isString()) {
            std::string key = item.first.toString();
            if (item.second.is<LuaTable>()) {
                obj[key] = item.second.as<LuaTable>()->toObject();
            } else {
                obj[key] = item.second;
            }
        }
    }
    return obj;
}

std::unique_ptr<LuaTableIterator> LuaTable::iterator()
{
    return std::make_unique<LuaTableIterator>(mTable);
}

void LuaTable::writeToByteArray(GByteArray &ba, const LuaTable &table)
{
    auto &tb = table.mTable;
    ba.write(static_cast<int32_t>(tb.size()));
    for (const auto &item: tb) {
        if (item.first.is<LuaTable>()) {
            ba.write(static_cast<uint8_t>(1));
            writeToByteArray(ba, *item.first.as<LuaTable>());
        } else {
            ba.write(static_cast<uint8_t>(0));
            ba.write(item.first);
        }
        if (item.second.is<LuaTable>()) {
            ba.write(static_cast<uint8_t>(1));
            writeToByteArray(ba, *item.second.as<LuaTable>());
        } else {
            ba.write(static_cast<uint8_t>(0));
            ba.write(item.second);
        }
    }
}

LuaTable LuaTable::readFromByteArray(GByteArray &ba)
{
    LuaTable table;
    int32_t size;
    ba.read(size);
    for (int32_t i = 0; i < size; i++) {
        uint8_t keyType;
        ba.read(keyType);
        GAny key;
        if (keyType == 1) {
            key = readFromByteArray(ba);
        } else {
            ba.read(key);
        }

        uint8_t valType;
        ba.read(valType);
        GAny val;
        if (valType == 1) {
            val = readFromByteArray(ba);
        } else {
            ba.read(val);
        }

        table.mTable.emplace_back(key, val);
    }
    return table;
}

void LuaTable::parse(lua_State *L, int idx)
{
    mTable.clear();
    if (!lua_istable(L, idx)) {
        return;
    }

    lua_pushnil(L); // Begin
    while (lua_next(L, idx)) {
        lua_pushvalue(L, -2); // Place the key at the top of the stack to prevent it from being modified
        GAny key = GAnyLuaImpl::makeLuaObjectToGAny(L, lua_gettop(L));
        lua_pop(L, 1);
        GAny val = GAnyLuaImpl::makeLuaObjectToGAny(L, lua_gettop(L));
        lua_pop(L, 1);
        mTable.emplace_back(key, val);
    }
}

bool LuaTable::isArray() const
{
    for (const auto &item: mTable) {
        if (!item.first.isInt()) {
            return false;
        }
    }
    // Empty also counts as an array
    return true;
}


GAny LuaTable::toArray() const
{
    std::vector<GAny> array;

    std::vector<std::pair<int64_t, GAny> > temp;
    temp.reserve(mTable.size());
    for (const auto &item: mTable) {
        if (item.first.isInt()) {
            temp.emplace_back(item.first.toInt64(), item.second);
        }
    }
    if (temp.empty()) {
        return array;
    }

    std::sort(temp.begin(), temp.end(), [](const auto &a, const auto &b) {
        return a.first < b.first;
    });

    const int64_t begin = temp[0].first;
    if (begin != 0 && begin != 1) {
        return array;
    }

    int64_t index = begin;
    for (const auto &it: temp) {
        if (it.first != index) {
            break;
        }
        if (it.second.is<LuaTable>()) {
            array.push_back(it.second.as<LuaTable>()->toObject());
        } else {
            array.push_back(it.second);
        }

        index++;
    }

    return array;
}

bool LuaTable::compareKey(const GAny &k1, const GAny &k2)
{
    if (k1.type() == k2.type()) {
        switch (k1.type()) {
            case AnyType::undefined_t:
            case AnyType::null_t: {
                return true;
            }
            case AnyType::boolean_t:
            case AnyType::int_t:
            case AnyType::float_t:
            case AnyType::double_t:
            case AnyType::string_t: {
                return k1 == k2;
            }
            default: {
                return k1.getPointer() == k2.getPointer();
            }
        }
    }
    return false;
}

bool LuaTable::isNonStringType(const GAny &v)
{
    return v.type() == AnyType::int_t
           || v.type() == AnyType::float_t
           || v.type() == AnyType::double_t
           || v.type() == AnyType::boolean_t
           || v.is<LuaTable>();
}
