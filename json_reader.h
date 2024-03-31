#pragma once
#include "map_renderer.h"
#include "json.h"
#include "svg.h"
#include "json_builder.h"
#include "transport_router.h"
#include "serialization.h"

namespace json_reader {
	
	class JsonReader {
	public:
		//считывает все данные из входного потока
        explicit  JsonReader(transport_catalogue::TransportCatalogue& catalogue,
                             transport_router_::TransportRouter& router,
                             renderer::MapRenderer& map_renderer,
                             serialize::Serializator& serializ)
            : catalogue_(catalogue) , transport_router_(router), map_renderer_(map_renderer), serializator_(serializ){}

		const std::vector<domain::BusInput> GetBuses()const;
		const std::vector<domain::StopInput> GetStops()const;
		const std::map<std::pair<std::string, std::string>, int> GetDistances()const;
		const std::vector<domain::query> GetQuery() const;

		void LoadDocument(std::istream &input);
		void ReadDocument();

		void PrintDocument(json::Document& document_answer, json::Builder& builder,
			std::vector<domain::OutputAnswers> answers_);

	private:
		void ParseBase(const json::Node& node_);
		void ParseStats(const json::Node& node_);
		void ParseSettings(const json::Node& node_);
		void ParseRoutingSettings(const json::Node& node_);

		void GetColor(const json::Node& node, svg::Color* color);

		struct CreateNode {
			friend class JsonReader;
			explicit CreateNode(transport_router_::TransportRouter& router) : transport_router_(router) {}
			json::Node operator() (int value);
			json::Node operator() (domain::StopOutput& value);
			json::Node operator() (domain::BusOutput& value);
			json::Node operator() (domain::MapOutput& value);
			json::Node operator() (domain::RouteOutput& value);
			//печать строки ошибки
			json::Node ErrorMassage(json::Builder& builder, int id);
		private:
			transport_router_::TransportRouter& transport_router_;
		};

		std::vector<domain::BusInput> buses_; // маршруты(автобусы)
		std::vector<domain::StopInput> stops_; // остановки
		std::vector<domain::query> stats_; // запрос базы
		std::map<std::pair<std::string, std::string>, int> distances_;// расстояние от остановки до остановки

		json::Document document_ = {};

		renderer::RenderSettings render_settings;
		transport_catalogue::TransportCatalogue& catalogue_;
		transport_router_::TransportRouter& transport_router_;
        renderer::MapRenderer& map_renderer_;
        serialize::Serializator& serializator_;
	};
}
