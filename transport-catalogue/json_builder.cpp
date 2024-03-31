#include "json_builder.h"
#include <stdexcept>

namespace json {
    
	using namespace std;

	Node Builder::Build() {
		if (!nodes_stack_.empty() || root_.IsNull() || key_.flag) {
			throw std::logic_error("Builder is invalid"s);
		}
		return move(root_);
	}

	Builder::ArrayItemContext Builder::StartArray() {
		if (AddNewNode(Node(Array{}))) {
			return *this;
		}
		throw logic_error("Start Array unexpected in this context"s);
	}

	Builder& Builder::EndArray() {
		if (nodes_stack_.empty()) {
			throw logic_error("Array is not started, but you say End"s);
		}
		if (nodes_stack_.back()->IsArray()) {
			nodes_stack_.pop_back();
			return *this;
		}
		throw logic_error("Unexpected End of Array"s);
	}

	Builder::DictItemContext Builder::StartDict() {
		if (AddNewNode(Node(Dict{}))) {
			return *this;
		}
		throw logic_error("Start Dict unexpected in this context"s);
	}

	Builder& Builder::EndDict() {
		if (nodes_stack_.empty()) {
			throw logic_error("Dict is not started, but you say End"s);
		}
		if (nodes_stack_.back()->IsMap() && !key_.flag) {
			nodes_stack_.pop_back();
			return *this;
		}
		throw logic_error("Unexpected End of Dict"s);
	}

	Builder::KeyItemContext Builder::Key(std::string key) {
		// ���� �� ������ ������� � ���� ��� �� ��������
		if (!nodes_stack_.empty() && nodes_stack_.back()->IsMap() && !key_.flag) {
			key_.flag = true;
			key_.value = move(key);

			return *this;
		}
		throw logic_error("Key unexpected in this context"s);
	}

	Builder& Builder::Value(Node value) 
	{
		// ���� ����� ��� ������
		if (nodes_stack_.empty() && root_.IsNull()) {
			root_ = move(value);
			return *this;
		}
		if (nodes_stack_.empty()) {
			throw logic_error("Not have container from Value"s);
		}
		//���� ������
		if (nodes_stack_.back()->IsArray()) {
			const_cast<Array&>(get<Array>(nodes_stack_.back()->GetValue())).emplace_back(value);
			return *this;
		}
		//���� �������
		if (nodes_stack_.back()->IsMap() && key_.flag) {
			const_cast<Dict&>(get<Dict>(nodes_stack_.back()->GetValue())).emplace(key_.value, value);
			key_.flag = false;
			return *this;
		}
		throw logic_error("Value unexpected in this context"s);
	}

	bool Builder::AddNewNode(Node node) 
	{
		//all empty
		if (root_.IsNull() && nodes_stack_.empty()) {
			root_ = move(node);
			nodes_stack_.push_back(&root_);
			return true;
		}

		if (node.IsArray() || node.IsMap())
		{
			//��� ���������
			if (nodes_stack_.empty()) {
				nodes_stack_.push_back(&root_);
				return true;
			}
			//���� ������
			if (nodes_stack_.back()->IsArray()) {
				auto& array = const_cast<Array&>(std::get<Array>(nodes_stack_.back()->GetValue()));
				array.emplace_back(node);
				nodes_stack_.push_back(&array.back());
				return true;
			}
			//���� �������
			if (nodes_stack_.back()->IsMap() && key_.flag) {
				auto& dict = const_cast<Dict&>(std::get<Dict>(nodes_stack_.back()->GetValue()));
				auto it = dict.emplace(key_.value, node).first;
				key_.flag = false;
				nodes_stack_.push_back(&(it->second));
				return true;
			}
		}
		return false;
	}


	Builder::KeyValueItemContext Builder::KeyItemContext::Value(Node value) {
		builder_.Value(move(value));
		return KeyValueItemContext{ builder_ };
	}

	Builder::ArrayItemContext Builder::ArrayItemContext::Value(Node value) {
		builder_.Value(std::move(value));
		return ArrayItemContext{ builder_ };
	}

	Builder::KeyItemContext Builder::BaseBuilder::Key(std::string key) {
		return builder_.Key(std::move(key));
	}

	Builder::DictItemContext Builder::BaseBuilder::StartDict() {
		return builder_.StartDict();
	}

	Builder& Builder::BaseBuilder::EndDict() {
		return builder_.EndDict();
	}

	Builder::ArrayItemContext Builder::BaseBuilder::StartArray() {
		return builder_.StartArray();
	}

	Builder& Builder::BaseBuilder::EndArray() {
		return builder_.EndArray();
	}

}