#pragma once

#include "transport_catalogue.h"
#include "router.h"

#include <memory>
#include <set>
#include <exception>
#include <map>


const double eps = 1e-6;
const double kmh_to_mmin = 1000 * 1.0 / 60;

namespace transport_router_ {

	struct RoutingSettings 
	{
		uint32_t bus_wait_time = 0;
		uint32_t bus_velocity = 0;
	};

	struct EdgeInfo
	{
		const domain::Stop* stop;
		const domain::Bus* bus;
		uint32_t count;
	};

	struct CompletedRoute {
		struct Line {
			const domain::Stop* stop;
			const domain::Bus* bus;
			double wait_time;
			double run_time;
			uint32_t count_stops;
		};
		double total_time;
		std::vector<Line> route;
	};

	class TransportRouter {
	public:

		explicit TransportRouter(transport_catalogue::TransportCatalogue& catalog) : catalog_(catalog) {}

		std::optional<CompletedRoute> ComputeRoute(graph::VertexId from, graph::VertexId to);
        void CreateGraph(bool flag_graph = true);
		void SetSettings(RoutingSettings&& settings);

        transport_catalog_serialize::Router Serialize (bool with_graph = false) const;
        bool Deserialize(transport_catalog_serialize::Router& router_data, bool with_graph = false);

	private:
		transport_catalogue::TransportCatalogue& catalog_;

		RoutingSettings routing_settings_;
		graph::DirectedWeightedGraph<double> graph_;
		std::unordered_map<graph::EdgeId, EdgeInfo> edges_;
		std::unique_ptr<graph::Router<double>> router_;
	};

}//namespace transport_router
