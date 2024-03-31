#pragma once
#include "geo.h"
#include "domain.h"
#include "graph.h"
#include <utility>
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <iomanip>
#include <string>
#include <string_view>
#include <optional>
#include <iostream>
#include <transport_catalogue.pb.h>
namespace transport_catalogue {
//Класс транспортного справочника
class TransportCatalogue
{
public:
    TransportCatalogue() {}
    ~TransportCatalogue() {}
    //добавление остановки в базу
    void AddStop(std::string_view stop_name, geo::Coordinates coordinate) noexcept;
    //добавление маршрута в базу
    void AddBus(std::string_view route_name,std::vector<std::string_view>& stops, const bool& ring);
    //получение всех автобусов на остановке
    std::optional <std::set<std::string_view>> GetBusesOnStop(std::string_view stop) const;
    //задание дистанции между остановками
    void SetDistance(std::string_view stop_from,std::string_view stop_to, int distance) noexcept;
    //получение дистанции между остановками
    int GetDistance(std::string_view stop_from, std::string_view stop_to) const;
    int GetDistance(const domain::Stop* lhs, const domain::Stop* rhs) const;
    double CalculateCurvature(std::string_view name) const;
    size_t GetVertexCount() const { return vertex_count_; }
    //получение инф о автобусе и остановке
    std::optional<const domain::Bus*> GetBusInfo(std::string_view name) const;
    std::optional<const domain::Stop*> GetStopInfo(std::string_view name) const;
    auto begin() const { return sorted_buses_.begin(); }
    auto end() const { return sorted_buses_.end(); }
    size_t size() const { return sorted_buses_.size(); }
    size_t empty() const { return sorted_buses_.empty(); }

    transport_catalog_serialize::Catalog Serialize () const;
    bool Deserialize (transport_catalog_serialize::Catalog& catalog);

    std::vector<std::string_view> GetSortedStopsNames() const;

private:
    class DistanceCompare {
    public:
        bool operator() (const std::pair<const domain::Stop*, const domain::Stop*> lhs,
                         const std::pair<const domain::Stop*, const domain::Stop*> rhs) const {
            return lhs.first == rhs.first && rhs.second == lhs.second;
        }
    };
    int CalculateAllDistance(std::string_view route_name) const;
    //поиск остановки по имени
    domain::Stop* FindStop(std::string_view stop_name) const;
    //поиск маршрута по имени
    domain::Bus* FindBus(std::string_view stop_name) const;
    //сортирует по имени и возвращает вектор остановок
    std::vector<const domain::Stop*> SortStops() const;

    //---контейнеры---//
    //Stops - остановки
    std::unordered_map<std::string_view, domain::Stop*, std::hash<std::string_view>> stops_to_stop_; //словарь, хеш-таблица
    std::deque<domain::Stop> stops_;// набор остановок
    //Buses - маршруты
    std::unordered_map<std::string_view, domain::Bus*, std::hash<std::string_view>> buses_to_bus_; //словарь, хеш-таблица
    std::deque<domain::Bus> buses_; // набор маршрутов
    // автобусы на каждой остановке
    std::unordered_map<std::string_view, std::set<std::string_view>, std::hash<std::string_view>> buses_on_stops_;
    //контейнер длин
    std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, domain::Hasher, DistanceCompare> distances_;
    //сортированные автобусы
    std::vector<std::string_view> sorted_buses_;
    size_t vertex_count_ = 0;
};
}
