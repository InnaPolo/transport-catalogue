#include "svg.h"
#include <string_view>

using namespace std::literals;
using namespace std::literals::string_view_literals;

namespace svg {
	inline std::ostream& operator<<(std::ostream& out, StrokeLineCap stroke_line_cap)
	{
		if (stroke_line_cap == StrokeLineCap::BUTT) {
			out << "butt"sv;
		}
		else if (stroke_line_cap == StrokeLineCap::ROUND) {
			out << "round"sv;
		}
		else if (stroke_line_cap == StrokeLineCap::SQUARE) {
			out << "square"sv;
		}
		return out;
	}
	inline std::ostream& operator<<(std::ostream& out, StrokeLineJoin stroke_line_join)
	{
		if (stroke_line_join == StrokeLineJoin::ARCS) {
			out << "arcs"sv;
		}
		else if (stroke_line_join == StrokeLineJoin::BEVEL) {
			out << "bevel"sv;
		}
		else if (stroke_line_join == StrokeLineJoin::MITER) {
			out << "miter"sv;
		}
		else if (stroke_line_join == StrokeLineJoin::MITER_CLIP) {
			out << "miter-clip"sv;
		}
		else if (stroke_line_join == StrokeLineJoin::ROUND) {
			out << "round"sv;
		}
		return out;
	}

	void Object::Render(const RenderContext& context) const {
		context.RenderIndent();

		// Делегируем вывод тега своим подклассам
		RenderObject(context);

		context.out << std::endl;
	}

	// ---------- Circle ------------------

	Circle& Circle::SetCenter(Point center) {
		center_ = center;
		return *this;
	}

	Circle& Circle::SetRadius(double radius) {
		radius_ = radius;
		return *this;
	}

	void Circle::RenderObject(const RenderContext& context) const {
		auto& out = context.out;
		out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
		out << "r=\""sv << radius_ << "\""sv;
		// Выводим атрибуты, унаследованные от PathProps
		RenderAttrs(context.out);
		out << "/>"sv;
	}
	
	Polyline & Polyline::AddPoint(Point point)
	{
		points_.push_back(point);
		return *this;
	}

	void Polyline::RenderObject(const RenderContext & context) const
	{
		//<polyline points="100,100 150,25 150,75 200,0" fill="none" stroke="black" />
		auto& out = context.out;
		out << "<polyline points=\""sv;
		int count = 0;
		for (const auto& point : points_)
		{
			count++;
			out << point.x << "," << point.y;
			if (count != static_cast<int>(points_.size())) {
				out << " ";
			}
		}
		out << "\"";
		// Выводим атрибуты, унаследованные от PathProps
		RenderAttrs(context.out);
		out << "/>";
	}
	
	Text & Text::SetPosition(Point pos)
	{
		position_ = pos;
		return *this;
	}

	Text & Text::SetOffset(Point offset)
	{
		offset_ = offset;
		return *this;
	}

	Text & Text::SetFontSize(uint32_t size)
	{
		font_size_ = size;
		return *this;
	}

	Text & Text::SetFontFamily(std::string font_family)
	{
		font_family_ = font_family;
		return *this;
	}

	Text & Text::SetFontWeight(std::string font_weight)
	{
		font_weight_ = font_weight;
		return *this;
	}

	Text & Text::SetData(std::string data)
	{
		data_ = data;
		ParsingStringData(data_);
		//std::cout << data_ << std::endl;
		return *this;
	}

	void Text::RenderObject(const RenderContext & context) const
	{
		auto& out = context.out;
		out << "<text";
		// Выводим атрибуты, унаследованные от PathProps
		RenderAttrs(context.out);
        //
		out << " x=\"" << position_.x << "\" y=\"" << position_.y 
			<< "\" dx=\"" << offset_.x<< "\" dy=\"" << offset_.y
			<< "\" font-size=\"" << font_size_ << "\"";

		if (font_family_ != "")
			out << " font-family=\"" << font_family_ << "\"";

		if (font_weight_ != "")
			out << " font-weight=\"" << font_weight_ << "\"";
	
		out << ">" << data_ << "</text>";
	}

	void Text::ParsingStringData(std::string& data) const {
		size_t pos = 0;
		while ((pos = data.find('&', pos + 1)) != data.npos) {
			data.insert(pos + 1, "&amp;");
			data.erase(pos, 1);
		}

		while ((pos = data.find_first_of(R"("'<>)")) != data.npos) {
			switch (data[pos])
			{
			case ('\"'):
				data.insert(pos + 1, "&quot;");
				break;

			case ('\''):
				data.insert(pos + 1, "&apos;");
				break;

			case ('<'):
				data.insert(pos + 1, "&lt;");
				break;

			case ('>'):
				data.insert(pos + 1, "&gt;");
				break;
			}
			data.erase(pos, 1);
		}
	}
	
	void Document::Render(std::ostream & out) const
	{
		out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv; //1
		out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv; //2
		//3...n-1
		for (const auto& obj : objects_)
		{
			RenderContext rc(out, 2, 2);
			obj->Render(rc);
		}
        out << "</svg>"sv; //n
	}
}  
