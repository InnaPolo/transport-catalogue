#include "transport_catalogue.h"
using namespace std::literals;
using namespace std::string_view_literals;
using namespace transport_catalogue;
domain::Stop* TransportCatalogue::FindStop(std::string_view stop_name) const
{
    if (!stops_to_stop_.count(stop_name)) {
        return nullptr;
    }
    return stops_to_stop_.at(stop_name);
}
domain::Bus* TransportCatalogue::FindBus(std::string_view route_name) const
{
    if (!buses_to_bus_.count(route_name)) {
        return nullptr;
    }
    return buses_to_bus_.at(route_name);
}
std::optional<const domain::Bus*> TransportCatalogue::GetBusInfo(std::string_view name) const {
    const auto bus = FindBus(name);
    if (!bus) {
        return std::nullopt;
    }
    return bus;
}
std::optional<const domain::Stop*> TransportCatalogue::GetStopInfo(std::string_view name) const {
    const auto stop = FindStop(name);
    if (!stop) {
        return std::nullopt;
    }
    return stop;
}
void TransportCatalogue::AddStop(std::string_view stop_name, geo::Coordinates coordinate) noexcept
{
    domain::Stop stop{static_cast<std::string>( stop_name), coordinate, vertex_count_++};
    //добавление в контейнеры
    stops_.push_back(std::move(stop));
    stops_to_stop_[stops_.back().name] = &(stops_.back());
}
void TransportCatalogue::AddBus(std::string_view route_name,
                                std::vector<std::string_view>& stops, const bool& ring)
{
    auto it = std::lower_bound(sorted_buses_.begin(), sorted_buses_.end(), route_name);
    if (it != sorted_buses_.end() && *it == route_name) {
        return;
    }
    domain::Bus bus;
    //добавление имени
    bus.name = route_name;
    buses_.push_back(std::move(bus));
    //add отсортированный ветор
    sorted_buses_.emplace(it, buses_.back().name);
    std::vector<const domain::Stop*> tmp_stops(stops.size());
    //из названий в указатели на существующие остановки
    std::transform(stops.begin(), stops.end(), tmp_stops.begin(), [&](std::string_view element) {
        return stops_to_stop_[element]; });
    //если остановок нет
    if (tmp_stops.empty()) {
        buses_to_bus_.insert({ buses_.back().name, &(buses_.back()) });
        return;
    }
    //запись указателей на уникальные остановки для подсчета
    std::unordered_set<const domain::Stop*> tmp_unique_stops;
    std::for_each(tmp_stops.begin(), tmp_stops.end(), [&](const domain::Stop* stop) { tmp_unique_stops.insert(stop);});
    //добавление уникаольных остановок в деку
    buses_.back().unique_stops = static_cast<int>(tmp_unique_stops.size());
    //если линейный, то добавление обратного направления
    if (!ring) {
        //размер * 2 -1 ( A-B-C-B-A)
        tmp_stops.reserve(tmp_stops.size() * 2 - 1);
        for (auto it = tmp_stops.end() - 2; it != tmp_stops.begin(); --it) {
            tmp_stops.push_back(*it);
        }
        tmp_stops.push_back(tmp_stops.front());
    }
    else {
        buses_.back().route_type = true;
    }
    //add stops
    buses_.back().stops = std::move(tmp_stops);
    //количество остановок
    buses_.back().number_stops = static_cast<int>(buses_.back().stops.size());
    buses_to_bus_.insert({ buses_.back().name, &(buses_.back()) });
    //расстояние
    buses_.back().distance = CalculateAllDistance(route_name);
    //извилистость, то есть отношение фактической длины маршрута к географическому рассто¤нию
    buses_.back().curvature = buses_.back().distance / CalculateCurvature(route_name);
    //добавляем автобусы на каждой остановке
    for (const auto& stop : buses_.back().stops) {
        buses_on_stops_[stop->name].insert(buses_.back().name);
    }
}

std::optional<std::set<std::string_view>> TransportCatalogue::GetBusesOnStop(std::string_view name) const
{
    //нет остановки
    const auto stop = FindStop(name);
    if (!stop) {
        return std::nullopt;
    }
    std::set<std::string_view > results_null = {};
    //возвращает пустой set
    if (buses_on_stops_.count(stop->name) == 0)
        return results_null;
    //есть остновка, нет автобуссов проходящих через нее and есть автобусы проходящие через нее
    return buses_on_stops_.at(stop->name);
}

void TransportCatalogue::SetDistance(std::string_view stop_from, std::string_view stop_to, int distance) noexcept
{
    distances_[{ FindStop(stop_from), FindStop(stop_to) }] = distance;
}

int TransportCatalogue::GetDistance(const domain::Stop* lhs, const domain::Stop* rhs) const
{
    if (distances_.count({ lhs, rhs })) {
        return distances_.at({ lhs, rhs });
    }
    if (distances_.count({ rhs, lhs })) {
        return distances_.at({ rhs, lhs });
    }
    return static_cast<int>(geo::ComputeDistance(lhs->coordinate, rhs->coordinate));
}


int TransportCatalogue::GetDistance(std::string_view stop_from, std::string_view stop_to) const
{
    const auto lhs = FindStop(stop_from);
    const auto rhs = FindStop(stop_to);

    return GetDistance(lhs, rhs);
}

int TransportCatalogue::CalculateAllDistance(std::string_view route_name) const
{
    auto bus = FindBus(route_name);
    auto stops = bus->stops;
    int result = 0;
    if (!stops.empty()) {
        for (size_t i = 1; i < stops.size(); ++i) {
            result += GetDistance(stops[i-1], stops[i]);
        }
    }
    return result;
}
double TransportCatalogue::CalculateCurvature(std::string_view name) const
{
    auto bus = FindBus(name);
    auto stops = bus->stops;
    double results = 0.;
    for (auto it1 = stops.begin(), it2 = it1 + 1; it2 < stops.end(); ++it1, ++it2) {
        results += geo::ComputeDistance((*it1)->coordinate, (*it2)->coordinate);
    }
    return results;
}
std::vector<const domain::Stop*> TransportCatalogue::SortStops() const {
    std::vector<const domain::Stop*> sorted_stops;
    sorted_stops.reserve(stops_.size());
    for (const domain::Stop& stop : stops_) {
        sorted_stops.push_back(&stop);
    }
    std::sort(sorted_stops.begin(), sorted_stops.end(),
              [](const domain::Stop* lhs, const domain::Stop* rhs){return lhs->name < rhs->name;});
    return sorted_stops;
}
//----------Serialize-----------
transport_catalog_serialize::Catalog TransportCatalogue::Serialize() const
{
    std::vector<const domain::Stop*> sorted_stops = SortStops();
    //buses
    transport_catalog_serialize::BusList bus_list;
    for (const domain::Bus& bus : buses_) {
        transport_catalog_serialize::Bus bus_to_out;
        bus_to_out.set_name(bus.name);
        bus_to_out.set_route_type(bus.route_type);
        if (!bus.stops.empty ()) {
            //если некольцевой маршрут, записывается только половина остановок
            int stops_count = bus.route_type ? bus.stops.size() : bus.stops.size()/2+1;
            //список остановок состоит из их порядковых номеров в отсортированном массиве
            for (int i = 0; i < stops_count; ++i) {
                const domain::Stop* stop = bus.stops[i];
                int pos = std::lower_bound(sorted_stops.begin(), sorted_stops.end(),
                                           stop, [](const domain::Stop* lhs, const domain::Stop* rhs){
                    return lhs->name < rhs->name;}) - sorted_stops.begin();
                bus_to_out.add_stop(pos);
            }
        }
        bus_list.add_bus();
        *bus_list.mutable_bus(bus_list.bus_size()-1) = bus_to_out;
    }
    //stops
    transport_catalog_serialize::StopList stop_list;
    for (const domain::Stop& stop : stops_) {
        transport_catalog_serialize::Stop stop_to_out;
        stop_to_out.set_name(stop.name);
        stop_to_out.set_latitude(stop.coordinate.lat);
        stop_to_out.set_longitude(stop.coordinate.lng);
        stop_list.add_stop();
        *stop_list.mutable_stop(stop_list.stop_size()-1) = stop_to_out;
    }
    //distances
    transport_catalog_serialize::DistanceList distance_list;
    for (const auto&[key, value] : distances_) {
        transport_catalog_serialize::Distance distance_to_out;
        int pos = std::lower_bound(sorted_stops.begin(), sorted_stops.end(),
                                   key.first, [](const domain::Stop* lhs, const domain::Stop* rhs){
            return lhs->name < rhs->name;}) - sorted_stops.begin();
        distance_to_out.set_index_from(pos);
        pos = std::lower_bound(sorted_stops.begin(), sorted_stops.end(),
                               key.second, [](const domain::Stop* lhs, const domain::Stop* rhs){
            return lhs->name < rhs->name;}) - sorted_stops.begin();
        distance_to_out.set_index_to(pos);
        distance_to_out.set_distance(value);
        distance_list.add_distance();
        *distance_list.mutable_distance(distance_list.distance_size()-1) = distance_to_out;
    }
    transport_catalog_serialize::Catalog catalog;
    *catalog.mutable_bus_list () = bus_list;
    *catalog.mutable_stop_list () = stop_list;
    *catalog.mutable_distance_list () = distance_list;
    return catalog;
}
bool TransportCatalogue::Deserialize(transport_catalog_serialize::Catalog& catalog)
{
    //stops
    transport_catalog_serialize::StopList stop_list = catalog.stop_list ();
    for (int i = 0; i < stop_list.stop_size(); ++i) {
        const transport_catalog_serialize::Stop& stop = stop_list.stop(i);
        AddStop(stop.name(), {stop.latitude(), stop.longitude()});
    }
    std::vector<const domain::Stop*> sorted_stops = SortStops();
    //distances
    transport_catalog_serialize::DistanceList distance_list = catalog.distance_list ();
    for (int i = 0; i < distance_list.distance_size (); ++i) {
        const transport_catalog_serialize::Distance distance = distance_list.distance (i);
        distances_[{sorted_stops[distance.index_from ()], sorted_stops[distance.index_to ()]}] = distance.distance ();
}
//buses
transport_catalog_serialize::BusList bus_list = catalog.bus_list ();
for (int i = 0; i < bus_list.bus_size(); ++i) {
    const transport_catalog_serialize::Bus& bus_from_input = bus_list.bus(i);
    std::vector<std::string_view> stops_in_bus;
    stops_in_bus.reserve(bus_from_input.stop_size());
    for (int i = 0; i < bus_from_input.stop_size(); ++i) {
        stops_in_bus.push_back(sorted_stops[bus_from_input.stop(i)]->name);
    }
    AddBus(bus_from_input.name(), stops_in_bus, bus_from_input.route_type());
}
return true;
}

std::vector<std::string_view> TransportCatalogue::GetSortedStopsNames() const
{
    std::vector<std::string_view> result;
    result.reserve(stops_.size());
    for (auto& stop : stops_) {
        result.emplace_back(stop.name);
    }
    std::sort(result.begin(), result.end());
    return result;
}
