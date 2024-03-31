#include "json.h"
#include  <cctype>

using namespace std;

namespace json {

	Node::Node(Array array) :
		value_(std::move(array)) {}

	Node::Node(bool value) :
		value_(value) {}

	Node::Node(std::nullptr_t) :
		Node() {}

	Node::Node(Dict map) :
		value_(std::move(map)) {}

	Node::Node(int value) :
		value_(value) {}

	Node::Node(double value) :
		value_(value) {}

	Node::Node(std::string value) :
		value_(std::move(value)) {}

	//функции возвращаюзщие хранящееся внутри Node значение заданного типа
	int Node::AsInt() const
	{
		if (!IsInt()) {
			throw std::logic_error("Not an int"s);
		}
		return std::get<int>(value_);
	}

	bool Node::AsBool() const
	{
		if (!IsBool()) {
			throw std::logic_error("Not a bool"s);
		}
		return std::get<bool>(value_);
	}
	//Возвращает значение типа double, если внутри хранится double либо int.
	//В последнем случае возвращается приведённое в double значение.
	double Node::AsDouble() const
	{
		if (!IsDouble()) {
			throw std::logic_error("Not a double"s);
		}
		return IsPureDouble() ? std::get<double>(value_) : AsInt();
	}

	const std::string & Node::AsString() const
	{
		if (!IsString()) {
			throw std::logic_error("Not a string"s);
		}
		return std::get<std::string>(value_);
	}

	const Array & Node::AsArray() const
	{
		if (!IsArray()) {
			throw std::logic_error("Not an array"s);
		}
		return std::get<Array>(value_);
	}

	const Dict & Node::AsMap() const
	{
		if (!IsMap()) {
			throw std::logic_error("Not a dict"s);
		}
		return std::get<Dict>(value_);
	}
	
    
	namespace{
	Node LoadNode(istream& input);

	Node LoadNumber(std::istream& input) {

		std::string parsed_num;

		// Считывает в parsed_num очередной символ из input
		auto read_char = [&parsed_num, &input] {
			parsed_num += static_cast<char>(input.get());
			if (!input) {
				throw ParsingError("Failed to read number from stream"s);
			}
		};

		// Считывает одну или более цифр в parsed_num из input
		auto read_digits = [&input, read_char] {
			if (!isdigit(input.peek())) {
				throw ParsingError("A digit is expected"s);
			}
			while (isdigit(input.peek())) {
				read_char();
			}
		};

		if (input.peek() == '-') {
			read_char();
		}
		// Парсим целую часть числа
		if (input.peek() == '0') {
			read_char();
			// После 0 в JSON не могут идти другие цифры
		}
		else {
			read_digits();
		}

		bool is_int = true;
		// Парсим дробную часть числа
		if (input.peek() == '.') {
			read_char();
			read_digits();
			is_int = false;
		}

		// Парсим экспоненциальную часть числа
		if (int ch = input.peek(); ch == 'e' || ch == 'E') {
			read_char();
			if (ch = input.peek(); ch == '+' || ch == '-') {
				read_char();
			}
			read_digits();
			is_int = false;
		}

		try {
			if (is_int) {
				return Node(stoi(parsed_num));
			}
			return Node(stod(parsed_num));
		}
		catch (...) {
			throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
		}
	}

	Node LoadString(std::istream& input) {
		using namespace std::literals;

		auto it = std::istreambuf_iterator<char>(input);
		auto end = std::istreambuf_iterator<char>();
		std::string s;
		while (true) {
			if (it == end) {
				// Поток закончился до того, как встретили закрывающую кавычку?
				throw ParsingError("String parsing error");
			}
			const char ch = *it;
			if (ch == '"') {
				// Встретили закрывающую кавычку
				++it;
				break;
			}
			else if (ch == '\\') {
				// Встретили начало escape-последовательности
				++it;
				if (it == end) {
					// Поток завершился сразу после символа обратной косой черты
					throw ParsingError("String parsing error");
				}
				const char escaped_char = *(it);
				// Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
				switch (escaped_char) {
				case 'n':
					s.push_back('\n');
					break;
				case 't':
					s.push_back('\t');
					break;
				case 'r':
					s.push_back('\r');
					break;
				case '"':
					s.push_back('"');
					break;
				case '\\':
					s.push_back('\\');
					break;
				default:
					// Встретили неизвестную escape-последовательность
					throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
				}
			}
			else if (ch == '\n' || ch == '\r') {
				// Строковый литерал внутри- JSON не может прерываться символами \r или \n
				throw ParsingError("Unexpected end of line"s);
			}
			else {
				// Просто считываем очередной символ и помещаем его в результирующую строку
				s.push_back(ch);
			}
			++it;
		}

		return Node(move(s));
	}

	Node LoadNull(istream& input) {
		string s;
		char c;
		for (int i = 0; input >> c && i < 4; ++i) {
			s += c;
		}
		if (s != "null"s) {
			throw ParsingError("Unexpected value"s);
		}
		return Node();
	}

	Node LoadBool(istream& input) {
		string s;
		char c;

		for (; input >> c && s.size() < 5 && c != 'e'; s += c);
		s += c;

		if (s == "true"s) {
			return Node(true);
		}
		if (s == "false"s) {
			return Node(false);
		}
		throw ParsingError("Unexpected value"s);
	}

	Node LoadArray(istream& input) {
		Array result;
		char c;
		for (; input >> c && c != ']';) {
			if (c != ',') {
				input.putback(c);
			}
			result.push_back(move(LoadNode(input)));
		}

		if (c != ']') {
			throw ParsingError("] not expected"s);
		}

		return Node(move(result));
	}

	Node LoadDict(istream& input) {
		Dict result;
		char c;
		for (; input >> c && c != '}';) {
			if (c == ',') {
				input >> c;
			}

			string key = LoadString(input).AsString();
			input >> c;
			result.insert({ move(key), LoadNode(input) });
		}

		if (c != '}') {
			throw ParsingError("} not expected"s);
		}

		return Node(move(result));
	}

	Node LoadNode(istream& input) {
		char c;
		if (input >> c) {
			if (c == '[') {
				return LoadArray(input);
			}
			else if (c == '{') {
				return LoadDict(input);
			}
			else if (c == '"') {
				return LoadString(input);
			}
			else if (c == 'n') {
				input.putback(c);
				return LoadNull(input);
			}
			else if (c == 't' || c == 'f') {
				input.putback(c);
				return LoadBool(input);
			}
			else {
				input.putback(c);
				return LoadNumber(input);
			}
		}
		else return {};
	}

}  // namespace

//------------------------------------Node-------------------------------------

	struct PrintContext {
		std::ostream& out;
		int indent_step = 4;
		int indent = 0;

		void PrintIndent() const {
			for (int i = 0; i < indent; ++i) {
				out.put(' ');
			}
		}

		[[nodiscard]] PrintContext Indented() const {
			return { out, indent_step, indent_step + indent };
		}
	};

	void PrintNode(const Node& node, const PrintContext& ctx);

	void PrintValue(const std::nullptr_t&, const PrintContext& ctx) {
		ctx.out << "null"sv;
	}

	template <typename Value>
	void PrintValue(const Value& value, const PrintContext& ctx) {
		ctx.out << value;
	}

	void PrintValue(bool value, const PrintContext& ctx) {
		ctx.out << std::boolalpha << value;
	}

	void PrintString(const std::string& value, const PrintContext& ctx) {
		//  \n, \r, \", \t, \\.
		ctx.out << '"';
		for (const char c : value) {
			switch (c) {
			case '"':
				ctx.out << "\\\""sv;
				break;
			case '\n':
				ctx.out << "\\n"sv;
				break;
			case '\t':
				ctx.out << "\t"sv;
				break;
			case '\r':
				ctx.out << "\\r"sv;
				break;
			case '\\':
				ctx.out << "\\\\"sv;
				break;
			default:
				ctx.out << c;
				break;
			}
		}
		ctx.out << '"';
	}

	template <>
	void PrintValue<std::string>(const std::string& value, const PrintContext& ctx) {
		PrintString(value, ctx);
	}

	void PrintValue(Array nodes, const PrintContext& ctx) {
		std::ostream& out = ctx.out;
		out << "[\n"sv;
		bool flag = true;
		auto inner_ctx = ctx.Indented();
		for (const Node& node : nodes) {
			if (flag) {
				flag = false;
			}
			else {
				out << ",\n"sv;
			}
			inner_ctx.PrintIndent();
			PrintNode(node, inner_ctx);
		}
		out.put('\n');
		ctx.PrintIndent();
		out.put(']');
	}

	[[maybe_unused]] void PrintValue(Dict nodes, const PrintContext& ctx) {
		std::ostream& out = ctx.out;
		out << "{\n"sv;
		bool flag = true;
		auto inner_ctx = ctx.Indented();
		for (const auto&[key, node] : nodes) {
			if (flag) {
				flag = false;
			}
			else {
				out << ",\n"sv;
			}
			inner_ctx.PrintIndent();
			PrintString(key, ctx);
			out << ": "sv;
			PrintNode(node, inner_ctx);
		}
		out.put('\n');
		ctx.PrintIndent();
		out.put('}');
	}

	void PrintNode(const Node& node, const PrintContext& ctx) {
		std::visit(
			[&ctx](const auto& value) {
			PrintValue(value, ctx);
		},
			node.GetValue());
	}

	Document& Document::operator= (Document& other) {
		root_ = other.root_;
		return *this;
	}

	Document& Document::operator= (Node& other) {
		root_ = other;
		return *this;
	}

	Document& Document::operator= (Document&& other) {
		root_ = move(other.root_);
		return *this;
	}

	Document& Document::operator= (Node&& other) {
		root_ = move(other);
		return *this;
	}

	bool Document::operator==(const Document& lhs) const {
		return root_ == lhs.root_;
	}
	bool Document::operator!=(const Document& lhs) const {
		return !(root_ == lhs.root_);
	}

	Document::Document(Node root)
		: root_(move(root)) {
	}

	const Node& Document::GetRoot() const {
		return root_;
	}

	Document Load(istream& input) {
		return Document{ LoadNode(input) };
	}

	void Print(const Document& doc, std::ostream& output) {
		PrintNode(doc.GetRoot(), PrintContext{ output });
	}
}  