#pragma once
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <optional>
#include <variant>
#include <string>
#include <string_view>

using namespace std::literals;
using namespace std::literals::string_view_literals;

namespace svg {

	struct Point {
		Point() = default;
		Point(double x, double y)
			: x(x)
			, y(y) {
		}
		double x = 0;
		double y = 0;
	};

	enum class StrokeLineCap {
		BUTT,
		ROUND,
		SQUARE,
	};
	enum class StrokeLineJoin {
		ARCS,
		BEVEL,
		MITER,
		MITER_CLIP,
		ROUND,
	};

	struct Rgb {
		Rgb() = default;
        Rgb(uint r, uint g, uint b) :red(r), green(g), blue(b) {}
		Rgb(uint8_t  r, uint8_t  g, uint8_t  b) : red(r), green(g), blue(b) {};
		Rgb(int  r, int  g, int  b) : red(static_cast<uint8_t>(r)), green(static_cast<uint8_t>(g)), blue(static_cast<uint8_t>(b)) {};

		uint8_t  red = 0;
		uint8_t  green = 0;
		uint8_t  blue = 0;
	};

	struct Rgba {
		Rgba() = default;
        Rgba(uint r, uint g, uint b, double o)
        :red(r), green(g), blue(b), opacity(o){}
		Rgba(uint8_t  r, uint8_t  g, uint8_t  b, double gradient) : red(r), green(g), blue(b), opacity(gradient) {};
		Rgba(int  r, int  g, int  b, double gradient) : red(static_cast<uint8_t>(r)), green(static_cast<uint8_t>(g)),
			blue(static_cast<uint8_t>(b)), opacity(gradient) {};
		//uint8_t || short
		uint8_t  red = 0;
		uint8_t  green = 0;
		uint8_t  blue = 0;
		double opacity = 1.0;
	};
	
	inline std::ostream& operator<<(std::ostream& out, StrokeLineCap stroke_line_cap);
	inline std::ostream& operator<<(std::ostream& out, StrokeLineJoin stroke_line_join);
	
	inline std::ostream& operator<<(std::ostream &out, Rgb rgb) {
		out << "rgb("sv << static_cast<short>(rgb.red) << ',' << static_cast<short>(rgb.green) << ',' << static_cast<short>(rgb.blue) << ')';
		return out;
	}

	inline std::ostream& operator<<(std::ostream &out, Rgba rgba) {
		out << "rgba("sv << static_cast<short>(rgba.red) << ',' << static_cast<short>(rgba.green)
			<< ',' << static_cast<short>(rgba.blue) << ',' << (rgba.opacity) << ')';
		return out;
	}

	using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
	
	inline const Color NoneColor{ "none" };

	inline void PrintColor(std::ostream &out, Rgb rgb) {
		out << rgb;
	}

	inline void PrintColor(std::ostream &out, Rgba rgba) {
		out << rgba;
	}

	inline void PrintColor(std::ostream &out, std::monostate) {
		out << "none"sv;
	}

	inline void PrintColor(std::ostream &out, std::string &string_color) {
		out << string_color;
	}

	inline std::ostream &operator<<(std::ostream &out, Color color) {
		std::visit([&out](auto value) {
			// Это универсальная лямбда-функция (generic lambda).
			// Внутри неё нужная функция PrintRoots будет выбрана за счёт перегрузки функций.
			PrintColor(out, value); }, color);
		return out;
	}
	
	template <typename Owner>
	class PathProps {
	public:
		Owner& SetFillColor(Color color) {
			fill_color_ = std::move(color);
			return AsOwner();
		}
		Owner& SetStrokeColor(Color color) {
			stroke_color_ = std::move(color);
			return AsOwner();
		}
		//задаёт значение свойства stroke-width — толщину линии
		Owner& SetStrokeWidth(double width)
		{
			stroke_width = std::move(width);
			return AsOwner();
		}
		//задаёт значение свойства stroke-linecap — тип формы конца линии. 
		Owner& SetStrokeLineCap(StrokeLineCap line_cap)
		{
			stroke_linecap = std::move(line_cap);
			return AsOwner();
		}
		//задаёт значение свойства stroke-linejoin — тип формы соединения линий. 
		Owner& SetStrokeLineJoin(StrokeLineJoin line_join)
		{
			stroke_linejoin = std::move(line_join);
			return AsOwner();
		}
	protected:
		~PathProps() = default;

		void RenderAttrs(std::ostream& out) const {
			using namespace std::literals;
			//По умолчанию свойство не выводится
			if (fill_color_ != std::nullopt) {
				out << " fill=\""sv << fill_color_.value() << "\""sv;
			}
			//По умолчанию свойство не выводится
			if (stroke_color_ != std::nullopt) {
				out << " stroke=\""sv << stroke_color_.value() << "\""sv;
			}
			//По умолчанию свойство не выводится
			if (stroke_width != std::nullopt) {
				out << " stroke-width=\""sv << stroke_width.value() << "\""sv;
			}
			//По умолчанию свойство не выводится
			if (stroke_linecap) {
				out << " stroke-linecap=\""sv << *stroke_linecap << "\""sv;
			}
			//По умолчанию свойство не выводится
			if (stroke_linejoin) {
				out << " stroke-linejoin=\""sv << *stroke_linejoin << "\""sv;
			}
		}

	private:
		Owner& AsOwner() {
			// static_cast безопасно преобразует *this к Owner&,
			// если класс Owner — наследник PathProps
			return static_cast<Owner&>(*this);
		}

		std::optional<Color> fill_color_;
		std::optional<Color> stroke_color_;
		std::optional<double> stroke_width;
		std::optional<StrokeLineCap> stroke_linecap;
		std::optional<StrokeLineJoin> stroke_linejoin;
	};
	/*
	 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
	 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
	 */
	struct RenderContext {
		RenderContext(std::ostream& out)
			: out(out) {
		}

		RenderContext(std::ostream& out, int indent_step, int indent = 0)
			: out(out)
			, indent_step(indent_step)
			, indent(indent) {
		}

		RenderContext Indented() const {
			return { out, indent_step, indent + indent_step };
		}

		void RenderIndent() const {
			for (int i = 0; i < indent; ++i) {
				out.put(' ');
			}
		}

		std::ostream& out;
		int indent_step = 0;
		int indent = 0;
	};

	class Object {
	public:
		void Render(const RenderContext& context) const;

		virtual ~Object() = default;

	private:
		virtual void RenderObject(const RenderContext& context) const = 0;
	};

	/*
	 * Класс Circle моделирует элемент <circle> для отображения круга
	 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
	 */
	class Circle final : public Object, public PathProps<Circle> {
	public:
		Circle& SetCenter(Point center);
		Circle& SetRadius(double radius);
		~Circle() override {}
	private:
		void RenderObject(const RenderContext& context) const override;

		Point center_;
		double radius_ = 1.0;
	};

	class Polyline final : public Object, public PathProps<Polyline> {
		void RenderObject(const RenderContext& context) const override;

		std::vector<Point> points_;
	public:
		// Добавляет очередную вершину к ломаной линии
		Polyline& AddPoint(Point point);

		~Polyline() override {}

	};

	class Text final : public Object, public PathProps<Text>
	{
		void RenderObject(const RenderContext& context) const override;

		void ParsingStringData(std::string& data) const;

		Point position_;
		Point offset_;
		uint32_t font_size_ = 1;
		std::string font_family_ = "";
		std::string font_weight_ = "";
		std::string data_ = "";
	public:
		~Text() override {}

		// Задаёт координаты опорной точки (атрибуты x и y)
		Text& SetPosition(Point pos);

		// Задаёт смещение относительно опорной точки (атрибуты dx, dy)
		Text& SetOffset(Point offset);

		// Задаёт размеры шрифта (атрибут font-size)
		Text& SetFontSize(uint32_t size);

		// Задаёт название шрифта (атрибут font-family)
		Text& SetFontFamily(std::string font_family);

		// Задаёт толщину шрифта (атрибут font-weight)
		Text& SetFontWeight(std::string font_weight);

		// Задаёт текстовое содержимое объекта (отображается внутри тега text)
		Text& SetData(std::string data);
	};
	//

	class ObjectContainer {
	public:
		// Добавляет в svg-документ объект-наследник svg::Object
		virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
		virtual ~ObjectContainer() {}
		template <typename Obj>
		void Add(Obj obj) {
			objects_.emplace_back(std::make_unique<Obj>(std::move(obj)));
		}
	protected:
		std::vector<std::unique_ptr<Object>> objects_;

	};

	class Document final : public ObjectContainer {
	public:
		// Добавляет в svg-документ объект-наследник svg::Object
		void AddPtr(std::unique_ptr<Object>&& obj) {
			objects_.emplace_back(std::move(obj));
		}
		void Render(std::ostream& out) const;
	};
	class Drawable {
	public:
		virtual void Draw(svg::ObjectContainer& container) const = 0;
		virtual ~Drawable() {}
	};
}  
