#include <Ark/VM/FFI.hpp>

#include <iostream>
#include <Ark/Log.hpp>
#undef abs
#include <cmath>

#define FFI_Function(name) Value name(const std::vector<Value>& n)

namespace Ark::internal::FFI
{
    extern const Value falseSym = Value(NFT::False);
    extern const Value trueSym  = Value(NFT::True);
    extern const Value nil      = Value(NFT::Nil);

    extern const std::vector<std::pair<std::string, Value>> builtins = {
        { "false",  falseSym },
        { "true",   trueSym },
        { "nil",    nil },
        { "append", Value(&append) },
        { "concat", Value(&concat) },
        { "list",   Value(&list) },
        { "print",  Value(&print) },
        { "input",  Value(&input) }
    };

    extern const std::vector<std::string> operators = {
        "+", "-", "*", "/",
        ">", "<", "<=", ">=", "!=", "=",
        "len", "empty?", "firstof", "tailof", "headof",
        "nil?", "assert",
        "toNumber", "toString",
        "@", "and", "or", "mod",
    };
    
    // ------------------------------
    
    FFI_Function(len)
    {
        if (n[0].valueType() == ValueType::List)
            return Value(static_cast<int>(n[0].const_list().size()));
        if (n[0].valueType() == ValueType::String)
            return Value(static_cast<int>(n[0].string().size()));

        throw Ark::TypeError("Argument of len must be a list or a String");
    }
    
    FFI_Function(empty)
    {
        if (n[0].valueType() != ValueType::List)
            throw Ark::TypeError("Argument of empty must be a list");
        
        return (n[0].const_list().size() == 0) ? trueSym : falseSym;
    }
    
    FFI_Function(firstof)
    {
        if (n[0].valueType() != ValueType::List)
            throw Ark::TypeError("Argument of firstof must be a list");
        
        return n[0].const_list()[0];
    }
    
    FFI_Function(tailof)
    {
        if (n[0].valueType() != ValueType::List)
            throw Ark::TypeError("Argument of tailof must be a list");
        
        if (n[0].const_list().size() < 2)
            return nil;
        
        Value r = n[0];
        r.list().erase(r.const_list().begin());
        return r;
    }

    FFI_Function(headof)
    {
        if (n[0].valueType() != ValueType::List)
            throw Ark::TypeError("Argument of headof must be a list");
        
        if (n[0].const_list().size() < 2)
            return nil;
        
        Value r = n[0];
        r.list().erase(r.const_list().end());
        return r;
    }

    FFI_Function(isnil)
    {
        return n[0] == nil ? trueSym : falseSym;
    }

    // ------------------------------

    FFI_Function(assert_)
    {
        if (n[0] == falseSym)
        {
            if (n[1].valueType() != ValueType::String)
                throw Ark::TypeError("Second argument of assert must be a String");

            throw Ark::AssertionFailed(n[1].string());
        }
        return nil;
    }

    // ------------------------------

    FFI_Function(toNumber)
    {
        if (n[0].valueType() != ValueType::String)
            throw Ark::TypeError("Argument of toNumber must be a String");
        
        return Value(std::stod(n[0].string().c_str()));
    }

    FFI_Function(toString)
    {
        std::stringstream ss;
        ss << n[0];
        return Value(ss.str());
    }

    // ------------------------------

    FFI_Function(at)
    {
        if (n[0].valueType() != ValueType::List)
            throw Ark::TypeError("Argument 1 of @ should be a List");
        if (n[1].valueType() != ValueType::Number)
            throw Ark::TypeError("Argument 2 of @ should be a Number");
        
        return n[0].const_list()[static_cast<long>(n[1].number())];
    }

    FFI_Function(and_)
    {
        return n[0] == trueSym && n[1] == trueSym;
    }

    FFI_Function(or_)
    {
        return n[0] == trueSym || n[1] == trueSym;
    }

    FFI_Function(mod)
    {
        if (n[0].valueType() != ValueType::Number)
            throw Ark::TypeError("Arguments of mod should be Numbers");
        if (n[1].valueType() != ValueType::Number)
            throw Ark::TypeError("Arguments of mod should be Numbers");
        
        return Value(std::fmod(n[0].number(), n[1].number()));
    }

    // ------------------------------

    FFI_Function(append)
    {
        if (n[0].valueType() != ValueType::List)
            throw Ark::TypeError("First argument of append must be a list");
        
        Value r = n[0];
        for (Value::Iterator it=n.begin()+1; it != n.end(); ++it)
            r.push_back(*it);
        return r;
    }

    FFI_Function(concat)
    {
        if (n[0].valueType() != ValueType::List)
            throw Ark::TypeError("First argument of concat should be a list");
        
        Value r = n[0];
        for (Value::Iterator it=n.begin()+1; it != n.end(); ++it)
        {
            if (it->valueType() != ValueType::List)
                throw Ark::TypeError("Arguments of concat must be lists");

            for (Value::Iterator it2=it->const_list().begin(); it2 != it->const_list().end(); ++it2)
                r.push_back(*it2);
        }
        return r;
    }

    FFI_Function(list)
    {
        Value r(ValueType::List);
        for (Value::Iterator it=n.begin(); it != n.end(); ++it)
            r.push_back(*it);
        return r;
    }

    FFI_Function(print)
    {
        for (Value::Iterator it=n.begin(); it != n.end(); ++it)
            std::cout << (*it) << " ";
        std::cout << std::endl;

        return nil;
    }

    FFI_Function(input)
    {
        if (n.size() == 1)
        {
            if (n[0].valueType() != ValueType::String)
                throw Ark::TypeError("Argument of input must be of type String");
            std::cout << n[0].string();
        }

        std::string line = "";
        std::getline(std::cin, line);

        return Value(line);
    }
}
