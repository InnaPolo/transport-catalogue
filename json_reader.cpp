#include "json_reader.h"
#include <sstream>
#include <filesystem>

using namespace json_reader;

const std::vector<domain::BusInput> JsonReader::GetBuses() const
{
    return buses_;
}

const std::vector<domain::StopInput> JsonReader::GetStops() const
{
    return stops_;
}

const std::map<std::pair<std::string, std::string>, int> JsonReader::GetDistances() const
{
    return distances_;
}

const std::vector<domain::query> json_reader::JsonReader::GetQuery() const
{
    return stats_;
}

void JsonReader::LoadDocument(std::istream &input)
{
    document_ = json::Load(input);
}

void JsonReader::ReadDocument()
{
    if (document_.GetRoot().IsNull())
        return;

    auto& it = document_.GetRoot().AsMap();

    if (it.count("base_requests"s))
    {
        ParseBase(it.at("base_requests"s));
    }
    if (it.count("stat_requests"s) && it.at("stat_requests"s).IsArray())
    {
        ParseStats(it.at("stat_requests"s));
    }
    if (it.count("render_settings"s))
    {
        ParseSettings(it.at("render_settings"s));
    }
    if (it.count("routing_settings"s)) {
        ParseRoutingSettings(it.at("routing_settings"s));
    }
    if (it.count ("serialization_settings"s)) {

        
        const std::filesystem::path  path = it.at("serialization_settings"s)
                .AsMap().at ("file"s).AsString();
        serializator_.SetPathToSerialize(path);
    }
}

void JsonReader::PrintDocument(json::Document& document_answer, json::Builder& builder,
                               std::vector<domain::OutputAnswers> answers_)
{
    builder.StartArray();
    for (auto& answer : answers_) {
        builder.Value(std::visit(CreateNode{ transport_router_ }, answer));//renderer_, transport_router_
    }
    builder.EndArray();
    document_answer = builder.Build();
}

void JsonReader::ParseBase(const json::Node& node_)
{
    auto& nodes = node_.AsArray();
    for (auto& node : nodes) {
        auto& tag = node.AsMap();
        if (tag.at("type"s).AsString() == "Stop"s)
        {
            const auto name_ = tag.at("name"s).AsString();
            double lat = tag.at("latitude"s).AsDouble();
            double lng = tag.at("longitude"s).AsDouble();
            stops_.push_back({ name_, lat, lng });

            //дистанция
            if (tag.count("road_distances"s)) {
                auto& distances = tag.at("road_distances"s).AsMap();
                for (const auto&[name, value] : distances) {
                    distances_[{name_, name}] = value.AsInt();
                }
            }
        }
        else if (tag.at("type"s).AsString() == "Bus"s)
        {
            const auto name = tag.at("name"s).AsString();
            const auto round = tag.at("is_roundtrip"s).AsBool();
            auto& it = tag.at("stops"s).AsArray();

            std::vector<std::string_view> stops_;
            stops_.reserve(it.size());

            for (const auto &stop : it) {
                if (stop.IsString()) {
                    stops_.push_back(stop.AsString());
                }
            }
            buses_.push_back({ name,stops_,round });
        }
        else {
            throw std::invalid_argument("Unknown type"s);
        }
    }
}

void JsonReader::ParseStats(const json::Node& node_)
{
    auto& nodes = node_.AsArray();
    for (auto& node : nodes) {
        const auto& tag = node.AsMap();
        const auto& type = tag.at("type"s).AsString();
        //{id,type,name}
        if (type == "Stop"s || type == "Bus"s)
        {
            stats_.push_back({ tag.at("id"s).AsInt(), type, tag.at("name"s).AsString(),""s,""s });
        }
        else if (type == "Map"s)
        {
            //{ "id": 1, "type": "Map" },
            stats_.push_back({ tag.at("id"s).AsInt(), type, type,""s,""s });
        }
        else if (type == "Route"s)
        {
            //{"id" : 4,"type": "Route",  "from" : "Biryulyovo Zapadnoye","to" : "Universam",}
            stats_.push_back({ tag.at("id"s).AsInt(), type,  type, tag.at("from"s).AsString(), tag.at("to"s).AsString() });
        }
        else {
            throw std::invalid_argument("Unknown type"s);
        }
    }
}

void JsonReader::ParseSettings(const json::Node& node_)
{
    renderer::RenderSettings render_settings;

    auto& settings = node_.AsMap();

    if (settings.count("width"s)) {
        render_settings.width = settings.at("width"s).AsDouble();
    }
    if (settings.count("height"s)) {
        render_settings.height = settings.at("height"s).AsDouble();
    }
    if (settings.count("padding"s)) {
        render_settings.padding = settings.at("padding"s).AsDouble();
    }
    if (settings.count("line_width"s)) {
        render_settings.line_width = settings.at("line_width"s).AsDouble();
    }
    if (settings.count("stop_radius"s)) {
        render_settings.stop_radius = settings.at("stop_radius"s).AsDouble();
    }
    if (settings.count("bus_label_font_size"s)) {
        render_settings.bus_label_font_size = settings.at("bus_label_font_size"s).AsDouble();
    }
    if (settings.count("bus_label_offset"s)) {
        auto it = settings.at("bus_label_offset"s).AsArray();
        render_settings.bus_label_offset = { it[0].AsDouble(), it[1].AsDouble() };
    }
    if (settings.count("stop_label_font_size"s)) {
        render_settings.stop_label_font_size = settings.at("stop_label_font_size"s).AsDouble();
    }
    if (settings.count("stop_label_offset"s)) {
        auto it = settings.at("stop_label_offset"s).AsArray();
        render_settings.stop_label_offset = { it[0].AsDouble(), it[1].AsDouble() };
    }

    if (settings.count("underlayer_color"s)) {
        GetColor(settings.at("underlayer_color"s), &render_settings.underlayer_color);
    }
    if (settings.count("underlayer_width"s)) {
        render_settings.underlayer_width = settings.at("underlayer_width"s).AsDouble();
    }
    //массив цветов
    if (settings.count("color_palette"s)) {
        auto& array = settings.at("color_palette"s).AsArray();
        render_settings.color_palette.reserve(array.size());
        for (auto& node : array) {
            render_settings.color_palette.push_back({});
            GetColor(node, &render_settings.color_palette.back());
        }
    }
    map_renderer_.SetRenderSettings(std::move(render_settings));
}

void JsonReader::ParseRoutingSettings(const json::Node& node_)
{
    auto& settings = node_.AsMap();

    int bus_velocity = settings.at("bus_velocity"s).AsInt();
    int bus_wait_time = settings.at("bus_wait_time"s).AsInt();
    //Значение — целое число от 1 до 1000
    if (bus_velocity < 0 || bus_wait_time < 0 || bus_velocity > 1000 || bus_wait_time > 1000) {
        throw std::invalid_argument("invalid routing_settings: 0 <= velocity, wait_time <= 1000"s);
    }
    transport_router_.SetSettings({ static_cast<uint32_t>(bus_wait_time), static_cast<uint32_t>(bus_velocity) });
}

void JsonReader::GetColor(const json::Node& node, svg::Color* color) {
    if (node.IsString()) {
        *color = node.AsString();
    }
    else if (node.AsArray().size() == 3) {
        *color = svg::Rgb({ node.AsArray()[0].AsInt(),node.AsArray()[1].AsInt(),node.AsArray()[2].AsInt() });
    }
    else if (node.AsArray().size() == 4) {
        *color = svg::Rgba({ node.AsArray()[0].AsInt(),node.AsArray()[1].AsInt(),node.AsArray()[2].AsInt(), node.AsArray()[3].AsDouble() });
    }
}


json::Node JsonReader::CreateNode::ErrorMassage(json::Builder& builder, int id)
{
    return builder.StartDict().Key("request_id"s).Value(id)
            .Key("error_message"s).Value("not found"s).EndDict().Build();
}

json::Node JsonReader::CreateNode::operator() (int value) {
    json::Builder builder;
    return ErrorMassage(builder, value);
}

json::Node JsonReader::CreateNode::operator() (domain::StopOutput& value)
{
    json::Builder builder;
    builder.StartDict().Key("buses"s).StartArray();

    for (std::string_view bus : value.buses) {
        builder.Value(static_cast<std::string>(bus));
    }
    return builder.EndArray().Key("request_id"s).Value(value.id).EndDict().Build();
}

json::Node JsonReader::CreateNode::operator() (domain::BusOutput& value)
{
    json::Builder builder;

    return builder.StartDict()
            .Key("curvature"s).Value(value.bus->curvature)
            .Key("request_id"s).Value(value.id)
            .Key("route_length"s).Value(static_cast<double>(value.bus->distance))
            .Key("stop_count"s).Value(static_cast<int>(value.bus->stops.size()))
            .Key("unique_stop_count"s).Value(value.bus->unique_stops).EndDict().Build();
}

json::Node JsonReader::CreateNode::operator() (domain::MapOutput& value)
{
    json::Builder builder;

    return	builder.StartDict()
            .Key("request_id"s).Value(value.id)
            .Key("map"s).Value(value.map_)
            .EndDict().Build();
}

json::Node JsonReader::CreateNode::operator() (domain::RouteOutput& value) {

    std::optional<transport_router_::CompletedRoute> result =
            transport_router_.ComputeRoute(value.from->vertex_id, value.to->vertex_id);

    json::Builder builder;

    if (!result) {
        return ErrorMassage(builder, value.id);
    }

    builder.StartDict().Key("request_id"s).Value(value.id)
            .Key("total_time"s).Value(result->total_time)
            .Key("items"s).StartArray();

    for (const transport_router_::CompletedRoute::Line& line : result->route) {
        builder.StartDict().Key("stop_name"s).Value(line.stop->name)
                .Key("time"s).Value(line.wait_time)
                .Key("type"s).Value("Wait"s).EndDict()
                .StartDict().Key("bus"s).Value(line.bus->name)
                .Key("span_count"s).Value(static_cast<int>(line.count_stops))
                .Key("time"s).Value(line.run_time)
                .Key("type").Value("Bus"s).EndDict();
    }
    builder.EndArray().EndDict();
    return builder.Build();
}
