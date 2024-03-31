#include "serialization.h"
#include <fstream>

namespace serialize {

size_t Serializator::Serialize(bool with_graph) const
 {
    transport_catalog_serialize::AllData all_data;
    *all_data.mutable_catalog() = catalog_.Serialize();
    *all_data.mutable_render_settings() = renderer_.Serialize();
    *all_data.mutable_router_data() = transport_router_.Serialize(with_graph);

    std::ofstream out (path_to_serialize_, std::ios::binary | std::ios::trunc);
    all_data.SerializePartialToOstream(&out);
    return std::filesystem::file_size(path_to_serialize_);
}

bool Serializator::Deserialize(bool with_graph) {
    transport_catalog_serialize::AllData all_data;
    std::ifstream in (path_to_serialize_, std::ios::binary);

    all_data.ParseFromIstream(&in);
    catalog_.Deserialize(*all_data.mutable_catalog());
    renderer_.Deserialize(*all_data.mutable_render_settings());
    transport_router_.Deserialize(*all_data.mutable_router_data(), with_graph);
    return true;
}
}
