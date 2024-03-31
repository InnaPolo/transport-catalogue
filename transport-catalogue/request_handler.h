#pragma once
#include "json_reader.h"

class RequestHandler {
public:
    explicit RequestHandler(transport_catalogue::TransportCatalogue &db )
         : db_(db), reader_(db_, router_,map_renderer_, serializator_), map_renderer_(db_),
           router_(db_),serializator_(db_, map_renderer_,router_){}

    RequestHandler(transport_catalogue::TransportCatalogue &db,
                   std::istream &input, std::ostream &output)
        : input_(input),output_(output), db_(db), reader_(db_, router_,map_renderer_,serializator_), map_renderer_(db_),
          router_(db_),serializator_(db_, map_renderer_,router_){}

    RequestHandler(transport_catalogue::TransportCatalogue &db,
                   std::istream &input)
        : input_(input), db_(db), reader_(db_, router_,map_renderer_,serializator_), map_renderer_(db_),
          router_(db_),serializator_(db_, map_renderer_,router_){}

    void ReadInputDocument();
    //печать ответа
    void PrintAnswers();

	void AddInfo();

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<const domain::Bus *> GetBusStat(const std::string_view &bus_name) const;

    // Возвращает маршруты, проходящие через
    std::optional<std::set<std::string_view>> GetBusesByStop(const std::string_view &stop_name) const;

	// Этот метод будет нужен в следующей части итогового проекта
	void RenderMapGlob();
	//создание графа
    void CreateGraph(bool flag_graph = true);

    size_t Serialize(bool with_graph = false) const  {return serializator_.Serialize(with_graph);}
    bool Deserialize(bool with_graph = false)  {return serializator_.Deserialize(with_graph); }

    void GetAnswers();

private:
	//функции добавления информации в транспортный справочник
	void AddStops();
	void AddBuses();
	void AddDistances();

	//потоки ввода/вывода
	std::istream &input_ = std::cin;
	std::ostream &output_ = std::cout;

	std::string RenderMap();
    //RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
	transport_catalogue::TransportCatalogue&db_;
    json_reader::JsonReader reader_;
    renderer::MapRenderer map_renderer_;
	transport_router_::TransportRouter router_;
    serialize::Serializator serializator_;

	std::vector<domain::OutputAnswers> answers_;
};
