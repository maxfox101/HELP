#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <stdexcept>
#include <initializer_list>

namespace json {

    class Node;
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;

    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    class Node {
    public:
        using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

        Node() = default;
        explicit Node(std::nullptr_t);
        explicit Node(Array array);
        explicit Node(Dict map);
        explicit Node(bool value);
        explicit Node(int value);
        explicit Node(double value);
        explicit Node(const char* value);
        explicit Node(std::string value);

        // Non-explicit constructors for implicit conversions
        Node(int value, bool is_explicit);
        Node(double value, bool is_explicit);
        Node(const char* value, bool is_explicit);
        Node(std::string value, bool is_explicit);

        bool IsNull() const;
        bool IsArray() const;
        bool IsMap() const;
        bool IsBool() const;
        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        bool IsString() const;

        const Array& AsArray() const;
        const Dict& AsMap() const;
        bool AsBool() const;
        int AsInt() const;
        double AsDouble() const;
        const std::string& AsString() const;

        const Value& GetValue() const { return value_; }

        bool operator==(const Node& rhs) const;
        bool operator!=(const Node& rhs) const;
        bool operator==(const Array& arr) const;
        bool operator==(const Dict& dict) const;

    private:
        Value value_;
    };

    class Document {
    public:
        explicit Document(Node root);
        explicit Document(Array array);
        explicit Document(Dict dict);

        const Node& GetRoot() const;

    private:
        Node root_;
    };

    Document Load(std::istream& input);
    void Print(const Document& doc, std::ostream& output);

}  // namespace json
