#pragma once
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include <variant>

using namespace std::literals;
using namespace std::string_view_literals;

namespace json {

	class Node;
	// Сохраните объявления Dict и Array без изменения
	using Dict = std::map<std::string, Node>;
	using Array = std::vector<Node>;

	// Эта ошибка должна выбрасываться при ошибках парсинга JSON
	class ParsingError : public std::runtime_error {
	public:
		using runtime_error::runtime_error;
	};

	using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

	class Node {
	public:

		Node() = default;
		Node(Array array);
		Node(Dict map);
		Node(bool value);
		Node(std::nullptr_t);
		Node(int value);
		Node(double value);
		Node(std::string value);
		//возвращает вариант
		[[nodiscard]] const Value& GetValue() const { return value_; }
		//Следующие методы Node сообщают, хранится ли внутри значение некоторого типа
		[[nodiscard]] bool IsInt() const { return std::holds_alternative<int>(value_); }
		[[nodiscard]] bool IsPureDouble() const { return std::holds_alternative<double>(value_); }
		[[nodiscard]] bool IsDouble() const { return IsInt() || IsPureDouble(); }
		[[nodiscard]] bool IsBool() const { return std::holds_alternative<bool>(value_); }
		[[nodiscard]] bool IsString() const { return std::holds_alternative<std::string>(value_); }
		[[nodiscard]] bool IsNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
		[[nodiscard]] bool IsArray() const { return std::holds_alternative<Array>(value_); }
		[[nodiscard]] bool IsMap() const { return std::holds_alternative<Dict>(value_); }

		//которые возвращают хранящееся внутри Node значение заданного типа
		//Если внутри содержится значение другого типа, должно выбрасываться исключение std::logic_error.
		int AsInt() const;
		bool AsBool() const;
		double AsDouble() const;
		const std::string& AsString() const;
		const Array& AsArray() const;
		const Dict& AsMap() const;

		bool operator==(const Node &lhs) const {
			return value_ == lhs.value_;
		}
		bool operator!=(const Node &lhs) const {
			return !(value_ == lhs.value_);
		}
	private:
		Value value_;
	};

	class Document {
	public:
		Document() = default;
		explicit Document(Node root);
		const Node& GetRoot() const ;

		Document& operator= (Document& other);
		Document& operator= (Node& other);
		Document& operator= (Document&& other);
		Document& operator= (Node&& other);

		bool operator==(const Document& lhs) const;
		bool operator!=(const Document& lhs) const;

	private:
		Node root_ = {};
	};

	Document Load(std::istream& input);

	void Print(const Document& doc, std::ostream& output);

}  
