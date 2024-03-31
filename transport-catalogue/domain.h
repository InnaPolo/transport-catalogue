#pragma once
#include "graph.h"
#include "geo.h"

#include <string>
#include <string_view>
#include <iostream>
#include <vector>
#include <iomanip>
#include <math.h>
#include <variant>
#include <set>

namespace domain {

struct Stop {
    std::string name;            
    geo::Coordinates coordinate = {0,0};  
    graph::VertexId vertex_id;

    friend bool operator==(const Stop &lhs, const Stop &rhs) {
        return (lhs.name == rhs.name && lhs.coordinate == rhs.coordinate);
    }
};

// Маршрут состоит из имени, типа и списка остановок
struct Bus {
    std::string name;  //номера автобуса
    bool route_type = false;  //тип маршрута
    std::vector<const Stop*> stops; // указатели должны указывать на остановки хранящиеся в этом же каталоге
    int number_stops = 0;    //количество  остновок
    int unique_stops = 0;    //уникальные остановки
    int distance = 0;        //фактическая длина маршрута
    double curvature = 0.;   //извилистость

    bool operator==(const Bus &lhs) {
        return (this->name == lhs.name);
    }
    //Bus X: R stops on route, U unique stops, L route length, C curvature.
    friend std::ostream& operator<<(std::ostream& out, const Bus& info)
    {
        out << "Bus " << info.name << ": " << info.number_stops << " stops on route, " << info.unique_stops
            << " unique stops, " << info.distance << " route length, " << std::setprecision(6) << info.curvature
            << " curvature" << std::endl;
        return out;
    }
};

//входные структуры для чтения документа
struct BusInput {
    std::string name;
    std::vector<std::string_view> stops;
    bool is_roundtrip = false;
};

struct StopInput {
    std::string name;
    geo::Coordinates coordinates;
};

struct query {
    int id;
    std::string type;
    std::string name;
    std::string from;
    std::string to;
};

struct StopOutput {
    int id;
    const std::set<std::string_view> buses;
};

struct BusOutput {
    int id;
    const Bus* bus;
};

struct MapOutput {
    int id;
    std::string map_;
};

struct RouteOutput {
    int id;
    const Stop* from;
    const Stop* to;
};

using OutputAnswers = std::variant<int, StopOutput, BusOutput, MapOutput, RouteOutput>;

class Hasher {
public:
    size_t operator() (const std::pair<const Stop*, const Stop*> element) const {
        const size_t shift = (size_t)log2(1 + sizeof(Stop));
        const size_t result = (size_t)(element.first) >> shift;
        return result + ((size_t)(element.second) >> shift) * 37;
    }
};
}
