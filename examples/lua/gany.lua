---@class GAny
GAny = {};

---@class GAnyClass
GAnyClass = {};

---Load the ls script file and pass env as the G environment variable
---@param name string
---@param env GAny
---@return GAny
function requireLs(name, env)
end

---Create a GAny object, and if the parameter v is valid, create the corresponding GAny object based on the parameter
---V can be number|boolean|string|function|table|GAny or non GAny userdata managed by reference counting
---@param v any
---@return GAny
function GAny._create(v)
end

---Create a GAnyObject
---When v is lua table, a GAnyObject will be created based on the table,
---requiring the structure of the table to conform to a JSON like structure with a string as the key
---@param v table
---@return GAny
---@overload fun():GAny
function GAny._object(v)
end

---Create a GAnyArray
---When v is lua table, a GAnyArray will be created based on the table, requiring the table to be a continuous array structure
---@param v table
---@return GAny
---@overload fun():GAny
function GAny._array(v)
end

---Create an undefined value for GAny
---@return GAny
function GAny._undefined()
end

---Create a GAny with an null value
---@return GAny
function GAny._null()
end

---Clone the current GAny object
---@return GAny
function GAny:_clone()
end

---Get the class type name of the current object
---@return string
function GAny:_classTypeName()
end

---Get the type name of the current object
---@return string
function GAny:_typeName()
end

---Get the current object type, which is defined in the AnyType global table
---@return number AnyType
function GAny:_type()
end

---Get the type object of the current object
---@return GAny GAnyClass
function GAny:_classObject()
end

---Obtain the length of the GAny object. If it is the container length of a GAnyObject or GAnyArray,
---attempt to call the "Length" meta method corresponding to the object type to obtain the length
---@return number
function GAny:_length()
end

---Same as GAny._length
---@return number
function GAny:_size()
end

---Determine whether the current object is the corresponding class type typeName
---@param typeName string
---@return boolean
function GAny:_is(typeName)
end

---Determine whether the current object is an undefined object
---@return boolean
function GAny:_isUndefined()
end

---Determine whether the current object is an null object
---@return boolean
function GAny:_isNull()
end

---Determine whether the current object is a function object
---@return boolean
function GAny:_isFunction()
end

---Determine whether the current object is a custom class
---@return boolean
function GAny:_isClass()
end

---Determine if the current object is a GAnyException exception
---@return boolean
function GAny:_isException()
end

---Determine whether the current object is a class member
---@return boolean
function GAny:_isProperty()
end

---Determine whether the current object is GAnyObject
---@return boolean
function GAny:_isObject()
end

---Determine whether the current object is GAnyArray
---@return boolean
function GAny:_isArray()
end

---Determine whether the current object is of Integer type
---@return boolean
function GAny:_isInt()
end

---Determine whether the current object is of floating-point type
---@return boolean
function GAny:_isFloat()
end

---Determine whether the current object is of double-precision-floating-point type
---@return boolean
function GAny:_isDouble()
end

---Determine whether the current object is a numerical type object
---@return boolean
function GAny:_isNumber()
end

---Determine whether the current object is of String type
---@return boolean
function GAny:_isString()
end

---Determine whether the current object is of Boolean type
---@return boolean
function GAny:_isBoolean()
end

---Determine whether the current object is a user object
---@return boolean
function GAny:_isUserObject()
end

---Determine if the current object is Lua table
---@return boolean
function GAny:_isTable()
end

---Convert the current object to Lua type, and if it fails, return the GAny object itself
---@return any|GAny
function GAny:_get()
end

---Get the element corresponding to the key
---@param key any
---@return GAny
function GAny:_getItem(key)
end

---Set element
---@param key any
---@param value any
---@return boolean
function GAny:_setItem(key, value)
end

---Delete the element corresponding to the key
---@param key any
---@return boolean
function GAny:_delItem(key)
end

---Determine whether the corresponding key element exists
---@param key any
---@return boolean
function GAny:_contains(key)
end

---Delete the corresponding key element, only applicable to containers
---@param key any
function GAny:_erase(key)
end

---Append elements to the end of the container, only applicable to array containers
---@param value any
function GAny:_pushBack(value)
end

---Clear all elements inside the container
---
function GAny:_clear()
end

---Get iterators (if available)
---@return GAny
---
function GAny:_iterator()
end

---Determine if the iterator has the next element
---@return boolean
---
function GAny:_hasNext()
end

---Get the next element of the iterator and move the iterator pointer forward
---@return GAny (GAnyIteratorItem)
---
function GAny:_next()
end

---Convert the current object to a string, and if it fails, an exception will be triggered
---@return string
function GAny:_toString()
end

---Convert the current object to a 32-bit integer. If it fails, an exception will be triggered
---@return number
function GAny:_toInt32()
end

---Convert the current object to a 64 bit integer. If it fails, an exception will be triggered
---@return number
function GAny:_toInt64()
end

---Convert the current object to a single precision floating-point. If it fails, an exception will be triggered
---@return number
function GAny:_toFloat()
end

---Convert the current object to a double precision floating-point. If it fails, an exception will be triggered
---@return number
function GAny:_toDouble()
end

---Convert the current object to a Boolean value, and if it fails, an exception will be triggered
---@return boolean
function GAny:_toBool()
end

---Convert the current object to a JSON string
---@return string
---@overload fun(indent:number):string
function GAny:_toJsonString()
end

---Parsing JSON as GAnyObject or GAnyArray
---@param json string
---@return GAny
function GAny._parseJson(json)
end

---Convert the current object to Lua table, only Lua table can be restored, and convert GAnyArray and GAnyObject to Lua table.
---Other types will obtain an empty table
---@return table
function GAny:_toTable()
end

---Convert the current object to a GAny object, only applicable to types that implement the ToObject meta method
---(basic data types support ToObject by default, basic types will return the original data, and other types depend on the specific implementation of ToObject)
---
function GAny:_toObject()
end

---Get debugging information for the current object (help information)
---@return string
function GAny:_dump()
end

---Calling object class member functions
---@param name string Member function name
---@vararg any Args
---@return any
function GAny:_call(name, ...)
end

---Compare whether two objects are equal
---@param lhs any
---@param rhs any
---@return boolean
function GAny._equalTo(lhs, rhs)
end

---Used to construct objects from a class, valid only if the current object is GAnyClass
---@vararg any Args
---@return GAny
function GAny:new(...)
end

---Import type based on path
---@param path string
---@return GAny
function GAny._import(path)
end

---Export class
---@param clazz GAnyClass
function GAny._export(clazz)
end

---Load native plugin
---@param path string
function GAny._load(path)
end

---Bind a member function call into a Caller
---@param obj GAny  user_obj_t
---@param methodName string
---@return GAny  A Caller
function GAny._bind(obj, methodName)
end

---Create Class
---@param classDef table Class property definition table
---@return GAnyClass
function GAnyClass.Class(classDef)
    --[[
    classDef = {
        __namespace = "Name space",
        __name = "Class name",
        __doc = "Class doc",
        __inherit = ClassA,

        __init = function(self)  -- or [MetaFunctionS.Init] = function(self, ...)
            self.c = 0;
        end,
        method1 = function(self, a, b)
            self.c = a + b;
            return self.c;
        end,
        __i_method1 = {
            doc = "Document content",
            args = {"a:number", "b:number"},
            returnType = "number"
        }
        enum1 = {
            A = 1,
            B = 2,
            C = 3
        },
        vc = {
            "get" = function(self) return self.c; end,
            "set" = function(self, v) self.c = v; end
        }
    }
    ]]
end

--- Enumeration of types
AnyType = {
    undefined_t = 0,
    null_t = 1,
    boolean_t = 2,
    int8_t = 3,
    int16_t = 4,
    int32_t = 5,
    int64_t = 6,
    float_t = 7,
    double_t = 8,
    string_t = 9,
    array_t = 10,
    object_t = 11,
    function_t = 12,
    class_t = 13,
    property_t = 14,
    enum_t = 15,
    exception_t = 16,
    user_obj_t = 17
};

--- Enumeration of meta functions
MetaFunction = {
    Init = 0,
    Negate = 1,
    Addition = 2,
    Subtraction = 3,
    Multiplication = 4,
    Division = 5,
    Modulo = 6,
    BitXor = 7,
    BitOr = 8,
    BitAnd = 9,
    BitNot = 10,
    EqualTo = 11,
    LessThan = 12,
    GetItem = 13,
    SetItem = 14,
    DelItem = 15,
    Length = 16,
    ToString = 17,
    ToInt32 = 18,
    ToInt64 = 19,
    ToDouble = 20,
    ToBoolean = 21,
    ToObject = 22,
    FromObject = 23
};

--- Enumeration of meta function string names
MetaFunctionS = {
    Init = "__init",
    Negate = "__neg",
    Addition = "__add",
    Subtraction = "__sub",
    Multiplication = "__mul",
    Division = "__div",
    Modulo = "__mod",
    BitXor = "__xor",
    BitOr = "__or",
    BitAnd = "__and",
    BitNot = "__not",
    EqualTo = "__eq",
    LessThan = "__lt",
    GetItem = "__getitem",
    SetItem = "__setitem",
    DelItem = "__delitem",
    Length = "__len",
    ToString = "__str",
    ToInt32 = "__to_int32",
    ToInt64 = "__to_int64",
    ToDouble = "__to_double",
    ToBoolean = "__to_bool",
    ToObject = "__to_object",
    FromObject = "__from_object"
};

---Output normal log
---@type function
function Log(...)
end

---Output debug log, Only debugging mode is valid
---@type function
function LogD(...)
end

---Output warning log
---@type function
function LogW(...)
end

---Output error log
---@type function
function LogE(...)
end