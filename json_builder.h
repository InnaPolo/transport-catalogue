#pragma once
#include "json.h"

#include <deque>

namespace json {

	class Builder 
	{
	public:
		class BaseBuilder;
		class KeyItemContext;
		class KeyValueItemContext;
		class DictItemContext;
		class ArrayItemContext;

		Builder() = default;

		Node Build();

		KeyItemContext Key(std::string key);
		Builder& Value(Node value);

		DictItemContext StartDict();
		Builder& EndDict();

		ArrayItemContext StartArray();
		Builder& EndArray();

	private:
		bool AddNewNode(Node node);

		Node root_;  //��� �������������� ������
		std::deque<Node*> nodes_stack_; //���� ���������� �� �� ������� JSON

		struct KeyFlag {
			std::string value;
			bool flag = false; //������ �� �������
		};
		KeyFlag key_; // ���� ��� �������
	};

	class Builder::BaseBuilder {
	public:
		BaseBuilder(Builder &builder) : builder_{ builder } {}
	protected:
		KeyItemContext Key(std::string key);

		DictItemContext StartDict();
		Builder& EndDict();

		ArrayItemContext StartArray();
		Builder& EndArray();

		Builder& builder_;
	};

	class Builder::KeyValueItemContext final : public BaseBuilder{
	public:
		using BaseBuilder::BaseBuilder;
		using BaseBuilder::Key;
		using BaseBuilder::EndDict;
	};

	class Builder::KeyItemContext final : public BaseBuilder{
	public:
		using BaseBuilder::BaseBuilder;
		KeyValueItemContext Value(Node value);
		using BaseBuilder::StartDict;
		using BaseBuilder::StartArray;
	};

	class Builder::DictItemContext final : public BaseBuilder{
	public:
		using BaseBuilder::BaseBuilder;
		using BaseBuilder::Key;
		using BaseBuilder::EndDict;
	};

	class Builder::ArrayItemContext final : public BaseBuilder{
	public:
		using BaseBuilder::BaseBuilder;
		ArrayItemContext Value(Node value);
		using BaseBuilder::StartDict;
		using BaseBuilder::StartArray;
		using BaseBuilder::EndArray;
	};
}