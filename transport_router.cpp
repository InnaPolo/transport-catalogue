#include "transport_router.h"

namespace transport_router_ {

	using namespace std::literals;
	using namespace std::string_literals;

	std::optional<CompletedRoute> TransportRouter::ComputeRoute(graph::VertexId from, graph::VertexId to) 
	{
		//std::optional<typename Router<Weight>::RouteInfo>
		std::optional<graph::Router<double>::RouteInfo> build_route_ = router_->BuildRoute(from, to);

		if (!build_route_) {
			return std::nullopt;
		}
		//точность
		if (build_route_->weight < eps) {
			return CompletedRoute({ 0, {} });
		}

		CompletedRoute result;
		result.total_time = build_route_->weight;
		result.route.reserve(build_route_->edges.size());

		for (auto& edge : build_route_->edges) 
		{
			EdgeInfo& info = edges_.at(edge);
			double wait_time_ = static_cast<double>(routing_settings_.bus_wait_time);
			double run_time_ = graph_.GetEdge(edge).weight - routing_settings_.bus_wait_time;
			result.route.push_back(CompletedRoute::Line{ info.stop, info.bus, wait_time_, run_time_ ,info.count });

		}
		return result;
	}

    void TransportRouter::CreateGraph(bool flag_graph) {

		if (graph_.GetVertexCount() > 0) {
			throw std::logic_error("Recreate graph"s);
		}

		graph_.SetVertexCount(catalog_.GetVertexCount());
		double bus_velocity = routing_settings_.bus_velocity * kmh_to_mmin;

		for (std::string_view bus_name : catalog_)
		{
			const domain::Bus* bus = *(catalog_.GetBusInfo(bus_name));
			auto it = bus->stops.begin();
			if (it == bus->stops.end() || it + 1 == bus->stops.end()) {
				continue;
			}
			for (; it + 1 != bus->stops.end(); ++it) {
				double time = double(routing_settings_.bus_wait_time);

				for (auto next_vertex = it + 1; next_vertex != bus->stops.end(); ++next_vertex) {
					time += catalog_.GetDistance(*prev(next_vertex), *next_vertex) / bus_velocity;
					edges_[graph_.AddEdge({ (*it)->vertex_id,(*next_vertex)->vertex_id, time })]
						= { *it,bus,static_cast<uint32_t>(next_vertex - it) };
				}
			}
		}
        if (flag_graph){
            router_ = std::make_unique<graph::Router<double>>(graph_);
        }
	}

	void TransportRouter::SetSettings(RoutingSettings && settings)
	{
		routing_settings_ = std::move(settings);
	}

    transport_catalog_serialize::Router TransportRouter::Serialize(bool with_graph) const {
        transport_catalog_serialize::Router data_out;
        transport_catalog_serialize::RoutingSettings settings;
        settings.set_bus_wait_time(routing_settings_.bus_wait_time);
        settings.set_bus_velocity(routing_settings_.bus_velocity);
        *data_out.mutable_settings() = settings;
        *data_out.mutable_data() = router_->GetSerializeData();
        if (with_graph) {
            *data_out.mutable_graph() = graph_.GetSerializeData();
            std::vector<std::string_view> buses(catalog_.begin(), catalog_.end());
            std::vector<std::string_view> stops = catalog_.GetSortedStopsNames();
            for (const auto& [edge_id, edge_info] : edges_) {
                transport_catalog_serialize::EdgeInfo info_to_out;
                auto it_stop = std::lower_bound(stops.begin(), stops.end(),
                                                edge_info.stop->name, std::less<>{});
                info_to_out.set_stop(static_cast<uint32_t>(it_stop - stops.begin()));
                auto it_bus = std::lower_bound(buses.begin(), buses.end(),
                                               edge_info.bus->name, std::less<>{});
                info_to_out.set_bus(static_cast<uint32_t>(it_bus - buses.begin()));
                info_to_out.set_count(edge_info.count);
                (*data_out.mutable_graph()->mutable_info())[edge_id] = info_to_out;
            }
        }
        return data_out;
    }

    bool TransportRouter::Deserialize(transport_catalog_serialize::Router &router_data, bool with_graph) {
        routing_settings_ = {router_data.settings().bus_wait_time(),
                             router_data.settings().bus_velocity()};
        const transport_catalog_serialize::Graph& graph = router_data.graph();
        if (with_graph) {
            std::vector<std::string_view> buses(catalog_.begin(), catalog_.end());
            std::vector<std::string_view> stops = catalog_.GetSortedStopsNames();
            graph_.SetVertexCount(stops.size());
            for (int i = 0; i < graph.edges_size(); ++i) {
                uint32_t edge_id = graph_.AddEdge ({ graph.edges(i).from(),
                                                     graph.edges(i).to(),
                                                     graph.edges(i).weight()});
                const transport_catalog_serialize::EdgeInfo& edge_info = (graph.info().at(edge_id));
                edges_[edge_id] = EdgeInfo{*catalog_.GetStopInfo(stops[edge_info.stop()]),
                                           *catalog_.GetBusInfo(buses[edge_info.bus()]),
                                           edge_info.count()};
            }
        } else {
            CreateGraph(false);
        }
        router_ = std::make_unique<graph::Router<double>>(graph_, router_data.data());
        return true;
    }



}//namespace routing_settings
