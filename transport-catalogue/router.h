#pragma once
#include "graph.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>
#include <transport_router.pb.h>

namespace graph {

template <typename Weight>
class Router {
private:
    using Graph = DirectedWeightedGraph<Weight>;
    struct RouteInternalData {
        Weight weight;
        std::optional<EdgeId> prev_edge;
    };
    using RoutesInternalData = std::vector<std::vector<std::optional<RouteInternalData>>>;

public:
    explicit Router(const Graph& graph);
    Router(const Graph& graph, const transport_catalog_serialize::RoutesData& routes_data);

    struct RouteInfo {
        Weight weight;
        std::vector<EdgeId> edges;
    };

    transport_catalog_serialize::RoutesData GetSerializeData() const;

    std::optional<RouteInfo> BuildRoute(VertexId from, VertexId to) const;

private:


    void InitializeRoutesInternalData(const Graph& graph) {
        const size_t vertex_count = graph.GetVertexCount();
        for (VertexId vertex = 0; vertex < vertex_count; ++vertex) {
            routes_internal_data_[vertex][vertex] = RouteInternalData{ZERO_WEIGHT, std::nullopt};
            for (const EdgeId edge_id : graph.GetIncidentEdges(vertex)) {
                const auto& edge = graph.GetEdge(edge_id);
                if (edge.weight < ZERO_WEIGHT) {
                    throw std::domain_error("Edges' weights should be non-negative");
                }
                auto& route_internal_data = routes_internal_data_[vertex][edge.to];
                if (!route_internal_data || route_internal_data->weight > edge.weight) {
                    route_internal_data = RouteInternalData{edge.weight, edge_id};
                }
            }
        }
    }

    void RelaxRoute(VertexId vertex_from, VertexId vertex_to, const RouteInternalData& route_from,
                    const RouteInternalData& route_to) {
        auto& route_relaxing = routes_internal_data_[vertex_from][vertex_to];
        const Weight candidate_weight = route_from.weight + route_to.weight;
        if (!route_relaxing || candidate_weight < route_relaxing->weight) {
            route_relaxing = {candidate_weight,
                              route_to.prev_edge ? route_to.prev_edge : route_from.prev_edge};
        }
    }

    void RelaxRoutesInternalDataThroughVertex(size_t vertex_count, VertexId vertex_through) {
        for (VertexId vertex_from = 0; vertex_from < vertex_count; ++vertex_from) {
            if (const auto& route_from = routes_internal_data_[vertex_from][vertex_through]) {
                for (VertexId vertex_to = 0; vertex_to < vertex_count; ++vertex_to) {
                    if (const auto& route_to = routes_internal_data_[vertex_through][vertex_to]) {
                        RelaxRoute(vertex_from, vertex_to, *route_from, *route_to);
                    }
                }
            }
        }
    }

    RoutesInternalData SetDeserializeData(const transport_catalog_serialize::RoutesData& data) const{
        RoutesInternalData routes_internal_data;
        routes_internal_data.reserve(data.data_size());
        for (int i = 0; i < data.data_size(); ++i) {
            const transport_catalog_serialize::ArrayRouteInternalData& array_in = data.data(i);
            std::vector<std::optional<RouteInternalData>> array_to_write;
            array_to_write.reserve(array_in.data_size());
            for (int j = 0; j < array_in.data_size(); ++j) {
                if (array_in.data(j).has_value()){
                    const transport_catalog_serialize::RouteInternalData& route_in = array_in.data(j);
                    std::optional<EdgeId> prev_edge;
                    if (route_in.prev_edge() == -1) {
                        prev_edge = std::nullopt;
                    } else {
                        prev_edge = static_cast<EdgeId>(route_in.prev_edge());
                    }
                    array_to_write.push_back(RouteInternalData{route_in.weight(), prev_edge});
                } else {
                    array_to_write.push_back(std::nullopt);
                }
            }
            routes_internal_data.push_back(move(array_to_write));
        }
        return routes_internal_data;
    }

    static constexpr Weight ZERO_WEIGHT{};
    const Graph& graph_;
    RoutesInternalData routes_internal_data_;
};
template <typename Weight>
transport_catalog_serialize::RoutesData Router<Weight>::GetSerializeData() const {
    transport_catalog_serialize::RoutesData data_out;
    for (const std::vector<std::optional<RouteInternalData>>& array_route_internal_data : routes_internal_data_) {
        transport_catalog_serialize::ArrayRouteInternalData array_out;
        for (const std::optional<RouteInternalData>& route : array_route_internal_data) {
            transport_catalog_serialize::RouteInternalData route_out;
            if (route) {
                route_out.set_weight(route->weight);
                if (route->prev_edge) {
                    route_out.set_prev_edge(*route->prev_edge);
                } else {
                    route_out.set_prev_edge(-1);
                }
                route_out.set_has_value(true);
            } else {
                route_out.set_has_value(false);
            }
            array_out.add_data();
            *array_out.mutable_data(array_out.data_size()-1) = std::move(route_out);
        }
        data_out.add_data();
        *data_out.mutable_data(data_out.data_size()-1) = std::move(array_out);
    }
    return data_out;
}

template <typename Weight>
Router<Weight>::Router(const Graph& graph)
    : graph_(graph)
    , routes_internal_data_(graph.GetVertexCount(),
                            std::vector<std::optional<RouteInternalData>>(graph.GetVertexCount()))
{
    InitializeRoutesInternalData(graph);

    const size_t vertex_count = graph.GetVertexCount();
    for (VertexId vertex_through = 0; vertex_through < vertex_count; ++vertex_through) {
        RelaxRoutesInternalDataThroughVertex(vertex_count, vertex_through);
    }
}
template <typename Weight>
Router<Weight>::Router(const Graph& graph, const transport_catalog_serialize::RoutesData& routes_data)
    :graph_(graph)
    ,routes_internal_data_(SetDeserializeData(routes_data)){}

template <typename Weight>
std::optional<typename Router<Weight>::RouteInfo> Router<Weight>::BuildRoute(VertexId from,
                                                                             VertexId to) const {
    const auto& route_internal_data = routes_internal_data_.at(from).at(to);
    if (!route_internal_data) {
        return std::nullopt;
    }
    const Weight weight = route_internal_data->weight;
    std::vector<EdgeId> edges;
    for (std::optional<EdgeId> edge_id = route_internal_data->prev_edge;
         edge_id;
         edge_id = routes_internal_data_[from][graph_.GetEdge(*edge_id).from]->prev_edge)
    {
        edges.push_back(*edge_id);
    }
    std::reverse(edges.begin(), edges.end());

    return RouteInfo{weight, std::move(edges)};
}
}  
