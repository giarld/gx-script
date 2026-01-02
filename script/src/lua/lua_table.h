//
// Created by Gxin on 2022/11/7.
//

#ifndef GX_SCRIPT_LUA_TABLE_H
#define GX_SCRIPT_LUA_TABLE_H

#include "gx/gany.h"
#include <gx/gbytearray.h>

#include <lua.hpp>


class LuaTableIterator;

/**
 * @class LuaTable
 * @brief The GAny packaging of Lua table data structure aims to provide a table that is out of the control of Lua garbage collector,
 *        allowing table data to be shared and passed between different threads
 */
class LuaTable
{
public:
    LuaTable();

    /**
     * @brief Build from Lua stack
     * @param L     Lua State
     * @param idx   The index of the table on the stack
     */
    explicit LuaTable(lua_State *L, int idx);

    LuaTable(const LuaTable &b);

    LuaTable(LuaTable &&b) noexcept;

    LuaTable &operator=(const LuaTable &b);

    LuaTable &operator=(LuaTable &&b) noexcept;

    static LuaTable fromGAnyObject(const GAny &obj);

public:
    GAny getItem(const GAny &key) const;

    void setItem(const GAny &key, const GAny &value);

    void delItem(const GAny &key);

    std::string toString() const;

    size_t length() const;

    /**
     * @brief Convert to Lua Table and push to Lua stack
     * @param L
     */
    void push(lua_State *L) const;

    /**
     * @brief Convert to GAnyObject, if the structure is an array,
     *        it will be converted to GAnyArray. If the key is of a non string type, it will be converted to a string
     * @return
     */
    GAny toObject() const;

    /**
     * @brief Get iterator
     * @return
     */
    std::unique_ptr<LuaTableIterator> iterator();

public:
    /**
     * @brief Serializing Write to GByteArray
     * @param ba
     * @param table
     */
    static void writeToByteArray(GByteArray &ba, const LuaTable &table);

    /**
     * @brief Read from GByteArray as LuaTable
     * @param ba
     * @return
     */
    static LuaTable readFromByteArray(GByteArray &ba);

private:
    void parse(lua_State *L, int idx);

    bool isArray() const;

    GAny toArray() const;

    static bool compareKey(const GAny &k1, const GAny &k2);

    static bool isNonStringType(const GAny &v);

private:
    std::vector<std::pair<GAny, GAny> > mTable;
};

/**
 * @class LuaTableIterator
 * @brief LuaTable iterator implemented according to the GAny iterator standard
 */
class LuaTableIterator
{
public:
    using TableType = std::vector<std::pair<GAny, GAny> >;
    using TableItem = std::pair<GAny, GAny>;

public:
    explicit LuaTableIterator(TableType &table)
        : mTable(table)
    {
        mIter = mTable.begin();
        mOpIter = mTable.end();
    }

    bool hasNext() const
    {
        return mIter != mTable.end();
    }

    TableItem next()
    {
        if (mIter == mTable.end()) {
            return std::make_pair(nullptr, nullptr);
        }
        TableItem v = *mIter;
        mOpIter = mIter;
        ++mIter;
        return v;
    }

    void remove()
    {
        if (mOpIter != mTable.end()) {
            mIter = mTable.erase(mOpIter);
            mOpIter = mTable.end();
        }
    }

    bool hasPrevious() const
    {
        return mIter != mTable.begin();
    }

    TableItem previous()
    {
        if (mIter == mTable.begin()) {
            return std::make_pair(nullptr, nullptr);
        }
        --mIter;
        mOpIter = mIter;
        TableItem v = *mIter;
        return v;
    }

    void toFront()
    {
        mIter = mTable.begin();
        mOpIter = mTable.end();
    }

    void toBack()
    {
        mIter = mTable.end();
        mOpIter = mTable.end();
    }

private:
    TableType &mTable;
    TableType::iterator mIter;
    TableType::iterator mOpIter;
};

#endif //GX_SCRIPT_LUA_TABLE_H
