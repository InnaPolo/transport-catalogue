#pragma once
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <filesystem>

namespace serialize {

class Serializator {
public:
    Serializator (transport_catalogue::TransportCatalogue& catalog,
                  renderer::MapRenderer& renderer,
                  transport_router_::TransportRouter& transport_router)
        :catalog_(catalog), renderer_(renderer), transport_router_(transport_router) {}

    void SetPathToSerialize(const std::filesystem::path& path) {path_to_serialize_= path;}
    size_t Serialize(bool with_graph = false) const;
    bool Deserialize(bool with_graph = false);

private:
    std::filesystem::path path_to_serialize_;

    transport_catalogue::TransportCatalogue& catalog_;
    renderer::MapRenderer& renderer_;
    transport_router_::TransportRouter& transport_router_;
};
}
