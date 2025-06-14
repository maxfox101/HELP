#include "json.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace std;

namespace json {

Node::Node(nullptr_t) : value_(nullptr) {}
Node::Node(Array array) : value_(move(array)) {}
Node::Node(Dict map) : value_(move(map)) {}
Node::Node(bool value) : value_(value) {}
Node::Node(int value) : value_(value) {}
Node::Node(double value) : value_(value) {}
Node::Node(const char* value) : value_(string(value)) {}
Node::Node(string value) : value_(move(value)) {}

// Реализация новых конструкторов
Node::Node(std::initializer_list<Node> list) : value_(Array(list)) {}
Node::Node(std::initializer_list<std::pair<const std::string, Node>> list) : value_(Dict(list)) {}

bool Node::IsNull() const { return holds_alternative<nullptr_t>(value_); }
bool Node::IsArray() const { return holds_alternative<Array>(value_); }
bool Node::IsMap() const { return holds_alternative<Dict>(value_); }
bool Node::IsBool() const { return holds_alternative<bool>(value_); }
bool Node::IsInt() const { return holds_alternative<int>(value_); }
bool Node::IsDouble() const { return holds_alternative<double>(value_) || IsInt(); }
bool Node::IsPureDouble() const { return holds_alternative<double>(value_); }
bool Node::IsString() const { return holds_alternative<string>(value_); }

const Array& Node::AsArray() const {
    if (!IsArray()) throw logic_error("Not an array");
    return get<Array>(value_);
}

const Dict& Node::AsMap() const {
    if (!IsMap()) throw logic_error("Not a map");
    return get<Dict>(value_);
}

bool Node::AsBool() const {
    if (!IsBool()) throw logic_error("Not a bool");
    return get<bool>(value_);
}

int Node::AsInt() const {
    if (!IsInt()) throw logic_error("Not an int");
    return get<int>(value_);
}

double Node::AsDouble() const {
    if (IsInt()) return get<int>(value_);
    if (!IsDouble()) throw logic_error("Not a double");
    return get<double>(value_);
}

const string& Node::AsString() const {
    if (!IsString()) throw logic_error("Not a string");
    return get<string>(value_);
}

bool Node::operator==(const Node& rhs) const {
    return value_ == rhs.value_;
}

bool Node::operator!=(const Node& rhs) const {
    return !(*this == rhs);
}

bool Node::operator==(const Array& arr) const {
    return IsArray() && AsArray() == arr;
}

bool Node::operator==(const Dict& dict) const {
    return IsMap() && AsMap() == dict;
}

Document::Document(Node root) : root_(move(root)) {}
Document::Document(Array array) : root_(Node(move(array))) {}
Document::Document(Dict dict) : root_(Node(move(dict))) {}

const Node& Document::GetRoot() const {
    return root_;
}

namespace {

Node LoadNode(istream& input);

void SkipWhitespace(istream& input) {
    while (isspace(input.peek())) {
        input.get();
    }
}

Node LoadNumber(istream& input) {
    string num_str;
    bool is_double = false;

    auto read_char = [&] {
        char c = input.get();
        num_str += c;
        return c;
    };

    if (input.peek() == '-') {
        read_char();
    }

    while (isdigit(input.peek())) {
        read_char();
    }

    if (input.peek() == '.') {
        is_double = true;
        read_char();
        while (isdigit(input.peek())) {
            read_char();
        }
    }

    if (tolower(input.peek()) == 'e') {
        is_double = true;
        read_char();
        if (input.peek() == '+' || input.peek() == '-') {
            read_char();
        }
        while (isdigit(input.peek())) {
            read_char();
        }
    }

    istringstream num_iss(num_str);
    if (is_double) {
        double num;
        num_iss >> num;
        return Node(num);
    } else {
        int num;
        num_iss >> num;
        return Node(num);
    }
}

string LoadStringToken(istream& input) {
    string line;
    bool escape = false;

    while (true) {
        char c = input.get();
        if (c == '\\' && !escape) {
            escape = true;
            continue;
        }

        if (c == '"' && !escape) {
            break;
        }

        if (escape) {
            switch (c) {
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case '"': c = '"'; break;
                case '\\': c = '\\'; break;
                default: throw ParsingError("Invalid escape sequence");
            }
            escape = false;
        }

        line += c;
    }

    return line;
}

Node LoadString(istream& input) {
    if (input.get() != '"') {
        throw ParsingError("String should start with \"");
    }
    return Node(LoadStringToken(input));
}

Node LoadArray(istream& input) {
    Array result;

    if (input.get() != '[') {
        throw ParsingError("Array should start with [");
    }

    SkipWhitespace(input);
    if (input.peek() == ']') {
        input.get();
        return Node(move(result));
    }

    while (true) {
        SkipWhitespace(input);
        result.push_back(LoadNode(input));
        SkipWhitespace(input);

        char c = input.get();
        if (c == ']') {
            break;
        } else if (c != ',') {
            throw ParsingError("Expected ',' or ']' in array");
        }
    }

    return Node(move(result));
}

Node LoadDict(istream& input) {
    Dict result;

    if (input.get() != '{') {
        throw ParsingError("Dict should start with {");
    }

    SkipWhitespace(input);
    if (input.peek() == '}') {
        input.get();
        return Node(move(result));
    }

    while (true) {
        SkipWhitespace(input);
        if (input.get() != '"') {
            throw ParsingError("Dict key should start with \"");
        }

        string key = LoadStringToken(input);
        SkipWhitespace(input);

        if (input.get() != ':') {
            throw ParsingError("Expected ':' after dict key");
        }

        SkipWhitespace(input);
        result.emplace(move(key), LoadNode(input));
        SkipWhitespace(input);

        char c = input.get();
        if (c == '}') {
            break;
        } else if (c != ',') {
            throw ParsingError("Expected ',' or '}' in dict");
        }
    }

    return Node(move(result));
}

Node LoadBoolOrNull(istream& input) {
    string token;
    while (isalpha(input.peek())) {
        token += static_cast<char>(input.get());
    }

    if (token == "true") {
        return Node(true);
    } else if (token == "false") {
        return Node(false);
    } else if (token == "null") {
        return Node(nullptr);
    } else {
        throw ParsingError("Unknown token: " + token);
    }
}

Node LoadNode(istream& input) {
    SkipWhitespace(input);
    char c = input.peek();

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (isdigit(c) || c == '-') {
        return LoadNumber(input);
    } else if (isalpha(c)) {
        return LoadBoolOrNull(input);
    } else {
        throw ParsingError("Unexpected character: " + string(1, c));
    }
}

void PrintNode(const Node& node, ostream& output, int indent = 0);

void PrintString(const string& value, ostream& output) {
    output << '"';
    for (char c : value) {
        switch (c) {
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            default: output << c;
        }
    }
    output << '"';
}

void PrintArray(const Array& array, ostream& output, int indent) {
    output << '[';
    bool first = true;
    for (const auto& node : array) {
        if (!first) {
            output << ',';
        }
        first = false;
        output << '\n' << string(indent + 2, ' ');
        PrintNode(node, output, indent + 2);
    }
    if (!array.empty()) {
        output << '\n' << string(indent, ' ');
    }
    output << ']';
}

void PrintDict(const Dict& dict, ostream& output, int indent) {
    output << '{';
    bool first = true;
    for (const auto& [key, node] : dict) {
        if (!first) {
            output << ',';
        }
        first = false;
        output << '\n' << string(indent + 2, ' ');
        PrintString(key, output);
        output << ": ";
        PrintNode(node, output, indent + 2);
    }
    if (!dict.empty()) {
        output << '\n' << string(indent, ' ');
    }
    output << '}';
}

void PrintNode(const Node& node, ostream& output, int indent) {
    visit([&output, indent](const auto& value) {
        using T = decay_t<decltype(value)>;

        if constexpr (is_same_v<T, nullptr_t>) {
            output << "null";
        } else if constexpr (is_same_v<T, bool>) {
            output << (value ? "true" : "false");
        } else if constexpr (is_same_v<T, int>) {
            output << value;
        } else if constexpr (is_same_v<T, double>) {
            output << value;
            if (floor(value) == value && abs(value) < 1e10) {
                output << ".0";
            }
        } else if constexpr (is_same_v<T, string>) {
            PrintString(value, output);
        } else if constexpr (is_same_v<T, Array>) {
            PrintArray(value, output, indent);
        } else if constexpr (is_same_v<T, Dict>) {
            PrintDict(value, output, indent);
        }
    }, node.GetValue());
}

}  // namespace

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

void Print(const Document& doc, ostream& output) {
    PrintNode(doc.GetRoot(), output);
}

}  // namespace json
