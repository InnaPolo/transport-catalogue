#pragma once
#include "transport_catalogue.h"
#include "svg.h"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <unordered_set>

inline const double EPSILON = 1e-6;

namespace renderer {

	struct RenderSettings {
		double width = 0.;
		double height = 0.;
		double padding = 0.;//отступ краёв карты от границ SVG-документа
		double line_width = 0.;//толщина линий, которыми рисуются автобусные маршруты
		double stop_radius = 0.;//радиус окружностей, которыми обозначаются остановки

		double bus_label_font_size = 0.; //размер текста, которым написаны названия автобусных маршрутов
		svg::Point bus_label_offset{ 0, 0 };//смещение надписи с названием маршрута относительно координат конечной остановки на карте.

		double stop_label_font_size = 0.;//размер текста, которым отображаются названия остановок
		svg::Point stop_label_offset{ 0, 0 };//смещение названия остановки относительно её координат на карте

		svg::Color underlayer_color;//цвет подложки под названиями остановок и маршрутов
		double underlayer_width = 0.;//толщина подложки под названиями остановок и маршрутов
		std::vector<svg::Color> color_palette; //цветовая палитра
	};

	inline bool IsZero(double value) {
		return std::abs(value) < EPSILON;
	}

	class SphereProjector {
	public:
		// points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
		template <typename PointInputIt>
		SphereProjector(PointInputIt points_begin, PointInputIt points_end,
			double max_width, double max_height, double padding);

		// Проецирует широту и долготу в координаты внутри SVG-изображения
		svg::Point operator()(geo::Coordinates coords) const;

	private:
		double padding_;
		double min_lon_ = 0.;
		double max_lat_ = 0.;
		double zoom_coeff_ = 0.;
	};

	struct CoordinatesHasher {
		size_t operator() (const geo::Coordinates& coords) const {
			return std::hash<double>{}(coords.lat) + std::hash<double>{}(coords.lng) * 37;
		}
	};
	//MapRenderer — отвечает за визуализацию карты
	class MapRenderer {
	public:
		MapRenderer() = delete;

        MapRenderer(transport_catalogue::TransportCatalogue& catalogue)
            : catalogue_(catalogue) {}

		~MapRenderer() {}

		void SetRenderSettings(RenderSettings&& settings) { settings_ = settings; }
		void Render(std::ostream& out);

        transport_catalog_serialize::RenderSettings Serialize() const;
        bool Deserialize (transport_catalog_serialize::RenderSettings& settings);

	private:
		//получение координат
		std::unordered_set<geo::Coordinates, CoordinatesHasher> GetCoordinates() const;
		
		std::pair<std::unique_ptr<svg::Text>, std::unique_ptr<svg::Text>> AddBusLabels(SphereProjector& project,
			int index_color, const domain::Stop* stop, std::string_view name);
		
		std::set<std::string_view> RenderBuses(SphereProjector& project, svg::Document& doc_to_render);

		void RenderStops(SphereProjector& project, svg::Document& doc_to_render, std::set<std::string_view> stops_in_buses);
		
		const transport_catalogue::TransportCatalogue& catalogue_;
		RenderSettings settings_;
	};
}
